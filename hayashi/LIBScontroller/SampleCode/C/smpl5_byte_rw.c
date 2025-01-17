/*
  8bit幅のアイテムへのアクセス
*/

#include  <stdio.h>
#include  <string.h>
#ifndef _WIN32
#include  "kbhit.h"
#endif

#include  "dx2lib.h"
#include  "dxmisc.h"
#include  "dx2memmap.h"

//===========================================================
// 実行時にポート番号・ボーレート・IDを選択
//===========================================================
bool InputNum (int32_t *num) {
  char buf[100];
#ifndef _WIN32
  disable_raw_mode ();
#endif
  fgets (buf, sizeof(buf), stdin);
  rewind (stdin);
#ifndef _WIN32
  enable_raw_mode ();
#endif
  return (strlen(buf) && sscanf(buf, "%d", num) == 1);
}

bool InputStr (char *str) {
  char buf[100];
#ifndef _WIN32
  disable_raw_mode ();
#endif
  fgets (buf, sizeof(buf), stdin);
  rewind (stdin);
#ifndef _WIN32
  enable_raw_mode ();
#endif
  return (strlen(buf) && sscanf(buf, "%s", str) == 1);
}

bool EditComAndBaudAndTarget (char *comport, uint32_t *baud, uint8_t *id) {
  bool result = true;

  int32_t n;

  printf ("Input comport name (default %s) = ", _COMPORT);
  if (!(result = InputStr (comport))) result = strcpy (comport, _COMPORT);

  printf ("Input baudrate (default %dbps) = ", _BAUDRATE);
  if (!InputNum (&n)) { n = _BAUDRATE; result = false; }
  if (n >= 9600 && n <= 4000000) *baud = n; else *baud = _BAUDRATE;

  printf ("Input target id (default %d) = ", _TARGETID);
  if (!InputNum (&n)) { n = _TARGETID; result = false; }
  if (n >= 1 && n < 0xfd) *id = n; else *id = _TARGETID;

  return result;
}

//===========================================================
// メイン
//===========================================================
int main (void) {
  uint8_t     led;
  TDeviceID   dev;
  TErrorCode  err;

  char comname[20];
  uint32_t baud;
  uint8_t id, ledval;

  // デフォルトの設定から初期条件を変更
  EditComAndBaudAndTarget (comname, &baud, &id);
  printf ("\nCOM=%s, Baudrate=%d, id=%d\n", comname, baud, id);

  // ポートオープン
  if ((dev = DX2_OpenPort (comname, baud))) {
    printf ("Successful opening of %s\n", comname);

    uint16_t modelno, addr_led;
    // モデル番号からシリーズを判定し使用するアドレスを変更
    if (DX2_ReadWordData (dev, id, 0, &modelno, NULL)) {
      switch (CheckType (modelno)) {
        case devtXL320: addr_led = 25;                    ledval = 1;   break;
        case devtX:     addr_led = ADDRESS_X_LED_RED;     ledval = 1;   break;
        case devtPRO:   addr_led = ADDRESS_PRO_LED_RED;   ledval = 255; break;
        case devtPROP:  addr_led = ADDRESS_PROP_LED_RED;  ledval = 255; break;
        default:        addr_led = 0;                                   break;
      }
      if (addr_led != 0) {
        for (int i = 0; i < 10; i++) {
          // LEDの状態を読み出し
          if (DX2_ReadByteData (dev, id, addr_led, &led, &err))
            printf ("Read ok led=%d\n", led);
          else
            printf ("Read ng [$%04X]\n", err);

          // 読み出したLEDの値をledvalでXOR
          led ^= ledval;

          // LEDへ値を書き込み
          if (DX2_WriteByteData (dev, id, addr_led, led, &err))
            printf ("Write ok led=%d\n", led);
          else
            printf ("Write ng [$%04X]\n", err);

          // 0.5秒待ち
          Sleep (500);
        }
      } else printf ("Not supported device ID=%d\n", id);
    } else printf ("Cannot read model no.\n");

    // ポートクローズ
    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", comname);

#ifndef _WIN32
  disable_raw_mode ();
#endif

  printf ("Fin\n");
}
