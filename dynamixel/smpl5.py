#!/usr/bin/env python
# -*- coding: utf-8 -*-

# 複数軸から現在角度取得

import sys, time, kbhit
from dx2lib import *    # dx2libをインポート

COMPort = b'\\\\.\\COM3'  # 任意のCOMポート名に修正の事
Baudrate = 57600          # Dynamixelのボーレートと合わせる事
AXISNUM = 8               # 全軸数

#---------------------------------------------
# (1) ポートを開いてdevを取得(必須)
dev = DX2_OpenPort(COMPort, Baudrate)
if dev != None:
  # (2) ID一覧設定 (AXISNUM分のIDを順に列挙すること)
  IDs = (c_uint8 * AXISNUM)(1,2,3,4,5,6,7,8)
  # (3) ID一覧分のDynamixelを検索しモデル名を表示
  for id in IDs:
    print(id, DXL_GetModelInfo(dev,id).contents.name.decode())

  # (4) ID一覧分のDynamixelをMultiTurnモード=4に変更
  DXL_SetOperatingModesEquival(dev, IDs, AXISNUM, 4)
  # (5) ID一覧分のDynamixelをトルクディスエーブル
  DXL_SetTorqueEnablesEquival(dev, IDs, AXISNUM, False)

  # (6) キー入力により処理を分岐
  ten = False
  k = ''
  kb = kbhit.KBHit()
  pangles = (c_double * AXISNUM)()
  while k != 'e':   # 'e'が押されると終了
    if kb.kbhit():
      k = kb.getch()
      # ' '(スペース)を押す度にトルクイネーブルをトグル
      if k==' ':
        ten = not ten
        DXL_SetTorqueEnablesEquival(dev, IDs, AXISNUM, ten)
        print('\nTorque Enable='),
        print(ten)
      else:
        print
    # ID一覧分の角度を取得し表示
    if DXL_GetPresentAngles(dev, IDs, pangles, AXISNUM):
      print('(', end='')
      print(('{:7.1f},'*len(pangles)).format(*pangles), end=')\r')
      sys.stdout.flush()
  kb.set_normal_term()

  # (7) トルクイネーブルディスエーブル
  DXL_SetTorqueEnablesEquival(dev, IDs, AXISNUM, False)

  # (8) ポートを閉じる(必須)
  DX2_ClosePort(dev)
else:
  print('Could not open COM port.')
