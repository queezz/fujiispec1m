/*
  DXL APIを使う
   複数軸への角速度指令
*/

#include <stdio.h>
#ifdef _WIN32
#include  <process.h>
#else
#include  <pthread.h>
#endif

#include  "dx2lib.h"
#include  "dxmisc.h"

//-----------------------------------------------------------
// 予約語
//-----------------------------------------------------------
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
// 複数軸の現在角度・角速度・エラー情報をモニタするスレッド
//===========================================================
#ifdef _WIN32
unsigned __stdcall monthread (void *pdev) {
#else
void *monthread (void *pdev) {
#endif
  TMonInfo *info = (TMonInfo *)pdev;
  double  pangle[info->num], pvelo[info->num], pcur[info->num];
  for (int i = 0; i < info->num; i++) pangle[i] = pvelo[i] = pcur[i] = 0.0;
  while (!term) {
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

  // 検出したDynamixelのID一覧
  uint8_t ids[253];
  // 検出したDynamixel数
  int num = 0;

  TDeviceID dev = DX2_OpenPort (_COMPORT, _BAUDRATE);
  if (dev != 0) {
    printf ("Successful opening of %s\n", _COMPORT);

    // 接続された全デバイスを検索しその一覧を表示
    printf ("scanning devices...");
    num = DXL_ScanDevices (dev, ids);
    printf (" %d device detected.\n", num);
    DXL_PrintDevicesList ((void *)&printf);

    // モニタスレッド開始
    info.dev = dev;
    info.ids = ids;
    info.num = num;
#ifdef _WIN32
    th = (HANDLE)_beginthreadex (NULL, 0, &monthread, (void *)&info, 0, NULL);
#else
    pthread_create (&th, NULL, monthread, (void *)&info);
#endif

    // 速度指令用変数
    double velos[num];
    // Velocityモードに設定
    if (DXL_SetOperatingModesEquival (dev, ids, num, 1)) {
      // 制御開始
      DXL_SetTorqueEnablesEquival (dev, ids, num, true);

      for (int i = 0; i < num; i++) velos[i] = 30;  // 30deg/s
      DXL_SetGoalVelocities (dev, ids, velos, num);
      Sleep (2000);
      for (int i = 0; i < num; i++) velos[i] = 50;  // 50deg/s
      DXL_SetGoalVelocities (dev, ids, velos, num);
      Sleep (2000);
      for (int i = 0; i < num; i++) velos[i] = 90;  // 90deg/s
      DXL_SetGoalVelocities (dev, ids, velos, num);
      Sleep (2000);
      for (int i = 0; i < num; i++) velos[i] = -90; // -90deg/s
      DXL_SetGoalVelocities (dev, ids, velos, num);
      Sleep (2000);
      for (int i = 0; i < num; i++) velos[i] = -50; // -50deg/s
      DXL_SetGoalVelocities (dev, ids, velos, num);
      Sleep (2000);
      for (int i = 0; i < num; i++) velos[i] = -30; // -30deg/s
      DXL_SetGoalVelocities (dev, ids, velos, num);
      Sleep (2000);
      for (int i = 0; i < num; i++) velos[i] = 0;   // 0deg/s
      DXL_SetGoalVelocities (dev, ids, velos, num);
      Sleep (500);

      // 制御停止
      DXL_SetTorqueEnablesEquival (dev, ids, num, false);
    }
    // モニタスレッド終了
    term = true;
#ifdef _WIN32
    WaitForSingleObject (th, 2000);
    CloseHandle (th);
#else
    pthread_join (th, NULL);
#endif

    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", _COMPORT);
}
