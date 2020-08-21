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


def log_progress(sequence, every=None, size=None, name="Items"):
    from ipywidgets import IntProgress, HTML, VBox
    from IPython.display import display

    is_iterator = False
    if size is None:
        try:
            size = len(sequence)
        except TypeError:
            is_iterator = True
    if size is not None:
        if every is None:
            if size <= 200:
                every = 1
            else:
                every = int(size / 200)  # every 0.5%
    else:
        assert every is not None, "sequence is iterator, set every"

    if is_iterator:
        progress = IntProgress(min=0, max=1, value=1)
        progress.bar_style = "info"
    else:
        progress = IntProgress(min=0, max=size, value=0)
    label = HTML()
    box = VBox(children=[label, progress])
    display(box)

    index = 0
    try:
        for index, record in enumerate(sequence, 1):
            if index == 1 or index % every == 0:
                if is_iterator:
                    label.value = "{name}: {index} / ?".format(name=name, index=index)
                else:
                    progress.value = index
                    label.value = u"{name}: {index} / {size}".format(
                        name=name, index=index, size=size
                    )
            yield record
    except:
        progress.bar_style = "danger"
        raise
    else:
        progress.bar_style = "success"
        progress.value = index
        label.value = "{name}: {index}".format(name=name, index=str(index or "?"))


if __name__ == "__main__":
    pass
