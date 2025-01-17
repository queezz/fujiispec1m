#!/usr/bin/env python
# -*- coding: utf-8 -*-

# 2軸へ角速度指令

import sys, time
from dx2lib import *  # dx2libをインポート

COMPort = b'\\\\.\\COM4'  # 任意のCOMポート名に修正の事
Baudrate = 57600          # Dynamixelのボーレートと合わせる事
TargetID1 = 1             # 1台目のDynamixelのIDと合わせる事
TargetID2 = 2             # 2台目のDynamixelのIDと合わせる事

#---------------------------------------------
# 角度指令と時間待ち関数
#---------------------------------------------
def SetVelo(dev, ids, velos, sec):
  for velo in velos: print(velo),
  print
  # 2軸分の角速度を指令
  DXL_SetGoalVelocities(dev, ids, (c_double * 2)(*velos), 2)
  # 指定時間待つ
  time.sleep(sec)

#---------------------------------------------
# (1) ポートを開いてdevを取得(必須)
dev = DX2_OpenPort(COMPort, Baudrate)
if dev != None:
  # (2) IDの一覧
  IDs = (c_uint8 * 2)(TargetID1, TargetID2)
  # (3) 指定IDのモデル情報を取得しモデル名を表示
  #   いちいちモデル名を表示する必要は無いが、情報取得のついでに表示
  #   また、ここでは情報の取得ができたかの確認をしていない
  for id in IDs:
    print(id, DXL_GetModelInfo(dev,id).contents.name.decode())

  # (4) DynamixelをVelocityモード=1に変更
  DXL_SetOperatingModesEquival(dev, IDs, 2, 1)
  # (5) トルクイネーブル(必須)
  DXL_SetTorqueEnablesEquival(dev, IDs, 2, True)

  # (6) 2軸分の角速度[deg/sec]と時間待ちを順次指令
  SetVelo(dev, IDs, [ 150, -150], 3);
  SetVelo(dev, IDs, [-150,  150], 3);
  SetVelo(dev, IDs, [ 150,  150], 3);
  SetVelo(dev, IDs, [-150, -150], 3);
  SetVelo(dev, IDs, [   0,    0], 1);

  # (7) トルクイネーブルディスエーブル(必要に応じて)
  DXL_SetTorqueEnablesEquival(dev, IDs, 2, False)

  # (8) ポートを閉じる(必須)
  DX2_ClosePort(dev)
else:
  print('Could not open COM port.')
