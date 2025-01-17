import multiprocessing as mp
import time
from dx2lib import *
import numpy as np
import keyboard
import PySimpleGUI as sg

def Get_Angles(d):
    angles = (c_double * d["AXISNUM"])()
    if DXL_GetPresentAngles(d["dev"], (c_uint8 * d["AXISNUM"])(*d["IDs"]), angles, d["AXISNUM"]):
        d["angles"] = np.ctypeslib.as_array(angles)

def Get_Loads(d):
    curs = (c_double * d["AXISNUM"])()
    
    if DXL_GetPresentCurrents(d["dev"], (c_uint8 * len(d["IDs"]))(*d["IDs"]), curs, d["AXISNUM"]):
        d["loads"] = np.ctypeslib.as_array(curs)

def Get_AL(d):
    Get_Angles(d)
    Get_Loads(d)
    d["the0"] = (d["angles"][0]-d["init_angles"][0])
    d["the1"] = (d["angles"][1]-d["init_angles"][1])
    d["the2"] = -(d["angles"][2]-d["init_angles"][2])

def Calc(d,nums,angles):
    a = d["angles_set"]
    for i in range(len(nums)):
        a[nums[i]] = angles[i]
        if nums[i] == 2:
            a[nums[i]] = -angles[i]
    d["angles_set"] = a

def AS(d,nums,angles):
    Calc(d,nums,angles)
    for i in range(len(nums)):
        DXL_SetGoalAngle(d["dev"], nums[i], d["angles_set"][nums[i]]+d["init_angles"][nums[i]])

def AVS(d,nums,angles,vels):
    Calc(d,nums,angles)
    for i in range(len(nums)):
        DXL_SetGoalAngleAndVelocity(d["dev"], nums[i], d["angles_set"][nums[i]]+d["init_angles"][nums[i]],vels[i])

def ATS(d,nums,angles,tim):
    Calc(d,nums,angles)
    for i in range(len(nums)):
        DXL_SetGoalAngleAndTime(d["dev"], nums[i], d["angles_set"][nums[i]]+d["init_angles"][nums[i]],tim)
    time.sleep(tim+0.3)

def ASWU(d,nums,angles):
    Calc(d,nums,angles)
    for i in range(len(nums)):
        DXL_SetGoalAngle(d["dev"], nums[i], d["angles_set"][nums[i]]+d["init_angles"][nums[i]])
    a = True
    while a:
        Get_AL(d)
        a = False
        for i in range(len(nums)):
            if not abs(d[f"the{nums[i]}"] - angles[i]) < 5:
                a = True
                break
        if keyboard.is_pressed("esc"): # ストップ
            DXL_StandStillAngle(d["dev"],0)
            DXL_StandStillAngle(d["dev"],1)
            DXL_StandStillAngle(d["dev"],2)
            d["stop"] = True
            print("Stopped...")
            return

def AVSWU(d,nums,angles,vel):
    Calc(d,nums,angles)
    for i in range(len(nums)):
        DXL_SetGoalAngleAndVelocity(d["dev"], nums[i], d["angles_set"][nums[i]]+d["init_angles"][nums[i]],vel)
    a = True
    while a:
        Get_AL(d)
        a = False
        for i in range(len(nums)):
            if not abs(d[f"the{nums[i]}"] - angles[i]) < 5:
                a = True
                break
        if keyboard.is_pressed("esc"): # ストップ
            DXL_StandStillAngle(d["dev"],0)
            DXL_StandStillAngle(d["dev"],1)
            DXL_StandStillAngle(d["dev"],2)
            d["stop"] = True
            print("Stopped...")
            return

def Do_the(d,num,diff):
    a = d[f"the{num}"] + diff
    AS(d,[num],[a])

def C(arr):
    return np.array(arr)*2*360 # 1 mm

def Origin(d):
    ASWU(d,[0,1,2],C([0,0,0]))

