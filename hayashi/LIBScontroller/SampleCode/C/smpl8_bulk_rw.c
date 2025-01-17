/*
  BULKインストラクションで複数台から一括で読み書き

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

// サンプル用のポート及びIDのマクロ宣言
#include  "dxmisc.h"

#define MAXPOS    (4095)
#define MINPOS    (0)
#define DIFFPOS   (2)

#define AXISNUM   (2)   // 対象となる軸数(_TARGETID=1だとしたら2,3,4...と連続しているものとし全軸存在している必要がある

// 現在位置をREAD BULKで読み出した際のデータ用構造体 (packed必須)
// 本来可変長のデータだが、ここでは各軸から同じアドレス・データ長を指定して取得するので固定化
typedef struct {
  uint16_t size;  // 1軸分のデータサイズ
  uint8_t  id;    // id
  uint16_t err;   // ステータス
  int32_t  ppos;  // データ
} _PACKED_ TBRData_PPos[AXISNUM];

// 目標位置をWRITE BULKで書き込む際のパラメータ用構造体 (packed必須)
typedef struct {
  uint8_t  id;    // id
  uint16_t addr;  // アドレス
  uint16_t len;   // データ長
  int32_t  pos;   // データ
} _PACKED_ TBWParam_Pos[AXISNUM];

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
  TDeviceID   dev;
  TErrorCode  err;

  // READ BULKで現在位置を取得するパラメータ
  TBulkReadParam BRP_Ppos[AXISNUM];
  for (int i = 0; i < AXISNUM; i++) {
    BRP_Ppos[i].id     = i + _TARGETID;
    BRP_Ppos[i].addr   = ADDRESS_X_PRESENT_POSITION;
    BRP_Ppos[i].length = sizeof (int32_t);
  };
  // READ BULKで取得した現在位置
  TBRData_PPos BR_PPos;

  // WRITE BULKで位置を指令するパラメータ
  TBWParam_Pos BWP_Pos;
  for (int i = 0; i < AXISNUM; i++) {
    BWP_Pos[i].id   = i + _TARGETID;
    BWP_Pos[i].addr = ADDRESS_X_GOAL_POSITION;
    BWP_Pos[i].len  = sizeof (int32_t);
  }

  // 目標位置指令毎の増分
  int32_t diff[AXISNUM];
  for (int i = 0; i < AXISNUM; i++) diff[i] = DIFFPOS;

  char comname[20];
  uint32_t baud;
  uint8_t id;

  EditComAndBaudAndTarget (comname, &baud, &id);
  printf("\nCOM=%s, Baudrate=%d, id=%d\n", comname, baud, id);

  if ((dev = DX2_OpenPort (comname, baud))) {
    printf ("Successful opening of %s\n", comname);

    // 全軸がXシリーズである事を確認
    bool pass = true;
    for (int i = 0; i < AXISNUM; i++) {
      uint16_t modelno;
      if (DX2_ReadWordData (dev, BRP_Ppos[i].id, 0, &modelno, NULL))
        pass = pass && (CheckType (modelno) == devtX);
      else
        pass = false;
    }

    if (pass) {
      // 現在位置を取得
      uint32_t num = AXISNUM;
      if (DX2_ReadBulkData (dev, &BRP_Ppos[0], &num, (uint8_t *)&BR_PPos, &err)) {
        printf ("present pos = ");
        for (int i = 0; i < AXISNUM; i++) {
          BWP_Pos[i].pos = BR_PPos[i].ppos; // 開始位置を現在位置に
          printf ("[%d]%4d%s", BR_PPos[i].id, BR_PPos[i].ppos, i == AXISNUM - 1 ? "\n\n" : " ");
        }

        // トルクイネーブル
        DX2_WriteByteData (dev, BROADCASTING_ID, ADDRESS_X_TORQUE_ENABLE, 1, &err);

        // 何かキーを押すとループを抜ける
        while (!kbhit() && DX2_Active (dev)) {
          for (int i = 0; i < AXISNUM; i++) {
            BWP_Pos[i].pos += diff[i];
            if (BWP_Pos[i].pos >= MAXPOS) {
              BWP_Pos[i].pos = MAXPOS;
              diff[i] *= -1;
            } else if (BWP_Pos[i].pos <= MINPOS) {
              BWP_Pos[i].pos = MINPOS;
              diff[i] *= -1;
            }
            printf ("[%d]%4d%s", BWP_Pos[i].id, BWP_Pos[i].pos, i == AXISNUM - 1 ? "\r" : " ");
          }
          // 目標角度指令
          DX2_WriteBulkData (dev, (uint8_t *)&BWP_Pos, sizeof (BWP_Pos), &err);

          // 1ミリ秒待ち
          Sleep (1);
        }

        // トルクディスエーブル
        DX2_WriteByteData (dev, BROADCASTING_ID, ADDRESS_X_TORQUE_ENABLE, 0, &err);
      } else printf ("ReadBulk Error [%04X]\n", err);
    }

    DX2_ClosePort (dev);
  } else printf ("Failed to open %s\n", comname);

#ifndef _WIN32
  disable_raw_mode ();
#endif

  printf ("\nFin");
}
