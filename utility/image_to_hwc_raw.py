import cv2
import numpy as np
import tensorflow as tf
import hashlib 
# Load image in HWC format
image = cv2.imread("data/pacman.jpg")        # BGR by default
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)  # convert to RGB

H, W, C = image.shape
print("Shape:", image.shape)  # (H, W, C)


# --- Dump HWC raw bytes ---
hwc_bytes = image.astype(np.float32).tobytes()
with open("data/image_hwc_512x512.raw", "wb") as f:
    f.write(hwc_bytes)
