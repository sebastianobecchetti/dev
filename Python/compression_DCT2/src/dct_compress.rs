/// Standalone DCT compressor — compresses a PLY file using block-wise DCT.
/// Full port of dct_compress.py.
/// Reads PLY → applies DCT with quantization → writes compressed PLY.

use clap::Parser;
use std::collections::HashMap;
use rayon::prelude::*;

use compression_dct2::dct_utils;
use compression_dct2::ply_io;

#[derive(Parser)]
#[command(name = "dct_compress")]
#[command(about = "Compress a 3D PLY file using Block-wise DCT with optional Reverse Water-Filling")]
struct Cli {
    /// Path to the input .ply file
    #[arg(long)]
    input: String,

    /// Path to save the output compressed .ply file
    #[arg(long)]
    output: String,

    /// Quantization factor. If using --stats_file, represents Distortion water-level 'D'.
    /// Otherwise, acts as a JPEG multiplier. 0 means no quantization. (default: 10.0)
    #[arg(long, default_value = "10.0")]
    q_factor: f64,

    /// Size of the blocks for localized DCT processing (default: 256)
    #[arg(long, default_value = "256")]
    block_size: usize,

    /// Calculate and save the mean/variance of DCT coefficients to a JSON file
    #[arg(long)]
    calc_stats: bool,

    /// Optional path to a previously generated JSON statistics file.
    /// Enables Reverse Water-Filling quantization based on coefficient variances.
    #[arg(long)]
    stats_file: Option<String>,
}

