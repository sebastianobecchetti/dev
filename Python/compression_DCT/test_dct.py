import numpy as np
from scipy.fft import dct, idct

# Create a block of ones
block_size = 10
ratio = 0.5
cutoff = 5
data = np.ones(block_size)

# DCT norm forward
transformed = dct(data, type=2, norm='forward')
truncated = transformed[:cutoff]
recon = idct(truncated, type=2, norm='forward')
print("Original:", data)
print("Recon ones:", recon)

# Check if magnitudes are preserved
data2 = np.arange(10, dtype=np.float64)
t2 = dct(data2, type=2, norm='forward')
r2 = idct(t2[:cutoff], type=2, norm='forward')
print("Original ramp:", data2)
print("Recon ramp:", r2)
