/// DCT compression utilities using rustdct.
/// Implements block-wise DCT-II / IDCT-II with quantization, delta coding,
/// RLE, and zstd compression — matching the Python pipeline exactly.

use rustdct::DctPlanner;
use rayon::prelude::*;
use crate::rle;
use crate::quantize;

/// Calculate per-frequency variances of DCT coefficients across all blocks.
/// Used by the water-filling quantization strategy.
pub fn calculate_channel_variances(
    channel_data: &[f64],
    block_size: usize,
    hop_size: usize,
) -> Vec<f64> {
    let n = channel_data.len();
    let use_overlap = hop_size < block_size;

    let mut num_blocks = if n >= block_size {
        (n - block_size) / hop_size + 1
    } else {
        1
    };
    if (num_blocks - 1) * hop_size + block_size < n {
        num_blocks += 1;
    }
    if num_blocks == 0 {
        return vec![0.0; block_size];
    }

    let padded_len = (num_blocks - 1) * hop_size + block_size;
    let mut padded = vec![0.0f64; padded_len];
    padded[..n].copy_from_slice(channel_data);

    // Build window
    let window: Vec<f64> = if use_overlap {
        (0..block_size)
            .map(|i| (std::f64::consts::PI * (i as f64 + 0.5) / block_size as f64).sin())
            .collect()
    } else {
        vec![1.0; block_size]
    };

    // Plan DCT
    let mut planner = DctPlanner::new();
    let dct = planner.plan_dct2(block_size);

    // Accumulate sum and sum_sq per frequency index
    let mut sum = vec![0.0f64; block_size];
    let mut sum_sq = vec![0.0f64; block_size];

    let ortho_scale = (2.0 / block_size as f64).sqrt();

    for b in 0..num_blocks {
        let start = b * hop_size;
        let mut block: Vec<f64> = padded[start..start + block_size].to_vec();

        // Apply window
        if use_overlap {
            for (j, w) in window.iter().enumerate() {
                block[j] *= w;
            }
        }

        // rustdct operates in-place
        dct.process_dct2(&mut block);

        // Apply ortho normalization (rustdct uses unnormalized)
        // ortho norm: all coefficients *= sqrt(2/N), then coeff[0] *= 1/sqrt(2)
        for val in block.iter_mut() {
            *val *= ortho_scale;
        }
        block[0] /= std::f64::consts::SQRT_2;

        for (j, &val) in block.iter().enumerate() {
            sum[j] += val;
            sum_sq[j] += val * val;
        }
    }

    let nb = num_blocks as f64;
    (0..block_size)
        .map(|j| {
            let mean = sum[j] / nb;
            sum_sq[j] / nb - mean * mean
        })
        .collect()
}

/// Compress a single channel using DCT + quantize + delta + RLE + zstd.
/// Returns (compressed_bytes, q_array).
pub fn compress_channel(
    channel_data: &[f64],
    q_factor: f64,
    block_size: usize,
    channel_variances: Option<&[f64]>,
    hop_size: usize,
) -> (Vec<u8>, Vec<f64>) {
    let n = channel_data.len();
    let use_overlap = hop_size < block_size;

    // Build quantization array
    let (q_array, dropped_mask) = if let Some(vars) = channel_variances {
        quantize::generate_waterfilling_quantization_array(vars, q_factor)
    } else {
        let q = quantize::generate_quantization_array(block_size, q_factor);
        let mask = vec![false; block_size];
        (q, mask)
    };

    // Compute number of blocks
    let mut num_blocks = if n >= block_size {
        (n - block_size) / hop_size + 1
    } else {
        1
    };
    if (num_blocks - 1) * hop_size + block_size < n {
        num_blocks += 1;
    }

    // Pad data
    let padded_len = (num_blocks - 1) * hop_size + block_size;
    let mut padded = vec![0.0f64; padded_len];
    let copy_len = n.min(padded_len);
    padded[..copy_len].copy_from_slice(&channel_data[..copy_len]);

    // Build window
    let window: Vec<f64> = if use_overlap {
        (0..block_size)
            .map(|i| (std::f64::consts::PI * (i as f64 + 0.5) / block_size as f64).sin())
            .collect()
    } else {
        vec![1.0; block_size]
    };

    // Process all blocks and quantize in parallel
    let all_quantized: Vec<Vec<i64>> = (0..num_blocks)
        .into_par_iter()
        .map(|b| {
            let start = b * hop_size;
            let mut block = padded[start..start + block_size].to_vec();

            // Apply analysis window
            if use_overlap {
                for (j, w) in window.iter().enumerate() {
                    block[j] *= w;
                }
            }

            // DCT-II (need a thread-local planner since it's not Sync)
            let mut planner = DctPlanner::new();
            let dct = planner.plan_dct2(block_size);
            dct.process_dct2(&mut block);

            // Ortho normalize
            for val in block.iter_mut() {
                *val *= ortho_scale;
            }
            block[0] /= std::f64::consts::SQRT_2;

            // Quantize
            let mut quantized_block: Vec<i64> = block
                .iter()
                .zip(q_array.iter())
                .map(|(&val, &q)| (val / q).round() as i64)
                .collect();

            // Drop masked frequencies
            for (j, &dropped) in dropped_mask.iter().enumerate() {
                if dropped {
                    quantized_block[j] = 0;
                }
            }

            quantized_block
        })
        .collect();

    // Delta encoding inter-block
    let mut flat = Vec::with_capacity(num_blocks * block_size);
    if num_blocks > 1 {
        // First block as-is
        flat.extend_from_slice(&all_quantized[0]);
        for b in 1..num_blocks {
            for j in 0..block_size {
                flat.push(all_quantized[b][j] - all_quantized[b - 1][j]);
            }
        }
    } else {
        flat.extend_from_slice(&all_quantized[0]);
    }

    // RLE encode
    let rle_data = rle::encode_rle(&flat);

    // Serialize i32 to bytes (little-endian)
    let mut bytes = Vec::with_capacity(rle_data.len() * 4);
    for &val in &rle_data {
        bytes.extend_from_slice(&val.to_le_bytes());
    }

    // zstd compress
    let compressed = quantize::zstd_compress(&bytes);

    (compressed, q_array)
}

