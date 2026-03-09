/// BEK Encoder binary — encodes a PLY file into the compressed .bek format.
/// Full port of encode_bek.py.

use clap::Parser;
use prost::Message;
use std::fs::File;
use std::io::Write;
use byteorder::{LittleEndian, WriteBytesExt};

use compression_dct2::bek_format;
use compression_dct2::morton;
use compression_dct2::quantize;
use compression_dct2::dct_utils;
use compression_dct2::ply_io;
use compression_dct2::{MAGIC_WORD, VERSION};

use rand::seq::SliceRandom;
use rayon::prelude::*;

#[derive(Parser)]
#[command(name = "encode_bek")]
#[command(about = "Encode a PLY file into a highly compressed .bek format")]
struct Cli {
    /// Input .ply file
    #[arg(long)]
    input: String,

    /// Output .bek file
    #[arg(long)]
    output: String,

    /// Distortion water level (default 0.001)
    #[arg(long, default_value = "0.001")]
    q_factor: f64,

    /// DCT block size
    #[arg(long, default_value = "256")]
    block_size: usize,

    /// If set, compresses Spherical Harmonics (f_rest_*) as well
    #[arg(long)]
    compress_sh: bool,

    /// Optional path to dump calculated variances to JSON
    #[arg(long)]
    dump_stats: Option<String>,
}

