
import matplotlib.pyplot as plt
import matplotlib as mpl
from os.path import join,exists
from numpy.lib.function_base import average
import xarray as xr
import pandas as pd
from pandas import read_csv
import tools_leprecon as tools
import numpy as np
from lmfit.models import GaussianModel, ConstantModel
import os
from os.path import join,exists
import re

class mkses:
    basepath = ""
    photopath = ""
    basefiles = 0
    rotation = 0
    wavelength = 0
    balmers = np.array([656.279,486.135,434.047])
    rs = np.array([44933,83562,93096])
    fun = np.poly1d(np.polyfit(rs,balmers,2))

    # def __init__(self, path):
    #     mkse_values.basepath = path
    
    def basepathis(a):
        mkses.basepath = a
        mkses.basefiles = os.listdir(a)
        
    def photopathis(a):
        mkses.photopath = a

    def rotationis(a):
        mkses.rotation = a
        if a == 400000:
            mkses.wavelength = 656.279
        if a == 585000:
            mkses.wavelength = 486.135
        if a == 635000:
            mkses.wavelength = 434.047

def display_nc(name, xlimit = 0, ylimit = 0, disp = 0):
    numbers = re.findall(r"\d+", name)
    rotation_number = []
    for i in range(len(numbers)):
        numbers[i] = int(numbers[i])
        if numbers[i] // 5000 != 0:
            rotation_number.append(numbers[i])
    mkses.rotationis(rotation_number[0])
    print(mkses.rotation)

    fp = join(mkses.basepath, name)
    data = np.array(xr.open_dataset(fp).to_array()[0].sum(axis=0)).astype(np.float64)
    data = np.stack([data, mkses.fun(np.arange(2048)+(mkses.rotation - 200000) * 1044.5 / 5000)], 0)
    if xlimit == 0:
        x = data[1]
        y = data[0]
    else:
        n = idx_of_the_nearest(data[1], xlimit)
        x = data[1][n[1]:n[0]]
        y = data[0][n[1]:n[0]]

    if disp == 0:
        # xdivision = 200
        tools.font_setup(size=28)
        fig = plt.figure(figsize=(16, 9), dpi=50)
        ax = fig.add_subplot(1,1,1, xlabel=r"${\rm Wavelength}{\ }{\rm (nm)}$", ylabel=r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
        ax.plot(data[1], data[0], '.-', label=name)
        ax.legend(bbox_to_anchor=(1, 1), loc='upper right', borderaxespad=0, fontsize=16)
        # ax.xaxis.set_major_locator(mpl.ticker.MultipleLocator(xdivision))
        # ax.yaxis.set_major_formatter(mpl.ticker.PercentFormatter((max(y) - (min(y)))*100, decimals=1, symbol=''))
        tools.ticks_visual(ax)
        tools.grid_visual(ax)
        if xlimit != 0:
            plt.xlim(xlimit)
        if ylimit != 0:
            plt.ylim(ylimit)
        else:
            # plt.ylim((min(y)), (max(y) * 1.1))
            pass
    
    return data

def display_nc_all():
    files = os.listdir(mkses.basepath)
    for file_name in files:
        numbers = re.findall(r"\d+", file_name)
        rotation_number = []
        for i in range(len(numbers)):
            numbers[i] = int(numbers[i])
            if numbers[i] // 5000 != 0:
                rotation_number.append(numbers[i])
        mkses.rotationis(rotation_number[0])
        display_nc(file_name)

def display_data(data, xlimit = 0, ylimit = 0, disp = 0):
    if xlimit == 0:
        x = data[1]
        y = data[0]
    else:
        n = idx_of_the_nearest(data[1], xlimit)
        x = data[1][n[1]:n[0]]
        y = data[0][n[1]:n[0]]

    if disp == 0:
        # xdivision = 200
        tools.font_setup(size=28)
        fig = plt.figure(figsize=(16, 9), dpi=50)
        ax = fig.add_subplot(1,1,1, xlabel=r"${\rm Wavelength}{\ }{\rm (nm)}$", ylabel=r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
        ax.plot(data[1], data[0], '.-')
        # ax.legend(bbox_to_anchor=(1, 1), loc='upper right', borderaxespad=0, fontsize=28)
        # ax.xaxis.set_major_locator(mpl.ticker.MultipleLocator(xdivision))
        ax.yaxis.set_major_formatter(mpl.ticker.PercentFormatter((max(y) - (min(y)))*100, decimals=1, symbol=''))
        tools.ticks_visual(ax)
        tools.grid_visual(ax)
        if xlimit != 0:
            plt.xlim(xlimit)
        if ylimit != 0:
            plt.ylim(ylimit)
        else:
            plt.ylim((min(y)), (max(y) * 1.1))
    
    return data

def calcgauss(data, rangeis, num_gauss = 1):
    n = idx_of_the_nearest(data[1], rangeis)
    x = data[1][n[1]:n[0]]
    y = data[0][n[1]:n[0]]

    if num_gauss == 1:
        mod = ConstantModel() + GaussianModel()
        params = mod.make_params()
        par_val = {
            'c' : min(y),
            'center' : x[np.argmax(y)],
            'sigma' : 0.005,
            'amplitude' : (max(y) - min(y)) / 200,
        }
        par_enable = True
        par_vary = { 
            'c' : par_enable,
            'center' : par_enable,
            'sigma' : par_enable,
            'amplitude' : par_enable,
        }
    elif num_gauss == 2:
        mod = ConstantModel() + GaussianModel(prefix='gauss1_') + GaussianModel(prefix='gauss2_')
        params = mod.make_params()
        par_val = {
            'c' : min(y),
            'gauss1_center' : x[np.argmax(y)],
            'gauss1_sigma' : 0.005,
            'gauss1_amplitude' : (max(y) - min(y)) / 200,
            'gauss2_center' : data[1][np.argmax(data[0])],
            'gauss2_sigma' : 0.01,
            'gauss2_amplitude' : (max(y) - min(y)) / 400,
        }
        par_enable = True
        par_vary = { 
            'c' : par_enable,
            'gauss1_center' : par_enable,
            'gauss1_sigma' : par_enable,
            'gauss1_amplitude' : par_enable,
            'gauss2_center' : par_enable,
            'gauss2_sigma' : par_enable,
            'gauss2_amplitude' : par_enable
        }
    for name in mod.param_names: 
        params[name].set(
            value=par_val[name], # 初期値
            vary=par_vary[name] # パラメータを動かすかどうか
        )
    out = mod.fit(data=y, x=x, params=params)
    return out

def gaussian(x, amp, cen, wid):
    """1-d gaussian: gaussian(x, amp, cen, wid)"""
    return (amp / (np.sqrt(2*np.pi) * wid)) * np.exp(-(x-cen)**2 / (2*wid**2))

def display_gaussian(data, out, rangeis, xlimit = 0, ylimit = 0, num_gauss = 1):
    x2 = np.linspace(data[1][0], data[1][-1], 10000)
    n = idx_of_the_nearest(data[1], rangeis)
    x = data[1][n[1]:n[0]]
    y = data[0][n[1]:n[0]]

    tools.font_setup(size=28)
    fig = plt.figure(figsize=(10, 8), dpi=50)
    ax = fig.add_subplot(1,1,1, xlabel=r"${\rm Wavelength}{\ }{\rm (nm)}$", ylabel=r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
    ax.plot(data[1], data[0] - out.best_values["c"], 'o', label=r"Data")
    if num_gauss == 1:
        ax.plot(x2, gaussian(x2, out.best_values["amplitude"], out.best_values["center"], out.best_values["sigma"]), '-', label=r'best fit')
    elif num_gauss == 2:
        ax.plot(x2, gaussian(x2, out.best_values["gauss1_amplitude"], out.best_values["gauss1_center"], out.best_values["gauss1_sigma"]), '-', label=r'Low temparature')
        ax.plot(x2, gaussian(x2, out.best_values["gauss2_amplitude"], out.best_values["gauss2_center"], out.best_values["gauss2_sigma"]), '-', label=r'High temparature')
        ax.plot(x2, gaussian(x2, out.best_values["gauss1_amplitude"], out.best_values["gauss1_center"], out.best_values["gauss1_sigma"]) + gaussian(x2, out.best_values["gauss2_amplitude"], out.best_values["gauss2_center"], out.best_values["gauss2_sigma"]), '-', label=r'Best fit')
    ax.legend(bbox_to_anchor=(1, 1), loc='upper right', borderaxespad=0, fontsize=16)
    ax.xaxis.set_major_locator(mpl.ticker.LinearLocator(3))
    ax.yaxis.set_major_formatter(mpl.ticker.PercentFormatter((max(y) - out.best_values["c"])*100, decimals=1, symbol=''))
    tools.ticks_visual(ax)
    tools.grid_visual(ax)
    if xlimit != 0:
        plt.xlim(xlimit)
    if ylimit != 0:
        plt.ylim(ylimit)
    else:
        plt.ylim((min(y)) - out.best_values["c"], (max(y) - out.best_values["c"]) * 1.1)
    
def calc_fwhm(name, rangeis, xlimit = None, ylimit = 0, data = None, num_gauss = 1):
    if data is None:
        data = display_nc(name, rangeis, disp=1)
    if xlimit is None:
        xlimit = rangeis
    out = calcgauss(data, rangeis, num_gauss=num_gauss)
    display_gaussian(data, out, rangeis=rangeis, xlimit=xlimit, ylimit=ylimit, num_gauss=num_gauss)
    if num_gauss == 1:    
        fwhm = 2 * out.best_values["sigma"] * np.sqrt(2 * np.log(2))
        print(f"FWHM {fwhm:.4f} nm   ", end="")
    elif num_gauss == 2:
        fwhm = [2 * out.best_values["gauss1_sigma"] * np.sqrt(2 * np.log(2)), 2 * out.best_values["gauss2_sigma"] * np.sqrt(2 * np.log(2)), ]
        print(f"FWHM {fwhm[0]:.4f} nm and {fwhm[1]:.4f} nm ", end="")
    return  fwhm

def subtract_nc(name1, name2):
    data_back = display_nc(name2, disp = 1)
    data_sig = display_nc(name1, disp = 1)

    for i in range(len(data_sig[0])):
        data_sig[0][i] = data_sig[0][i] - data_back[0][i]  
    
    return data_sig

def calc_temperature(fwhm1, fwhm2, explain = ""):
    if len(fwhm1) == 1:
        deltalam = (fwhm1**2 - fwhm2**2)**0.5
        atom_temparature = (deltalam / (7.16e-7 * mkses.wavelength))**2 * 1.0079
        print(f'{atom_temparature:.2f} K    {explain}')
    elif len(fwhm1) == 2:
        deltalam = [(fwhm1[0]**2 - fwhm2**2)**0.5, (fwhm1[1]**2 - fwhm2**2)**0.5]
        atom_temparature = [(deltalam[0] / (7.16e-7 * mkses.wavelength))**2 * 1.0079, (deltalam[1] / (7.16e-7 * mkses.wavelength))**2 * 1.0079]
        print(f'{atom_temparature[0]:.2f} K and {atom_temparature[1]:.2f} K   {explain}')

def idx_of_the_nearest(data, value):
    if type(value) == float:
        idx = np.argmin(np.abs(np.array(data) - value))
        #print(np.abs(np.array(data) - value))
        return idx
    if type(value) == list:
        idx = [None]*len(value)
        for i in range(len(value)):
            idx[i] = np.argmin(np.abs(np.array(data) - value[i]))
            #idx[i] = [value[i], np.argmin(np.abs(np.array(data) - value[i]))] #としてもよい
            #print(np.abs(np.array(data) - value[i]))
        return idx  

def read_raspi_data(basepath,day,index):
    raspipath = join(basepath, "raspi")
    ls_raspi = sorted([i for i in os.listdir(raspipath) if '.csv' in i and day in i])
    print("All raspi data is...")
    print(f"{ls_raspi}")
    print("Selected data is...")
    print(f"['{ls_raspi[index*2]}', '{ls_raspi[index*2+1]}']")
    raspi_data_t = pd.read_csv(join(raspipath,ls_raspi[index*2+1]),)
    raspi_data = pd.read_csv(join(raspipath,ls_raspi[index*2]),)
    pgas = 10.0**(1.667*raspi_data['P2'] - 11.46)       # 11.46 convert Pfeiffer signal to pressure
    pion = raspi_data['P1']*10.0**raspi_data['IGscale'] # convert linear IG signal to pressure
    ipla = 5.0*(raspi_data['Ip'] - 2.52)                # plasma current
    return raspi_data_t,raspi_data,pgas,pion,ipla

def read_qms_data(basepath, day):
    qmspath = join(basepath,'qms')
    ls_qms =  sorted([i for i in os.listdir(qmspath) if '.CSV' in i and day in i])
    print("All qms data is...")
    print(f"{ls_qms}")
    print("Selected data is...")
    print(f"['{ls_qms[0]}']")
    qms_data = tools.qms_csv(join(qmspath,ls_qms[0]))['data']
    qms_para = tools.qms_csv(join(qmspath,ls_qms[0]))['qms parameters']
    qms_data['tsec'] = tools.t2sa(qms_data['Time'].values)
    return qms_data

def qms_edge_check(qms_data, next_index, xlimit = 0):
    #first and next index
    qms_rising_index = []
    qms_falling_index = []
    for i in range(len(qms_data["Trigger"])-1):
        if qms_data["Trigger"][i+1] > (qms_data["Trigger"][i] + 1e-5):
            qms_rising_index.append(i)
            break
    qms_rising_index.extend(next_index[0])
    qms_falling_index.extend(next_index[1])

    # print
    print(qms_rising_index)
    print(qms_falling_index)

    # plot
    fig = plt.figure(figsize=(36, 4), dpi=150,facecolor="white")
    ax = fig.add_subplot(111)
    ax.plot(qms_data["m2"])
    if xlimit != 0:
        plt.xlim(xlimit)

    # plot vlines
    for i in qms_rising_index:
        plt.axvline(i, color="red")
    for i in qms_falling_index:
        plt.axvline(i, color="green")
    
    return [qms_rising_index]+[qms_falling_index]

def del_qms_signal(raspi_data, delrange):
    raspi_data["QMS_signal"][delrange[0]:delrange[1]] = [0] * (delrange[1]-delrange[0])

def raspi_edge_check(raspi_data, qms_data, del_num = 0, xlimit = 0):
    # get first index
    raspi_rising_index = []
    # raspi_falling_index = []
    for i in range(len(raspi_data["QMS_signal"])-1):
        if raspi_data["QMS_signal"][i+1] > (raspi_data["QMS_signal"][i]):
            raspi_rising_index.append(i)
        # if raspi_data["QMS_signal"][i+1] < (raspi_data["QMS_signal"][i]):
        #     raspi_falling_index.append(i)

    if del_num != 0:
        for i in range(del_num):
            del raspi_rising_index[0]
            # del raspi_falling_index[0]

    # print
    # print(raspi_rising_index)
    # print(raspi_falling_index)
    print(raspi_rising_index[0])
    
    # plot
    fig = plt.figure(figsize=(36, 12), dpi=150,facecolor="white")
    ax = fig.add_subplot(311)
    ax.plot(qms_data["Trigger"])
    ax = fig.add_subplot(312)
    ax.plot(raspi_data["QMS_signal"])
    if xlimit != 0:
        ax = fig.add_subplot(313)
        ax.plot(raspi_data["QMS_signal"])
        ax.set_xlim(xlimit)

    return raspi_rising_index[0]

def ipla_edge_check(ipla, ipla_index = 0, xlimit = 0):
    # plot
    fig = plt.figure(figsize=(36, 8), dpi=150,facecolor="white")
    ax = fig.add_subplot(1,1,1)
    ax.plot(ipla)
    for i in ipla_index[0]:
        plt.axvline(i, color="red")
    for i in ipla_index[1]:
        plt.axvline(i, color="green")
    if xlimit != 0:
        plt.xlim(xlimit)

def fft_filtering(sample, dt):
    # ローパスフィルタ
    N = len(sample)           # サンプル数
    # dt is サンプリング間隔   
    t = np.arange(0, N*dt, dt) # 時間軸
    freq = np.linspace(0, 1.0/dt, N) # 周波数軸

    fc = 0.1        # カットオフ周波数
    fs = 1 / dt     # サンプリング周波数
    # fm = (1/2) * fs # アンチエリアジング周波数
    fc_upper = fs - fc # 上側のカットオフ　fc～fc_upperの部分をカット

    # 元波形をfft
    F = np.fft.fft(sample)

    # 元波形をコピーする
    G = F.copy()

    # ローパス
    G[((freq > fc)&(freq< fc_upper))] = 0 + 0j

    # 高速逆フーリエ変換
    g = np.fft.ifft(G)

    # 実部の値のみ取り出し
    return np.array(g.real)

def ipla_start_end_qmsindex(raspi_data, qms_data, raspi_first_index, ipla_index, qms_index):
    

    ipla_start_qmstime = []
    ipla_end_qmstime = []
    ipla_start_qmsindex = []
    ipla_end_qmsindex = []
    qms_raspi_timediff = raspi_data['time'][raspi_first_index] - qms_data['tsec'][qms_index[0][0]]

    for i in range(len(ipla_index[1])):
        ipla_start_qmstime.append(raspi_data["time"][ipla_index[0][i]] - qms_raspi_timediff)
        ipla_end_qmstime.append(raspi_data["time"][ipla_index[1][i]] - qms_raspi_timediff)
    # print(plasma_end_qmstime)
    for j in range(len(ipla_end_qmstime)):
        for i in range(len(qms_data['tsec'])):
            if ipla_end_qmstime[j] < qms_data['tsec'][i]:
                ipla_end_qmsindex.append(i)
                break
    for j in range(len(ipla_start_qmstime)):
        for i in range(len(qms_data['tsec'])):
            if ipla_start_qmstime[j] < qms_data['tsec'][i]:
                ipla_start_qmsindex.append(i)
                break
    print(ipla_start_qmsindex)
    print(ipla_end_qmsindex)

    # ipla_start_qmsindex = [ipla_start_qmsindex[0]] * len(ipla_index[1])

    return ipla_start_qmsindex, ipla_end_qmsindex, qms_raspi_timediff

# def lookforspectrum():
#     pass

    