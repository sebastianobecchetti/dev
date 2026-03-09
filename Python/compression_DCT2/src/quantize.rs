/// Quantization utilities for the BEK compression pipeline.
/// Includes JPEG-like linear penalty, Reverse Water-Filling, and position quantization.

use byteorder::{LittleEndian, WriteBytesExt, ReadBytesExt};
use std::io::Cursor;

/// Generates a 1D quantization array similar in spirit to JPEG.
/// Lower frequencies get lower quantization values (preserve more precision).
pub fn generate_quantization_array(block_size: usize, q_factor: f64) -> Vec<f64> {
    let bs = block_size as f64;
    (0..block_size)
        .map(|i| {
            let base_q = 1.0 + (i as f64 / bs) * (bs - 1.0);
            (base_q * q_factor).max(1.0)
        })
        .collect()
}

/// Generates a quantization array using Reverse Water-Filling principles.
/// Returns (q_array, dropped_mask).
pub fn generate_waterfilling_quantization_array(
    variances: &[f64],
    distortion_d: f64,
) -> (Vec<f64>, Vec<bool>) {
    let block_size = variances.len();
    let mut q_array = vec![0.0f64; block_size];
    let mut dropped_mask = vec![false; block_size];

    let target_q = if distortion_d > 0.0 {
        (12.0 * distortion_d).sqrt().max(1.0)
    } else {
        1.0
    };

    for (i, &var) in variances.iter().enumerate() {
        if var <= distortion_d {
            dropped_mask[i] = true;
            q_array[i] = 1.0;
        } else {
            q_array[i] = target_q;
        }
    }

    (q_array, dropped_mask)
}

/// Quantize float positions to uint32 delta-encoded + zstd compressed bytes.
pub fn quantize_position(data: &[f64], min_val: f64, max_val: f64) -> Vec<u8> {
    let range_val = max_val - min_val + 1e-10;
    let quantized: Vec<u32> = data
        .iter()
        .map(|&v| {
            let normalized = (v - min_val) / range_val;
            (normalized * 4_294_967_295.0).clamp(0.0, 4_294_967_295.0) as u32
        })
        .collect();

    // Delta encode as i64
    let mut delta = Vec::with_capacity(quantized.len());
    delta.push(quantized[0] as i64);
    for i in 1..quantized.len() {
        delta.push(quantized[i] as i64 - quantized[i - 1] as i64);
    }

    // Serialize to bytes
    let mut buf = Vec::with_capacity(delta.len() * 8);
    for &d in &delta {
        buf.write_i64::<LittleEndian>(d).unwrap();
    }

    // zstd compress
    zstd::bulk::compress(&buf, 19).unwrap()
}

/// Dequantize uint32 delta-encoded positions back to f64.
pub fn dequantize_position(compressed_bytes: &[u8], min_val: f64, max_val: f64, count: usize) -> Vec<f64> {
    let raw = zstd::bulk::decompress(compressed_bytes, count * 8 + 1024).unwrap();
    let mut cursor = Cursor::new(&raw);

    // Read delta-encoded i64
    let mut quantized = Vec::with_capacity(count);
    let mut cumulative: i64 = 0;
    for _ in 0..count {
        let delta = cursor.read_i64::<LittleEndian>().unwrap();
        cumulative += delta;
        quantized.push(cumulative as f64);
    }

    let range_val = max_val - min_val + 1e-10;
    quantized
        .iter()
        .map(|&q| (q / 4_294_967_295.0) * range_val + min_val)
        .collect()
}

/// Compress zstd at level 19 (matching the Python pipeline).
pub fn zstd_compress(data: &[u8]) -> Vec<u8> {
    zstd::bulk::compress(data, 19).unwrap()
}

/// Decompress zstd.
pub fn zstd_decompress(data: &[u8], max_size: usize) -> Vec<u8> {
    zstd::bulk::decompress(data, max_size).unwrap()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_quantization_array() {
        let q = generate_quantization_array(8, 1.0);
        assert_eq!(q.len(), 8);
        assert!((q[0] - 1.0).abs() < 1e-6);
        assert!(q[7] > q[0]);
    }

    #[test]
    fn test_waterfilling() {
        let vars = vec![10.0, 5.0, 0.01, 0.001];
        let (q, mask) = generate_waterfilling_quantization_array(&vars, 1.0);
        assert!(!mask[0]); // high variance, not dropped
        assert!(!mask[1]);
        assert!(mask[2]); // low variance, dropped
        assert!(mask[3]);
        assert_eq!(q.len(), 4);
    }

    #[test]
    fn test_position_quantize_roundtrip() {
        let data = vec![0.0, 0.5, 1.0, 0.25, 0.75];
        let compressed = quantize_position(&data, 0.0, 1.0);
        let decompressed = dequantize_position(&compressed, 0.0, 1.0, 5);
        for (a, b) in data.iter().zip(decompressed.iter()) {
            assert!((a - b).abs() < 1e-6, "{} vs {}", a, b);
        }
    }
}