fn main() {
    let cli = Cli::parse();

    // Disable overlap-add to prevent blurring artifacts
    let hop_size = cli.block_size;

    println!("Loading '{}'...", cli.input);
    let start = std::time::Instant::now();

    let ply_data = ply_io::read_ply(&cli.input).unwrap_or_else(|e| {
        eprintln!("Error: {}", e);
        std::process::exit(1);
    });

    let properties = &ply_data.property_names;
    let count = ply_data.count;
    println!("Loaded {} vertices in {:.2}s.", count, start.elapsed().as_secs_f64());

    // ===== Spatial sorting (Morton Z-order) =====
    println!("Spatially sorting vertices (Morton Z-order curve)...");
    let x = ply_io::extract_channel(&ply_data, "x");
    let y = ply_io::extract_channel(&ply_data, "y");
    let z = ply_io::extract_channel(&ply_data, "z");

    let sort_idx = morton::morton_sort_3d(&x, &y, &z);
    let sorted_vertices = ply_io::reorder_vertices(&ply_data, &sort_idx);

    // ===== Build protobuf header =====
    let x_min = x.iter().cloned().fold(f64::INFINITY, f64::min);
    let x_max = x.iter().cloned().fold(f64::NEG_INFINITY, f64::max);
    let y_min = y.iter().cloned().fold(f64::INFINITY, f64::min);
    let y_max = y.iter().cloned().fold(f64::NEG_INFINITY, f64::max);
    let z_min = z.iter().cloned().fold(f64::INFINITY, f64::min);
    let z_max = z.iter().cloned().fold(f64::NEG_INFINITY, f64::max);

    let mut header = bek_format::CompressedGeometryHeader {
        vertex_count: count as u32,
        block_size: cli.block_size as u32,
        hop_size: hop_size as u32,
        sort_min_x: x_min as f32,
        sort_max_x: x_max as f32,
        sort_min_y: y_min as f32,
        sort_max_y: y_max as f32,
        sort_min_z: z_min as f32,
        sort_max_z: z_max as f32,
        ..Default::default()
    };

    // ===== Channel classification =====
    let has_prop = |name: &str| properties.contains(&name.to_string());

    // Dropped: normals
    let drop_channels: Vec<String> = ["nx", "ny", "nz"]
        .iter()
        .filter(|c| has_prop(c))
        .map(|c| c.to_string())
        .collect();

    // Int8 Quantized: high-order SH
    let int8_channels: Vec<String> = if cli.compress_sh {
        (8..45)
            .map(|i| format!("f_rest_{}", i))
            .filter(|c| has_prop(c))
            .collect()
    } else {
        vec![]
    };

    // Positions
    let position_channels: Vec<String> = ["x", "y", "z"]
        .iter()
        .filter(|c| has_prop(c))
        .map(|c| c.to_string())
        .collect();

    // Float16: opacity, scale, rotation
    let float16_channels: Vec<String> = [
        "opacity", "scale_0", "scale_1", "scale_2",
        "rot_0", "rot_1", "rot_2", "rot_3",
    ]
    .iter()
    .filter(|c| has_prop(c))
    .map(|c| c.to_string())
    .collect();

    // Vector Quantized (K-Means): base colors
    let vq_channels: Vec<String> = ["f_dc_0", "f_dc_1", "f_dc_2"]
        .iter()
        .filter(|c| has_prop(c))
        .map(|c| c.to_string())
        .collect();

    // DCT compressed: low-order SH
    let dct_channels: Vec<String> = if cli.compress_sh {
        (0..8)
            .map(|i| format!("f_rest_{}", i))
            .filter(|c| has_prop(c))
            .collect()
    } else {
        vec![]
    };

    // Uncompressed: everything else
    let mut classified: std::collections::HashSet<String> = std::collections::HashSet::new();
    for c in drop_channels.iter().chain(int8_channels.iter())
        .chain(position_channels.iter()).chain(float16_channels.iter())
        .chain(vq_channels.iter()).chain(dct_channels.iter())
    {
        classified.insert(c.clone());
    }
    let uncompressed_channels: Vec<String> = properties
        .iter()
        .filter(|c| !classified.contains(*c))
        .cloned()
        .collect();

    // ===== DROPPED CHANNELS =====
    for c in &drop_channels {
        println!("  Dropping channel '{}' (not needed for rendering)", c);
        header.dropped_channels.push(c.clone());
    }

    // ===== POSITION QUANTIZATION (uint32) =====
    let pos_bounds = [
        ("x", x_min, x_max),
        ("y", y_min, y_max),
        ("z", z_min, z_max),
    ];
    for &(name, min_v, max_v) in &pos_bounds {
        if !position_channels.contains(&name.to_string()) {
            continue;
        }
        println!("  Quantizing position '{}' to uint32 (range [{:.2}, {:.2}])...", name, min_v, max_v);
        let sorted_data = ply_io::extract_channel(&sorted_vertices, name);
        let compressed = quantize::quantize_position(&sorted_data, min_v, max_v);

        header.quantized_channels.push(bek_format::QuantizedChannel {
            name: name.to_string(),
            min_val: min_v,
            max_val: max_v,
            data: compressed,
        });
    }

    // ===== INT8 QUANTIZATION (High-order SH) =====
    for c in &int8_channels {
        println!("  Quantizing SH '{}' to Int8...", c);
        let sorted_data = ply_io::extract_channel(&sorted_vertices, c);
        let min_v = sorted_data.iter().cloned().fold(f64::INFINITY, f64::min);
        let max_v = sorted_data.iter().cloned().fold(f64::NEG_INFINITY, f64::max);
        let range_val = max_v - min_v + 1e-10;

        let quantized: Vec<u8> = sorted_data
            .iter()
            .map(|&v| ((v - min_v) / range_val * 255.0).clamp(0.0, 255.0).round() as u8)
            .collect();

        let compressed = quantize::zstd_compress(&quantized);

        header.quantized_channels.push(bek_format::QuantizedChannel {
            name: c.clone(),
            min_val: min_v,
            max_val: max_v,
            data: compressed,
        });
    }

    // ===== FLOAT16 (opacity, scale, rotation) =====
    for c in &float16_channels {
        println!("  Storing channel '{}' as float16 (zstd)...", c);
        let sorted_data = ply_io::extract_channel(&sorted_vertices, c);

        // Convert to float16 bytes (using half crate would be better, but
        // we use f32→truncated f16 via bit manipulation)
        let f16_bytes: Vec<u8> = sorted_data
            .iter()
            .flat_map(|&v| {
                let f = half::f16::from_f64(v);
                f.to_le_bytes().to_vec()
            })
            .collect();

        let compressed = quantize::zstd_compress(&f16_bytes);

        header.uncompressed_channels.push(bek_format::UncompressedChannel {
            name: c.clone(),
            dtype: "float16".to_string(),
            data: compressed,
        });
    }

    // ===== VECTOR QUANTIZATION (K-Means) =====
    if !vq_channels.is_empty() {
        let k = 8192usize;
        println!("  Clustering {} channels ({}) via K-Means (K={})...",
            vq_channels.len(), vq_channels.join(", "), k);

        let num_channels = vq_channels.len();

        // Build feature matrix (row-major: [count, num_channels])
        let features: Vec<Vec<f64>> = (0..count)
            .map(|i| {
                vq_channels
                    .iter()
                    .map(|c| sorted_vertices.properties.get(c.as_str()).map(|v| v[i]).unwrap_or(0.0))
                    .collect()
            })
            .collect();

        // Subsample for faster K-Means
        let sample_size = 50000.min(count);
        let mut rng = rand::thread_rng();
        let mut indices: Vec<usize> = (0..count).collect();
        indices.shuffle(&mut rng);
        let subset_idx = &indices[..sample_size];

        // Simple K-Means implementation
        let num_clusters = k.min(count);
        let (codebook, labels) = simple_kmeans(&features, subset_idx, num_clusters, num_channels, 15);

        println!("  Assigning cluster indices for {} points...", count);

        // Full assignment: find nearest centroid for each point
        let all_labels: Vec<u16> = features
            .par_iter()
            .map(|point| {
                let mut best_dist = f64::MAX;
                let mut best_idx = 0u16;
                for (ci, centroid) in codebook.chunks(num_channels).enumerate() {
                    let dist: f64 = point
                        .iter()
                        .zip(centroid.iter())
                        .map(|(&a, &b)| (a - b) * (a - b))
                        .sum();
                    if dist < best_dist {
                        best_dist = dist;
                        best_idx = ci as u16;
                    }
                }
                best_idx
            })
            .collect();

        // Codebook as f32 bytes
        let codebook_f32: Vec<u8> = codebook
            .iter()
            .flat_map(|&v| (v as f32).to_le_bytes().to_vec())
            .collect();

        // Indices as u16 bytes, zstd compressed
        let indices_bytes: Vec<u8> = all_labels
            .iter()
            .flat_map(|&v| v.to_le_bytes().to_vec())
            .collect();
        let compressed_indices = quantize::zstd_compress(&indices_bytes);

        header.vector_quantized_groups.push(bek_format::VectorQuantizedGroup {
            channel_names: vq_channels.clone(),
            codebook: codebook_f32,
            indices: compressed_indices,
        });
    }

    // ===== UNCOMPRESSED (fallback) =====
    for c in &uncompressed_channels {
        println!("  Storing uncompressed channel '{}'...", c);
        let sorted_data = ply_io::extract_channel(&sorted_vertices, c);
        let bytes: Vec<u8> = sorted_data
            .iter()
            .flat_map(|&v| (v as f32).to_le_bytes().to_vec())
            .collect();
        let compressed = quantize::zstd_compress(&bytes);

        // Try to determine dtype from original data
        header.uncompressed_channels.push(bek_format::UncompressedChannel {
            name: c.clone(),
            dtype: "float32".to_string(),
            data: compressed,
        });
    }

    // ===== DCT COMPRESSED =====
    let mut payload_streams: Vec<Vec<u8>> = Vec::new();
    let mut all_stats: std::collections::HashMap<String, serde_json::Value> = std::collections::HashMap::new();

    // Collect info parallelly
    let dct_results: Vec<_> = dct_channels
        .par_iter()
        .map(|c| {
            println!("  Encoding channel '{}' using DCT+RLE+Zstd (overlap-add)...", c);

            let chan_data = ply_io::extract_channel(&sorted_vertices, c);
            let chan_vars = dct_utils::calculate_channel_variances(&chan_data, cli.block_size, hop_size);

            // Adaptive q_factor per channel type
            let effective_q = if c.starts_with("f_dc_") {
                cli.q_factor * 0.01
            } else if c.starts_with("f_rest_") {
                let sh_index: usize = c.rsplit('_').next().unwrap().parse().unwrap_or(0);
                if sh_index >= 3 {
                    cli.q_factor * 2.0
                } else {
                    cli.q_factor * 1.5
                }
            } else {
                cli.q_factor
            };

            let stats_val = if cli.dump_stats.is_some() {
                Some(serde_json::json!({
                    "channel": c,
                    "variance": chan_vars.clone(),
                }))
            } else {
                None
            };

            let (compressed_bytes, q_array) = dct_utils::compress_channel(
                &chan_data,
                effective_q,
                cli.block_size,
                Some(&chan_vars),
                hop_size,
            );

            let compressed_chan = bek_format::CompressedChannel {
                name: c.clone(),
                dtype: "float32".to_string(),
                quantization_array: q_array.iter().map(|&v| v as f32).collect(),
                arithmetic_payload_bytes: compressed_bytes.len() as u64,
            };

            (c.clone(), stats_val, compressed_chan, compressed_bytes)
        })
        .collect();

    for (c, stats_val, compressed_chan, compressed_bytes) in dct_results {
        if let Some(val) = stats_val {
            all_stats.insert(c, val);
        }
        header.compressed_channels.push(compressed_chan);
        payload_streams.push(compressed_bytes);
    }

    // ===== WRITE BEK FILE =====
    println!("\nWriting final .bek file to '{}'...", cli.output);
    let mut outfile = File::create(&cli.output).unwrap_or_else(|e| {
        eprintln!("Error creating output file: {}", e);
        std::process::exit(1);
    });

    outfile.write_all(MAGIC_WORD).unwrap();
    outfile.write_u32::<LittleEndian>(VERSION).unwrap();

    let header_bytes = header.encode_to_vec();
    outfile.write_u32::<LittleEndian>(header_bytes.len() as u32).unwrap();
    outfile.write_all(&header_bytes).unwrap();

    for stream in &payload_streams {
        outfile.write_all(stream).unwrap();
    }

    // Dump stats if requested
    if let Some(stats_path) = &cli.dump_stats {
        if !all_stats.is_empty() {
            println!("\n[Stats] Dumping calculated statistics to '{}'...", stats_path);
            let json_str = serde_json::to_string_pretty(&all_stats).unwrap();
            std::fs::write(stats_path, json_str).unwrap_or_else(|e| {
                eprintln!("Warning: Failed to write stats file: {}", e);
            });
        }
    }

    // Summary
    let orig_size = std::fs::metadata(&cli.input).map(|m| m.len()).unwrap_or(0);
    let new_size = std::fs::metadata(&cli.output).map(|m| m.len()).unwrap_or(1);
    let ratio = orig_size as f64 / new_size as f64;

    println!("\n--- BEK Format Encoding Status ---");
    println!("Original PLY size: {:.2} MB", orig_size as f64 / 1024.0 / 1024.0);
    println!("Encoded BEK size:  {:.2} MB", new_size as f64 / 1024.0 / 1024.0);
    println!("Actual Disk Ratio: {:.2}x", ratio);
    println!("Dropped: {}", if drop_channels.is_empty() { "none".to_string() } else { drop_channels.join(", ") });
    println!("Quantized (uint32): {}", position_channels.join(", "));
    println!("DCT compressed: {} channels", dct_channels.len());
    println!("Overlap: 50% (hop={}, block={})", hop_size, cli.block_size);
    println!("Total time: {:.2}s", start.elapsed().as_secs_f64());
}