fn main() {
    let cli = Cli::parse();

    if cli.q_factor < 0.0 {
        eprintln!("Error: --q_factor must be >= 0.0");
        std::process::exit(1);
    }

    println!("Loading '{}'...", cli.input);
    let start = std::time::Instant::now();

    let ply_data = ply_io::read_ply(&cli.input).unwrap_or_else(|e| {
        eprintln!("Failed to load ply file: {}", e);
        std::process::exit(1);
    });

    let properties = &ply_data.property_names;
    let count = ply_data.count;
    println!("Loaded {} vertices in {:.2} seconds.", count, start.elapsed().as_secs_f64());

    // ===== Spatial Sorting =====
    println!("Spatially sorting vertices to preserve local topology...");
    let sort_start = std::time::Instant::now();

    let x = ply_io::extract_channel(&ply_data, "x");
    let y = ply_io::extract_channel(&ply_data, "y");
    let z = ply_io::extract_channel(&ply_data, "z");

    let x_range = x.iter().cloned().fold(f64::NEG_INFINITY, f64::max)
        - x.iter().cloned().fold(f64::INFINITY, f64::min) + 1e-8;
    let y_range = y.iter().cloned().fold(f64::NEG_INFINITY, f64::max)
        - y.iter().cloned().fold(f64::INFINITY, f64::min) + 1e-8;
    let z_range = z.iter().cloned().fold(f64::NEG_INFINITY, f64::max)
        - z.iter().cloned().fold(f64::INFINITY, f64::min) + 1e-8;

    let x_min = x.iter().cloned().fold(f64::INFINITY, f64::min);
    let y_min = y.iter().cloned().fold(f64::INFINITY, f64::min);
    let z_min = z.iter().cloned().fold(f64::INFINITY, f64::min);

    let grid_size = 1024.0f64;
    let mut sort_keys: Vec<(i32, i32, i32, usize)> = (0..count)
        .map(|i| {
            let xi = ((x[i] - x_min) / x_range * grid_size) as i32;
            let yi = ((y[i] - y_min) / y_range * grid_size) as i32;
            let zi = ((z[i] - z_min) / z_range * grid_size) as i32;
            (xi, yi, zi, i)
        })
        .collect();

    sort_keys.sort_unstable_by(|a, b| a.0.cmp(&b.0).then(a.1.cmp(&b.1)).then(a.2.cmp(&b.2)));

    let sort_idx: Vec<usize> = sort_keys.iter().map(|&(_, _, _, i)| i).collect();
    let sorted_vertices = ply_io::reorder_vertices(&ply_data, &sort_idx);

    println!("Finished sorting in {:.2} seconds.", sort_start.elapsed().as_secs_f64());

    // ===== Setup channels for DCT =====
    let mut channels_to_compress: Vec<String> = Vec::new();

    let color_channels = ["red", "green", "blue"];
    let dc_channels = ["f_dc_0", "f_dc_1", "f_dc_2"];

    for group in [&color_channels[..], &dc_channels[..]] {
        if group.iter().all(|c| properties.contains(&c.to_string())) {
            for c in group {
                channels_to_compress.push(c.to_string());
            }
        }
    }

    channels_to_compress.retain(|c| properties.contains(c));

    println!("\nChannels selected for DCT compression: {:?}", channels_to_compress);
    println!("Block size: {}", cli.block_size);

    if cli.q_factor > 0.0 {
        if cli.stats_file.is_some() {
            println!("Distortion water level (D): {:.1} (Using Reverse Water-Filling)", cli.q_factor);
        } else {
            println!("Quantization factor multiplier: {:.1} (Using JPEG-like Linear Penalty)", cli.q_factor);
        }
    } else {
        println!("Skipping compression (q_factor=0.0).");
    }

    // Load stats file if provided
    let loaded_stats: Option<HashMap<String, serde_json::Value>> = if let Some(ref stats_path) = cli.stats_file {
        let content = std::fs::read_to_string(stats_path).unwrap_or_else(|e| {
            eprintln!("Failed to load stats file: {}", e);
            std::process::exit(1);
        });
        let parsed: HashMap<String, serde_json::Value> = serde_json::from_str(&content).unwrap();
        println!("Loaded statistics from '{}'.", stats_path);
        Some(parsed)
    } else {
        None
    };

    // ===== Apply block-wise DCT =====
    let mut new_vertices = sorted_vertices.clone();
    let mut all_stats: HashMap<String, serde_json::Value> = HashMap::new();
    let mut total_original_coeffs = 0usize;
    let mut total_nonzero_coeffs = 0usize;

    // Process channels in parallel
    let processed_channels: Vec<_> = channels_to_compress
        .par_iter()
        .map(|channel| {
            println!("  Processing channel '{}'...", channel);
            let chan_data = ply_io::extract_channel(&sorted_vertices, channel);

            // Get channel variances if available
            let chan_vars: Option<Vec<f64>> = loaded_stats.as_ref().and_then(|stats| {
                stats.get(channel).and_then(|s| {
                    s.get("variance").and_then(|v| {
                        let arr: Option<Vec<f64>> = serde_json::from_value(v.clone()).ok();
                        arr.and_then(|a| {
                            if a.len() == cli.block_size {
                                Some(a)
                            } else {
                                println!("    Warning: Block size ({}) does not match stats variance length ({}). Falling back to generic quantization.",
                                    cli.block_size, a.len());
                                None
                            }
                        })
                    })
                })
            });

            let (processed, non_zero_count) = dct_utils::apply_block_dct_compression(
                &chan_data,
                cli.q_factor,
                cli.block_size,
                chan_vars.as_deref(),
            );

            (channel.clone(), processed, chan_data.len(), non_zero_count)
        })
        .collect();

    for (channel, processed, orig_len, nz_count) in processed_channels {
        total_original_coeffs += orig_len;
        total_nonzero_coeffs += nz_count;
        ply_io::set_channel(&mut new_vertices, &channel, processed);
    }

    if cli.calc_stats && !all_stats.is_empty() {
        let stats_file = format!("{}_dct_stats.json",
            cli.input.trim_end_matches(".ply"));
        println!("\nSaving statistics to '{}'...", stats_file);
        let json_str = serde_json::to_string_pretty(&all_stats).unwrap();
        std::fs::write(&stats_file, json_str).unwrap_or_else(|e| {
            eprintln!("Warning: Failed to write stats: {}", e);
        });
    }

    // ===== Save =====
    println!("\nSaving compressed data to '{}'...", cli.output);
    ply_io::write_ply(&cli.output, properties, &new_vertices).unwrap_or_else(|e| {
        eprintln!("Failed to write output PLY: {}", e);
        std::process::exit(1);
    });

    println!("Finished successfully in {:.2} total seconds.", start.elapsed().as_secs_f64());

    if cli.q_factor > 0.0 {
        println!("\n--- Compression Status ---");
        if total_original_coeffs > 0 {
            let percentage_kept = (total_nonzero_coeffs as f64 / total_original_coeffs as f64) * 100.0;
            let ratio = total_original_coeffs as f64 / total_nonzero_coeffs.max(1) as f64;
            println!("Total Coefficients Processed: {}", total_original_coeffs);
            println!("Non-Zero Coefficients Kept:   {} ({:.2}%)", total_nonzero_coeffs, percentage_kept);
            println!("Zeroed Coefficients Dropped:  {}", total_original_coeffs - total_nonzero_coeffs);
            println!("Theoretical Ratio (Data):     {:.2}x", ratio);
            println!("Note: Actual file size reduction requires entropy encoding (e.g. gzip) on the output file.");
        }
    }
}