/// Decompress a channel from DCT+RLE+zstd compressed bytes.
pub fn decompress_channel(
    compressed_bytes: &[u8],
    q_array: &[f32],
    block_size: usize,
    count: usize,
    hop_size: usize,
) -> Vec<f64> {
    let use_overlap = hop_size > 0 && hop_size < block_size;
    let effective_hop = if !use_overlap { block_size } else { hop_size };

    let mut num_blocks = if count >= block_size {
        (count - block_size) / effective_hop + 1
    } else {
        1
    };
    if (num_blocks - 1) * effective_hop + block_size < count {
        num_blocks += 1;
    }

    let expected_len = num_blocks * block_size;

    // Decompress zstd
    let rle_bytes = quantize::zstd_decompress(compressed_bytes, expected_len * 4 + 4096);

    // Parse i32 values
    let rle_data: Vec<i32> = rle_bytes
        .chunks_exact(4)
        .map(|chunk| i32::from_le_bytes([chunk[0], chunk[1], chunk[2], chunk[3]]))
        .collect();

    // RLE decode
    let mut quantized_flat = rle::decode_rle(&rle_data);

    // Pad or truncate
    if quantized_flat.len() < expected_len {
        quantized_flat.resize(expected_len, 0.0);
    } else if quantized_flat.len() > expected_len {
        quantized_flat.truncate(expected_len);
    }

    // Reshape into blocks
    let mut blocks: Vec<Vec<f64>> = quantized_flat
        .chunks(block_size)
        .map(|c| c.to_vec())
        .collect();

    // Undo delta encoding (cumulative sum)
    if blocks.len() > 1 {
        for b in 1..blocks.len() {
            for j in 0..block_size {
                blocks[b][j] += blocks[b - 1][j];
            }
        }
    }

    // Dequantize
    let q_array_f64: Vec<f64> = q_array.iter().map(|&v| v as f64).collect();

    let mut recon_blocks: Vec<Vec<f64>> = blocks
        .into_par_iter()
        .map(|block| {
            let mut dequant: Vec<f64> = block
                .iter()
                .zip(q_array_f64.iter())
                .map(|(&val, &q)| val * q)
                .collect();

            // Undo ortho normalization before IDCT
            // Inverse of: all *= sqrt(2/N), [0] /= sqrt(2)
            // => [0] *= sqrt(2), all /= sqrt(2/N)
            dequant[0] *= std::f64::consts::SQRT_2;
            for val in dequant.iter_mut() {
                *val /= ortho_scale;
            }

            // IDCT (DCT-III in rustdct) (requires thread-local planner)
            let mut planner = DctPlanner::new();
            let idct = planner.plan_dct3(block_size);
            idct.process_dct3(&mut dequant);

            // rustdct DCT-III result needs scaling by 1/N
            let inv_n = 1.0 / block_size as f64;
            for val in dequant.iter_mut() {
                *val *= inv_n;
            }

            dequant
        })
        .collect();

    if use_overlap {
        let window: Vec<f64> = (0..block_size)
            .map(|i| (std::f64::consts::PI * (i as f64 + 0.5) / block_size as f64).sin())
            .collect();

        // Apply synthesis window
        for block in recon_blocks.iter_mut() {
            for (j, w) in window.iter().enumerate() {
                block[j] *= w;
            }
        }

        // Overlap-add
        let padded_len = (num_blocks - 1) * effective_hop + block_size;
        let mut output = vec![0.0f64; padded_len];
        let mut norm = vec![0.0f64; padded_len];

        for (i, block) in recon_blocks.iter().enumerate() {
            let start = i * effective_hop;
            for j in 0..block_size {
                output[start + j] += block[j];
                norm[start + j] += window[j] * window[j];
            }
        }

        for i in 0..padded_len {
            if norm[i] > 1e-10 {
                output[i] /= norm[i];
            }
        }

        output.truncate(count);
        output
    } else {
        let mut flat: Vec<f64> = recon_blocks.into_iter().flatten().collect();
        flat.truncate(count);
        flat
    }
}

