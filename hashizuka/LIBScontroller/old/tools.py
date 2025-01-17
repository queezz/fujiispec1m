import time
import numpy as np

def get_info(obj): #コマンドの実行状況及び座標値取得
    obj.write(('1Q\r\n').encode())
    return obj.readline()

def checker(obj): # check connection and wait for movement
    while True:
        if get_info(obj).decode()[-6] == 'N':
            break
        time.sleep(0.05)

def return_zero(obj,axis=3): #機械原点に復帰
    obj.write((f'1H{axis}' + '\r\n').encode())
    checker(obj)

def return_home(obj,axis=3): #理論原点に復帰  
    obj.write((f'1N{axis}' + '\r\n').encode())
    checker(obj)

def stop(obj,axis=3): #停止
    obj.write((f'1L{axis}' + '\r\n').encode())

def input_a(obj, input,axis=3): #目標座標に移動
    if axis == 3:
        input = str(input[0]) + ',' + str(input[1])
    else:
        input = str(input)
    obj.write((f'1A{axis}:{input}'+'\r\n').encode())
    checker(obj)

def input_m(obj, input,axis=3): #相対変化
    if axis == 3:
        input = str(input[0]) + ',' + str(input[1])
    else:
        input = str(input)
    obj.write((f'1M{axis}:{input}'+'\r\n').encode())
    checker(obj)

def set_home(obj,axis=3): #現座標を理論原点に設定
    obj.write((f'1R{axis}' + '\r\n').encode())

def initializer(obj,initial_point):
    checker(obj)
    return_zero(obj)
    input_a(obj,(initial_point))
    prepos = get_info(obj).decode()[:14]
    set_home(obj)
    nowpos = get_info(obj).decode()[:14]
    print('initialized',prepos,'as',nowpos)

def membrane_pos(obj, input): #X,Y → Zx,Zyに変換
    r = 40 / 2 # membrane radius (mm)
    l = 300 # distance to membrane (mm)
    a = 50 # 台座の1辺の長さ (mm)
    x = input[0]
    y = -input[1]

    input_a(obj,(int(x/l*a*1000),int(y/l*a*1000)))