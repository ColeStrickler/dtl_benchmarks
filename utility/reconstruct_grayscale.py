import numpy as np
import cv2
import imageio.v2 as imageio

rows = 512
cols = 512

# Load raw float32 bytes
img = np.fromfile("data/cat_gaussian.raw", dtype=np.float32)
img = img.reshape((rows, cols))

# If you normalized before saving, values are in [0, 1]
# So multiply by 255 if you want to visualize as uint8
img_disp = (img * 255).astype(np.uint8)

# Show the image
cv2.imshow("Loaded Image", img_disp)
cv2.waitKey(0)

# Optionally save to confirm visually
cv2.imwrite("data/cat_grayscale_reconstructed.jpg", img_disp)
