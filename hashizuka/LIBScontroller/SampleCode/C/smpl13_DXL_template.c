/*
  DXL APIを使う
   デバイスの検索
*/

#include  <stdio.h>

#include  "dx2lib.h"
#include  "dxmisc.h"

//===========================================================
// メイン
//===========================================================
int main (void) {
  TDeviceID dev = DX2_OpenPort (_COMPORT, _BAUDRATE);  // ポートオープン

  if (dev != 0) {
    printf ("Successful opening of %s\n", _COMPORT);

    // 以下のいずれかの方法でAPI内のIDリストを更新
#if 1
    printf("scanning devices...");
    int n = DXL_ScanDevices (dev, NULL);
    printf(" %d device detected.\n", n);
    DXL_PrintDevicesList ((void *)&printf);
#else
    for (uint8_t id = 0; id <= 252; id++) {
      PDXL_ModelInfo p = DXL_GetModelInfo (dev, id);
      if (p->modelno != 0) {
        printf ("[%3d] %-15s ($%04X) %d\n", id, p->name, p->modelno, p->devtype);
      }
    }
#endif

    DX2_ClosePort (dev);   // ポートクローズ
  } else printf ("Failed to open %s\n", _COMPORT);
}
