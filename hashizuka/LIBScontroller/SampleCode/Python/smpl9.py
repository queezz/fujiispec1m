#!/usr/bin/env python
# -*- coding: utf-8 -*-

# 簡易モーション再生2
# Velocity-based profile + 現在位置取得スレッド

import sys, time, kbhit
import threading
from dx2lib import *    # dx2libをインポート

COMPort = b'\\\\.\\COM3'  # 任意のCOMポート名に修正の事
Baudrate = 4000000       # Dynamixelのボーレートと合わせる事
AXISNUM = 8               # 全軸数
term = False;

# モーションの1パターン分ストラクチャ
#  軸数分の角度と移動時間)
class TPose(Structure):
  _fields_ = [("angles", c_double * AXISNUM), ("sec", c_double)]


#---------------------------------------------
# モニタスレッド
#---------------------------------------------
def Mon():
  v = (TAngleVelocity * AXISNUM)()
  prevang = (c_double * AXISNUM)()
  while not term:
    if DXL_GetPresentAngles(dev, IDs, prevang, AXISNUM) == False: print('xxxxxxxxxxxxxxxxxxxxxxxx')
    time.sleep(0.005)


#---------------------------------------------
# モーション再生関数
#---------------------------------------------
def PlayMotion(ids, m):
  for p in m:
    if kb.kbhit(): break
    for a in p.angles: print(a, end=' ')
    print(p.sec)
    # 角度と遷移時間を指令(開始位置は現在位置)
    DXL_SetGoalAnglesAndTime(dev, ids, p.angles, len(ids), p.sec)
    # 複数パターンあるときは遷移するまでの時間を待機する
    if len(m) != 1: time.sleep(p.sec)

#---------------------------------------------
dev = DX2_OpenPort(COMPort, Baudrate)
if dev != None:
  # ID一覧 (AXISNUM分のIDを列挙すること)
  IDs = (c_uint8 * AXISNUM)(1,2,3,4,5,6,7,8)
  # ID一覧分のDynamixelを検索しモデル名を表示
  for id in IDs:
    print(id,DXL_GetModelInfo(dev,id).contents.name.decode())

  DXL_SetOperatingModesEquival(dev, IDs, AXISNUM, 3)
  # Velocity-base profile
  DXL_SetDriveModesEquival (dev, IDs, AXISNUM, 0x0)

  DXL_SetTorqueEnablesEquival(dev, IDs, AXISNUM, True)

  thread_1 = threading.Thread(target=Mon)
  thread_1.start()

  # モーションデータ
  # []でくくられた情報が1つのモーションデータ
  # その中に「(TPose)((?,?,?...),?)」で角度[deg]と遷移時間[sec]のキーフレームを記述
  # キーフレームはカンマで区切って必要数分記述
  m_0 = [(TPose)((   0,   0,   0,   0,   0,   0,   0,   0), 1.0)]

  m_1 = [(TPose)((  90,  90,  90,  90,  90,  90,  90,  90), 1.0)]

  m_2 = [(TPose)(( -90, -90, -90, -90, -90, -90, -90, -90), 1.0)]

  m_3 = [(TPose)(( 180, 180, 180, 180, 180, 180, 180, 180), 1.0)]

  m_4 = [(TPose)((-180,-180,-180,-180,-180,-180,-180,-180), 1.0)]

  m_5 = [(TPose)(( -30, -30, -30, -30, -30, -30, -30, -30), 0.2),
         (TPose)((  30,  30,  30,  30,  30,  30,  30,  30), 0.2),
         (TPose)(( -30, -30, -30, -30, -30, -30, -30, -30), 0.2),
         (TPose)((  30,  30,  30,  30,  30,  30,  30,  30), 0.2),
         (TPose)(( -30, -30, -30, -30, -30, -30, -30, -30), 0.2),
         (TPose)((  30,  30,  30,  30,  30,  30,  30,  30), 0.2),
         (TPose)(( -30, -30, -30, -30, -30, -30, -30, -30), 0.2),
         (TPose)((  30,  30,  30,  30,  30,  30,  30,  30), 0.2)]

  m_6 = [(TPose)(( -45,  45, -45,  45, -45,  45, -45,  45), 2.0),
         (TPose)((  90,  90,  90,  90,  90,  90,  90,  90), 2.0),
         (TPose)(( -30, -30, -30, -30, -30, -30, -30, -30), 2.0),
         (TPose)((-180,-180,-180,-180,-180,-180,-180,-180), 2.0)]

  kb = kbhit.KBHit ()
  k = 'a'
  # キーボードからの数値入力で再生するモーションを選択
  while k != 'e':
    print('key=',end='')
    sys.stdout.flush()
    k = kb.getch()
    print(k)
    if   k == '0': PlayMotion(IDs, m_0)
    elif k == '1': PlayMotion(IDs, m_1)
    elif k == '2': PlayMotion(IDs, m_2)
    elif k == '3': PlayMotion(IDs, m_3)
    elif k == '4': PlayMotion(IDs, m_4)
    elif k == '5': PlayMotion(IDs, m_5)
    elif k == '6': PlayMotion(IDs, m_6)
    elif k == 's': DXL_StandStillAngles (dev, IDs, AXISNUM)

  time.sleep(0.5)

  kb.set_normal_term()

  DXL_SetTorqueEnablesEquival(dev, IDs, AXISNUM, False)

  term = True;
  thread_1.join()

  DX2_ClosePort(dev)
else:
  print('Could not open COM port.')
