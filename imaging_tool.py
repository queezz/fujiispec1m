"""
Useful functins for imagin
by A. Kuzmin 2020/08
"""

from FLI import FLI
import numpy as np
import thr640
import time
import logging
import csv
import xarray as xr
import matplotlib.pyplot as plt


def plot_image(image_data, **kws):
    """ plot image data"""
    fig, axs = plt.subplots(2, 1)
    fig.set_size_inches([10, 8])

    axs[1].imshow(image_data)
    axs[0].plot(image_data.sum(axis=0), "k")

    xlim = kws.get("xlim", [0, 2058])
    [ax.set_xlim(*xlim) for ax in axs]


if __name__ == "__main__":
    pass
