import numpy as np
from PIL import Image
import sys


def rmse(im1, im2):
    px1 = np.array(im1) / 255.
    px2 = np.array(im2) / 255.
    
    total = (px1-px2)**2

    return total.mean()**0.5


if __name__ == '__main__':
    print(sys.argv)

    if len(sys.argv) != 3:
        print("Please pass in two image files")
    else:
        im1 = Image.open(sys.argv[1])
        im2 = Image.open(sys.argv[2])

        print(rmse(im1, im2))
