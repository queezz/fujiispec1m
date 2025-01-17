/*----------------------------------------------------------*
    dx2lib.c
    Dynamixelプロトコル2用API V2.9
                                       Last Edit '22 03/01
   Copyright (c) 2005, 2022 BestTechnology CO.,LTD.
 *----------------------------------------------------------*/

/*----------------------------------------------------------------------
  【プログラム概要】
    Dynamixel2の基本的なパケット送受信ルーチン

  【注意事項】
    COMポート番号が提供されるDynamixel用RS485ないしTLL I/Fが対象です。

  【ドキュメント・その他】
    Serial Communications in Win32:
      https://docs.microsoft.com/en-us/previous-versions/ms810467(v=msdn.10)?redirectedfrom=MSDN
    Serial HOWTO in Linux
      https://tldp.org/HOWTO/Text-Terminal-HOWTO-7.html
    Dynamixel通信プロトコル:
      https://www.besttechnology.co.jp/modules/knowledge/?DYNAMIXEL%20Communiation%20Protocol%202.0
  ----------------------------------------------------------------------*/

#ifdef _WIN32
#include  <tchar.h>
#include  <process.h>
#else
#include  <fcntl.h>
#include  <termios.h>
#include  <unistd.h>
#include  <errno.h>
#include  <sys/ioctl.h>
#include  <sys/time.h>
#ifndef __APPLE__
#include  <linux/serial.h>
#else
#include  <IOKit/serial/ioss.h>
#endif

#define Sleep(w)  usleep(w*1000)
#endif

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  "./dx2lib.h"

#define MAX_COMPORT         (256)

// 通信管理用構造体
typedef struct {
  char              name[30];   // ポート名
  uint32_t          baudrate;   // 通信速度
  uint32_t          offsettime; // タイムアウトのオフセット時間
  HANDLE            hComm;      // デバイスハンドル
  CRITICAL_SECTION  tpcs_s;     // 共通API排他処理用
  CRITICAL_SECTION  tpcs_m;     // 汎用API排他処理用
#ifndef _WIN32
  fd_set            rfds;
#endif
} TComInfo;
static CRITICAL_SECTION tpcs;                 // DLL内排他処理用
static TDeviceID        ComInfo[MAX_COMPORT]; // 通信ポート管理テーブル
static int              RegNum;
#ifdef _WIN32
static LARGE_INTEGER    sysfreq;              // マイクロ秒計測用
static bool called = false;
#endif

uint16_t crc16_lutable[256];
static void MakeCRC16Table (uint16_t poly, uint16_t *table);

//------------------------------------------------------------------------------
#ifdef _WIN32
BOOL APIENTRY DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  int i;
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
      if (!called) {
        called = true;
        InitializeCriticalSection (&tpcs);
        QueryPerformanceFrequency (&sysfreq);
        for (i = 0; i < MAX_COMPORT; i++) ComInfo[i] = 0;
        MakeCRC16Table (0x8005, crc16_lutable);
      }
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      if (called == true) {
        called = false;
        for (i = 0; i < MAX_COMPORT; i++) {
          if (ComInfo[i]) DX2_ClosePort (ComInfo[i]);
        }
        DeleteCriticalSection (&tpcs);
      }
      break;
    default:
      break;
  }
  return  true;
}

bool APIENTRY DllEntryPoint (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  return DllMain (hinstDLL, fdwReason, lpvReserved);
}
#else
static void __attribute__ ((constructor)) attach (void) {
  pthread_mutex_init (&tpcs, NULL);
  for (int i = 0; i < MAX_COMPORT; i++) ComInfo[i] = 0;
  RegNum = 0;
  MakeCRC16Table (0x8005, crc16_lutable);
}

static void __attribute__ ((destructor)) detach (void) {
  for (int i = 0; i < MAX_COMPORT; i++) {
    if (ComInfo[i]) DX2_ClosePort (ComInfo[i]);
  }
  pthread_mutex_destroy (&tpcs);
  RegNum = 0;
}
#endif

/*----------------------------------------------------------------------------
    double GetQueryPerformanceCounter (void)
  ----------------------------------------------------------------------------
    DESCRIPTION:
        ミリ秒起動時間計
  ----------------------------------------------------------------------------*/
DXAPIDLL double GetQueryPerformanceCounter (void) {
#ifdef _WIN32
  LARGE_INTEGER now;

  QueryPerformanceCounter (&now);
  return ((double)now.QuadPart / (double)sysfreq.QuadPart) * (double)1000.0;
#else
  struct timeval nowtime;
  if (gettimeofday (&nowtime, NULL) == 0) {
    return ((double) (nowtime.tv_sec) + (double) (nowtime.tv_usec) * 0.000001) * (double)1000.0;
  }
  return 0;
#endif
}

