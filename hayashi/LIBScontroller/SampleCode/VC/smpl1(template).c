/*
  ライブラリを使用するための骨子
*/

#include <stdio.h>    // 何も無くともとりあえずインクルード
#include "dx2lib.h"   // Dynamixelライブラリのヘッダ

// マクロ定義
#define COMPORT   "\\\\.\\COM8" // ポート番号
#define BAUDRATE  (1000000)     // ボーレート[bps]

void main (void) {
  // デバイスID (COMポート毎のユニーク値)
  TDeviceID  dev;

  // 指定されたパラメータでポートを開く
  // 成功すれば0以外のユニークな値(デバイスID)が返される
  // 以後このデバイスIDを使用する
  dev = DX2_OpenPort (COMPORT, BAUDRATE);
  // オープン直後に通信を開始すると挙動が怪しいI/Fがあるので1秒程度待つことをお勧めする
  // 問題ない環境であれば削除して構わない
  Sleep(1000);

  // devが0でなければポートを開くのに成功
  if (dev) {
    printf ("Open success\n");

    // ----ここにやりたい事を書く------------


    // --------------------------------------

    // 使い終わったポートは閉じる
    DX2_ClosePort (dev);

  } else {
    printf ("Open error\n");
  }
  printf ("Fin\n");

  // プログラムがいきなり終了しない様、キー入力待ちをさせる
  getchar ();
}
