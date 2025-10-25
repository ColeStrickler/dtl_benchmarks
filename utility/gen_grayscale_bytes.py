import cv2
import numpy as np

# Load grayscale image
img = cv2.imread("data/cat.gif", cv2.IMREAD_GRAYSCALE)
if img is None:
    raise RuntimeError("Failed to load image")

# Convert to float32 and normalize
img = img.astype(np.float32) / 255.0

# Write raw bytes (no header, no shape)
img.tofile("data/cat_grayscale.raw")