def Scan(d):
    a = True
    x = np.linspace(-4,4,16)
    # y = np.linspace(-4,4,16)
    # zmin = -2
    # zmax = 2
    for i in range(len(x)):
        if a:
            a = False
            AVSWU(d,[0,1,2],C([x[i],-5,0]),1440)
            if d["stop"]:return
            AVSWU(d,[0,1,2],C([x[i], 3,0]),1440)
            if d["stop"]:return
        else:
            a = True
            AVSWU(d,[0,1,2],C([x[i], 3,0]),1440)
            if d["stop"]:return
            AVSWU(d,[0,1,2],C([x[i],-5,0]),1440)
            if d["stop"]:return
        # ASWU(d,[0,1,2],C([x[i],y[j],zmax]))
        # time.sleep(2)
        # if d["stop"]:return
        # if a:
        #     a = False
        #     if d["stop"]:return
        #     ASWU(d,[0,1,2],C([x[i],y[j],zmin]))
        #     if d["stop"]:return
        #     ASWU(d,[0,1,2],C([x[i],y[j],zmax]))
        #     if d["stop"]:return
        # else:
        #     a = True
        #     if d["stop"]:return
        #     ASWU(d,[0,1,2],C([x[i],y[j],zmax]))
        #     if d["stop"]:return
        #     ASWU(d,[0,1,2],C([x[i],y[j],zmin]))
        #     if d["stop"]:return
    AVSWU(d,[0,1,2],C([-6,-6,0]),1440)
    if d["stop"]:return

def Scan_Origin(d):
    ASWU(d,[0,1,2],C([-6,-6,0]))
    if d["stop"]:return

def Check_for_all_motors_connected(d):
    if d["dev"] == None:
        print('Could not open COM port...')
        return 0
    else:
        for id in d["IDs"]:
            if (DXL_GetModelInfo(d["dev"],id).contents.name.decode()) != "XL430-W250" and (DXL_GetModelInfo(d["dev"],id).contents.name.decode()) != "XM430-W350":
                print(f"motor {id} is not connected.")
        print("connected!")
        return 1

#---------------------------------------------
# コントロール
#---------------------------------------------

def Control(d):
    while not d["kill"]:
        Get_AL(d)

        # Else
        if keyboard.is_pressed("f10"):
            DXL_SetTorqueEnablesEquival(d["dev"], (c_uint8 * d["AXISNUM"])(*d["IDs"]), d["AXISNUM"], True)
            print("ON...")
        if keyboard.is_pressed("f12"):
            DXL_SetTorqueEnablesEquival(d["dev"], (c_uint8 * d["AXISNUM"])(*d["IDs"]), d["AXISNUM"], False)
            print("OFF...")
        if keyboard.is_pressed("o+p"): # save origin point
            np.save("origin",d["angles"])
            d["init_angles"] = np.load("origin.npy")
            print("origin saved...")
        if keyboard.is_pressed("f1"):
            d["kill"] = True

        if keyboard.is_pressed("d"):
            Do_the(d,0,360)
        elif keyboard.is_pressed("a"):
            Do_the(d,0,-360)
        else:
            DXL_StandStillAngle(d["dev"],0)

        if keyboard.is_pressed("w"):
            Do_the(d,1,360)
        elif keyboard.is_pressed("s"):
            Do_the(d,1,-360)
        else:
            DXL_StandStillAngle(d["dev"],1)

        if keyboard.is_pressed("e"):
            Do_the(d,2,360)
        elif keyboard.is_pressed("q"):
            Do_the(d,2,-360)
        else:
            DXL_StandStillAngle(d["dev"],2)

        if keyboard.is_pressed("j"): # 原点に移動
            Origin(d)

        if keyboard.is_pressed("k"): # スキャン
            Scan(d)
            d["stop"] = False

        if keyboard.is_pressed("l"): # スキャン
            Scan_Origin(d)
            d["stop"] = False

#---------------------------------------------
# GUI
#---------------------------------------------

