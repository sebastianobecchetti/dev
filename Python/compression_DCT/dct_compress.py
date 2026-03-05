import argparse
import sys
import numpy as np
import json
import os
from plyfile import PlyData, PlyElement
from scipy.fft import dct, idct
import time

def generate_quantization_array(block_size, q_factor):
    """
    Generates a 1D quantization array similar in spirit to JPEG.
    Lower frequencies get lower quantization values (preserve more precision).
    Higher frequencies get exponentially/linearly higher quantization values.
    """
    # Create indices from 0 to block_size - 1
    indices = np.arange(block_size)
    
    # Base quantization array: linearly increasing penalty for higher frequencies
    # This formula scales from 1 (DC component) up to 1 + block_size at the highest frequency
    base_q = 1.0 + (indices / float(block_size)) * (block_size - 1)
    
    # Apply the user's overall quantization factor parameter
    # q_factor of 1.0 means baseline. Larger q_factor means harsher quantization (more compression).
    q_array = base_q * q_factor
    
    # Prevent divide by zero or extreme preservation
    q_array = np.maximum(q_array, 1.0)
    return q_array

def generate_waterfilling_quantization_array(variances, distortion_d):
    """
    Generates a quantization array using Reverse Water-Filling principles.
    frequencies where variance <= D are "dropped" (Q -> infinity).
    For frequencies where variance > D, Q = sqrt(12 * D).
    """
    block_size = len(variances)
    q_array = np.zeros(block_size, dtype=np.float64)
    dropped_mask = np.zeros(block_size, dtype=bool)
    
    # Standard quantization error variance is Q^2 / 12
    # So if we want error variance to be D, Q = sqrt(12*D)
    target_q = np.sqrt(12.0 * distortion_d) if distortion_d > 0 else 1.0
    # Provide a floor to prevent dividing by tiny numbers
    target_q = max(target_q, 1.0)
    
    for i, var in enumerate(variances):
        if var <= distortion_d:
            # Water level is above the "terrain" (variance).
            # This frequency is completely submerged and should be dropped.
            dropped_mask[i] = True
            q_array[i] = 1.0 # arbitrary, since we will force the coefficient to 0 later
        else:
            # Frequency is above water level. It gets quantized with target Q.
            q_array[i] = target_q
            
    return q_array, dropped_mask

def apply_block_dct_compression(channel_data, q_factor, block_size, calc_stats=False, channel_name="", channel_variances=None):
    """
    Chunks 1D data into blocks, applies DCT, quantizes independently, and applies IDCT.
    This simulates JPEG-style lossy compression.
    Returns: (reconstructed_data, stats_dict, non_zero_count)
    """
    if q_factor <= 0.0 and not calc_stats:
        return channel_data, None, len(channel_data)
        
    N = len(channel_data)
    num_blocks = N // block_size
    remainder = N % block_size
    
    reconstructed = np.empty_like(channel_data, dtype=np.float64)
    
    stats = None
    non_zero_count = 0

    if num_blocks > 0:
        # Reshape main part into blocks
        main_data = channel_data[:num_blocks * block_size]
        blocks = main_data.reshape((num_blocks, block_size))
        
        # Apply DCT along the block axis (axis=1)
        # Shape is (num_blocks, block_size)
        transformed = dct(blocks, type=2, norm='ortho', axis=1)
        
        if calc_stats:
            # We want statistics PER FREQUENCY INDEX across all blocks.
            # So axis=0 averages across the num_blocks dimension.
            mean_coeffs = np.mean(transformed, axis=0)
            var_coeffs = np.var(transformed, axis=0)
            stats = {
                'channel': channel_name,
                'mean': mean_coeffs.tolist(),
                'variance': var_coeffs.tolist()
            }
        
        # If we are only calculating stats and not compressing, we can just skip the rest
        if q_factor <= 0.0:
             reconstructed[:num_blocks * block_size] = channel_data[:num_blocks * block_size]
        else:
            # Generate quantization array
            if channel_variances is not None:
                 q_array, dropped_mask = generate_waterfilling_quantization_array(channel_variances, q_factor)
            else:
                 q_array = generate_quantization_array(block_size, q_factor)
                 dropped_mask = None
            
            # Apply Quantization: Divide by Q, round to integer
            quantized = np.round(transformed / q_array)
            
            # Force dropped coefficients to absolute zero
            if dropped_mask is not None:
                quantized[:, dropped_mask] = 0.0
                
            non_zero_count += np.count_nonzero(quantized)
            
            # De-quantization: Multiply by Q
            dequantized = quantized * q_array
            
            # Apply IDCT along block axis
            recon_blocks = idct(dequantized, type=2, norm='ortho', axis=1)
            reconstructed[:num_blocks * block_size] = recon_blocks.flatten()
        
    if remainder > 0:
        # Process the remaining points as one smaller block
        rem_data = channel_data[num_blocks * block_size:]
        rem_transformed = dct(rem_data, type=2, norm='ortho')
        
        # Quantize remainder
        if channel_variances is not None:
            # Slice variances for the remainder block size
            rem_variances = channel_variances[:remainder]
            rem_q_array, rem_dropped_mask = generate_waterfilling_quantization_array(rem_variances, q_factor)
        else:
            rem_q_array = generate_quantization_array(remainder, q_factor)
            rem_dropped_mask = None
            
        rem_quantized = np.round(rem_transformed / rem_q_array)
        
        if rem_dropped_mask is not None:
            rem_quantized[rem_dropped_mask] = 0.0
            
        non_zero_count += np.count_nonzero(rem_quantized)
            
        rem_dequantized = rem_quantized * rem_q_array
        
        if q_factor > 0.0:
            rem_recon = idct(rem_dequantized, type=2, norm='ortho')
            reconstructed[num_blocks * block_size:] = rem_recon
        else:
             reconstructed[num_blocks * block_size:] = channel_data[num_blocks * block_size:]
             non_zero_count += remainder
        
    return reconstructed, stats, non_zero_count

