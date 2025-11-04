import cv2
import numpy as np

# Load grayscale image
img = cv2.imread("data/1024x1024.jpg", cv2.IMREAD_GRAYSCALE)
if img is None:
    raise RuntimeError("Failed to load image")

# Convert to float32 and normalize
img = img.astype(np.float32) / 255.0

# Write raw bytes (no header, no shape)
img.tofile("data/1024x1024.raw")

