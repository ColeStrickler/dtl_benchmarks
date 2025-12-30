import numpy as np
import cv2

C = 3
H = 64-4
W = 64*8-4  # batch_size * single image width

# load raw float32 file
img = np.fromfile("data/batch2space_64x64_tme_out.raw", dtype=np.float32)

# reshape CHW
img = img.reshape((C, H, W))

# CHW → HWC
img = np.transpose(img, (1, 2, 0))

# scale to 0–255
img_disp = (img * 255).clip(0, 255).astype(np.uint8)

# convert RGB → BGR for OpenCV
img_disp = cv2.cvtColor(img_disp, cv2.COLOR_RGB2BGR)

# show
cv2.imshow("Glued batch", img_disp)
cv2.waitKey(0)
