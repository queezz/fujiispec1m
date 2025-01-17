/*
  SYNCインストラクションで複数IDの同一アドレスに対して個別の値の読み書き

  ターゲット:X, MX
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

#define MAXPOS    (4095)
#define MINPOS    (0)
#define DIFFPOS   (2)

#define AXISNUM   (2)   // 対象となる軸数(_TARGETID=1だとしたら2,3,4...と連続しているものとし全軸存在している必要がある

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
  char        comname[20];
  uint32_t    baud;
  uint8_t     id;

  uint32_t    num;
  int32_t     diff[AXISNUM];
  TDeviceID   dev;
  TErrorCode  err;

  // goal positionをWriteSyncDataで書き込む際のパラメータ構造体 (1バイトアライメント必須)
  struct {
    uint16_t  addr;     // アドレス
    uint16_t  length;   // データ長
    struct {
      uint8_t id;       // id
      int32_t gpos;     // データ
    } _PACKED_ axis[AXISNUM];
  } _PACKED_ sw_param = {
    ADDRESS_X_GOAL_POSITION,
    sizeof (int32_t),
  };

  // present positionをReadSyncDataで読み出す際の構造体 (1バイトアライメント必須)
  TSyncReadParam sr_param = {
    ADDRESS_X_PRESENT_POSITION,
    sizeof (int32_t),
  };
  // ReadSyncDataで読み出されたデータの構造体 (1バイトアライメント必須)
  struct {
    uint8_t     id;     // id
    TErrorCode  err;    // ステータス
    int32_t     ppos;   // データ
  } _PACKED_ sr_data[AXISNUM];

  for (int i = 0; i < AXISNUM; i++) {
    diff[i] = DIFFPOS;
    sw_param.axis[i].id = i + _TARGETID;
    sr_param.ids[i] = i + _TARGETID;
  }

  // デフォルトの設定から初期条件を変更
  EditComAndBaudAndTarget (comname, &baud, &id);

  // ポートオープン
  if ((dev = DX2_OpenPort (comname, baud))) {
    printf ("Successful opening of %s\n", comname);

    // 全軸がXシリーズである事を確認
    bool pass = true;
    for (int i = 0; i < AXISNUM; i++) {
      uint16_t modelno;
      if (DX2_ReadWordData (dev, sr_param.ids[i], 0, &modelno, NULL))
        pass = pass && (CheckType (modelno) == devtX);
      else
        pass = false;
    }

    if (pass) {
      num = AXISNUM;
      // 各軸の現在位置を読み出し初期値とする
      if (DX2_ReadSyncData (dev, &sr_param, &num, (uint8_t *)&sr_data, &err)) {
        printf ("present pos = ");
        for (int i = 0; i < AXISNUM; i++) {
          sw_param.axis[i].gpos = sr_data[i].ppos;
          printf ("[%d]%4d%s", sr_data[i].id, sr_data[i].ppos, i == AXISNUM - 1 ? "\n\n" : " ");
        }

        // 全軸トルクイネーブル
        DX2_WriteByteData (dev, BROADCASTING_ID, ADDRESS_X_TORQUE_ENABLE, 1, &err);

        // 何かキーを押すとループを抜ける
        while (!kbhit() && DX2_Active (dev)) {
          // 往復運動させるための算数
          for (int i = 0; i < AXISNUM; i++) {
            sw_param.axis[i].gpos += diff[i];
            if (sw_param.axis[i].gpos >= MAXPOS) {
              sw_param.axis[i].gpos = MAXPOS;
              diff[i] *= -1;
            } else if (sw_param.axis[i].gpos <= MINPOS) {
              sw_param.axis[i].gpos = MINPOS;
              diff[i] *= -1;
            }
            printf ("[%d]%4d%s", sw_param.axis[i].id, sw_param.axis[i].gpos, i == AXISNUM - 1 ? "\r" : " ");
          }
          // 目標角度指令
          DX2_WriteSyncData (dev, (uint8_t *)&sw_param, sizeof (sw_param), &err);

          // 1ミリ秒待ち
          Sleep (1);
        }

        // 全軸トルクディスエーブル
        DX2_WriteByteData (dev, BROADCASTING_ID, ADDRESS_X_TORQUE_ENABLE, 0, &err);
      } else printf ("ReadSync Error [%04X]\n", err);
    }

    // ポートクローズ
    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", comname);

#ifndef _WIN32
  disable_raw_mode ();
#endif

  printf ("\nFin");
}
