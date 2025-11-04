import numpy as np
import cv2
import imageio.v2 as imageio

size = 512
kernel = 2

rows = (size + 2*0 -  kernel) // 1 + 1
cols = (size + 2*0 -  kernel) // 1 + 1
print(f"rowz {rows}, col {cols}")

# Load raw float32 bytes
img = np.fromfile("data/conv2d_tme_512x512_k2_out.raw", dtype=np.float32)
img = img.reshape((rows, cols))

# If you normalized before saving, values are in [0, 1]
# So multiply by 255 if you want to visualize as uint8
img_disp = (img * 255).astype(np.uint8)

# Show the image
cv2.imshow("Loaded Image", img_disp)
cv2.waitKey(0)

# Optionally save to confirm visually
cv2.imwrite("data/cat_grayscale_reconstructed.jpg", img_disp)
