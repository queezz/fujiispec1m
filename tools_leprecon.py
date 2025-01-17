def qms_file_parser(filepath):
    """" Given filepath to ULVAC's Qulee BGM QMS file, converted to *.csv,
    Parses the header, extracts column names, QMS settings, and line number where data starts
    """
    import codecs

    channels = []
    mlist = []
    params = dict([])
    with codecs.open(filepath, "r", "shift_jisx0213") as f:
        for index, line in enumerate(f):
            string = line.strip()
            # get masslist
            if string.startswith(u'"測定質量数 '):
                masslist = string.split('"')[2].split(",")
                mlist = []
                for m in masslist:
                    if m != u"" and m != u"--":
                        mlist.append(int(m))
            if string.startswith(u'"測定スピード'):
                channels.append(string)
            if string.startswith(u'"SEM 電圧'):
                params["sem"] = string.split(",")[1]

            if string.startswith(u'"選択 FIL'):
                params["filament"] = string.split(",")[1][1:-1].strip()
            if string.startswith(u'"選択 SEM / FC'):
                params["semfil"] = string.split(",")[1][1:-1].strip()
            if string.startswith(u'"測定開始日時 '):
                params["start"] = string.split(",")[1].strip()
            if string.startswith(u'"測定終了日時 '):
                params["end"] = string.split(",")[1].strip()
            if string.startswith(u'"レシピ名称  '):
                try:
                    params["recipe"] = str(string.split(",")[1][1:-1]).strip()
                except NameError:
                    params["recipe"] = str(string.split(",")[1][1:-1]).strip()
            if string.startswith(u'"イオン化電圧 '):
                params["ionization"] = string.split(",")[1].strip()
            if string.startswith(u'"測定スピード   '):
                params["mass_sampling"] = string.split(",")[1].strip()

            if string.startswith("1"):
                break
        chName = ["No", "Time", "Trigger", "analog2", "qmsTP"]
        for m in mlist:
            chName.append("m%d" % m)

    return {"colnames": chName, "qms parameters": params, "skiprows": index}


def qms_csv(filepath):
    """
    Reads QULEE csv file and returns full data and
    QMS operation parameters.
    """
    from pandas import read_csv

    p = qms_file_parser(filepath)

    data = read_csv(
        filepath,
        skiprows=p["skiprows"],
        delimiter=",",
        names=p["colnames"],
        usecols=p["colnames"],
        na_values=["-------", "-"],
    )
    return {
        "data": data,
        "colnames": p["colnames"],
        "qms parameters": p["qms parameters"],
    }


def t2s(t):
    """
    Converts QMS timing into seconds.
    """
    import os, datetime, time

    ms = t.split(".")[1]
    hh = int(t.split(".")[0].split(":")[0])
    mm = t.split(".")[0].split(":")[1]
    ss = t.split(".")[0].split(":")[2]
    hoffset = 0
    if int(t.split(".")[0].split(":")[0]) > 23:
        hoffset = hh // 24
        tt = "0%d:%s:%s" % (hh - hoffset * 24, mm, ss)
    else:
        tt = "0%d:%s:%s" % (hh, mm, ss)
    x = time.strptime(tt, "0%H:%M:%S")
    return (
        datetime.timedelta(
            hours=hoffset * 24 + x.tm_hour, minutes=x.tm_min, seconds=x.tm_sec
        ).total_seconds()
        + float(ms) * 1e-3
    )


def t2sa(ta):
    """
    convert an array of strings of the Qulee format: '000:00:00.625' to time in seconds.
    '000:00:00.625' -> 'hhh:mm:ss.ms'
    """
    import numpy as np

    return np.array([t2s(tt) for tt in ta])


# Matploblib adjustments
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
