import numpy as np
from PIL import Image
import os

# === User-specified parameters ===
path = "data/hwc_512x512_k3_tme_out.raw"  # path to your raw file
N, C, H, W = 8, 3, 510, 510  # example dimensions
dtype = np.float32  # assuming 32-bit floats

# === Read the raw data ===
data = np.fromfile(path, dtype=dtype)

# Sanity check
expected_size = N * C * H * W
if data.size != expected_size:
    raise ValueError(f"File size mismatch! Expected {expected_size} floats, got {data.size}")

# === Reshape to (N, C, H, W) ===
data = data.reshape(N, C, H, W)

# === Convert to images and save ===
os.makedirs("recovered_images", exist_ok=True)
for i in range(N):
    img = data[i]
    # NCHW → HWC for display/saving
    img = np.transpose(img, (1, 2, 0))

    # Optional: normalize or clip to [0, 1]
    img = np.clip(img, 0.0, 1.0)

    # Convert to uint8
    img_uint8 = (img * 255).astype(np.uint8)
    Image.fromarray(img_uint8).save(f"recovered_images/img_{i:04d}.png")
