import numpy as np
import struct
import zstandard as zstd
import time
import sys
import argparse
from plyfile import PlyData, PlyElement
from scipy.fft import idct
import bek_format_pb2

MAGIC_WORD = b'BEK3DGS'
VERSION = 1

def decode_rle(rle_data):
    decoded = []
    i = 0
    n = len(rle_data)
    while i < n:
        val = rle_data[i]
        if val == 0:
            count = rle_data[i+1]
            decoded.extend([0] * count)
            i += 2
        else:
            decoded.append(val)
            i += 1
    return np.array(decoded, dtype=np.float64)

def dequantize_position(compressed_bytes, min_val, max_val, count):
    """Dequantize uint32 delta-encoded positions back to float64."""
    raw = zstd.ZstdDecompressor().decompress(compressed_bytes)
    delta = np.frombuffer(raw, dtype=np.int64)
    quantized = np.cumsum(delta).astype(np.float64)
    range_val = max_val - min_val + 1e-10
    return (quantized / 4294967295.0) * range_val + min_val

def decompress_channel(compressed_bytes, q_array, block_size, count, hop_size=0):
    rle_bytes = zstd.ZstdDecompressor().decompress(compressed_bytes)
    rle_data = np.frombuffer(rle_bytes, dtype=np.int32)
    quantized_flat = decode_rle(rle_data)
    
    q_array_np = np.array(q_array)
    
    use_overlap = (hop_size > 0 and hop_size < block_size)
    if not use_overlap:
        hop_size = block_size
    
    num_blocks = max(1, (count - block_size) // hop_size + 1)
    if (num_blocks - 1) * hop_size + block_size < count:
        num_blocks += 1
    
    expected_len = num_blocks * block_size
    
    if len(quantized_flat) != expected_len:
        print(f"    Warning: decoded length {len(quantized_flat)} != expected {expected_len}")
        if len(quantized_flat) < expected_len:
            quantized_flat = np.pad(quantized_flat, (0, expected_len - len(quantized_flat)))
        else:
            quantized_flat = quantized_flat[:expected_len]
    
    blocks = quantized_flat.reshape((num_blocks, block_size))
    
    if num_blocks > 1:
        blocks = np.cumsum(blocks, axis=0)
    
    dequantized = blocks * q_array_np
    recon_blocks = idct(dequantized, type=2, norm='ortho', axis=1)
    
    if use_overlap:
        window = np.sin(np.pi * (np.arange(block_size) + 0.5) / block_size)
        recon_blocks *= window
        
        padded_len = (num_blocks - 1) * hop_size + block_size
        output = np.zeros(padded_len)
        norm = np.zeros(padded_len)
        
        for i in range(num_blocks):
            start = i * hop_size
            output[start:start + block_size] += recon_blocks[i]
            norm[start:start + block_size] += window ** 2
        
        norm = np.maximum(norm, 1e-10)
        output /= norm
        return output[:count]
    else:
        return recon_blocks.flatten()[:count]

def main():
    parser = argparse.ArgumentParser(description="Decode a .bek file back into a standard .ply point cloud.")
    parser.add_argument('--input', type=str, required=True, help="Input .bek file.")
    parser.add_argument('--output', type=str, required=True, help="Output .ply file.")
    
    args = parser.parse_args()
    
    print(f"Reading '{args.input}'...")
    start_time = time.time()
    
    with open(args.input, 'rb') as f:
        magic = f.read(7)
        if magic != MAGIC_WORD:
            print(f"Error: Invalid magic word (Found: {magic}).")
            sys.exit(1)
            
        ver_bytes = f.read(4)
        version = struct.unpack('<I', ver_bytes)[0]
        if version != VERSION:
            print(f"Warning: Unexpected version {version}.")
            
        size_bytes = f.read(4)
        header_size = struct.unpack('<I', size_bytes)[0]
        header_bytes = f.read(header_size)
        header = bek_format_pb2.CompressedGeometryHeader()
        header.ParseFromString(header_bytes)
        
        count = header.vertex_count
        block_size = header.block_size
        hop_size = header.hop_size
        
        overlap_str = f", hop {hop_size} (OLA)" if hop_size > 0 and hop_size < block_size else ""
        print(f"  Geometry: {count} vertices, block {block_size}{overlap_str}")
        
        if header.dropped_channels:
            print(f"  Dropped channels: {', '.join(header.dropped_channels)}")
        
        # Build dtype list from ALL channel types
        dtypes = []
        for q_chan in header.quantized_channels:
            dtypes.append((q_chan.name, 'float32'))  # positions always float32 in PLY
        for u_chan in header.uncompressed_channels:
            if u_chan.dtype == 'float16':
                dtypes.append((u_chan.name, 'float32'))
            else:
                dtypes.append((u_chan.name, u_chan.dtype))
        for c_chan in header.compressed_channels:
            if c_chan.dtype == 'float16':
                dtypes.append((c_chan.name, 'float32'))
            else:
                dtypes.append((c_chan.name, c_chan.dtype))
        for d_name in header.dropped_channels:
            dtypes.append((d_name, 'float32'))
            
        # Add VQ channels (always float32)
        for vq_group in header.vector_quantized_groups:
            for c_name in vq_group.channel_names:
                dtypes.append((c_name, 'float32'))
            
        reconstructed_data = np.empty(count, dtype=dtypes)
        
        # Dequantize (uint32 for positions, uint8 for SH)
        for q_chan in header.quantized_channels:
            if q_chan.name in ['x', 'y', 'z']:
                print(f"  Dequantizing position '{q_chan.name}' (uint32 → float32)...")
                recon = dequantize_position(q_chan.data, q_chan.min_val, q_chan.max_val, count)
                reconstructed_data[q_chan.name] = recon.astype(np.float32)
            else:
                print(f"  Dequantizing SH '{q_chan.name}' (uint8 → float32)...")
                raw = zstd.ZstdDecompressor().decompress(q_chan.data)
                quantized = np.frombuffer(raw, dtype=np.uint8).astype(np.float32)
                range_val = q_chan.max_val - q_chan.min_val + 1e-10
                recon = (quantized / 255.0) * range_val + q_chan.min_val
                reconstructed_data[q_chan.name] = recon.astype(np.float32)
        
        # Uncompressed channels
        for u_chan in header.uncompressed_channels:
            print(f"  Unpacking uncompressed channel '{u_chan.name}'...")
            raw_data = zstd.ZstdDecompressor().decompress(u_chan.data)
            np_arr = np.frombuffer(raw_data, dtype=u_chan.dtype)
            
            # If the channel was float16 compressed, we must cast back to float32 for the PLY output
            if u_chan.dtype == 'float16':
                reconstructed_data[u_chan.name] = np_arr.astype(np.float32)
            else:
                reconstructed_data[u_chan.name] = np_arr
                
        # Vector Quantized channels
        for vq_group in header.vector_quantized_groups:
            channels = list(vq_group.channel_names)
            print(f"  Unpacking Vector Quantized group ({', '.join(channels)})...")
            
            # Decompress indices and reshape codebook
            raw_indices = zstd.ZstdDecompressor().decompress(vq_group.indices)
            indices = np.frombuffer(raw_indices, dtype=np.uint16)
            
            codebook = np.frombuffer(vq_group.codebook, dtype=np.float32)
            num_channels = len(channels)
            codebook = codebook.reshape(-1, num_channels)
            
            # Reconstruct the features by looking up the codebook
            reconstructed_features = codebook[indices]
            
            # Scatter back to the structured array
            for i, c_name in enumerate(channels):
                reconstructed_data[c_name] = reconstructed_features[:, i]
            
        # DCT compressed channels
        for c_chan in header.compressed_channels:
            print(f"  Decoding DCT channel '{c_chan.name}'...")
            payload_size = c_chan.arithmetic_payload_bytes
            compressed_bytes = f.read(payload_size)
            
            recon_arr = decompress_channel(
                compressed_bytes, c_chan.quantization_array, 
                block_size, count, hop_size=hop_size
            )
            
            if 'int' in c_chan.dtype or 'uint' in c_chan.dtype:
                 recon_arr = np.clip(np.round(recon_arr), 0, 255)
                 
            reconstructed_data[c_chan.name] = recon_arr.astype(c_chan.dtype)
        
        # Fill dropped channels with zeros
        for d_name in header.dropped_channels:
            print(f"  Filling dropped channel '{d_name}' with zeros...")
            reconstructed_data[d_name] = np.zeros(count, dtype=np.float32)
    
    print(f"\nWriting decoded PLY to '{args.output}'...")
    el = PlyElement.describe(reconstructed_data, 'vertex')
    PlyData([el], text=False).write(args.output)
    
    print(f"Finished successfully in {time.time()-start_time:.2f} seconds.")

if __name__ == '__main__':
    main()
