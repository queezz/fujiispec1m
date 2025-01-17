#!/usr/bin/env python
# -*- coding: utf-8 -*-

# 1軸へ角度と遷移時間を同時指令

import sys, time
from dx2lib import *  # dx2libをインポート

COMPort = b'\\\\.\\COM3'  # 任意のCOMポート名に修正の事
Baudrate = 57600          # Dynamixelのボーレートと合わせる事
TargetID = 1              # DynamixelのIDと合わせる事

#---------------------------------------------
# (1) ポートを開いてdevを取得(必須)
dev = DX2_OpenPort(COMPort, Baudrate)
if dev != None:
  # (2) 指定IDのモデル情報を取得(必須)
  if DXL_GetModelInfo(dev, TargetID).contents.devtype != devtNONE:
    # (3) 取得されたモデル情報からモデル名を表示(必須ではない)
    print (DXL_GetModelInfo(dev, TargetID).contents.name.decode())

    # (4) DynamixelをJointモード=3に変更
    DXL_SetOperatingMode(dev, TargetID, 3)
    # (5) トルクイネーブル(必須)
    DXL_SetTorqueEnable(dev, TargetID, True)

    # (6) 速度[deg]と遷移時間[sec]を順次指令
    DXL_SetGoalAngleAndTime(dev, TargetID,    0.0, 2.0)
    time.sleep(2)
    DXL_SetGoalAngleAndTime(dev, TargetID,  -90.0, 2.0)
    time.sleep(2)
    DXL_SetGoalAngleAndTime(dev, TargetID,    0.0, 2.0)
    time.sleep(2)
    DXL_SetGoalAngleAndTime(dev, TargetID,   90.0, 2.0)
    time.sleep(2)
    DXL_SetGoalAngleAndTime(dev, TargetID, -180.0, 2.0)
    time.sleep(2)
    DXL_SetGoalAngleAndTime(dev, TargetID,  180.0, 3.0)
    time.sleep(3)

    # (7) トルクイネーブルディスエーブル(必要に応じて)
    DXL_SetTorqueEnable(dev, TargetID, False)
    # (8) ポートを閉じる(必須)
    DX2_ClosePort(dev)
  else:
    print('Device information could not be acquired.')
else:
  print('Could not open COM port.')
