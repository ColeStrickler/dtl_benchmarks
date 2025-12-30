import cv2
import numpy as np

# --- Load image in HWC format ---
image = cv2.imread("data/64x64.png")          # BGR by default
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

H, W, C = image.shape
print("Original shape (HWC):", image.shape)  # (H, W, C)

# --- Convert HWC → CHW ---
image_chw = np.transpose(image, (2, 0, 1))  # shape: (C, H, W)
print("Converted shape (CHW):", image_chw.shape)

# --- Convert to float32 in [0,1] ---
image_chw = image_chw.astype(np.float32) / 255.0

# --- Dump CHW raw bytes ---
with open("data/image_batch2space_64x64.raw", "wb") as f:
    f.write(image_chw.tobytes())