/// Apply block-wise DCT compression (for standalone dct_compress tool).
/// Returns (reconstructed_data, non_zero_count).
pub fn apply_block_dct_compression(
    channel_data: &[f64],
    q_factor: f64,
    block_size: usize,
    channel_variances: Option<&[f64]>,
) -> (Vec<f64>, usize) {
    if q_factor <= 0.0 {
        return (channel_data.to_vec(), channel_data.len());
    }

    let n = channel_data.len();
    let num_blocks = n / block_size;
    let remainder = n % block_size;

    let mut reconstructed = vec![0.0f64; n];
    let mut non_zero_count = 0usize;

    let mut planner = DctPlanner::new();

    let ortho_scale = (2.0 / block_size as f64).sqrt();

    if num_blocks > 0 {
        let (q_array, dropped_mask) = if let Some(vars) = channel_variances {
            quantize::generate_waterfilling_quantization_array(vars, q_factor)
        } else {
            let q = quantize::generate_quantization_array(block_size, q_factor);
            let mask = vec![false; block_size];
            (q, mask)
        };

        let processed_blocks: Vec<(Vec<f64>, usize)> = (0..num_blocks)
            .into_par_iter()
            .map(|b| {
                let start = b * block_size;
                let mut block: Vec<f64> = channel_data[start..start + block_size].to_vec();

                let mut planner = DctPlanner::new();
                let dct_plan = planner.plan_dct2(block_size);
                let idct_plan = planner.plan_dct3(block_size);
                let mut nz_count = 0usize;

                // DCT-II
                dct_plan.process_dct2(&mut block);

                // Ortho normalize
                for val in block.iter_mut() {
                    *val *= ortho_scale;
                }
                block[0] /= std::f64::consts::SQRT_2;

                // Quantize
                for j in 0..block_size {
                    block[j] = (block[j] / q_array[j]).round();
                    if dropped_mask[j] {
                        block[j] = 0.0;
                    }
                    if block[j] != 0.0 {
                        nz_count += 1;
                    }
                }

                // Dequantize
                for j in 0..block_size {
                    block[j] *= q_array[j];
                }

                // IDCT (undo ortho normalization first)
                block[0] *= std::f64::consts::SQRT_2;
                for val in block.iter_mut() {
                    *val /= ortho_scale;
                }
                idct_plan.process_dct3(&mut block);
                let inv_n = 1.0 / block_size as f64;
                for val in block.iter_mut() {
                    *val *= inv_n;
                }

                (block, nz_count)
            })
            .collect();

        for (b, (block, nz_count)) in processed_blocks.into_iter().enumerate() {
            let start = b * block_size;
            reconstructed[start..start + block_size].copy_from_slice(&block);
            non_zero_count += nz_count;
        }
    }

    // Remainder block
    if remainder > 0 {
        let start = num_blocks * block_size;
        let rem_ortho_scale = (2.0 / remainder as f64).sqrt();

        let (rem_q, rem_mask) = if let Some(vars) = channel_variances {
            let rem_vars = &vars[..remainder.min(vars.len())];
            quantize::generate_waterfilling_quantization_array(rem_vars, q_factor)
        } else {
            let q = quantize::generate_quantization_array(remainder, q_factor);
            let mask = vec![false; remainder];
            (q, mask)
        };

        let rem_dct = planner.plan_dct2(remainder);
        let rem_idct = planner.plan_dct3(remainder);

        let mut block: Vec<f64> = channel_data[start..].to_vec();

        // DCT-II
        rem_dct.process_dct2(&mut block);
        for val in block.iter_mut() {
            *val *= rem_ortho_scale;
        }
        block[0] /= std::f64::consts::SQRT_2;

        // Quantize
        for j in 0..remainder {
            block[j] = (block[j] / rem_q[j]).round();
            if rem_mask[j] {
                block[j] = 0.0;
            }
            if block[j] != 0.0 {
                non_zero_count += 1;
            }
        }

        // Dequantize
        for j in 0..remainder {
            block[j] *= rem_q[j];
        }

        // IDCT
        block[0] *= std::f64::consts::SQRT_2;
        for val in block.iter_mut() {
            *val /= rem_ortho_scale;
        }
        rem_idct.process_dct3(&mut block);
        let inv_n = 1.0 / remainder as f64;
        for val in block.iter_mut() {
            *val *= inv_n;
        }

        reconstructed[start..].copy_from_slice(&block);
    }

    (reconstructed, non_zero_count)
}
