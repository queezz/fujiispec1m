/*
  PINGでネットワーク上の対象を検索
*/

#include  <stdio.h>
#ifdef _WIN32
#include  <conio.h>
#else
#include  "kbhit.h"
#endif
#include  "dx2lib.h"

// サンプル用のポート及びIDのマクロ宣言
#include  "dxmisc.h"

int main (void) {
  TDeviceID   dev;
  TErrorCode  err;

  if ((dev = DX2_OpenPort (_COMPORT, _BAUDRATE))) {
    printf ("Open success\n");

    // ID=0～252もしくはキー入力があるまで検索
    for (int i = 0; (i <= 252) && !kbhit(); i++) {
      // 指定したIDのデバイスがあるかどうか確認
      if (DX2_Ping (dev, i, &err)) printf ("\rFound     ID=%3d [$%04X]\n", i, err);
      else                         printf ("\rNot found ID=%3d [$%04X]\r", i, err);
    }

    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", _COMPORT);

  printf ("\nFin\n");
}
