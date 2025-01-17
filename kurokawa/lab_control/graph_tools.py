def ticks_visual(ax, **kwarg):
    """
    makes auto minor and major ticks for matplotlib figure
    makes minor and major ticks thicker and longer
    """
    which = kwarg.get("which", "both")
    from matplotlib.ticker import AutoMinorLocator

    if which == "both" or which == "x":
        ax.xaxis.set_minor_locator(AutoMinorLocator())
    if which == "both" or which == "y":
        ax.yaxis.set_minor_locator(AutoMinorLocator())

    l1 = kwarg.get("l1", 7)
    l2 = kwarg.get("l2", 4)
    w1 = kwarg.get("w1", 1.0)
    w2 = kwarg.get("w2", 0.8)
    ax.xaxis.set_tick_params(width=w1, length=l1, which="major")
    ax.xaxis.set_tick_params(width=w2, length=l2, which="minor")
    ax.yaxis.set_tick_params(width=w1, length=l1, which="major")
    ax.yaxis.set_tick_params(width=w2, length=l2, which="minor")
    return


def grid_visual(ax, alpha=[0.1, 0.3]):
    """
    Sets grid on and adjusts the grid style.
    """
    ax.grid(which="minor", linestyle="-", alpha=alpha[0])
    ax.grid(which="major", linestyle="-", alpha=alpha[1])
    return


def gritix(**kws):
    """
    Automatically apply ticks_visual and grid_visual to the
    currently active pylab axes.
    """
    import matplotlib.pyplot as plt

    ticks_visual(plt.gca())
    grid_visual(plt.gca())
    return


def font_setup(size=28, weight="normal", family="serif"):
    import matplotlib.pyplot as plt
    from matplotlib import rc

    plt.rcParams["font.family"] = family
    plt.rcParams["font.size"]   = size
    plt.rcParams["font.weight"] = weight
    rc('mathtext', **{'rm': 'serif',
                      'it': 'serif:itelic',
                      'bf': 'serif:bold',
                      'fontset': 'cm'})