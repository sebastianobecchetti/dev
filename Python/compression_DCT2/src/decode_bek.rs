/// BEK Decoder binary — decodes a .bek file back into a standard .ply point cloud.
/// Full port of decode_bek.py.

use clap::Parser;
use prost::Message;
use std::fs::File;
use std::io::Read;
use byteorder::{LittleEndian, ReadBytesExt};
use std::io::Cursor;

use compression_dct2::bek_format;
use compression_dct2::quantize;
use compression_dct2::dct_utils;
use compression_dct2::ply_io;
use compression_dct2::{MAGIC_WORD, VERSION};

#[derive(Parser)]
#[command(name = "decode_bek")]
#[command(about = "Decode a .bek file back into a standard .ply point cloud")]
struct Cli {
    /// Input .bek file
    #[arg(long)]
    input: String,

    /// Output .ply file
    #[arg(long)]
    output: String,
}

fn main() {
    let cli = Cli::parse();
    let start = std::time::Instant::now();

    println!("Reading '{}'...", cli.input);

    let mut file = File::open(&cli.input).unwrap_or_else(|e| {
        eprintln!("Error: Failed to open '{}': {}", cli.input, e);
        std::process::exit(1);
    });

    // Read magic word
    let mut magic = [0u8; 7];
    file.read_exact(&mut magic).unwrap();
    if &magic != MAGIC_WORD {
        eprintln!("Error: Invalid magic word (Found: {:?})", magic);
        std::process::exit(1);
    }

    // Read version
    let mut cursor_buf = [0u8; 4];
    file.read_exact(&mut cursor_buf).unwrap();
    let version = u32::from_le_bytes(cursor_buf);
    if version != VERSION {
        eprintln!("Warning: Unexpected version {}", version);
    }

    // Read header
    file.read_exact(&mut cursor_buf).unwrap();
    let header_size = u32::from_le_bytes(cursor_buf) as usize;

    let mut header_bytes = vec![0u8; header_size];
    file.read_exact(&mut header_bytes).unwrap();

    let header = bek_format::CompressedGeometryHeader::decode(&header_bytes[..]).unwrap();

    let count = header.vertex_count as usize;
    let block_size = header.block_size as usize;
    let hop_size = header.hop_size as usize;

    let overlap_str = if hop_size > 0 && hop_size < block_size {
        format!(", hop {} (OLA)", hop_size)
    } else {
        String::new()
    };
    println!("  Geometry: {} vertices, block {}{}", count, block_size, overlap_str);

    if !header.dropped_channels.is_empty() {
        println!("  Dropped channels: {}", header.dropped_channels.join(", "));
    }

    // ===== Build property list and vertex data =====
    // We'll collect all property names and their values
    let mut property_names: Vec<String> = Vec::new();
    let mut property_values: std::collections::HashMap<String, Vec<f64>> = std::collections::HashMap::new();

    // ===== Dequantize positions and SH =====
    for q_chan in &header.quantized_channels {
        property_names.push(q_chan.name.clone());

        if q_chan.name == "x" || q_chan.name == "y" || q_chan.name == "z" {
            println!("  Dequantizing position '{}' (uint32 → float32)...", q_chan.name);
            let recon = quantize::dequantize_position(&q_chan.data, q_chan.min_val, q_chan.max_val, count);
            property_values.insert(q_chan.name.clone(), recon);
        } else {
            println!("  Dequantizing SH '{}' (uint8 → float32)...", q_chan.name);
            let raw = quantize::zstd_decompress(&q_chan.data, count + 1024);
            let range_val = q_chan.max_val - q_chan.min_val + 1e-10;
            let recon: Vec<f64> = raw
                .iter()
                .map(|&b| (b as f64 / 255.0) * range_val + q_chan.min_val)
                .collect();
            property_values.insert(q_chan.name.clone(), recon);
        }
    }

    // ===== Uncompressed channels =====
    for u_chan in &header.uncompressed_channels {
        println!("  Unpacking uncompressed channel '{}'...", u_chan.name);
        property_names.push(u_chan.name.clone());

        let raw = quantize::zstd_decompress(&u_chan.data, count * 4 + 1024);

        let recon: Vec<f64> = if u_chan.dtype == "float16" {
            // Parse as f16 (2 bytes each)
            raw.chunks_exact(2)
                .map(|chunk| {
                    let f = half::f16::from_le_bytes([chunk[0], chunk[1]]);
                    f.to_f64()
                })
                .collect()
        } else {
            // Assume float32
            raw.chunks_exact(4)
                .map(|chunk| f32::from_le_bytes([chunk[0], chunk[1], chunk[2], chunk[3]]) as f64)
                .collect()
        };

        property_values.insert(u_chan.name.clone(), recon);
    }

    // ===== Vector Quantized channels =====
    for vq_group in &header.vector_quantized_groups {
        let channels: Vec<String> = vq_group.channel_names.clone();
        println!("  Unpacking Vector Quantized group ({})...", channels.join(", "));

        // Decompress indices
        let raw_indices = quantize::zstd_decompress(&vq_group.indices, count * 2 + 1024);
        let indices: Vec<u16> = raw_indices
            .chunks_exact(2)
            .map(|chunk| u16::from_le_bytes([chunk[0], chunk[1]]))
            .collect();

        // Parse codebook
        let num_channels = channels.len();
        let codebook: Vec<f32> = vq_group
            .codebook
            .chunks_exact(4)
            .map(|chunk| f32::from_le_bytes([chunk[0], chunk[1], chunk[2], chunk[3]]))
            .collect();
        let num_clusters = codebook.len() / num_channels;

        // Reconstruct
        for (ci, c_name) in channels.iter().enumerate() {
            property_names.push(c_name.clone());
            let recon: Vec<f64> = indices
                .iter()
                .map(|&idx| codebook[(idx as usize) * num_channels + ci] as f64)
                .collect();
            property_values.insert(c_name.clone(), recon);
        }
    }

    // ===== DCT compressed channels =====
    for c_chan in &header.compressed_channels {
        println!("  Decoding DCT channel '{}'...", c_chan.name);
        property_names.push(c_chan.name.clone());

        let payload_size = c_chan.arithmetic_payload_bytes as usize;
        let mut compressed_bytes = vec![0u8; payload_size];
        file.read_exact(&mut compressed_bytes).unwrap();

        let recon = dct_utils::decompress_channel(
            &compressed_bytes,
            &c_chan.quantization_array,
            block_size,
            count,
            hop_size,
        );

        property_values.insert(c_chan.name.clone(), recon);
    }

    // ===== Dropped channels (fill with zeros) =====
    for d_name in &header.dropped_channels {
        println!("  Filling dropped channel '{}' with zeros...", d_name);
        property_names.push(d_name.clone());
        property_values.insert(d_name.clone(), vec![0.0; count]);
    }

    // ===== Build vertices =====
    let ply_data = ply_io::PlyVertexData {
        property_names: property_names.clone(),
        properties: property_values,
        count,
    };

    // ===== Write PLY =====
    println!("\nWriting decoded PLY to '{}'...", cli.output);
    ply_io::write_ply(&cli.output, &property_names, &ply_data).unwrap_or_else(|e| {
        eprintln!("Error: {}", e);
        std::process::exit(1);
    });

    println!("Finished successfully in {:.2} seconds.", start.elapsed().as_secs_f64());
}
