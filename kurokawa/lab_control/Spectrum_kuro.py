import numpy as np
import xarray as xr
from os.path import join,exists
from os import listdir
import matplotlib.pyplot as plt
import importlib
from graph_tools import *
import pandas as pd

balmers = np.array([777.194,656.279, 486.135, 434.0472, 410.1734]) #OI, Halph, Hbeta, Hgammma Hdelta
rs = np.array([4950, 52378,88590,97600,101540])
fun =  np.poly1d(np.polyfit(rs,balmers,2))
fwhm_function = {"OI":0.02117, "alpha":0.03594, "beta":0.04283, "gamma":0.04395, "delta":0.04503}


class Spectrometer(object):
    def __init__(self,path,exception_word=["test","background"]):
        ls = [i for i in listdir(path)]
        for word in exception_word:
            ls = [i for i in ls if not word in i]
        data_list = [convert_nc(join(path,i)) for i in ls]
        df = [[data_list[n], min(data_list[n][1]), data_list[n][1][np.argmax(data_list[n][0])],max(data_list[n][1]),get_rot(i),i,join(path,i)] for n,i in enumerate(ls)]
        # df 形の一覧表をつくる．columns = "data","wavelenght(min)",wavelength(median)", "wavelenght(max)","path"
        self.columns = ["data","wavelenght(min)","wavelength(spectrum)", "wavelenght(max)","rotation","title","path"]
        self.df = pd.DataFrame(data=df,columns=self.columns)
        self.df["name"] = self.df["wavelength(spectrum)"].map(self.spec_name)
        self.df = self.df.sort_values(by="rotation").reset_index(drop=True)
        self.df = self.df[["data","name","wavelenght(min)","wavelength(spectrum)", "wavelenght(max)","rotation","title","path"]]
        self.path = path

        # settings
        self.subtracted_flag = False
        pass

    def subtract(self,background_path=None,background_indicator="background",exception_word=["test"]):
        if not self.subtracted_flag:
            if not background_path:
                background_path = self.path
            ls = [i for i in listdir(background_path)]
            ls = [i for i in ls if background_indicator in i]
            for word in exception_word:
                ls = [i for i in ls if not word in i]
            for n in range(len(self.df)):
                background = [i for i in ls if str(self.df.loc[n]["rotation"]) in i]
                if background:
                    self.df.loc[n]["data"] = subtract_background(fp=self.df.loc[n]["data"],disp=False,bp=join(background_path,background[0]))
            self.subtracted_flag = True
        else:
            print("The data was already subtracted!")
                


    def display(self,style=True):
        # name 毎に横一列に並べて表示するのがよさげ？ 横最大4つで
        df_name = [self.df[self.df["name"] == i] for i in ["OI","alpha","beta","gamma","delta"]]
        for i in df_name:
            h = len(i)//4 + 1
            w = 4 if len(i) > 4 else len(i)
            fig = plt.figure(figsize=(8*w,8*h))
            # print(i["name"].reset_index(drop=True)[0])
            for n,j in enumerate(i["data"]):
                ax = fig.add_subplot(h,w,n+1,xlabel=r"${\rm Wavelength}{\ }{\rm (nm)}$", ylabel=r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
                display_nc(j,ax = ax,style=style)

    def gauss(self,num_gauss=[1,2,2,1,1],single=False,disp=True):
        # df_name = [self.df[self.df["name"] == i] for i in ["OI","alpha","beta","gamma","delta"]]
        sigma = []
        df_name = [self.df[self.df["rotation"] == i] for i in [212000,410000,595000,640000,662000]]
        df = pd.DataFrame(columns=["data","name","wavelenght(min)","wavelength(spectrum)", "wavelenght(max)","rotation","title","path"])
        for i,num in zip(df_name,num_gauss):
            h = len(i)//4 + 1
            w = 4 if len(i) > 4 else len(i)
            if not single and disp:
                fig = plt.figure(figsize=(8*w,8*h))
            for n,j in enumerate(i["data"]):
                if disp:
                    if single:
                        fig = plt.figure(figsize=(16,9))
                        ax = fig.add_subplot(1,1,1,xlabel=r"${\rm Wavelength}{\ }{\rm (nm)}$", ylabel=r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
                        title = i.reset_index(drop=True)["title"][n]
                    else:
                        ax = fig.add_subplot(h,w,n+1,xlabel=r"${\rm Wavelength}{\ }{\rm (nm)}$", ylabel=r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
                        title=None
                lim = [j[1][np.argmax(j[0])] - 0.1,j[1][np.argmax(j[0])] + 0.1]
                out = gaussfit(j, lim, num_gauss=num, par_enable=True)
                if disp:
                    display_nc(j,ax=ax,subtraction=out.best_values["c"],xlim=lim,title=title)
                    plot_gauss(j, out, num_gauss=num, ax=ax, fontsize=28,xlim=lim)
                if num == 1:
                    sigma.append([out.best_values["sigma"]])
                elif num == 2:
                    sigma.append([out.best_values["gauss1_sigma"],out.best_values["gauss2_sigma"]])
            df = df.append(i)
        sigma = pd.DataFrame(data=sigma,columns=["sigma1","sigma2"])
        self.df_gauss = df
        self.df_gauss = pd.concat([df.reset_index(drop=True), sigma], axis=1)

    def fwhm(self):
        self.df_gauss["fwhm1"] = self.df_gauss["sigma1"].map(calc_fwhm)
        self.df_gauss["fwhm2"] = self.df_gauss["sigma2"].map(calc_fwhm)

    def temperature(self):
        def reflect(name):
            return fwhm_function[name]
        self.df_gauss["fwhm_function"] = self.df_gauss["name"].map(reflect)
        self.df_gauss["temperature1"] = self.df_gauss["fwhm1"].copy()
        self.df_gauss["temperature1(eV)"] = self.df_gauss["fwhm1"].copy()
        self.df_gauss["temperature2"] = self.df_gauss["fwhm2"].copy()
        self.df_gauss["temperature2(eV)"] = self.df_gauss["fwhm2"].copy()
        for i in range(len(self.df_gauss)):
            self.df_gauss["temperature1"].loc[i] = calc_temp((self.df_gauss.loc[i][10]**2 - self.df_gauss.loc[i][12]**2)**0.5,self.df_gauss.loc[i][3])
            self.df_gauss["temperature2"].loc[i] = calc_temp((self.df_gauss.loc[i][11]**2 - self.df_gauss.loc[i][12]**2)**0.5,self.df_gauss.loc[i][3])
        def to_ev(T):
            from scipy.constants import k,e
            return T * k/e
        self.df_gauss["temperature1(eV)"] = self.df_gauss["temperature1"].map(to_ev)
        self.df_gauss["temperature2(eV)"] = self.df_gauss["temperature2"].map(to_ev)
 

    def spec_name(self,wl):
        if 760 < wl < 790:
            return "OI"
        elif 640 < wl < 670:
            return "alpha"
        elif 470 < wl < 500:
            return "beta"
        elif 420 < wl < 445:
            return "gamma"
        elif 400 < wl < 420:
            return "delta"
        else:
            return "else"

    def split_df(self,df=None,splitin=["OI","alpha","beta","gamma","delta"]):
        if not df:
            df = self.df
        print("DataFrame was splitted into")
        if "OI" in splitin:
            self.df_o = df[df["name"] == "OI"]
            print("df_o")
        if "alpha" in splitin:
            self.df_a = df[df["name"] == "alpha"]
            print("df_a")
        if "beta" in splitin:
            self.df_b = df[df["name"] == "beta"]
            print("df_b")
        if "gamma" in splitin:
            self.df_c = df[df["name"] == "gamma"]
            print("df_c")
        if "delta" in splitin:
            self.df_d = df[df["name"] == "delta"]
            print("df_d")
        

    pass

def convert_nc(fp):
    rotation = get_rot(fp)
    if type(rotation) == int:
        data = np.array(xr.open_dataset(fp).to_array()[0].sum(axis=0)).astype(np.float64)
        data = np.stack([data, fun(np.arange(2048)+get_pixel(rotation))], 0)

        return data
    else:
        pass

def get_rot(fp):
    if iscd(fp):
        return int(fp.split('-')[1])
    elif exists(fp):
        split = "\\"
        if len(fp.split(split)) == 1:
            split = "/"
        return int(fp.split(split)[-1].split('-')[1])
    else:
        print('File does not exist')
        return

def rotationis(rotation,old=False):
    if old:
        if rotation == 415000:
            wavelength = balmers[1]
        elif rotation == 600000:
            wavelength = balmers[2]
        elif rotation == 650000:
            wavelength = balmers[3]
        else:
            wavelength = 0
    else:
        if rotation == 411000:
            wavelength = balmers[1]
        elif rotation == 595000:
            wavelength = balmers[2]
        elif rotation == 644000:
            wavelength = balmers[3]
        elif rotation == 666000:
            wavelength = balmers[4]
        elif rotation == 213000:
            wavelength = balmers[0]
        else:
            wavelength = 0
    return wavelength

def display_nc(fp,xlim=None,ylim=None,disp=True,ax = None,style=False,yoffset=None,title=None,line=None,GUI=False,bp=None,subtraction=0):
    if bp:
        join(bp,fp)
    if type(fp) == str:#when row nc data is selected
        data = convert_nc(fp)
    elif type(fp) != np.ndarray:
        return 'please insert fp="row nc data" or data="converted nc data"'
    else:
        data = fp
    font_setup(size=28)
    if disp:
        if GUI:
            import plotly.graph_objects as go
            pg_data = [go.Scatter(x=data[1],y=data[0])]
            fig = go.Figure(data=pg_data)
            fig.update_layout(template="plotly_white",title=title,xaxis_title=r"Wavelength (nm)",yaxis_title=r"Intensity (arb unit)")

            fig.show()
        else:
            if not ax:
                fig = plt.figure(figsize=(16,9))
                ax = fig.add_subplot(111)
            if line:
                ax.axvline(line,c='r')
            if style:
                a = r"${\rm Intensity}{\ }{\rm (\times 10^"
                b = f"{yoffset}"
                c = r"{\ }arb.unit)}$"
                ax.set_ylabel(a+b+c)
                ax.set_ylabel(r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
                ax.yaxis.offsetText.set_fontsize(0)
                ax.plot(data[1], data[0]/max(data[0]), '.-',c='k')
            else:
                ax.set_ylabel(r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
                ax.plot(data[1], data[0] - subtraction, '.-',c='k')
                ax.set_ylim(top=((max(data[0]) - min(data[0]))*1.2+min(data[0])))
                
            ax.set_title(title)
            # ax.legend(bbox_to_anchor=(1, 1), loc='upper right', borderaxespad=0, fontsize=16)
            ticks_visual(ax)
            grid_visual(ax)
            if xlim:
                ax.set_xlim(xlim)
            if ylim:
                ax.set_ylim(ylim)

def display_all(path,xlim=None,ylim=None,disp=True,subplot=None,fig=None,style=False,yoffset=None,title=None,line=None,GUI=False,):
    [display_nc(join(path,i),title=i,style=style,xlim=xlim,ylim=ylim,GUI=GUI) for i in listdir(path)]

def subtract_background(fp,disp=True,bp=None):
    split = "\\"
    from os.path import join
    if not bp:
        bp = join("../../data/spectrometer/20211208",'background-420000-10000.0ms-.nc')
    image = convert_nc(fp) if type(fp) != np.ndarray else fp
    back = convert_nc(bp) if type(bp) != np.ndarray else bp
    sub = image
    for i in range(len(image[0])):
        sub[0][i] = image[0][i] - back[0][i]
    if disp:  
        fig = plt.figure(figsize=(16, 9), dpi=50)
        ax = fig.add_subplot(1,1,1, xlabel=r"${\rm Wavelength}{\ }{\rm (nm)}$", ylabel=r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
        ax.plot(sub[1], sub[0], '.-')
        # ax.legend(bbox_to_anchor=(1, 1), loc='upper right', borderaxespad=0, fontsize=16)
        ticks_visual(ax)
        grid_visual(ax)    
    return sub

def sub_bg(self,fp,bgpath):
    import os
    rot = fp.split("-")[1]
    op = fp.split("-")[4]
    ls = [i for i in os.listdir(bgpath) if rot in i]
    ls = [i for i in ls if "-"+op+"-" in i]
    if ls == []:
        ls = [os.listdir(bgpath)]
    return self.subtract_background(fp,disp=False,bp=os.path.join(bgpath,ls[0]))

def gaussian(x,amp,cen,wid):
    return (amp/(np.sqrt(2*np.pi)*wid)) * np.exp(-(x-cen)**2 / (2*wid**2))

def gaussfit(data, lim, num_gauss=1, par_enable=True): # 適切なlimを入れてやらないとうまくフィッティんグしない
    n_min = np.argmin(abs(data[1]-lim[0]))
    n_max = np.argmin(abs(data[1]-lim[1]))
    x = data[1][n_max:n_min]
    y = data[0][n_max:n_min]
    
    from lmfit.models import GaussianModel, ConstantModel
    if num_gauss == 1:
        mod = ConstantModel() + GaussianModel()
        params = mod.make_params()
        par_val = {
            "c" : min(y),
            "center" : x[np.argmax(y)],
            "sigma" : 0.005,
            "amplitude" : (max(y) - min(y)) / 200,
        }
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
            'gauss2_center' : x[np.argmax(y)],
            'gauss2_sigma' : 0.01,
            'gauss2_amplitude' : (max(y) - min(y)) / 400,
        }
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

def plot_gauss(data, out, xlim=None, ylim=None, num_gauss=1, ax=None, fontsize=28):
    import matplotlib.ticker as mt
    font_setup(size=fontsize)
    x = np.linspace(data[1][0],data[1][-1],10000)
    y = data[0]
    if not ax:
        fig = plt.figure(figsize=(16,9))
        ax = fig.add_subplot(1,1,1,xlabel=r"${\rm Wavelength}{\ }{\rm (nm)}$", ylabel=r"${\rm Intensity}{\ }{\rm (arb.unit)}$")
    if num_gauss == 1:
        ax.plot(x, gaussian(x, out.best_values["amplitude"], out.best_values["center"], out.best_values["sigma"]), '-', label=r'Fitting',c='r')
    elif num_gauss == 2:
        ax.plot(x, gaussian(x, out.best_values["gauss1_amplitude"], out.best_values["gauss1_center"], out.best_values["gauss1_sigma"]), '-', label=r'Low temperature',c='C1')
        ax.plot(x, gaussian(x, out.best_values["gauss2_amplitude"], out.best_values["gauss2_center"], out.best_values["gauss2_sigma"]), '-', label=r'High temperature',c='C2')
        ax.plot(x, gaussian(x, out.best_values["gauss1_amplitude"], out.best_values["gauss1_center"], out.best_values["gauss1_sigma"]) + gaussian(x, out.best_values["gauss2_amplitude"], out.best_values["gauss2_center"], out.best_values["gauss2_sigma"]), '-', label=r'Summation',c='r')
    ax.legend(loc="best")
    ax.xaxis.set_major_locator(mt.LinearLocator(3))
    ax.yaxis.set_major_formatter(mt.PercentFormatter((max(y) - out.best_values["c"])*100, decimals=1, symbol=''))
    ticks_visual(ax)
    grid_visual(ax)
    if xlim:
        ax.set_xlim(xlim)
    if ylim:
        ax.set_ylim(ylim)
    else:
        ax.set_ylim((min(y)) - out.best_values["c"], (max(y) - out.best_values["c"]) * 1.1)

def calc_fwhm(sigma):
    if not sigma:
        sigma = 0
    return 2 * sigma * np.sqrt(2 * np.log(2))

def calc_temp(fwhm,wavelength,A=1.0079):
    return (fwhm / (7.16e-7 * wavelength))**2 * A

def iscd(fp):
    split1 = "\\"
    split2 = "/"
    return len(fp.split(split1)) == 1 and len(fp.split(split2)) == 1


    # Wavelength Calibration
def rotation_to_pixelgap(rot): #rot=rotation -> pixel
    pos = [8.095936072604022e-10, -0.0013425818271034543, 1450.5866519988974, 3.925402434315537e-09, -0.004457380306986346, 2202.744402995958]
    res = []
    if type(rot) == int:
        rot = [rot]
    for r in rot:
        if 408000 < r: #balmer alpha
            res += [pos[0]*r**2 + pos[1]*r + pos[2]]
        else:
            res += [pos[3]*r**2 + pos[4]*r + pos[5]]
    return res

rot_pixel = [[200000,0]]
for i in range(133):
    rot = 5000*(i+1) + 200000 
    pixel = rot_pixel[i][1] + rotation_to_pixelgap(rot)[0]
    rot_pixel += [[rot,pixel]]

def get_pixel(rot):
    i = (rot - 200000)//5000
    return rot_pixel[i][1] + rotation_to_pixelgap(rot)[0]*(rot%5000)/5000