//------------------------------------------------------------------------------
// bool DX2_Active (TDeviceID dvid)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    通信ポートが開いているかどうか
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//  RETURNS:
//    true            正常
//    false           エラー
//
DXAPIDLL bool DX2_Active (TDeviceID dvid) {
#ifdef _WIN32
  DCB           cDcb;
#else
  struct termios term;
#endif
  bool          result = false;

  if (dvid != 0 && RegNum != 0) {
#ifdef _WIN32
    result = GetCommState (((TComInfo *)dvid)->hComm, &cDcb);
#else
    result = (tcgetattr (((TComInfo *)dvid)->hComm, &term) >= 0);
#endif
  }
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_ClosePort (TDeviceID dvid)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    通信ポートを閉じる
//  PARAMS:
//    TDeviceID       dvid        デバイスIDを指定
//  RETURNS:
//    true            成功
//    false           失敗
//
DXAPIDLL bool DX2_ClosePort (TDeviceID dvid) {
  int           i;
  TComInfo      *pInfo = (TComInfo *)dvid;
  bool          result = false;

  EnterCriticalSection (&tpcs);

  if (dvid != 0 && RegNum != 0) {
    for (i = 0; i < MAX_COMPORT; i++) {
      if (ComInfo[i] == dvid) {
        ComInfo[i] = 0;
        RegNum--;
        break;
      }
    }
    if (i < MAX_COMPORT) {
#ifdef _WIN32
      if (pInfo->hComm != INVALID_HANDLE_VALUE) {
        PurgeComm (pInfo->hComm, (PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR));
        CloseHandle (pInfo->hComm);
      }
#else
      if (pInfo->hComm != INVALID_HANDLE_VALUE) {
        tcflush (pInfo->hComm, TCIOFLUSH);
        close (pInfo->hComm);
      }
#endif

      DeleteCriticalSection (& (pInfo->tpcs_s));
      DeleteCriticalSection (& (pInfo->tpcs_m));

      free (pInfo);
      result = true;
    }
  }

  LeaveCriticalSection (&tpcs);

  return result;
}

//------------------------------------------------------------------------------
//  TDeviceID DX2_OpenPort (char *name, uint32_t baud) {
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    通信ポートを開く
//  PARAMS:
//    char            *name       通信ポートを指定
//    uint32_t        baud        ボーレートを指定
//  RETURNS:
//    TDeviceID       NULLでない場合：Device ID
//                    NULLの場合は失敗
//
DXAPIDLL TDeviceID DX2_OpenPort (char *name, uint32_t baud) {
  int           i, index;
  bool          errflag = false;
  TComInfo      *pInfo = NULL;
  TDeviceID     result = 0;
#ifdef _WIN32
  COMMTIMEOUTS  timeouts = { 0, 0, 0, 0, 0 };
  DCB           cDcb;
#endif

  if (name == NULL) return 0;

  for (i = 0; i < MAX_COMPORT; i++) {
    if (ComInfo[i]) {
      if (!strcmp (((TComInfo *)ComInfo[i])->name, name)) {
        DX2_ClosePort (ComInfo[i]);
        break;
      }
    }
  }

  EnterCriticalSection (&tpcs);

  for (i = 0; i < MAX_COMPORT; i++) {
    if (ComInfo[i] == 0) {
      index = i;
      RegNum++;
      break;
    }
  }
  if (i < MAX_COMPORT) {
    pInfo = (TComInfo *)malloc (sizeof (TComInfo));
    if (pInfo != NULL) {
      ComInfo[index] = (TDeviceID)pInfo;
      memset (pInfo, 0, sizeof (TComInfo));

      strncpy (pInfo->name, name, sizeof (pInfo->name) - 1);
      pInfo->hComm = INVALID_HANDLE_VALUE;

      // デバイスオープン
#ifdef _WIN32
      memset (&cDcb, 0, sizeof (cDcb));
      cDcb.BaudRate           = baud;
      cDcb.ByteSize           = 8;
      cDcb.fParity            = false;
      cDcb.Parity             = NOPARITY;
      cDcb.StopBits           = ONESTOPBIT;
      cDcb.fOutX              = false;
      cDcb.fInX               = false;
      cDcb.fTXContinueOnXoff  = true;
      cDcb.XonLim             = 512;
      cDcb.XoffLim            = 512;
      cDcb.XonChar            = 0x11;
      cDcb.XoffChar           = 0x13;
      cDcb.fOutxDsrFlow       = false;
      cDcb.fOutxCtsFlow       = false;
      cDcb.fRtsControl        = RTS_CONTROL_DISABLE;
      cDcb.fDtrControl        = DTR_CONTROL_DISABLE; //DTR_CONTROL_ENABLE;
      cDcb.fErrorChar         = 0;
      cDcb.fBinary            = true;
      cDcb.fNull              = 0;
      cDcb.fAbortOnError      = false;
      cDcb.wReserved          = 0;
      cDcb.EofChar            = 0x03;
      cDcb.EvtChar            = 0;

      pInfo->hComm = CreateFileA (name, (GENERIC_READ | GENERIC_WRITE), 0, NULL, OPEN_EXISTING, 0, NULL);
      if ((pInfo->hComm != INVALID_HANDLE_VALUE) && (pInfo->hComm != NULL)) {
#else
      pInfo->hComm = open (name, O_RDWR | O_NOCTTY);
      if ((pInfo->hComm != INVALID_HANDLE_VALUE)) {
#endif

        // 諸設定
#ifdef _WIN32
        if (!SetCommTimeouts (pInfo->hComm, &timeouts)) errflag = true;
        if (!SetCommMask (pInfo->hComm, 0)) errflag = true;
        if (!SetCommState (pInfo->hComm, &cDcb)) errflag = true;
        PurgeComm (pInfo->hComm, (PURGE_TXCLEAR | PURGE_RXCLEAR));
#else
        if (!DX2_SetBaudrate ((TDeviceID)pInfo, baud)) errflag = true;
        FD_ZERO (&pInfo->rfds);
        FD_SET (pInfo->hComm, &pInfo->rfds);
#endif
        if (!errflag) {
          InitializeCriticalSection (& (pInfo->tpcs_s));
          InitializeCriticalSection (& (pInfo->tpcs_m));
          pInfo->baudrate = baud;
          pInfo->offsettime = 30;
          result = (TDeviceID)pInfo;
        }
      }
    }
  }

  LeaveCriticalSection (&tpcs);

  if ((pInfo != NULL) && result == 0) DX2_ClosePort ((TDeviceID)pInfo);
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_SetBaudrate (TDeviceID dvid, uint32_t baud)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    通信ポートのボーレートを変更する
//  PARAMS:
//    TDeviceID       dvid        デバイスIDを指定
//    uint32_t        baud        ボーレートを指定
//  RETURNS:
//    true            成功
//    false           失敗
//
DXAPIDLL bool DX2_SetBaudrate (TDeviceID dvid, uint32_t baud) {
  bool          result = false;
  TComInfo      *pInfo = (TComInfo *)dvid;
#ifdef _WIN32
  DCB           cDcb;
#else
  struct termios       term;
#endif

  if (dvid == 0 || RegNum == 0) {

  } else if (pInfo->hComm == INVALID_HANDLE_VALUE) {

  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_s));

#ifdef _WIN32
    PurgeComm (pInfo->hComm, (PURGE_TXCLEAR | PURGE_RXCLEAR));

    GetCommState (pInfo->hComm, &cDcb);
    cDcb.BaudRate = baud;
    if (SetCommState (pInfo->hComm, &cDcb)) result = true;
#else
    tcflush (pInfo->hComm, TCIOFLUSH);

    if (tcgetattr (pInfo->hComm, &term) != -1) {
      // OSによっては仮設定しておかないと後の設定が利かない場合があるので
      result = (cfsetispeed (&term, B9600) == 0) && (cfsetospeed (&term, B9600) == 0);
      term.c_iflag &= ~ (BRKINT | ICRNL | INPCK | ISTRIP | IXON);
      term.c_lflag &= ~ (ICANON | ISIG | IEXTEN | ECHO);
      term.c_cflag = (CS8 | CLOCAL | CREAD);
      term.c_oflag &= ~OPOST;
      term.c_cc[VMIN]  = 1; // 1 byte receive wait
      term.c_cc[VTIME] = 0; // no character timer
      result = (tcsetattr (pInfo->hComm, TCSANOW, &term) == 0);
#ifdef __APPLE__
      // IOSSDATALATは現状効果無し
      unsigned long usec = 1000UL;
      result = result && (ioctl (pInfo->hComm, IOSSDATALAT, &usec) != -1) && (ioctl (pInfo->hComm, IOSSIOSPEED, &baud) != -1);
#else
#include <asm/termios.h>
      struct termios2 tio;
      if (ioctl (pInfo->hComm, TCGETS2, &tio) != -1) {
        tio.c_cflag &= ~CBAUD;
        tio.c_cflag |= BOTHER;
        tio.c_ispeed = baud;
        tio.c_ospeed = baud;
        result = result && (ioctl (pInfo->hComm, TCSETS2, &tio) != -1);
      }
#endif
    }
#endif

    if (result) pInfo->baudrate = baud;

    Sleep (10);
    LeaveCriticalSection (& (pInfo->tpcs_s));
  }
  return result;
}

//------------------------------------------------------------------------------
//  void DX2_SetTimeOutOffset (TDeviceID dvid, uint32_t t)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    通信ポートのボーレートを変更する
//  PARAMS:
//    TDeviceID       dvid        デバイスIDを指定
//    uint32_t        t           タイムアウトのオフセット時間[ms]
//
DXAPIDLL void DX2_SetTimeOutOffset (TDeviceID dvid, uint32_t t) {
  if (dvid != 0 && RegNum != 0) {
    if (((TComInfo *)dvid)->hComm != INVALID_HANDLE_VALUE) {
      ((TComInfo *)dvid)->offsettime = t;
    }
  }
}

//------------------------------------------------------------------------------
//  double CalcTimeout (uint32_t baud, uint32_t num)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    タイムアウト時間の算出
//  PARAMS:
//    uint32_t        baud        ボーレート
//    uint32_t        num         バイト数
//  RETURNS:
//    理論タイムアウト時間[ms]
//
static double CalcTimeout (uint32_t baud, uint32_t num) {
  return (10000.0 * (double)num) / (double)baud;
}

//------------------------------------------------------------------------------
//  void MakeCRC16Table (uint16_t poly, uint16_t* table)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    CRC-16ルックアップテーブルの生成
//  PARAMS:
//    uint16_t        poly        多項式
//    uint16_t        * table     テーブルのポインタ
//
static void MakeCRC16Table (uint16_t poly, uint16_t *table) {
  uint16_t nData;
  uint16_t nAccum;

  for (int i = 0; i < 256; i++) {
    nData = (uint16_t) (i << 8);
    nAccum = 0;
    for (int j = 0; j < 8; j++) {
      if ((nData ^ nAccum) & 0x8000) nAccum = (nAccum << 1) ^poly;
      else nAccum <<= 1;
      nData <<= 1;
    }
    table[i] = nAccum;
  }
}

//------------------------------------------------------------------------------
//  uint16_t crc16 (uint16_t *crc, uint8_t dat)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    CRC-16の算出
//  PARAMS:
//    uint16_t        *crc        CRC値のポインタ
//    uint8_t         dat         バイト値
//  RETURNS:
//    算出後のCRC値
//
static uint16_t crc16 (uint16_t *crc, uint8_t dat) {
  *crc = (*crc << 8) ^ crc16_lutable[ ((*crc >> 8) ^ dat) & 0xff];
  return *crc;
}

//------------------------------------------------------------------------------
//  bool DX2_TxPacket (TDeviceID dvid, uint8_t id, TInstruction inst, const uint8_t *param, uint32_t len, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    インストラクションパケットの送信
//  PARAMS:
//    TDeviceID       dvid        デバイスIDを指定
//    uint8_t         id          送信先IDを指定
//    TInstruction    inst        インストラクション番号を指定
//    uint8_t         *paramm     パラメータデータを指定
//    uint32_t        len         パラメータデータのバイト数を指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            送信正常終了
//    false           エラー

//
//  インストラクションパケット構造
//    $FF $FF $FD $00 [ID] [LEN_L] [LEN_H] [INST] [PARAM1]....[PARAMn] [CRC_L] [CRC_H]
//
DXAPIDLL bool DX2_TxPacket (TDeviceID dvid, uint8_t id, TInstruction inst, const uint8_t *param, uint32_t len, TErrorCode *err) {
  int           i;
  bool          result = false;
  int           patternmatch = 0, exlen = 0;
  uint8_t       buf[32767];
  uint8_t       *pbuf = buf;
  uint16_t      crc = 0;
  uint32_t      wlen;
  TComInfo      *pInfo = (TComInfo *)dvid;
#ifdef _WIN32
  COMSTAT       comStat;
  DWORD         dwErrors;
#endif

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else if (id > BROADCASTING_ID) {
    if (err != NULL) *err = ERR_INVALID_ID;
  } else if (len > DXMAXLENGTH) {
    if (err != NULL) *err = ERR_ILLEGAL_SIZE;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_s));

    if (err != NULL) *err = ERR_NON;

    // ヘッダ
    * (uint32_t *) (void *)&pbuf[0] = 0x00fdffffU;
    // id
    pbuf[4] = id;
    // インストラクション
    pbuf[7] = inst;
    // パラメータ
    for (i = 0; i < len; i++) {
      switch (pbuf[8 + i + exlen] = param[i]) {
        // ヘッダ部との一致パターン検出
        case 0xff:
          patternmatch++;
          break;
        case 0xfd:
          // Suffix付与
          if (patternmatch == 2) {
            exlen++;
            pbuf[8 + i + exlen] = 0xfd;
          }
          patternmatch = 0;
          break;
        default:
          patternmatch = 0;
          break;
      }
    }
    // データ長 (INST+PARAM数+Suffix数+CRC)
    * (uint16_t *) (void *)&pbuf[5] = len + exlen + 3;

    // CRC
    for (i = 0; i < 8 + len + exlen; i++) crc16 (&crc, pbuf[i]);
    * (uint16_t *) (void *)&pbuf[8 + len + exlen] = crc;

    // 全パケット長(指定Param数+suffix追加分+Param以外)
    len += exlen + 10;

    // 送信
#ifdef _WIN32
    ClearCommError (pInfo->hComm, &dwErrors, &comStat);
    PurgeComm (pInfo->hComm, (PURGE_RXCLEAR | PURGE_RXABORT));
    if (WriteFile (pInfo->hComm, buf, len, (LPDWORD)&wlen, NULL)) {
#else
    tcflush (pInfo->hComm, TCIFLUSH);
    if ((wlen = write (pInfo->hComm, buf, len)) >= 0) {
#endif
      result = (wlen == len);
    } else {
      if (err != NULL) *err = ERR_COMM;
    }

    LeaveCriticalSection (& (pInfo->tpcs_s));
  }

  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_RxPacket (TDeviceID dvid, uint8_t *rdata, uint32_t rdatasize, uint32_t *rlen, uint32_t timeout, TErrorCode *err)
//-- ----------------------------------------------------------------------------
//  DESCRIPTION:
//    ステータスパケットの受信
//  PARAMS:
//    TDeviceID       dvid        デバイスIDを指定
//    uint8_t         *rdata      受信されたデータの保存先
//    uint32_t        rdatasize   受信データの保存先サイズ
//    uint32_t        *rlen       受信できたデータのバイト数の保存先
//    uint32_t        timeout     受信タイムアウト時間をmsで指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            パケットとして正常
//    false           エラー
//
//  ステータスパケット構造
//    $FF $FF $FD $00 [ID] [LEN_L] [LEN_H] $55 [ERR] [PARAM1]....[PARAMn] [CRC_L] [CRC_H]
//
static bool readdata (TDeviceID d, uint8_t *buf, int bytetoread, int *bytesread, double tout) {
  TComInfo      *pInfo = (TComInfo *)d;

  if (pInfo->hComm == INVALID_HANDLE_VALUE || bytesread == NULL) {
    return false;
  } else {
    *bytesread = 0;
#ifdef _WIN32
    COMMTIMEOUTS timeouts = { 0, 0, (DWORD) (tout + 1.5), 0, 0 };
    SetCommTimeouts (pInfo->hComm, &timeouts);
    if (ReadFile (pInfo->hComm, buf, bytetoread, (LPDWORD)bytesread, NULL)) {
      return (bytetoread == *bytesread);
    }
    return false;
#else
    struct timeval timeouts;
    int reqestlen = bytetoread, availablelen;

    timeouts.tv_sec = (uint32_t)tout / 1000;
    timeouts.tv_usec = ((uint32_t)tout % 1000) * 1000;

    FD_ZERO (&pInfo->rfds);
    FD_SET (pInfo->hComm, &pInfo->rfds);
    double t = GetQueryPerformanceCounter() + tout;

    if (select (pInfo->hComm + 1, &pInfo->rfds, NULL, NULL, &timeouts) == 0) {
      return false;
    } else {
      // タイムアウト内にQUEUEのサイズが指定バイト長になるまで待機
      while (1) {
        if (GetQueryPerformanceCounter() > t) return false;
        ioctl (pInfo->hComm, FIONREAD, &availablelen);
        if (availablelen >= reqestlen) break;
        else usleep (100);
      }
      // 読み出し(事前のタイミングによっては分割される可能性があるためループで処置)
      while (GetQueryPerformanceCounter() < t) {
        int r = read (pInfo->hComm, &buf[*bytesread], (availablelen > reqestlen) ? reqestlen : availablelen);
        reqestlen -= r;
        *bytesread += r;
        if (*bytesread >= bytetoread) return (bytetoread == *bytesread);
      }
    }
#endif
  }
  return false;
}

static uint32_t squeeze (uint8_t *dest, uint8_t *src, uint32_t len) {
  uint32_t *p, l = 0;

  for (int i = 0; i < len; i++) {
    if (i <= (int)len - 4) {
      p = (uint32_t *) (void *)src;
      if (*p == 0xfdfdffffU) {
        * (uint32_t *) (void *)dest = 0xfdffffU;
        src += 4;
        dest += 3;
        l += 3;
        i += 3;
      } else {
        *dest++ = *src++;
        l++;
      }
    } else {
      *dest++ = *src++;
      l++;
    }
  }

  return l;
}

// 受信時のsuffix処理を有効・無効化
static bool EnableSuffixRemove = true;
extern "C" DXAPIDLL void WINAPI DX2_EnableSuffixRemoval (bool en) {
  EnableSuffixRemove = en;
}

DXAPIDLL bool DX2_RxPacket (TDeviceID dvid, uint8_t *rdata, uint32_t rdatasize, uint32_t *rlen, uint32_t timeout, TErrorCode *err) {

  bool          result = false;
  uint16_t      crc = 0;
  int           i, l, l1 = 0, l2 = 0;
  double        t, t2;

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else if (rdata == NULL) {
    if (err != NULL) *err = ERR_COMM;
  } else if (rdatasize < 5) {
    if (err != NULL) *err = ERR_INVALID_PARAM;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_s));

    if (rlen != NULL) *rlen  = 0;
    if (err != NULL) *err = ERR_NON;

    t = GetQueryPerformanceCounter() + timeout;
    // 先頭9バイト($ff,$ff,$fd,$00,ID,LEN_L,LEN_H,$55,ERR)読み込み
    if (readdata (dvid, rdata, 9, &l1, (double)timeout)) {
      // 受信サイズとタイムアウトを確認
      if (l1 == 9 && GetQueryPerformanceCounter() <= t) {
        // 先頭9バイト整合性チェック
        if ((* (uint32_t *) (void *)&rdata[0] == 0x00fdffffU) && (rdata[7] == INST_STATUS) && (rdata[4] <= 252) && (* (uint16_t *) (void *)&rdata[5] + 3 <= rdatasize)) {
          // インストラクションのエラーフラグをエラーコードに反映
          if (err != NULL) *err |= rdata[8];
          // 残りタイムアウト計算
          t2 = t - GetQueryPerformanceCounter();
          // 残り(LEN-2)バイト読み込み
          if (readdata (dvid, &rdata[9], * (uint16_t *) (void *)&rdata[5] - 2, &l2, t2)) {
            // タイムアウトを確認(ここまで来たら受信データの確認は行う)
            if (GetQueryPerformanceCounter() > t) if (err != NULL) *err |= ERR_TIMEOUT;
            // チェックサム計算
            for (i = 0; i < l1 + l2 - 2; i++) crc16 (&crc, rdata[i]);
            if (* (uint16_t *) (void *)&rdata[l1 + l2 - 2] == crc) {
              // suffix除去
              l = * (uint16_t *) (void *)&rdata[5] - 3;
              if (EnableSuffixRemove) l = squeeze (&rdata[8], &rdata[8], l);
              // 受信データサイズを改変
              * (uint16_t *) (void *)&rdata[5] = l + 3;
              l2 = l + 1;
              result = ((rdata[8] & 0x7f) == 0);
            } else if (err != NULL) *err |= ERR_CHECKSUM;
          } else if (err != NULL) *err |= ERR_TIMEOUT;
        } else if (err != NULL) *err |= ERR_INVALID_PARAM;
      } else if (err != NULL) *err |= ERR_TIMEOUT;
    } else if (err != NULL) *err |= ERR_TIMEOUT;

    if (rlen != NULL) *rlen = l1 + l2;

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_s));
  }

  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_ReadByteData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, TErrorCode *err)
