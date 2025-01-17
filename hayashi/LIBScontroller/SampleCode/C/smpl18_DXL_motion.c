/*
  DXL APIを使う
   モーション
*/
//#define _DYNAMICLOAD

#include <stdio.h>
#ifdef _WIN32
#include  <process.h>
#else
#include  <pthread.h>
#endif

#include  "dx2lib.h"
#include  "dxmisc.h"

#define AXISNUM     (8)           // 軸数

// DX2LIB Ver2.8以降に追加されたAPIを使用する際には次行のコメントを外す
//#define USE_NEWAPI

#define MSize(d)    (sizeof(d) / sizeof(d[0]))

//-----------------------------------------------------------
// 予約語
//-----------------------------------------------------------
// 各軸の角度と遷移時間の構造体
typedef double TPose[AXISNUM];

typedef struct {
  TPose   angles; // 軸数分の角度情報
  double  sec;    // 遷移時間
} TMotion;

// スレッド伝達用構造体
typedef struct {
  TDeviceID dev;      // デバイスID
  const uint8_t *ids; // IDの配列
  int num;            // ID数
} TMonInfo;

//-----------------------------------------------------------
// グローバル変数
//-----------------------------------------------------------
bool term = false;  // スレッド終了フラグ

//===========================================================
// 2点間補間 (1フレーム)
//===========================================================
void P2P (TDeviceID dev, const uint8_t *ids, const TPose *p0, const TPose *p1, int num, uint32_t ms) {
  double t0 = GetQueryPerformanceCounter ();
  double t1 = t0 + ms;
  double p[num];
  while (t1 > GetQueryPerformanceCounter ()) {  // 目標時間までループ
    for (int i = 0; i < num; i++)
      p[i] = (*p0)[i] + ((*p1)[i] - (*p0)[i]) * (GetQueryPerformanceCounter () - t0) / (t1 - t0);
    DXL_SetGoalAngles(dev, ids, p, num);  // 角度指令
  }
  DXL_SetGoalAngles(dev, ids, (double *)p1, num);
}

//===========================================================
// モーション再生 (複数フレーム)
//===========================================================
void PLAY (TDeviceID dev, const uint8_t *ids, const TMotion *motion, int framenum) {
#if 1
  for (int i = 0; i < framenum; i++) {
    double t = GetQueryPerformanceCounter () + motion[i].sec * 1000;
#ifdef USE_NEWAPI
    DXL_SetGoalAnglesAndTime2 (dev, ids, motion[i].angles, AXISNUM, motion[i].sec);
#else
    DXL_SetGoalAnglesAndTime (dev, ids, motion[i].angles, AXISNUM, motion[i].sec);
#endif
    while (t > GetQueryPerformanceCounter ()) Sleep (1);
  }
#else
  TPose CurAngle;
  const TPose *p0, *p1;
  for (int i = 0; i < framenum; i++) {
    if (i == 0) {
      DXL_GetPresentAngles (dev, ids, CurAngle, AXISNUM); // 現在角度取得
      p0 = &CurAngle;
      p1 = &motion[0].angles;
    } else {
      p0 = &motion[i - 1].angles;
      p1 = &motion[i].angles;
    }
    P2P (dev, ids, p0, p1, AXISNUM, motion[i].sec * 1000.0);
  }
#endif
}

//-----------------------------------------------------------
// モーションデータ等
//-----------------------------------------------------------
// 存在の有無にかかわらず指定軸数分のIDのテーブル
const uint8_t my_id_list[AXISNUM] = {
  1,  2,  3,  4,  5,  6,  7,  8
};

// モーションデータ1
const TMotion motion1[] = {
  {{   0,   0,   0,   0,   0,   0,   0,   0}, 1.0},
};

// モーションデータ2
const TMotion motion2[] = {
  {{  90, 390,  90,  90,  90,  90,  90,  90}, 3.0},
  {{   0,   0,   0,   0,   0,   0,   0,   0}, 2.0},
  {{-290,-190, -90, -90, -90, -90, -90, -90}, 3.0},
  {{  45, 145,  45,  45,  45,  45,  45,  45}, 3.0},
  {{ -30,-230, -30, -30, -30, -30, -30, -30}, 3.0},
};

