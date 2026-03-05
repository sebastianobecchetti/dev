import numpy as np
from scipy.fft import dct, idct

data = np.ones(10)
print("Data:", data)

# Method 1: norm='ortho'
t = dct(data, type=2, norm='ortho')
r = idct(t[:5], type=2, norm='ortho')
r = r * np.sqrt(5 / 10)
print("Ortho + Correction:", r)

# Method 2: Let's test a ramp
ramp = np.arange(10, dtype=np.float64)
# mean is 4.5
t2 = dct(ramp, type=2, norm='ortho')
r2 = idct(t2[:5], type=2, norm='ortho') * np.sqrt(5 / 10)
print("Ramp original:", ramp)
print("Ramp resampled:", r2)
