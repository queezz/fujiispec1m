/*
  dxmisc.h
   サンプルプログラムで使用するポート名・ボーレート・IDのデフォルト値
   その他Dynamixelのモデルから種別取得とLinux上のSleepを提供
   Windows/Linux/macOSで使用可
*/

#ifndef _DXMISC
#define _DXMISC

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#ifndef _WIN32  // LinuxやmacOS時
#include <time.h>
#include <sys/time.h>
#endif

// マクロ定義
#if defined _WIN32 || defined __CYGWIN__ // Windows/MSYS2時
#define _COMPORT    "\\\\.\\COM4"   // ポート名
#define _COMPORT2   "\\\\.\\COM4"
#else       // LinuxやmacOS時
#define _COMPORT    "/dev/ttyUSB0"  // ポート名
#define _COMPORT2   "/dev/ttyUSB1"
#endif
#define _BAUDRATE   (3000000)         // ボーレート[bps]
#define _TARGETID   (2)             // 対象ID
#define _TARGETID2  (4)             // 対象ID

#ifndef _WIN32  // LinuxやmacOS時
//------------------------------------
// ミリ秒スリープ
//------------------------------------
void Sleep (int milliseconds) {
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}
#endif

//------------------------------------
// モデル番号からシリーズを特定
//------------------------------------
TDXL_DevType CheckType (uint16_t modelno) {
  switch (modelno) {
    case 0x015E:  // XL-320
      return devtXL320;

    case 0x001E:  // MX-28(2.0)
    case 0x0137:  // MX-64(2.0)
    case 0x0141:  // MX-106(2.0)

    case 0x04A6:  // XL330-M077
    case 0x04B0:  // XL330-M288
    case 0x04CE:  // XC330-M181
    case 0x04D8:  // XC330-M288
    case 0x04BA:  // XC330-T181
    case 0x04C4:  // XC330-T288
    case 0x0424:  // XL430-W250
    case 0x0442:  // 2XL430-W250
    case 0x0488:  // 2XC430-W250
    case 0x042E:  // XC430-W150
    case 0x0438:  // XC430-W240
    case 0x0406:  // XM430-W210
    case 0x03F2:  // XH430-W210
    case 0x041A:  // XH430-V210
    case 0x03FC:  // XM430-W350
    case 0x03E8:  // XH430-W350
    case 0x0410:  // XH430-V350
    case 0x046A:  // XM540-W150
    case 0x0456:  // XH540-W150
    case 0x047E:  // XH540-V150
    case 0x0460:  // XM540-W270
    case 0x044C:  // XH540-W270
    case 0x0474:  // XH540-V270
    case 0x049C:  // XW540-T140
    case 0x0492:  // XW540-T260
      return devtX;

    case 0xD308:  // H54-200-S500-R
    case 0xD208:  // H54-100-S500-R
    case 0xC800:  // H42-20-S300-R
    case 0xB510:  // M54-60-S250-R
    case 0xB410:  // M54-40-S250-R
    case 0xA918:  // M42-10-S260-R
    case 0x9520:  // L54-50-S290-R
    case 0x9508:  // L54-50-S500-R
    case 0x9428:  // L54-30-S400-R
    case 0x9408:  // L54-30-S500-R
    case 0x8900:  // L42-10-S300-R
      return devtPRO;

    case 0xD309:  // H54-200-S500-RA
    case 0xD209:  // H54-100-S500-RA
    case 0xC801:  // H42-20-S300-RA
    case 0xB511:  // M54-60-S250-RA
    case 0xB411:  // M54-40-S250-RA
    case 0xA919:  // M42-10-S260-RA
    case 0x07E4:  // H54P-200-S500-R
    case 0x07DA:  // H54P-100-S500-R
    case 0x07D0:  // H42P-020-S300-R
    case 0x0848:  // M54P-60-S250-R
    case 0x083E:  // M54P-40-S250-R
    case 0x0834:  // M42P-10-S260-R
      return devtPROP;
    // その他
    default:
      return devtNONE;
  }
}

#if defined __CYGWIN__ // MSYS2時
__attribute__((constructor))
void stdio_no_buffering () {
  setvbuf(stdout, NULL, _IONBF, 0);
}
#endif

#ifdef __cplusplus
}
#endif

#endif //_DXMISC