//===========================================================
// 複数軸の現在角度・角速度・エラー情報をモニタするスレッド
//===========================================================
#ifdef _WIN32
unsigned __stdcall monthread (void *pdev) {
#else
void *monthread (void *pdev) {
#endif
  TMonInfo *info = (TMonInfo *)pdev;
  double  pangle[info->num], pvelo[info->num], pcur[info->num];
  while (!term) {
    for (int i = 0; i < info->num; i++) pangle[i] = pvelo[i] = pcur[i] = 0.0;
    // 現在位置取得
    DXL_GetPresentAngles (info->dev, info->ids, pangle, info->num);
    // 現在角速度取得
    DXL_GetPresentVelocities (info->dev, info->ids, pvelo, info->num);
    // 現在電流取得
    DXL_GetPresentCurrents(info->dev, info->ids, pcur, info->num);
    for (int i = 0; i < info->num; i++)
      printf ("(%d:$%04X %4.0f%6.1f)", info->ids[i], DXL_GetErrorCode (info->dev, info->ids[i]), pangle[i], pvelo[i]);
    printf("\r");
    Sleep (10);
  }
#ifdef _WIN32
  _endthreadex (0);
  return 0;
#else
  return NULL;
#endif
}

//===========================================================
// メイン
//===========================================================
int main (void) {
  TMonInfo info;
#ifdef _WIN32
  HANDLE th;
#else
  pthread_t th;
#endif

#if defined(_DYNAMICLOAD) && defined(_WIN32)
  if (LoadDLL ()) {
#endif
    TDeviceID dev = DX2_OpenPort (_COMPORT, _BAUDRATE);
    if (dev != 0) {
      printf ("Successful opening of %s\n", _COMPORT);
      // 指定されたIDを検索しその一覧を表示
      for (int i = 0; i < AXISNUM; i++)
        printf ("id=%2d, ModelName=%s\n", my_id_list[i], DXL_GetModelInfo (dev, my_id_list[i])->name);

      info.dev = dev;
      info.ids = my_id_list;
      info.num = AXISNUM;

#ifdef _WIN32
      th = (HANDLE)_beginthreadex (NULL, 0, &monthread, (void *)&info, 0, NULL);
#else
      pthread_create (&th, NULL, monthread, (void *)&info);
#endif

#ifdef USE_NEWAPI
      // Drive ModeのProfile configurationをTime-based Profileに設定
      printf("SetDriveMode=%s\n",DXL_SetDriveModesEquival (dev, my_id_list, AXISNUM, 0x4) ? "OK" : "NG");
#else
      // Drive ModeのProfile configurationをVelocity-based Profileに設定
      printf("SetDriveMode=%s\n",DXL_SetDriveModesEquival (dev, my_id_list, AXISNUM, 0x0) ? "OK" : "NG");
#endif
      // MultiTurnJointモードに設定
      printf("SetOpMode=%s\n",DXL_SetOperatingModesEquival (dev, my_id_list, AXISNUM, 4) ? "OK" : "NG");

      // 制御開始
      DXL_SetTorqueEnablesEquival (dev, my_id_list, AXISNUM, true);

      printf("\nMOTION1\n");
      PLAY (dev, my_id_list, motion1, MSize(motion1));

      printf("\nMOTION2\n");
      PLAY (dev, my_id_list, motion2, MSize(motion2));

      Sleep (5000);
      // 制御停止
      DXL_SetTorqueEnablesEquival (dev, my_id_list, AXISNUM, false);

      term = true;
#ifdef _WIN32
      WaitForSingleObject (th, 2000);
      CloseHandle (th);
#else
      pthread_join (th, NULL);
#endif

      DX2_ClosePort (dev);
    } else printf ("Failed to open %s\n", _COMPORT);
#if defined(_DYNAMICLOAD) && defined(_WIN32)
    UnloadDLL();
  }
#endif
}
