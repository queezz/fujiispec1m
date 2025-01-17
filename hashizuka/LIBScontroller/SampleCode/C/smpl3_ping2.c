/*
  PING2でネットワーク上の対象を検索
*/

#include  <stdio.h>

#include  "dx2lib.h"
// サンプル用のポート及びIDのマクロ宣言
#include  "dxmisc.h"

uint32_t baudlist[7] = { 9600, 57600, 115200, 1000000, 2000000, 3000000, 4000000 };

int main (void) {
  TDeviceID   dev;
  TErrorCode  err;
  uint32_t    num;            // 最大検索数と検索結果
  TDx2AlarmStatus stat[253];  // 検索結果の保存先

  if ((dev = DX2_OpenPort (_COMPORT, _BAUDRATE))) {
      printf ("Successful opening of %s\n", _COMPORT);

    for (int b = 0; b < 7; b++) {
      DX2_SetBaudrate (dev, baudlist[b]);
      printf("\rBaud=%d[bps]         ", baudlist[b]);

      num = 253;

      if (DX2_Ping2 (dev, &num, stat, &err)) {
        printf ("\n %d device found\n", num);
        for (int i = 0; i < num; i++)
          printf (" Found ID=%d stat:$%04X\n", stat[i].id, stat[i].Status);
      }
    }
    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", _COMPORT);

  printf ("\nFin\n");
}
