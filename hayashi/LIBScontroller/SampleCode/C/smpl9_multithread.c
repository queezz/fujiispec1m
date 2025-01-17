/*
  マルチスレッドで1つのターゲットへの非同期アクセス

  ターゲット:PRO, PRO+, X, MX
*/

#include  <stdio.h>
#ifdef _WIN32
#include  <conio.h>
#include  <process.h>
#else
#include  "kbhit.h"
#include  <pthread.h>
#endif

#include  "dx2lib.h"
#include  "dxmisc.h"
#include  "dx2memmap.h"

bool terminate = false;  // スレッド終了フラグ
int16_t adr_max, adr_min, adr_ppos, adr_pos, adr_ten; // 各アイテムのアドレス

//===========================================================
// 現在位置読み出し
//===========================================================
bool GetPosition (TDeviceID dvid, uint8_t id, int32_t *pos, TErrorCode *err) {
  return DX2_ReadLongData (dvid, id, adr_ppos, (uint32_t *)pos, err);
}

//===========================================================
// 目標位置書き込み
//===========================================================
bool SetPosition (TDeviceID dvid, uint8_t id, int32_t pos, TErrorCode *err) {
  return DX2_WriteLongData (dvid, id, adr_pos, (uint32_t)pos, err);
}

//===========================================================
// スレッド
//===========================================================
#ifdef _WIN32
unsigned __stdcall testThread (void *pdev) {
#else
void *testThread (void *pdev) {
#endif
  TDeviceID   dev = (TDeviceID)pdev;
  TErrorCode  err;
  int p;

  while (!terminate && DX2_Active (dev)) {
    // 現在位置の読み取り
    GetPosition (dev, _TARGETID, &p, &err) ? printf ("\rPresent Pos=%7d     ", p) : printf ("\rGet Present Pos error [$%04X]", err);

    Sleep (1);
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
  TDeviceID   dev;
  TErrorCode  err;
  int32_t     maxpos, minpos, diffpos, pos;
#ifdef _WIN32
  HANDLE      mythread;
#else
  pthread_t   mythread;
#endif

#ifdef _WIN32
  SetPriorityClass (GetCurrentProcess(), REALTIME_PRIORITY_CLASS);  // プライオリティを上げる
#endif

  if ((dev = DX2_OpenPort (_COMPORT, _BAUDRATE))) {
    printf ("Successful opening of %s\n", _COMPORT);

    adr_max  = 0;
    adr_min  = 0;
    adr_ppos = 0;
    adr_ten  = 0;
    adr_pos  = 0;

    uint16_t modelno;
    if (DX2_ReadWordData (dev, _TARGETID, 0, &modelno, NULL)) {
      switch (CheckType (modelno)) {
        case devtX:
          adr_max  = ADDRESS_X_MAX_POSITION_LIMIT;
          adr_min  = ADDRESS_X_MIN_POSITION_LIMIT;
          adr_ppos = ADDRESS_X_PRESENT_POSITION;
          adr_ten  = ADDRESS_X_TORQUE_ENABLE;
          adr_pos  = ADDRESS_X_GOAL_POSITION;
          break;
        case devtPRO:
          adr_max  = ADDRESS_PRO_MAX_POSITION_LIMIT;
          adr_min  = ADDRESS_PRO_MIN_POSITION_LIMIT;
          adr_ppos = ADDRESS_PRO_PRESENT_POSITION;
          adr_ten  = ADDRESS_PRO_TORQUE_ENABLE;
          adr_pos  = ADDRESS_PRO_GOAL_POSITION;
          break;
        case devtPROP:
          adr_max  = ADDRESS_PROP_MAX_POSITION_LIMIT;
          adr_min  = ADDRESS_PROP_MIN_POSITION_LIMIT;
          adr_ppos = ADDRESS_PROP_PRESENT_POSITION;
          adr_ten  = ADDRESS_PROP_TORQUE_ENABLE;
          adr_pos  = ADDRESS_PROP_GOAL_POSITION;
          break;
        default:
          adr_max  = 0;
          break;
      }
    }

    if (adr_max != 0) {
      if (
        // 最大角度取得
        DX2_ReadLongData (dev, _TARGETID, adr_max, (uint32_t *)&maxpos, &err) &&
        // 最小角度取得
        DX2_ReadLongData (dev, _TARGETID, adr_min, (uint32_t *)&minpos, &err) &&
        // 現在角度取得
        DX2_ReadLongData (dev, _TARGETID, adr_ppos, (uint32_t *)&pos, &err)
      ) {
        diffpos = (maxpos - minpos) / 2000;
        printf ("maxpos=%d, minpos=%d, presentpos=%d\n", maxpos, minpos, pos);
        // 読み出し専用スレッド作成
#ifdef _WIN32
        if ((mythread = (HANDLE)_beginthreadex (NULL, 0, &testThread, (void *)dev, 0, NULL)) == INVALID_HANDLE_VALUE) {
#else
        if (pthread_create (&mythread, NULL, testThread, (void *)dev)) {
#endif
          printf ("Beginthread error\n");
        } else {
          // トルクイネーブル
          DX2_WriteByteData (dev, _TARGETID, adr_ten, 1, &err);

          // 何かキーを押すとループを抜ける
          while (!kbhit() && DX2_Active (dev)) {
            pos += diffpos;
            if (pos >= maxpos)      {
              pos = maxpos;
              diffpos *= -1;
            } else if (pos <= minpos) {
              pos = minpos;
              diffpos *= -1;
            }
            if (!SetPosition (dev, _TARGETID, pos, &err)) printf ("\nSet Goal Position error [$%04X]\n", err);
            Sleep (1);
          }
          // スレッド終了
          terminate = true;

#ifdef _WIN32
          WaitForSingleObject (mythread, 2000);
          CloseHandle (mythread);
#else
          pthread_join (mythread, NULL);
#endif

          // トルクディスエーブル
          DX2_WriteByteData (dev, _TARGETID, adr_ten, 0, NULL);
        }
      } else printf ("Initalization error\n");
    } else printf ("Not support device or no ack\n");

    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", _COMPORT);

  printf ("\nFin");
}
