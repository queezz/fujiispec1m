/*
  32bitアイテムへのアクセス

  ターゲット:PRO, PRO+, X, MX

  モデルを間違えてコントロールテーブルにアクセスすると危険なため、
  対象のモデルが何であるかを判別してから処理を行う。
*/

#include  <stdio.h>
#include  <string.h>
#ifdef _WIN32
#include  <conio.h>
#else
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
  uint16_t    adr_max = 0, adr_min = 0, adr_ppos = 0, adr_pos = 0, adr_ten = 0; // 各アイテムのアドレス
  int32_t     maxpos, minpos, diffpos, pos;
  TDeviceID   dev;
  TErrorCode  err;

  char comname[20];
  uint32_t baud;
  uint8_t id;

  EditComAndBaudAndTarget (comname, &baud, &id);
  printf("\nCOM=%s, Baudrate=%d, id=%d\n", comname, baud, id);

  // ポートオープン
  if ((dev = DX2_OpenPort (comname, baud))) {
    printf ("Successful opening of %s\n", comname);

    uint16_t modelno;
    // モデル番号からシリーズを判定し使用するアドレスを変更
    if (DX2_ReadWordData (dev, id, 0, &modelno, NULL)) {
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
          adr_min  = 0;
          adr_ppos = 0;
          adr_ten  = 0;
          adr_pos  = 0;
          break;
      }
    }
    if (adr_max != 0) {
      // 初期条件の取得とトルクイネーブル
      if (
        // 最大角度取得
        DX2_ReadLongData (dev, id, adr_max, (uint32_t *)&maxpos, &err) &&
        // 最小角度取得
        DX2_ReadLongData (dev, id, adr_min, (uint32_t *)&minpos, &err) &&
        // 現在角度取得
        DX2_ReadLongData (dev, id, adr_ppos, (uint32_t *)&pos, &err) &&
        // トルクイネーブル
        DX2_WriteByteData (dev, id, adr_ten, 1, &err)
      ) {
        DX2_WriteByteData (dev, id, adr_ten, 1, &err);
        diffpos = (maxpos - minpos) / 1000;
        printf ("maxpos=%d, minpos=%d, presentpos=%d\n", maxpos, minpos, pos);

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
          // 目標角度指令
          if (DX2_WriteLongData (dev, id, adr_pos, (uint32_t)pos, &err))
            printf ("Sent position = %7d [%04X]\r\n", pos, err);
          else
            printf ("Sent position error [$%04X]\n", err);
          // 1ミリ秒待ち
          Sleep (1);
        }
        // トルクディスエーブル
        DX2_WriteByteData (dev, id, adr_ten, 0, &err);
        printf ("\n");
      }
    } else printf ("Not supported device ID=%d\n", id);

    // ポートクローズ
    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", comname);

#ifndef _WIN32
  disable_raw_mode ();
#endif

  printf ("Fin\n");
}
