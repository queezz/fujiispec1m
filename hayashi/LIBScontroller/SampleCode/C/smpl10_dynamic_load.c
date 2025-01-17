
/*
  DLLが無くてもひとまず死なないだけ(Windows Only)

  ターゲット:PRO+
*/

#include  <stdio.h>
#ifdef _WIN32
#include  <conio.h>
#else
#include  "kbhit.h"
#endif

#define _DYNAMICLOAD  // ★★dx2lib.hをインクルードする前に宣言★★
#include  "dx2lib.h"
#include  "dxmisc.h"
#include  "dx2memmap.h"

//===========================================================
// メイン
//===========================================================
int main (void) {
  int32_t     maxpos, minpos, diffpos, pos;
  TDeviceID   dev;
  TErrorCode  err;

#if defined(_DYNAMICLOAD) && defined(_WIN32)
  // DLLをロード
  if (LoadDLL()) {
#endif

    if ((dev = DX2_OpenPort (_COMPORT, _BAUDRATE))) {
      printf ("Successful opening of %s\n", _COMPORT);
      uint16_t modelno;
      if (DX2_ReadWordData (dev, _TARGETID, 0, &modelno, NULL)) {
        if (CheckType (modelno) == devtPROP) {
          if (
            // 最大角度取得
            DX2_ReadLongData (dev, _TARGETID, ADDRESS_PROP_MAX_POSITION_LIMIT, (uint32_t *)&maxpos, &err) &&
            // 最小角度取得
            DX2_ReadLongData (dev, _TARGETID, ADDRESS_PROP_MIN_POSITION_LIMIT, (uint32_t *)&minpos, &err) &&
            // 現在角度取得
            DX2_ReadLongData (dev, _TARGETID, ADDRESS_PROP_PRESENT_POSITION, (uint32_t *)&pos, &err) &&
            // トルクイネーブル
            DX2_WriteByteData (dev, _TARGETID, ADDRESS_PROP_TORQUE_ENABLE, 1, &err)
          ) {
            diffpos = maxpos / 2000;
            printf ("maxpos=%d, minpos=%d, presentpos = %d\n", maxpos, minpos, pos);

            // 何かキーを押すとループを抜ける
            while (!kbhit()) {
              pos += diffpos;
              if (pos >= maxpos) {
                pos = maxpos;
                diffpos *= -1;
              } else if (pos <= minpos) {
                pos = minpos;
                diffpos *= -1;
              }
              // 目標位置を書き込み
              if (DX2_WriteLongData (dev, _TARGETID, ADDRESS_PROP_GOAL_POSITION, pos, &err))
                printf ("Sent position = %7d\r", pos);
              else
                printf ("Write error [$%04X]\n", err);
              // 1ミリ秒待ち
              Sleep (1);
            }
            // トルクディスエーブル
            DX2_WriteByteData (dev, _TARGETID, ADDRESS_PROP_TORQUE_ENABLE, 0, &err);
            printf ("\n");
          } else printf ("Read or Torque enable error [$%04X]\n", err);
        } else printf ("Type did not match\n");
      } else printf ("Coundn't read modelno\n");

      DX2_ClosePort (dev);
    } else printf ("Failed to open %s\n", _COMPORT);

#if defined(_DYNAMICLOAD) && defined(_WIN32)
    // DLLをアンロード
    UnloadDLL();
  } else printf ("Fail to load DLL!\n");
#endif

  printf ("Fin\n");
}
