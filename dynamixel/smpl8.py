#!/usr/bin/env python
# -*- coding: utf-8 -*-

# 角度の連続取得と再生テスト
# モーションキャプチャのお遊びなのでかなりいいかげんなツクリ

import sys, time, kbhit
import csv              # csvファイルを扱う
from dx2lib import *    # dx2libをインポート

COMPort = b'\\\\.\\COM3'  # 任意のCOMポート名に修正の事
Baudrate = 57600          # Dynamixelのボーレートと合わせる事
AXISNUM = 8               # 全軸数
CSVFILE = 'CAPDATA.CSV'

#---------------------------------------------
# キャプチャデータ再生関数
#---------------------------------------------
# CSVファイルを連続したモーションデータとして再生
# 角度&時間指令だと都度取得する現在位置のキャプチャが遅延要素になるので
# 角度&速度指令に変換し直している
def PlayCaptureFile(dev, ids, axis, fn):
  v = (TAngleVelocity * axis)()
  prevang = (c_double * axis)()
  DXL_GetPresentAngles(dev, ids, prevang, axis)
  with open(fn, 'r') as fp:
    r = csv.reader(fp)
    for row in r:
      print('[',end='')
      for i in range(axis):
        v[i].angle = float(row[i])
        v[i].velocity = (v[i].angle - prevang[i]) / float(row[axis])  # 角速度に変換
        print ('{:7.1f}'.format(v[i].angle),end='')
      print(']',end='\r')
      sys.stdout.flush()
      DXL_SetGoalAnglesAndVelocities(dev, ids, v, axis)
      time.sleep(float(row[axis]))
      for i in range(axis): prevang[i] = v[i].angle

#---------------------------------------------
dev = DX2_OpenPort(COMPort, Baudrate)
if dev != None:
  IDs = (c_uint8 * AXISNUM)(1,2,3,4,5,6,7,8)
  for id in IDs: print(id, DXL_GetModelInfo(dev,id).contents.name.decode())

  # マルチターンモードに
  DXL_SetOperatingModesEquival(dev, IDs, AXISNUM, 4)
  DXL_SetTorqueEnablesEquival(dev, IDs, AXISNUM, False)

  ten = False
  cap = False
  k = ''
  kb = kbhit.KBHit()
  pangles = (c_double * AXISNUM)()
  t = oldt = GetQueryPerformanceCounter() / 1000
  # 'e'が押されると終了
  while k != 'e':
    t = GetQueryPerformanceCounter() / 1000
    if kb.kbhit():
      k = kb.getch()
      # ' '(スペース):トルクイネーブルのON/OFFトグル
      if k==' ':
        ten = not ten
        DXL_SetTorqueEnablesEquival(dev, IDs, AXISNUM, ten)
        print('\nTorque Enable=',ten)
      # 's':キャプチャのON/OFFトグル
      if k=='s':
        cap = not cap
        if cap:
          print('\nStart Capture')
          fp = open(CSVFILE, 'w')
          wr = csv.writer(fp, lineterminator='\n')
        else:
          print('\nStop Capture')
          fp.close()
      # 'p':キャプチャデータの再生
      if k=='p':
        cap = False
        ten = True
        DXL_SetTorqueEnablesEquival(dev, IDs, AXISNUM, True)
        fp.close()
        print ('\nPlay Capture data')
        PlayCaptureFile(dev, IDs, AXISNUM, CSVFILE)
        print
      else:
        print
    # ID一覧分の角度を取得し表示
    if DXL_GetPresentAngles(dev, IDs, pangles, AXISNUM):
      if not cap:
        print('(',end='')
        print(('{:7.1f},'*len(pangles)).format(*pangles),end='')
        print('), {:6.4f}'.format(t - oldt),end='\r')
        sys.stdout.flush()
      else:
        print('<')
        print(('{:7.1f},'*len(pangles)).format(*pangles),end='')
        print('>, {:6.4f}'.format(t - oldt),end='\r')
        sys.stdout.flush()
        wr.writerow(list(pangles) + [t - oldt])
    # 刻みが細か過ぎると保存データが大きくなるので0.1秒周期程度が良いかな
    time.sleep(0.1)
    oldt = t

  fp.close()
  kb.set_normal_term()
  DXL_SetTorqueEnablesEquival(dev, IDs, AXISNUM, False)
  DX2_ClosePort(dev)
else:
  print('Could not open COM port.')
