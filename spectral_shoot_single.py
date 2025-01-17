import thr640
import imaging_tool

import time, os, logging
from FLI import FLI
import numpy as np
import xarray as xr
from os.path import join
import datetime



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

def move_stepmotor(param):
    step_motor = param['step_motor']
    count = param['count']

    step_motor.goto(count)
    step_motor.waitUntilReady()

def take_image(param):
    count = param['count']
    vbin = param['vbin']
    camera = param['camera']
    exposure = param['exposure']
    basepath = param['basepath']
    name = param['name']
    tag = param['tag']

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