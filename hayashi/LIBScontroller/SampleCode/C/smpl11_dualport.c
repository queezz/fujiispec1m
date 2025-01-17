/*
  複数I/Fの同時使用

  ターゲット:X
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

bool terminate = false;

//===========================================================
// 現在位置読み出し
//===========================================================
bool DxGetPosition (TDeviceID dvid, uint8_t id, int32_t *pos, TErrorCode *err) {
  return DX2_ReadLongData (dvid, id, ADDRESS_X_PRESENT_POSITION, (uint32_t *)pos, err);
}

//===========================================================
// 目標位置書き込み
//===========================================================
bool DxSetPosition (TDeviceID dvid, uint8_t id, int32_t pos, TErrorCode *err) {
  return DX2_WriteLongData (dvid, id, ADDRESS_X_GOAL_POSITION, (uint32_t)pos, err);
}

//===========================================================
// スレッド
//===========================================================
#ifdef _WIN32
unsigned __stdcall thread (void *dum) {
#else
void *thread (void *dum) {
#endif
  TDeviceID   dev;
  TErrorCode  err;
  if ((dev = DX2_OpenPort (_COMPORT2, _BAUDRATE))) {
    printf ("Successful opening of %s\n", _COMPORT2);
    uint16_t modelno;
    if (DX2_ReadWordData (dev, _TARGETID2, 0, &modelno, NULL)) {
      if (CheckType (modelno) == devtX) {
        if (DX2_WriteByteData (dev, _TARGETID2, ADDRESS_X_TORQUE_ENABLE, 1, &err)) {
          int32_t pos = 2047, ppos;
          int     f = 5;
          while (!kbhit()) {
            pos += f;
            if (pos >= 4095) {
              pos = 4095;
              f *= -1;
            } else if (pos <= 0) {
              pos = 0;
              f *= -1;
            }

            if (!DxSetPosition (dev, _TARGETID2, pos, &err))
              printf ("\nSetPos (%s, %d) error [%04X]", _COMPORT2, _TARGETID2, err);
            if (DxGetPosition (dev, _TARGETID2, &ppos, &err))
              printf ("\nGetPos (%s, %d) = %d", _COMPORT2, _TARGETID2, ppos);
            else
              printf ("\nGetPos (%s, %d) error [%04X]", _COMPORT2, _TARGETID2, err);
            Sleep (10);
          }
          DX2_WriteByteData (dev, _TARGETID2, ADDRESS_X_TORQUE_ENABLE, 0, &err);
        } else printf ("Torque could not be enabled\n");
      } else printf ("Type did not match\n");
    } else printf ("Coundn't read modelno\n");
    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", _COMPORT2);

#ifdef _WIN32
  _endthreadex (0);
  return 0;
#else
  return NULL;
#endif
  return 0;
}

//===========================================================
// メイン
//===========================================================
int main (void) {
  TDeviceID   dev;
  TErrorCode  err;
#ifdef _WIN32
  HANDLE      mythread;
#else
  pthread_t   mythread;
#endif

  // _COMPORT2の処理を別スレッドで
#ifdef _WIN32
  if ((mythread = (HANDLE)_beginthreadex (NULL, 0, &thread, 0, 0, NULL)) == INVALID_HANDLE_VALUE) {
#else
  int ret;
  if ((ret = pthread_create (&mythread, NULL, thread, NULL))) {
#endif
    printf ("beginthread error\n");
  }

  // _COMPORTの処理をmainで
  if ((dev = DX2_OpenPort (_COMPORT, _BAUDRATE))) {
    printf ("Successful opening of %s\n", _COMPORT);
    uint16_t modelno;
    if (DX2_ReadWordData (dev, _TARGETID, 0, &modelno, NULL)) {
      if (CheckType (modelno) == devtX) {
        if (DX2_WriteByteData (dev, _TARGETID, ADDRESS_X_TORQUE_ENABLE, 1, &err)) {
          int32_t pos = 2047, ppos;
          int     f = 5;
          while (!kbhit()) {
            pos += f;
            if (pos >= 4095) {
              pos = 4095;
              f *= -1;
            } else if (pos <= 0) {
              pos = 0;
              f *= -1;
            }

            if (!DxSetPosition (dev, _TARGETID, pos, &err))
              printf ("\nSetPos (%s, %d) error [%04X]", _COMPORT, _TARGETID, err);
            if (DxGetPosition (dev, _TARGETID, &ppos, &err))
              printf ("\nGetPos (%s, %d) = %d", _COMPORT, _TARGETID, ppos);
            else
              printf ("\nGetPos (%s, %d) error [%04X]", _COMPORT, _TARGETID, err);
            Sleep (10);
          }
          DX2_WriteByteData (dev, _TARGETID, ADDRESS_X_TORQUE_ENABLE, 0, &err);
        } else printf ("Torque could not be enabled\n");
      } else printf ("Type did not match\n");
    } else printf ("Coundn't read modelno\n");
    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", _COMPORT);

#ifdef _WIN32
  if (mythread != INVALID_HANDLE_VALUE) {
    terminate = true;
    WaitForSingleObject (mythread, 1000);
  }
#else
  if (ret == 0) {
    terminate = true;
    pthread_join (mythread, NULL);
  }
#endif

  printf ("Fin\n");
}
