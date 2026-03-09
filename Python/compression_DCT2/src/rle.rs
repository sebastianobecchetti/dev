/// Run-Length Encoding / Decoding optimized for sparse DCT coefficient arrays.
/// Since DCT high frequencies are almost all zeros, we encode:
/// - If non-zero: write the integer directly.
/// - If zero: write 0, then write the number of consecutive zeros.

/// RLE-encode a slice of i64 values (quantized DCT coefficients).
/// Returns a Vec<i32> ready for serialization.
pub fn encode_rle(data: &[i64]) -> Vec<i32> {
    let mut encoded = Vec::with_capacity(data.len());
    let mut zero_count: i32 = 0;

    for &val in data {
        let v = val as i32;
        if v == 0 {
            zero_count += 1;
        } else {
            if zero_count > 0 {
                encoded.push(0);
                encoded.push(zero_count);
                zero_count = 0;
            }
            encoded.push(v);
        }
    }

    if zero_count > 0 {
        encoded.push(0);
        encoded.push(zero_count);
    }

    encoded
}

/// Decode an RLE-encoded stream back to f64 values.
pub fn decode_rle(rle_data: &[i32]) -> Vec<f64> {
    let mut decoded = Vec::new();
    let mut i = 0;
    let n = rle_data.len();

    while i < n {
        let val = rle_data[i];
        if val == 0 {
            if i + 1 < n {
                let count = rle_data[i + 1] as usize;
                decoded.resize(decoded.len() + count, 0.0);
                i += 2;
            } else {
                // Malformed: trailing zero with no count
                decoded.push(0.0);
                i += 1;
            }
        } else {
            decoded.push(val as f64);
            i += 1;
        }
    }

    decoded
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_roundtrip() {
        let data: Vec<i64> = vec![5, 0, 0, 0, -3, 0, 7, 0, 0];
        let encoded = encode_rle(&data);
        let decoded = decode_rle(&encoded);
        let expected: Vec<f64> = data.iter().map(|&v| v as f64).collect();
        assert_eq!(decoded, expected);
    }

    #[test]
    fn test_all_zeros() {
        let data: Vec<i64> = vec![0, 0, 0, 0, 0];
        let encoded = encode_rle(&data);
        assert_eq!(encoded, vec![0, 5]);
        let decoded = decode_rle(&encoded);
        assert_eq!(decoded, vec![0.0; 5]);
    }

    #[test]
    fn test_no_zeros() {
        let data: Vec<i64> = vec![1, 2, 3];
        let encoded = encode_rle(&data);
        assert_eq!(encoded, vec![1, 2, 3]);
        let decoded = decode_rle(&encoded);
        assert_eq!(decoded, vec![1.0, 2.0, 3.0]);
    }
}