def main():
    parser = argparse.ArgumentParser(description="Compress a 3D PLY file using Block-wise DCT with optional Reverse Water-Filling adaptive quantization.")
    parser.add_argument('--input', type=str, required=True, help="Path to the input .ply file.")
    parser.add_argument('--output', type=str, required=True, help="Path to save the output compressed .ply file.")
    parser.add_argument('--q_factor', type=float, default=10.0, help="Quantization factor. If using --stats_file, this represents the Distortion water-level 'D'. Otherwise, acts as a JPEG multiplier. 0 means no quantization. (default: 10.0)")
    parser.add_argument('--block_size', type=int, default=256, help="Size of the blocks for localized DCT processing. (default: 256)")
    parser.add_argument('--calc_stats', action='store_true', help="Calculate and save the mean and variance of the DCT coefficients to a JSON file.")
    parser.add_argument('--stats_file', type=str, default=None, help="Optional path to a previously generated JSON statistics file. If provided, enables Reverse Water-Filling quantization based on coefficient variances.")
    
    args = parser.parse_args()
    
    if args.q_factor < 0.0:
        print("Error: --q_factor must be >= 0.0")
        sys.exit(1)
        
    print(f"Loading '{args.input}'...")
    start_time = time.time()
    try:
        plydata = PlyData.read(args.input)
    except Exception as e:
        print(f"Failed to load ply file: {e}")
        sys.exit(1)
    
    vertex_element = plydata.elements[0]
    if vertex_element.name != 'vertex':
        print("Expected the first element to be 'vertex'.")
    
    vertex_data = vertex_element.data
    properties = [p.name for p in vertex_element.properties]
    
    count = len(vertex_data)
    print(f"Loaded {count} vertices in {time.time()-start_time:.2f} seconds.")
    
    # 1. Spatial Sorting
    # We must sort the points to ensure adjacent points in the array are close in 3D space.
    # We do this by quantizing points into a 3D grid and using lexsort.
    print("Spatially sorting vertices to preserve local topology...")
    sort_start = time.time()
    
    # Use float64 to prevent overflows parsing coordinates
    x = vertex_data['x'].astype(np.float64)
    y = vertex_data['y'].astype(np.float64)
    z = vertex_data['z'].astype(np.float64)
    
    x_range = np.max(x) - np.min(x) + 1e-8
    y_range = np.max(y) - np.min(y) + 1e-8
    z_range = np.max(z) - np.min(z) + 1e-8
    
    # 10-bit grid (1024^3 voxels)
    grid_size = 1024
    xi = ((x - np.min(x)) / x_range * grid_size).astype(np.int32)
    yi = ((y - np.min(y)) / y_range * grid_size).astype(np.int32)
    zi = ((z - np.min(z)) / z_range * grid_size).astype(np.int32)
    
    # Sort by X, then Y, then Z locally
    sort_idx = np.lexsort((zi, yi, xi))
    
    # Reorder all data!
    vertex_data = vertex_data[sort_idx]
    print(f"Finished sorting in {time.time()-sort_start:.2f} seconds.")

    # 2. Setup channels for DCT
    channels_to_compress = []
    
    if 'red' in properties and 'green' in properties and 'blue' in properties:
        channels_to_compress.extend(['red', 'green', 'blue'])
        
    if 'f_dc_0' in properties and 'f_dc_1' in properties and 'f_dc_2' in properties:
        channels_to_compress.extend(['f_dc_0', 'f_dc_1', 'f_dc_2'])
        
    # Only pick available properties
    channels_to_compress = [c for c in channels_to_compress if c in properties]
    
    print(f"\nChannels selected for DCT compression: {channels_to_compress}")
    print(f"Block size: {args.block_size}")
    if args.q_factor > 0.0:
        if args.stats_file:
            print(f"Distortion water level (D): {args.q_factor:.1f} (Using Reverse Water-Filling)")
        else:
            print(f"Quantization factor multiplier: {args.q_factor:.1f} (Using JPEG-like Linear Penalty)")
    else:
        print("Skipping compression (q_factor=0.0).")
        
    if args.calc_stats:
        print("Will compute and save coefficient statistics.")
        
    # Process stats file
    loaded_stats = None
    if args.stats_file:
        try:
            with open(args.stats_file, 'r') as f:
                loaded_stats = json.load(f)
            print(f"Loaded statistics from '{args.stats_file}'.")
        except Exception as e:
            print(f"Failed to load stats file: {e}")
            sys.exit(1)
    
    new_vertex_data = np.empty(count, dtype=vertex_data.dtype)
    
    # Copy unsorted/uncompressed properties intact
    for prop in properties:
        new_vertex_data[prop] = vertex_data[prop]
    
    
    # 3. Apply block-wise DCT
    all_stats = {}
    total_original_coeffs = 0
    total_nonzero_coeffs = 0
    
    for channel in channels_to_compress:
        print(f"  Processing channel '{channel}'...")
        chan_data = vertex_data[channel]
        
        # Get channel variances if available
        chan_vars = None
        if loaded_stats and channel in loaded_stats:
             chan_vars = loaded_stats[channel].get('variance')
             # Make sure it's the right length
             if chan_vars and len(chan_vars) != args.block_size:
                  print(f"    Warning: Block size ({args.block_size}) does not match stats variance length ({len(chan_vars)}). Falling back to generic quantization.")
                  chan_vars = None
                  
        # Ensure it's treated as float64 for DCT precision
        processed, stats, non_zero_count = apply_block_dct_compression(
            chan_data.astype(np.float64), 
            args.q_factor, 
            args.block_size,
            calc_stats=args.calc_stats,
            channel_name=channel,
            channel_variances=chan_vars
        )
        
        total_original_coeffs += len(chan_data)
        total_nonzero_coeffs += non_zero_count
        
        if stats:
            all_stats[channel] = stats
        
        # Cast back to original dtype
        original_dtype = vertex_data.dtype[channel]
        if np.issubdtype(original_dtype, np.integer):
            # Clip bounds for integers to prevent under/overflow artifacts
            iinfo = np.iinfo(original_dtype)
            processed_clipped = np.clip(np.round(processed), iinfo.min, iinfo.max)
            new_vertex_data[channel] = processed_clipped.astype(original_dtype)
        else:
            new_vertex_data[channel] = processed.astype(original_dtype)
            
    if args.calc_stats and all_stats:
        stats_file = f"{os.path.splitext(args.input)[0]}_dct_stats.json"
        print(f"\nSaving statistics to '{stats_file}'...")
        with open(stats_file, 'w') as f:
            json.dump(all_stats, f, indent=2)
            
    # 4. Save
    print(f"\nSaving compressed data to '{args.output}'...")
    save_start = time.time()
    
    new_vertex_element = PlyElement.describe(new_vertex_data, vertex_element.name)
    new_elements = [new_vertex_element] + list(plydata.elements)[1:]
    new_plydata = PlyData(new_elements, text=plydata.text)
    
    try:
        new_plydata.write(args.output)
    except Exception as e:
        print(f"Failed to write output PLY: {e}")
        sys.exit(1)
        
    print(f"Finished successfully in {time.time()-start_time:.2f} total seconds.")
    
    if args.q_factor > 0.0:
        print("\n--- Compression Status ---")
        if total_original_coeffs > 0:
            percentage_kept = (total_nonzero_coeffs / total_original_coeffs) * 100
            ratio = total_original_coeffs / max(1, total_nonzero_coeffs)
            print(f"Total Coefficients Processed: {total_original_coeffs}")
            print(f"Non-Zero Coefficients Kept:   {total_nonzero_coeffs} ({percentage_kept:.2f}%)")
            print(f"Zeroed Coefficients Dropped:  {total_original_coeffs - total_nonzero_coeffs}")
            print(f"Theoretical Ratio (Data):     {ratio:.2f}x")
            print("Note: Actual file size reduction requires entropy encoding (e.g. gzip) on the output file.")

if __name__ == '__main__':
    main()
