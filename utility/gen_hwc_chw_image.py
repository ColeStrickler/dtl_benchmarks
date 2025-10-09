import cv2
import numpy as np
import tensorflow as tf
import hashlib 
# Load image in HWC format
image = cv2.imread("data/test_image.jpg")        # BGR by default
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)  # convert to RGB

H, W, C = image.shape
print("Shape:", image.shape)  # (H, W, C)


# --- Dump HWC raw bytes ---
hwc_bytes = image.astype(np.float32).tobytes()
with open("data/image_hwc.raw", "wb") as f:
    f.write(hwc_bytes)


chw_image = np.transpose(image, (2, 0, 1))  # C x H x W
chw_bytes = chw_image.astype(np.float32).tobytes()
with open("data/image_chw.raw", "wb") as f:
    f.write(chw_bytes)