//-- ----------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスの1バイトデータ読み出し
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    uint16_t        adr         アドレスを指定
//    uint8_t         *rdata      受信されたデータの保存先
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_ReadByteData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, TErrorCode *err) {
  return DX2_ReadBlockData (dvid, id, adr, rdata, 1, err);
}

//------------------------------------------------------------------------------
//  bool DX2_ReadWordData (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t *rdata, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスの2バイトデータ読み出し
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    uint16_t        adr         アドレスを指定
//    uint16_t        *rdata      受信されたデータの保存先
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_ReadWordData (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t *rdata, TErrorCode *err) {
  return DX2_ReadBlockData (dvid, id, adr, (uint8_t *)rdata, 2, err);
}

//------------------------------------------------------------------------------
//  bool DX2_ReadLongData (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t *rdata, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスの4バイトデータ読み出し
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    uint16_t        adr         アドレスを指定
//    uint32_t        *rdata      受信されたデータの保存先
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_ReadLongData (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t *rdata, TErrorCode *err) {
  return DX2_ReadBlockData (dvid, id, adr, (uint8_t *)rdata, 4, err);
}

//------------------------------------------------------------------------------
//  bool DX2_WriteByteData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t dat, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスへ1バイトデータ書き込み
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    uint16_t        adr         アドレスを指定
//    uint8_t         dat         送信するデータを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_WriteByteData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t dat, TErrorCode *err) {
  return DX2_WriteBlockData (dvid, id, adr, (uint8_t *)&dat, 1, err);
}

