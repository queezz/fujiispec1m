#!/usr/bin/env python
# -*- coding: utf-8 -*-

# 1軸のLED明滅

import sys, time
from ctypes import *
from dx2lib import *  # dx2libをインポート

COMPort = b'\\\\.\\COM3'  # 任意のCOMポート名に修正の事
Baudrate = 57600          # Dynamixelのボーレートと合わせる事
IDs = (c_uint8 * 253)()

if ('mingw' in get_platform()) or ('win' in get_platform()):
  libc = cdll.msvcrt
else:
  libc = cdll.LoadLibrary("libc.so.6")


#---------------------------------------------
# (1) ポートを開いてTDeviceIDを取得(必須)
dev = DX2_OpenPort(COMPort, Baudrate)
if dev != None:
  num = DXL_ScanDevices(dev, IDs)
  print ('detect num=', num)
  DXL_PrintDevicesList (libc.printf)

  # (2) 指定IDのモデル情報を取得(必須)
  if num > 0:
    # (3) 取得されたモデル情報からモデル名を表示(必須ではない)
    print (DXL_GetModelInfo(dev, TargetID).contents.name)

    # (4) LED点灯
    DXL_SetLED(dev, TargetID, True)
    # (5) 1秒待ち
    time.sleep(1)
    # (6) LED消灯
    DXL_SetLED(dev, TargetID, False)

    # (7) ポートを閉じる(必須)
    DX2_ClosePort(dev)
  else:
    print('Device information could not be acquired.')
else:
  print('Could not open COM port.')
