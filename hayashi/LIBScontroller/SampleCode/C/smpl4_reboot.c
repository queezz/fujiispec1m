/*
  リブート

  ターゲット:PRO, PRO+, X, MX
*/

#include  <stdio.h>

#include  "dx2lib.h"
// サンプル用のポート及びIDのマクロ宣言
#include  "dxmisc.h"

int main (void) {
  TDeviceID   dev;
  TErrorCode  err;

  if ((dev = DX2_OpenPort (_COMPORT, _BAUDRATE))) {
    printf ("Successful opening of %s\n", _COMPORT);

    if (DX2_Reboot (dev, _TARGETID, &err)) printf ("reboot ok\n");
    else printf ("reboot ng err=%04x\n", err);

    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", _COMPORT);

  printf ("\nFin\n");
}