/// Simple K-Means clustering.
/// Returns (codebook_flat, labels).
fn simple_kmeans(
    features: &[Vec<f64>],
    subset_indices: &[usize],
    k: usize,
    dim: usize,
    max_iter: usize,
) -> (Vec<f64>, Vec<u16>) {
    let subset: Vec<&Vec<f64>> = subset_indices.iter().map(|&i| &features[i]).collect();
    let n = subset.len();

    // Initialize centroids from first k subset points
    let mut centroids: Vec<Vec<f64>> = subset.iter().take(k).map(|p| (*p).clone()).collect();

    // Pad if not enough points
    while centroids.len() < k {
        centroids.push(vec![0.0; dim]);
    }

    let mut labels = vec![0u16; n];

    for _iter in 0..max_iter {
        // Assignment step (run in parallel)
        labels.par_iter_mut().enumerate().for_each(|(i, label)| {
            let point = subset[i];
            let mut best_dist = f64::MAX;
            let mut best_idx = 0u16;
            for (ci, centroid) in centroids.iter().enumerate() {
                let dist: f64 = point
                    .iter()
                    .zip(centroid.iter())
                    .map(|(&a, &b)| (a - b) * (a - b))
                    .sum();
                if dist < best_dist {
                    best_dist = dist;
                    best_idx = ci as u16;
                }
            }
            *label = best_idx;
        });

        // Update step
        let mut new_centroids = vec![vec![0.0; dim]; k];
        let mut counts = vec![0usize; k];

        for (i, point) in subset.iter().enumerate() {
            let ci = labels[i] as usize;
            counts[ci] += 1;
            for (j, &v) in point.iter().enumerate() {
                new_centroids[ci][j] += v;
            }
        }

        for ci in 0..k {
            if counts[ci] > 0 {
                for j in 0..dim {
                    new_centroids[ci][j] /= counts[ci] as f64;
                }
            } else {
                new_centroids[ci] = centroids[ci].clone();
            }
        }

        centroids = new_centroids;
    }

    // Flatten codebook
    let codebook_flat: Vec<f64> = centroids.into_iter().flatten().collect();

    (codebook_flat, labels)
}
