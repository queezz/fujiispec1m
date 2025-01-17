import thr640
import imaging_tool

import time, os, logging
from FLI import FLI
import numpy as np
import matplotlib.pyplot as plt
import xarray as xr
from os.path import join
import datetime
import pandas as pd


def setup_camera():
    logger = logging.getLogger("Logging")
    logger.setLevel(50) # 10 - DEBUG 50 - CRITICAL 20 - INFO
    camera = FLI()
    step_motor = thr640.THR640() # Can only have one instance.

    param = {
        'camera': camera,
        'step_motor': step_motor,
    }

    return param


def read_peak(param):
    step_motor = param['step_motor']
    count = param['count']
    vbin = param['vbin']
    camera = param['camera']
    exposure = param['exposure']
    basepath = param['basepath']
    name = param['name']
    tag = param['tag']


    step_motor.goto(count)
    step_motor.waitUntilReady()
    attributes = {
        "temperature": camera.getTemperature(),
        "device_status": camera.getDeviceStatus(),
        "exposure": exposure,
        "frame_type": "light",
    }
    camera.setExposureTime(exposure);camera.setVBin(vbin);camera.setImageArea(10,0,2058,512//vbin);camera.exposeFrame()
    time.sleep(.1)
    # load image
    image_data = camera.grabFrame(out=np.empty((512//vbin,2048), np.uint16))
    # convert to xarray
    data = xr.DataArray(
        image_data, dims=["y", "x"], coords={"image_counter": 0}, attrs=attributes
    )
    # save data as NetCDF file
    filepath = join(basepath,f'{name}-{count}-{exposure}ms-{tag}.nc')
    data.to_netcdf(filepath)
    imaging_tool.plot_image(image_data)

    # calibration
    y_center_start = image_data.shape[0] // 2 - 10
    y_center_end = image_data.shape[0] // 2 + 10
    intensity_profile = np.mean(image_data[y_center_start:y_center_end, :], axis=0)

    # Hα輝線の位置を特定（最大値の位置をHα輝線と仮定）
    ha_pixel_position = np.argmax(intensity_profile)
    print(f"Hα line is detected at pixel position: {ha_pixel_position}")

    return ha_pixel_position



def take_spectrum_time_variation(param):
    step_motor = param['step_motor']
    count = param['count']
    vbin = param['vbin']
    camera = param['camera']
    exposure = param['exposure']
    basepath = param['basepath']
    name = param['name']
    tag = param['tag']
    duration = param['duration']
    ha_pixel_position = param['ha_pixel_position']

    # 計測データを格納するリスト
    times = []
    intensities = []

    # ステップモーターの設定
    step_motor.goto(count)
    step_motor.waitUntilReady()

    attributes = {
        "temperature": camera.getTemperature(),
        "device_status": camera.getDeviceStatus(),
        "exposure": exposure,
        "frame_type": "light",
    }

    basepath_point = basepath + f'\\time_variation\\{tag}\\point'
    basepath_entire = basepath + f'\\time_variation\\{tag}\\entire'

    if not os.path.exists(basepath_point):
        os.makedirs(basepath_point)

    if not os.path.exists(basepath_entire):
        os.makedirs(basepath_entire)

    # 各計測の開始時刻を記録
    start_time = time.time()  

    # 計測ループ
    while True:
        current_time = time.time()
        elapsed_time = current_time - start_time if times else 0
        if elapsed_time >= duration:
            break

        # 経過時間をリストに追加
        times.append(elapsed_time)

        # カメラ設定と撮影
        camera.setExposureTime(exposure)
        camera.setVBin(vbin)
        camera.setImageArea(10, 0, 2058, 512 // vbin)
        camera.exposeFrame()
        time.sleep(0.1)  # カメラの応答待機

        # 画像データの取得
        image_data = camera.grabFrame(out=np.empty((512 // vbin, 2048), np.uint16))

        # 強度の計算 (指定した範囲のピクセルを取得)
        intensity = sum(image_data[:, ha_pixel_position])
        intensities.append(intensity)

        # convert to xarray
        data = xr.DataArray(
            image_data, dims=["y", "x"], coords={"image_counter": 0}, attrs=attributes
        )

        approximate_time = round(elapsed_time, 1)
        print(approximate_time)
        # save data as NetCDF file
        filepath = join(basepath_point,f'{name}-{approximate_time}s-{tag}.nc')
        data.to_netcdf(filepath)

        imaging_tool.plot_image(image_data)

    # CSVファイルに保存
    data = {
        "Time (s)": times,
        "Intensity": intensities
    }
    df = pd.DataFrame(data)
    csv_filepath = join(basepath_entire, f"{name}-{count}-{exposure}ms_data.csv")
    df.to_csv(csv_filepath, index=False)

    # プロットと保存
    attributes = {
        "exposure": exposure,
        "count": count,
    }
    data_array = xr.DataArray(
        intensities, dims=["time"], coords={"time": times}, attrs=attributes
    )

    # 全データの保存
    summary_filepath = join(basepath, f"{name}-{count}-{exposure}ms.nc")
    data_array.to_netcdf(summary_filepath)

    # プロット
    plt.plot(times, intensities)
    # plt.xlabel("Time (seconds)")
    # plt.ylabel("Hα Line Intensity")
    # plt.title("Hα Line Intensity over Time")
    plt.savefig(join(basepath, f"{name}-{count}-{exposure}ms_plot.png"))
    plt.show()