def Scanner_GUI(d):
    sg.change_look_and_feel("GrayGrayGray")
    header = ["", "0", "1", "2"]
    width = [10,6,6,6]
    member_list = [
        ["realの角度", *[round(d["angles"][i], 1) for i in range(d["AXISNUM"])]],
        ["現在の角度", *[round(d["angles"][i] - d["init_angles"][i], 1) for i in range(d["AXISNUM"])]],
        ["指定の角度", *[round(d["angles_set"][i], 1) for i in range(d["AXISNUM"])]],
        ["現在の負荷", *[round(d["loads"][i], 1) for i in range(d["AXISNUM"])]],
    ]

    header2 = ["the0","the1","the2"]
    width2 = [6,6,6]
    member_list2 = [[round(d["the0"],1),round(d["the1"],1),round(d["the2"],1)]]

    layout = [[sg.Table(member_list, headings=header, key="table1",
                        col_widths=width,num_rows=len(member_list),
                        auto_size_columns=False,vertical_scroll_only=False)],
              [sg.Table(member_list2, headings=header2, key="table2",
                        col_widths=width2,num_rows=1,
                        auto_size_columns=False,vertical_scroll_only=False)]]
            #   [sg.Button("Scan")]]
    window = sg.Window('Auto_Scanner', layout, finalize=True)

    while True:
        event, values = window.read(timeout=0)
        if event in (None, 'Exit'):
            break

        # if event in ("Scan"):
        #     d["scan"] = True

        # 表の更新
        window["table1"].update(values=[["realの角度", *[round(d["angles"][i], 1) for i in range(d["AXISNUM"])]],
                                        ["現在の角度", *[round(d["angles"][i] - d["init_angles"][i], 1) for i in range(d["AXISNUM"])]],
                                        ["指定の角度", *[round(d["angles_set"][i], 1) for i in range(d["AXISNUM"])]],
                                        ["現在の負荷", *[round(d["loads"][i], 1) for i in range(d["AXISNUM"])]]])
        window["table2"].update(values=[[round(d["the0"],1),round(d["the1"],1),round(d["the2"],1)]])
    d["stop"] = True
    d["kill"] = True

if __name__ == '__main__':
    # メインプログラム
    d = mp.Manager().dict()# 共有変数の作成
    d["dev"] = DX2_OpenPort(b'\\\\.\\COM4', 57600) # ポートを開いてdevを取得
    d["IDs"] = [0,1,2] # ID一覧
    d["AXISNUM"] = len(d["IDs"]) # 全軸数

    if Check_for_all_motors_connected(d):
        d["angles"] = [0.0] * d["AXISNUM"]
        Get_Angles(d) # 現在の角度
        d["angles_set"] = [0.0] * d["AXISNUM"] # 指定の角度。
        d["loads"] = [0.0] * d["AXISNUM"]
        Get_Loads(d) # 現在の負荷
        try:
            d["init_angles"] = np.load("origin.npy")
        except:
            d["init_angles"] = [0.0] * d["AXISNUM"]

        d["kill"] = False

        d["the0"] = 0
        d["the1"] = 0
        d["the2"] = 0

        d["scan"] = False
        d["stop"] = False

        # MultiTurnモード＝４に変更
        DXL_SetOperatingModesEquival(d["dev"], (c_uint8 * d["AXISNUM"])(*d["IDs"]), d["AXISNUM"], 4)
        # 最初はトグルオフ
        DXL_SetTorqueEnablesEquival(d["dev"], (c_uint8 * d["AXISNUM"])(*d["IDs"]), d["AXISNUM"], False)

        # 子プロセスの作成
        p0 = mp.Process(target=Scanner_GUI, args=[d])
        # 子プロセスの開始
        p0.start()
        # 親プロセス上でスレッドを作成
        Control(d)
        # 子プロセスの終了を待つ。
        p0.join()

        # 最後もトグルオフ
        DXL_SetTorqueEnablesEquival(d["dev"], (c_uint8 * d["AXISNUM"])(*d["IDs"]), d["AXISNUM"], False)
        # ポートを閉じる
        DX2_ClosePort(d["dev"])
