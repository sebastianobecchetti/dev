/// Morton Z-order curve sorting for 3D points.
/// Provides much better spatial locality than lexsort.

/// Spread bits of a 10-bit integer for 3D Morton encoding.
fn part1by2(mut n: u64) -> u64 {
    n &= 0x3FF; // clamp to 10 bits
    n = (n | (n << 16)) & 0x030000FF;
    n = (n | (n << 8)) & 0x0300F00F;
    n = (n | (n << 4)) & 0x030C30C3;
    n = (n | (n << 2)) & 0x09249249;
    n
}

/// Compute Morton code for a single 3D point (10-bit per axis).
fn morton_code(xi: u32, yi: u32, zi: u32) -> u64 {
    part1by2(xi as u64) | (part1by2(yi as u64) << 1) | (part1by2(zi as u64) << 2)
}

/// Sort 3D points using Morton Z-order curve.
/// Returns the index permutation that sorts the points.
///
/// Normalizes coordinates to 10-bit integers (0–1023) and interleaves
/// bits to produce 30-bit Morton codes.
pub fn morton_sort_3d(x: &[f64], y: &[f64], z: &[f64]) -> Vec<usize> {
    let n = x.len();
    assert_eq!(n, y.len());
    assert_eq!(n, z.len());

    let x_min = x.iter().cloned().fold(f64::INFINITY, f64::min);
    let x_max = x.iter().cloned().fold(f64::NEG_INFINITY, f64::max);
    let y_min = y.iter().cloned().fold(f64::INFINITY, f64::min);
    let y_max = y.iter().cloned().fold(f64::NEG_INFINITY, f64::max);
    let z_min = z.iter().cloned().fold(f64::INFINITY, f64::min);
    let z_max = z.iter().cloned().fold(f64::NEG_INFINITY, f64::max);

    let x_range = x_max - x_min + 1e-8;
    let y_range = y_max - y_min + 1e-8;
    let z_range = z_max - z_min + 1e-8;

    // Compute Morton codes
    let mut codes: Vec<(u64, usize)> = (0..n)
        .map(|i| {
            let xi = ((x[i] - x_min) / x_range * 1023.0) as u32;
            let yi = ((y[i] - y_min) / y_range * 1023.0) as u32;
            let zi = ((z[i] - z_min) / z_range * 1023.0) as u32;
            (morton_code(xi, yi, zi), i)
        })
        .collect();

    codes.sort_unstable_by_key(|&(code, _)| code);

    codes.into_iter().map(|(_, idx)| idx).collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_morton_sort_basic() {
        let x = vec![0.0, 1.0, 0.5, 0.25];
        let y = vec![0.0, 1.0, 0.5, 0.25];
        let z = vec![0.0, 1.0, 0.5, 0.25];
        let idx = morton_sort_3d(&x, &y, &z);
        assert_eq!(idx.len(), 4);
        // Origin should come first
        assert_eq!(idx[0], 0);
    }

    #[test]
    fn test_part1by2() {
        assert_eq!(part1by2(0), 0);
        assert_eq!(part1by2(1), 1);
        // bit 1 should be spread to bit 3
        assert_eq!(part1by2(0b11), 0b001001);
    }
}
