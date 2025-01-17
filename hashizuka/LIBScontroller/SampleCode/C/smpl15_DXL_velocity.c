/*
  DXL APIを使う
   角速度指令
*/

#include  <stdio.h>

#include  "dx2lib.h"
#include  "dxmisc.h"

//===========================================================
// メイン
//===========================================================
int main (void) {
  TDeviceID dev = DX2_OpenPort (_COMPORT, _BAUDRATE);

  if (dev != 0) {
    printf ("Successful opening of %s\n", _COMPORT);

    // 指定IDのデバイス情報を取得
    // モデルNo.が既存のDynamixelと一致しないと以後機能しない
    printf ("[id:%d] Model Name=%s\n", _TARGETID, DXL_GetModelInfo (dev, _TARGETID)->name);

    // Velocityモードに設定
    if (DXL_SetOperatingMode (dev, _TARGETID, 1)) {
      // LED点灯
      DXL_SetLED (dev, _TARGETID, true);
      // トルクイネーブル
      DXL_SetTorqueEnable (dev, _TARGETID, true);

      DXL_SetGoalVelocity (dev, _TARGETID, 30.0);   // 30deg/s
      Sleep (5000);
      DXL_SetGoalVelocity (dev, _TARGETID, -30.0);  // -30deg/s
      Sleep (5000);
      DXL_SetGoalVelocity (dev, _TARGETID, 0.0);    // 0deg/s
      Sleep (1000);

      // トルクディスエーブル
      DXL_SetTorqueEnable (dev, _TARGETID, false);
      // LED消灯
      DXL_SetLED (dev, _TARGETID, false);
    } else {
      printf ("SetOPmode error\n");
    }
    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", _COMPORT);
}