//------------------------------------------------------------------------------
//  bool DX2_WriteWordData (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t dat, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスへ2バイトデータ書き込み
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    uint16_t        adr         アドレスを指定
//    uint16_t        dat         送信するデータを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_WriteWordData (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t dat, TErrorCode *err) {
  return DX2_WriteBlockData (dvid, id, adr, (uint8_t *)&dat, 2, err);
}

//------------------------------------------------------------------------------
//  bool DX2_WriteLongData (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t dat, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスへ4バイトデータ書き込み
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    uint16_t        adr         アドレスを指定
//    uint32_t        dat         送信するデータを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_WriteLongData (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t dat, TErrorCode *err) {
  return DX2_WriteBlockData (dvid, id, adr, (uint8_t *)&dat, 4, err);
}

//------------------------------------------------------------------------------
//  bool DX2_ReadBlockData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, uint32_t len, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスから任意バイト数のデータ読み出し
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    uint16_t        adr         アドレスを指定
//    uint8_t         *rdata      受信されたデータの保存先
//    uint32_t        len         読み出すデータのサイズを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_ReadBlockData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, uint32_t len, TErrorCode *err) {
  uint32_t      l;
  uint8_t       param[10];
  uint8_t       *rbuf, *pbuf = param;
  bool          result = false;
  uint32_t      timeout;

  if (err != NULL) *err = ERR_NON;

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else  if (id == BROADCASTING_ID) {
    if (err != NULL) *err = ERR_INVALID_ID;
  } else  if (len > DXMAXLENGTH || len < 1) {
    if (err != NULL) *err = ERR_ILLEGAL_SIZE;
  } else  if (rdata == NULL) {
    if (err != NULL) *err = ERR_INVALID_PARAM;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));

    * (uint16_t *) (void *)&pbuf[0] = adr;
    * (uint16_t *) (void *)&pbuf[2] = len;
    rbuf = (uint8_t *)malloc (len + (len / 3) + 20);

    if (DX2_TxPacket (dvid, id, INST_READ, param, 4, err)) {
      // 送信完了までのタイムアウト時間を加算する
      timeout = ((TComInfo *)dvid)->offsettime + CalcTimeout (((TComInfo *)dvid)->baudrate, 11 + len + 6);
      if (DX2_RxPacket (dvid, rbuf, len + (len / 3) + 20, &l, timeout, err)) {
        if (rbuf[4] == id) {
          if (l == 11 + len) {
            if ((rbuf[8] & 0x7f) == 0) {
              memcpy (rdata, rbuf + 9, (int)len);
              result = true;
            } else if (err != NULL) *err |= ERR_INVALID_PARAM;
          } else if (err != NULL) *err |= ERR_ILLEGAL_SIZE;
        } else if (err != NULL) *err |= ERR_DIFF_ID;
      }
    }
    free (rbuf);

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_WriteBlockData (TDeviceID dvid, uint8_t id, uint16_t adr, const uint8_t *dat, uint32_t len, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスへ任意バイト数のデータ書き込み
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    uint16_t        adr         アドレスを指定
//    uint8_t         *dat        送信するデータを指定
//    uint32_t        len         書き込むデータのサイズを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_WriteBlockData (TDeviceID dvid, uint8_t id, uint16_t adr, const uint8_t *dat, uint32_t len, TErrorCode *err) {
  uint32_t      l;
  uint8_t       param[len + 2], *pbuf = param;
  uint8_t       rbuf[20];
  bool          result = false;
  uint32_t      timeout;

  if (err != NULL) *err = ERR_NON;

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else if (len > DXMAXLENGTH || len < 1) {
    if (err != NULL) *err = ERR_ILLEGAL_SIZE;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));

    * (uint16_t *) (void *)&pbuf[0] = adr;
    memcpy (&param[2], dat, len);
    if (DX2_TxPacket (dvid, id, INST_WRITE, param, len + 2, err)) {
      if (id != BROADCASTING_ID) {
        timeout = ((TComInfo *)dvid)->offsettime + CalcTimeout (((TComInfo *)dvid)->baudrate, 11 + 6);
        if (DX2_RxPacket (dvid, rbuf, sizeof (rbuf), &l, timeout, err)) {
          if (l == 11) {
            if (rbuf[4] == id) {
              if ((rbuf[8] & 0x7f) == 0) {
                result = true;
              } else if (err != NULL) *err |= ERR_INVALID_PARAM;
            } else if (err != NULL) *err |= ERR_DIFF_ID;
          } else if (err != NULL) *err |= ERR_ILLEGAL_SIZE;
        }
      } else result = true;
    }

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_Ping (TDeviceID dvid, uint8_t id, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスのデバイスへPINGを送信し応答を受信
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            検出
//    false           エラー
//
DXAPIDLL bool DX2_Ping (TDeviceID dvid, uint8_t id, TErrorCode *err) {
  uint32_t      l;
  uint8_t       rbuf[20];
  bool          result = false;
  uint32_t      timeout;

  if (err != NULL) *err = ERR_NON;

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));

    if (DX2_TxPacket (dvid, id, INST_PING, NULL/*param*/, 0, err)) {
      timeout = ((TComInfo *)dvid)->offsettime + CalcTimeout (((TComInfo *)dvid)->baudrate, 14 + 6);
      if (DX2_RxPacket (dvid, rbuf, sizeof (rbuf), &l, timeout, err)) {
        result = ((l == 14) && (rbuf[4] == id) && ((rbuf[8] & 0x7f) == 0));
      }
    }

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_Ping2 (TDeviceID dvid, uint32_t *num, TDx2AlarmStatus *AlarmStatus, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスのデバイスへPINGを送信し応答を受信
//  PARAMS:
//    TDeviceID       dvid          デバイスID
//    uint32_t        *num          検出するデバイス数を指定　APIの処理後に検出されたデバイス数が入る
//    TDx2AlarmStatus *AlarmStatus  検出するデバイス数分のアラームステータスを指定
//    TErrorCode      *err          エラーコード
//  RETURNS:
//    true            検出
//    false           エラー
//
DXAPIDLL bool DX2_Ping2 (TDeviceID dvid, uint32_t *num, TDx2AlarmStatus *AlarmStatus, TErrorCode *err) {
  uint32_t      l;
  uint8_t       rbuf[20];
  TErrorCode    ackerr;
  double        timeout, detecttime;
  int           detectnum = 0;

  if (err != NULL) *err = ERR_NON;

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else if (*num < 1 || 254 < *num) {
    if (err != NULL) *err = ERR_INVALID_PARAM;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
    timeout = ((TComInfo *)dvid)->offsettime + 800;
    detecttime = GetQueryPerformanceCounter() + timeout;
    if (AlarmStatus != NULL) {
      if (DX2_TxPacket (dvid, BROADCASTING_ID, INST_PING, NULL/*param*/, 0, err)) {
        while (detecttime >= GetQueryPerformanceCounter() && *num > 0) {
          if (DX2_RxPacket (dvid, rbuf, sizeof (rbuf), &l, timeout, &ackerr)) {
            if ((rbuf[4] < BROADCASTING_ID) && (l == 14) && (*num > 0)) {
              AlarmStatus->id = rbuf[4];
              AlarmStatus->Status = ackerr;
              AlarmStatus++;
              detectnum++;
              *num = *num - 1;
            } else if (err != NULL) *err |= ERR_TIMEOUT;
          }
        }
      }
    }
    *num = detectnum;

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return detectnum > 0;
}

//------------------------------------------------------------------------------
//  bool DX2_WriteSyncData (TDeviceID dvid, uint8_t *dat, uint32_t size, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    SYNC WRITEインストラクションパケットを送信
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         *dat        SYNCインストラクションのパラメータ部を指定
//    uint32_t        size        datのサイズを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_WriteSyncData (TDeviceID dvid, uint8_t *dat, uint32_t size, TErrorCode *err) {
  bool          result = false;

  if (err != NULL) *err = ERR_NON;

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else if (size > DXMAXLENGTH || size <= 0) {
    if (err != NULL) *err = ERR_ILLEGAL_SIZE;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));

    result = DX2_TxPacket (dvid, BROADCASTING_ID, INST_SYNC_WRITE, dat, size, err);

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_ReadSyncData (TDeviceID dvid, const TSyncReadParam *param, uint32_t *num, uint8_t *dat, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    SYNC READインストラクションパケットを送信
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    TSyncReadParam  *param      パラメータデータを指定
//    uint32_t        *num        読み出すデバイス数を指定　APIの処理後に応答したデバイス数が入る
//    uint8_t         *dat        取得データのポインタ [ID1] {ERR1] {PARAM1...] [ID2] {ERR2] {PARAM2...]  (受信データ長 + 3)*idnumバイト確保の事
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功 (応答数が正しい)
//    false           エラー
//
DXAPIDLL bool DX2_ReadSyncData (TDeviceID dvid, const TSyncReadParam *param, uint32_t *num, uint8_t *dat, TErrorCode *err) {
  int           rnum = 0, acknum = 0;
  uint32_t      l;
  uint8_t       *rbuf;
  TErrorCode    ackerr;
  double        timeout, timeout1, detecttime;
  bool          result = false;

  if (err != NULL) *err = ERR_NON;
  if (num != NULL) {
    rnum = *num;
    *num = 0;
  }

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else if (dat == NULL) {
    if (err != NULL) *err = ERR_INVALID_PARAM;
  } else if (num == NULL) {
    if (err != NULL) *err = ERR_INVALID_PARAM;
  } else if (rnum < 1 || 253 < rnum) {
    if (err != NULL) *err = ERR_INVALID_PARAM;
  } else if (param->length > DXMAXLENGTH || param->length < 0) {
    if (err != NULL) *err = ERR_ILLEGAL_SIZE;
  } else {

    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));

    memset (dat, 0xff, rnum * (param->length + 3));

    timeout1 = ((TComInfo *)dvid)->offsettime + CalcTimeout (((TComInfo *)dvid)->baudrate, rnum * (11 + param->length));
    timeout = ((TComInfo *)dvid)->offsettime + CalcTimeout (((TComInfo *)dvid)->baudrate, (10 + 4 + rnum) + (rnum * (11 + param->length)));
    detecttime = GetQueryPerformanceCounter() + timeout;

    rbuf = (uint8_t *)malloc (param->length + (param->length / 3) + 20);

    if (DX2_TxPacket (dvid, BROADCASTING_ID, INST_SYNC_READ, (uint8_t *)param, rnum + 4, err)) {
      while ((detecttime >= GetQueryPerformanceCounter()) && (rnum > 0)) {
        if (DX2_RxPacket (dvid, rbuf, (param->length + (param->length / 3) + 20), &l, timeout1, &ackerr)) {
          if ((rbuf[4] < BROADCASTING_ID) && (l > 0)) {
            dat[ (param->length + 3) * acknum] = rbuf[4];
            if (l == param->length + 11) {
              dat[ (param->length + 3) * acknum + 1] = ackerr >> 8;
              dat[ (param->length + 3) * acknum + 2] = ackerr;
              memcpy (&dat[ (param->length + 3) * acknum + 3], &rbuf[9], param->length);
            } else {
              ackerr |= ERR_ILLEGAL_SIZE;
              if (err != NULL) *err |= ackerr;
              dat[ (param->length + 3) * acknum + 1] = ackerr >> 8;
              dat[ (param->length + 3) * acknum + 2] = ackerr;
            }
            acknum++;
            rnum--;
          }
        } else if (err != NULL) *err |= ackerr;
      }
    }
    free (rbuf);
    *num = acknum;
    result = (rnum == 0);

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_WriteBulkData (TDeviceID dvid, uint8_t *dat, uint32_t size, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    SYNC WRITEインストラクションパケットを送信
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         *dat        BULKインストラクションのパラメータ部を指定
//    uint32_t        size        datのサイズを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_WriteBulkData (TDeviceID dvid, uint8_t *dat, uint32_t size, TErrorCode *err) {
  bool          result = false;

  if (err != NULL) *err = ERR_NON;

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else if (size > DXMAXLENGTH || size <= 0) {
    if (err != NULL) *err = ERR_ILLEGAL_SIZE;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));

    result = DX2_TxPacket (dvid, BROADCASTING_ID, INST_BULK_WRITE, dat, size, err);

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_ReadBulkData (TDeviceID dvid, const TBulkReadParam *param, uint32_t *num, uint8_t *dat, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    SYNC READインストラクションパケットを送信
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    TBulkReadParam  *param      パラメータデータを指定
//    uint32_t        *num        読み出すデバイス数を指定　APIの処理後に応答したデバイス数が入る
//    uint8_t         *dat        取得データのポインタ [SIZE1] [ID1] {ERR1] {PARAM1...] [SIZE2] [ID2] {ERR2] {PARAM2...]
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功 (応答数が正しい)
//    false           エラー
//
DXAPIDLL bool DX2_ReadBulkData (TDeviceID dvid, const TBulkReadParam *param, uint32_t *num, uint8_t *dat, TErrorCode *err) {
  int           rnum = 0, acknum = 0, ofs = 0;
  uint32_t      l, len;
  uint8_t       *rbuf;
  TErrorCode    ackerr;
  double        timeout, timeout1, detecttime;
  bool          result = false;

  typedef struct {
    uint16_t    size;
    uint8_t     id;
    TErrorCode  Status;
    uint8_t     dat[];
  } _PACKED_ *PResult;
  PResult rdat;

  if (err != NULL) *err = ERR_NON;
  if (num != NULL) {
    rnum = *num;
    *num = 0;
  }

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else if (dat == NULL) {
    if (err != NULL) *err = ERR_INVALID_PARAM;
  } else if (num == NULL) {
    if (err != NULL) *err = ERR_INVALID_PARAM;
  } else if (rnum < 1 || 253 < rnum) {
    if (err != NULL) *err = ERR_INVALID_PARAM;
  } else {
    for (int i = 0; i < rnum; i++) {
      if (param[i].id >= BROADCASTING_ID) {
        if (err != NULL) *err = ERR_INVALID_ID;
        return result;
      }
      if (param[i].length > DXMAXLENGTH || param[i].length < 1) {
        if (err != NULL) *err = ERR_ILLEGAL_SIZE;
        return result;
      }
    }

    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));

    int maxlen = 0, totallen = 0;
    for (int i = 0; i < rnum; i++) {
      if (param[i].length > maxlen) maxlen = param[i].length;
      totallen += (param[i].length + 5);
    }
    memset (dat, 0, totallen);

    timeout1 = ((TComInfo *)dvid)->offsettime + CalcTimeout (((TComInfo *)dvid)->baudrate, rnum * (11 + maxlen));
    timeout = ((TComInfo *)dvid)->offsettime + CalcTimeout (((TComInfo *)dvid)->baudrate, (10 + 4 + rnum) + (rnum * (11 + maxlen)));
    detecttime = GetQueryPerformanceCounter() + timeout;

    rbuf = (uint8_t *)malloc (maxlen + (maxlen / 3) + 20);

    if (DX2_TxPacket (dvid, BROADCASTING_ID, INST_BULK_READ, (uint8_t *)param, rnum * 5, err)) {
      while ((detecttime >= GetQueryPerformanceCounter()) && (rnum > 0)) {
        if (DX2_RxPacket (dvid, rbuf, maxlen + 20, &l, timeout1, &ackerr)) {
          if ((rbuf[4] < BROADCASTING_ID) && (l > 11)) {
            len = l - 11;
            rdat = (PResult)&dat[ofs];
            rdat->size = len + 5;
            rdat->id = rbuf[4];
            rdat->Status = ackerr;
            memcpy (&rdat->dat, &rbuf[9], len);
            acknum++;
            rnum--;
            ofs += len + 5;
          }
        } else if (err != NULL) *err |= ackerr;
      }
    }
    free (rbuf);
    *num = acknum;
    result = (rnum == 0);

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_Reset (TDeviceID dvid, uint8_t id, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスのデバイスへRESETを送信し応答を受信
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_Reset (TDeviceID dvid, uint8_t id, TErrorCode *err) {
  uint32_t      l;
  uint8_t       param[5];
  uint8_t       rbuf[20];
  bool          result = false;
  uint32_t      timeout;

  if (err != NULL) *err = ERR_NON;

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));

    param[0] = 0xff;
    if (DX2_TxPacket (dvid, id, INST_RESET, param, 1, err)) {
      timeout = ((TComInfo *)dvid)->offsettime + CalcTimeout (((TComInfo *)dvid)->baudrate, 11 + 6);
      if (DX2_RxPacket (dvid, rbuf, sizeof (rbuf), &l, timeout, err)) {
        result = ((l == 11) && (rbuf[4] == id) && ((rbuf[8] & 0x7f) == 0));
      }
    }

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return result;
}

//------------------------------------------------------------------------------
//  bool DX2_Reboot (TDeviceID dvid, uint8_t id, TErrorCode *err)
//------------------------------------------------------------------------------
//  DESCRIPTION:
//    任意アドレスのデバイスへREBOOTを送信し応答を受信
//  PARAMS:
//    TDeviceID       dvid        デバイスID
//    uint8_t         id          送信先IDを指定
//    TErrorCode      *err        エラーコード
//  RETURNS:
//    true            成功
//    false           エラー
//
DXAPIDLL bool DX2_Reboot (TDeviceID dvid, uint8_t id, TErrorCode *err) {
  uint32_t      l;
  uint8_t       rbuf[20];
  bool          result = false;
  uint32_t      timeout;

  if (err != NULL) *err = ERR_NON;

  if (dvid == 0 || RegNum == 0) {
    if (err != NULL) *err = ERR_INVALID_DEVID;
  } else if (((TComInfo *)dvid)->hComm == INVALID_HANDLE_VALUE) {
    if (err != NULL) *err = ERR_COMM;
  } else {
    EnterCriticalSection (& (((TComInfo *)dvid)->tpcs_m));

    if (DX2_TxPacket (dvid, id, INST_REBOOT, NULL/*param*/, 0, err)) {
      timeout = ((TComInfo *)dvid)->offsettime + CalcTimeout (((TComInfo *)dvid)->baudrate, 11 + 6);
      if (DX2_RxPacket (dvid, rbuf, sizeof (rbuf), &l, timeout, err)) {
        result = ((l == 11) && (rbuf[4] == id) && ((rbuf[8] & 0x7f) == 0));
      }
    }

    LeaveCriticalSection (& (((TComInfo *)dvid)->tpcs_m));
  }
  return result;
}
