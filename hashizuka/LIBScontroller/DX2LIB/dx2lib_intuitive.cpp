/*----------------------------------------------------------*
   dx2lib_intuitive.cpp
   アドレスやモデルの差異を吸収するサブルーチン集 V2.9
                                       Last Edit '22 03/01
   Copyright (c) 2005, 2022 BestTechnology CO.,LTD.
 *----------------------------------------------------------*/
/*
   モデル毎に異なるコントロールテーブルのアイテムの扱いを吸収し、
   物理値で指令やモニタを行う。
   モデルによって存在しない機能は別として、できるだけモデルやプ
   ロトコルの違いを意識せずにDynamixelを扱う事ができる。

   なお、基本的に通信環境が最良である前提としたため、エラーが生
   じた際のリカバリは行わない。
 */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <math.h>
#include  "./dx2lib.h"
#include  "./dx2memmap.h"

#ifndef _WIN32
#include <unistd.h>
#define Sleep(w)  usleep(w*1000)
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifdef __APPLE__
#define WaitForLittleBit  {\
  struct timespec ___ts = {0, 1};\
  nanosleep(&___ts, NULL);\
}
#else
#define WaitForLittleBit
#endif

static const TDXL_ModelInfo ModelInfoList[] = {
  { 0,      "NONE",             devtNONE },  // 不定な時
  { 0xffff, "UNKNOWN",          devtNONE },  // リストに無い時

  // mode no, name,             type,      position range,        angle range,    velocity range,    pwm range,      velocity coefficient, current coefficient,     pwm coefficient
  { 0x015E, "XL-320",           devtXL320, {    1023,        0 }, {  150, -150 }, {  1023,  -1023 }, {    0,    0 },         0.111 * 6.0,  1100.0 / 1023.0 * 1.000,             0 },

  { 0x001E, "MX-28(2.0)",       devtX,     {    4095,        0 }, {  180, -180 }, {   230,   -230 }, {  885, -885 },         0.229 * 6.0,  1400.0 / 1000.0 * 1.000, 100.0 / 885.0 },
  { 0x0137, "MX-64(2.0)",       devtX,     {    4095,        0 }, {  180, -180 }, {   285,   -285 }, {  885, -885 },         0.229 * 6.0,             3.36 * 1.000, 100.0 / 885.0 },
  { 0x0141, "MX-106(2.0)",      devtX,     {    4095,        0 }, {  180, -180 }, {   210,   -210 }, {  885, -885 },         0.229 * 6.0,             3.36 * 1.000, 100.0 / 885.0 },
  { 0x04A6, "XL330-M077",       devtX,     {    4095,        0 }, {  180, -180 }, {  1620,  -1620 }, {  885, -885 },         0.229 * 6.0,                      1.0, 100.0 / 885.0 },
  { 0x04B0, "XL330-M288",       devtX,     {    4095,        0 }, {  180, -180 }, {   445,   -445 }, {  885, -885 },         0.229 * 6.0,                      1.0, 100.0 / 885.0 },
  { 0x04CE, "XC330-M181",       devtX,     {    4095,        0 }, {  180, -180 }, {  2047,  -2047 }, {  885, -885 },         0.229 * 6.0,                      1.0, 100.0 / 885.0 },
  { 0x04D8, "XC330-M288",       devtX,     {    4095,        0 }, {  180, -180 }, {  2047,  -2047 }, {  885, -885 },         0.229 * 6.0,                      1.0, 100.0 / 885.0 },
  { 0x04BA, "XC330-T181",       devtX,     {    4095,        0 }, {  180, -180 }, {  2047,  -2047 }, {  885, -885 },         0.229 * 6.0,                      1.0, 100.0 / 885.0 },
  { 0x04C4, "XC330-T288",       devtX,     {    4095,        0 }, {  180, -180 }, {  2047,  -2047 }, {  885, -885 },         0.229 * 6.0,                      1.0, 100.0 / 885.0 },
  { 0x0424, "XL430-W250",       devtX,     {    4095,        0 }, {  180, -180 }, {   265,   -265 }, {  885, -885 },         0.229 * 6.0,  1400.0 / 1000.0 * 1.000, 100.0 / 885.0 },
  { 0x0442, "2XL430-W250",      devtX,     {    4095,        0 }, {  180, -180 }, {   250,   -250 }, {  885, -885 },         0.229 * 6.0,  1400.0 / 1000.0 * 1.000, 100.0 / 885.0 },
  { 0x0488, "2XC430-W250",      devtX,     {    4095,        0 }, {  180, -180 }, {   275,   -275 }, {  885, -885 },         0.229 * 6.0,  1400.0 / 1000.0 * 1.000, 100.0 / 885.0 },
  { 0x042E, "XC430-W150",       devtX,     {    4095,        0 }, {  180, -180 }, {   460,   -460 }, {  885, -885 },         0.229 * 6.0,  1400.0 / 1000.0 * 1.000, 100.0 / 885.0 },
  { 0x0438, "XC430-W240",       devtX,     {    4095,        0 }, {  180, -180 }, {   306,   -306 }, {  885, -885 },         0.229 * 6.0,  1400.0 / 1000.0 * 1.000, 100.0 / 885.0 },
  { 0x0406, "XM430-W210",       devtX,     {    4095,        0 }, {  180, -180 }, {   330,   -330 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x03F2, "XH430-W210",       devtX,     {    4095,        0 }, {  180, -180 }, {   210,   -210 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x03F3, "XD430-T210",       devtX,     {    4095,        0 }, {  180, -180 }, {   210,   -210 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x041A, "XH430-V210",       devtX,     {    4095,        0 }, {  180, -180 }, {   230,   -230 }, {  885, -885 },         0.229 * 6.0,             1.34 * 1.000, 100.0 / 885.0 },
  { 0x03FC, "XM430-W350",       devtX,     {    4095,        0 }, {  180, -180 }, {   200,   -200 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x03E8, "XH430-W350",       devtX,     {    4095,        0 }, {  180, -180 }, {   130,   -130 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x03E9, "XD430-T350",       devtX,     {    4095,        0 }, {  180, -180 }, {   130,   -130 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x0410, "XH430-V350",       devtX,     {    4095,        0 }, {  180, -180 }, {   135,   -135 }, {  885, -885 },         0.229 * 6.0,             1.34 * 1.000, 100.0 / 885.0 },
  { 0x0500, "XW430-T200",       devtX,     {    4095,        0 }, {  180, -180 }, {   235,   -235 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x04F6, "XW430-T333",       devtX,     {    4095,        0 }, {  180, -180 }, {   139,   -139 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x046A, "XM540-W150",       devtX,     {    4095,        0 }, {  180, -180 }, {   230,   -230 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x0456, "XH540-W150",       devtX,     {    4095,        0 }, {  180, -180 }, {   300,   -300 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x0457, "XD540-T150",       devtX,     {    4095,        0 }, {  180, -180 }, {   300,   -300 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x047E, "XH540-V150",       devtX,     {    4095,        0 }, {  180, -180 }, {   230,   -230 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x0460, "XM540-W270",       devtX,     {    4095,        0 }, {  180, -180 }, {   128,   -128 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x044C, "XH540-W270",       devtX,     {    4095,        0 }, {  180, -180 }, {   167,   -167 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x044D, "XD540-T270",       devtX,     {    4095,        0 }, {  180, -180 }, {   167,   -167 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x0474, "XH540-V270",       devtX,     {    4095,        0 }, {  180, -180 }, {   128,   -128 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x049C, "XW540-T140",       devtX,     {    4095,        0 }, {  180, -180 }, {   304,   -304 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },
  { 0x0492, "XW540-T260",       devtX,     {    4095,        0 }, {  180, -180 }, {   167,   -167 }, {  885, -885 },         0.229 * 6.0,             2.69 * 1.000, 100.0 / 885.0 },

  { 0x8900, "L42-10-S300-R",    devtPRO,   {    2048,    -2047 }, {  180, -180 }, {   400,   -400 }, {    0,    0 },   1.0 / 303.8 * 6.0 * 36.0, 8250.0 / 2048.0 * 1000,        0 }, //  8476.019999999993
  { 0x9428, "L54-30-S400-R",    devtPRO,   {  144197,  -144197 }, {  180, -180 }, {  9000,  -9000 }, {    0,    0 }, 1.0 / 400.550 * 6.0, 33000.0 / 2048.0 * 1.000,             0 }, // 11495.785
  { 0x9408, "L54-30-S500-R",    devtPRO,   {  180692,  -180692 }, {  180, -180 }, {  9000,  -9000 }, {    0,    0 }, 1.0 / 501.923 * 6.0, 33000.0 / 2048.0 * 1.000,             0 }, // 11544.2290000002
  { 0x9520, "L54-50-S290-R",    devtPRO,   {  103846,  -103846 }, {  180, -180 }, {  8000,  -8000 }, {    0,    0 }, 1.0 / 288.461 * 6.0, 33000.0 / 2048.0 * 1.000,             0 }, //  7499.985999999907
  { 0x9508, "L54-50-S500-R",    devtPRO,   {  180692,  -180692 }, {  180, -180 }, {  8000,  -8000 }, {    0,    0 }, 1.0 / 501.923 * 6.0, 33000.0 / 2048.0 * 1.000,             0 }, //  8030.768000000136
  { 0xA918, "M42-10-S260-R",    devtPRO,   {  131593,  -131593 }, {  180, -180 }, {  8000,  -8000 }, {    0,    0 }, 1.0 / 257.019 * 6.0,  8250.0 / 2048.0 * 1.000,             0 }, //  7196.532000000086
  { 0xB410, "M54-40-S250-R",    devtPRO,   {  125708,  -125708 }, {  180, -180 }, {  8000,  -8000 }, {    0,    0 }, 1.0 / 251.417 * 6.0, 33000.0 / 2048.0 * 1.000,             0 }, //  7140.242799999926
  { 0xB510, "M54-60-S250-R",    devtPRO,   {  125708,  -125708 }, {  180, -180 }, {  8000,  -8000 }, {    0,    0 }, 1.0 / 251.417 * 6.0, 33000.0 / 2048.0 * 1.000,             0 }, //  8321.902699999914
  { 0xC800, "H42-20-S300-R",    devtPRO,   {  151875,  -151875 }, {  180, -180 }, { 10300, -10300 }, {    0,    0 }, 1.0 / 303.750 * 6.0,  8250.0 / 2048.0 * 1.000,             0 }, //  9932.625000000144
  { 0xD208, "H54-100-S500-R",   devtPRO,   {  250961,  -250961 }, {  180, -180 }, { 17000, -17000 }, {    0,    0 }, 1.0 / 501.923 * 6.0, 33000.0 / 2048.0 * 1.000,             0 }, // 16714.03590000028
  { 0xD308, "H54-200-S500-R",   devtPRO,   {  250961,  -250961 }, {  180, -180 }, { 17000, -17000 }, {    0,    0 }, 1.0 / 501.923 * 6.0, 33000.0 / 2048.0 * 1.000,             0 }, // 16613.65130000028
  { 0xA919, "M42-10-S260-RA",   devtPROP,  {  262931,  -262931 }, {  180, -180 }, {  2600,  -2600 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0xB411, "M54-40-S250-RA",   devtPROP,  {  251173,  -251173 }, {  180, -180 }, {  2840,  -2840 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0xB511, "M54-60-S250-RA",   devtPROP,  {  251173,  -251173 }, {  180, -180 }, {  2830,  -2830 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0xC801, "H42-20-S300-RA",   devtPROP,  {  303454,  -303454 }, {  180, -180 }, {  2920,  -2920 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0xD209, "H54-100-S500-RA",  devtPROP,  {  501433,  -501433 }, {  180, -180 }, {  2920,  -2920 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0xD309, "H54-200-S500-RA",  devtPROP,  {  501433,  -501433 }, {  180, -180 }, {  2900,  -2900 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },

  { 0x0834, "PM42-010-S260-R",  devtPROP,  {  262931,  -262931 }, {  180, -180 }, {  2600,  -2600 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0x083E, "PM54-040-S250-R",  devtPROP,  {  251173,  -251173 }, {  180, -180 }, {  2840,  -2840 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0x0848, "PM54-060-S250-R",  devtPROP,  {  251173,  -251173 }, {  180, -180 }, {  2830,  -2830 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0x07D0, "PH42-020-S300-R",  devtPROP,  {  303454,  -303454 }, {  180, -180 }, {  2920,  -2920 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0x07DA, "PH54-100-S500-R",  devtPROP,  {  501433,  -501433 }, {  180, -180 }, {  2920,  -2920 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
  { 0x07E4, "PH54-200-S500-R",  devtPROP,  {  501433,  -501433 }, {  180, -180 }, {  2900,  -2900 }, { 2009, -2009 },          0.01 * 6.0,                      1.0, 100.0 / 2009 },
};

// 全デバイスの情報
static struct TDevices {
  uint8_t     num;
  uint8_t     id2model[256];
  uint8_t     drivemode[256];
  uint8_t     opmode[256];
  TErrorCode  err[256];
} Devices;

// 全デバイス情報初期化
static void initarray (void) {
  static bool init = false;
  if (!init) {
    DXL_InitDevicesList();
    init = true;
  }
}

// 角度→位置
static int32_t ang2pos (double angle, const TDXL_ModelInfo *m) {
  return (angle - m->anglelimit.min) * (m->positionlimit.max - m->positionlimit.min) / (m->anglelimit.max - m->anglelimit.min) + m->positionlimit.min;
}

// 位置→角度
static double pos2ang (int32_t position, const TDXL_ModelInfo *m) {
  return (position - m->positionlimit.min) * (m->anglelimit.max - m->anglelimit.min) / (m->positionlimit.max - m->positionlimit.min) + m->anglelimit.min;
}

//-------------------------------------------------
// LED明滅
//-------------------------------------------------
DXAPIDLL bool DXL_SetLED (TDeviceID dvid, uint8_t id, bool en) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      switch (ModelInfoList[idx].devtype) {
        case devtXL320:
          return DX2_WriteByteData (dvid, id, 25, en ? 1 : 0, &Devices.err[id]);
          break;
        case devtX:
          return DX2_WriteByteData (dvid, id, ADDRESS_X_LED_RED, en ? 1 : 0, &Devices.err[id]);
          break;
        case devtPRO:
          return DX2_WriteByteData (dvid, id, ADDRESS_PRO_LED_RED, en ? 255 : 0, &Devices.err[id]);
          break;
        case devtPROP:
          return DX2_WriteByteData (dvid, id, ADDRESS_PROP_LED_RED, en ? 255 : 0, &Devices.err[id]);
          break;
        default:
          break;
      }
    }
  }
  return false;
}

//-------------------------------------------------
// トルクイネーブル指令
//-------------------------------------------------
DXAPIDLL bool DXL_SetTorqueEnable (TDeviceID dvid, uint8_t id, bool en) {
  initarray();
  if (id <= 252) {
    uint8_t ren;
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      switch (ModelInfoList[idx].devtype) {
        case devtXL320:
          if (DX2_WriteByteData (dvid, id, 24, en ? 1 : 0, &Devices.err[id])) {
            if (DX2_ReadByteData (dvid, id, 24, &ren, &Devices.err[id])) {
              return (((ren == 1) ? true : false) == en);
            }
          }
          break;
        case devtX:
          if (DX2_WriteByteData (dvid, id, ADDRESS_X_TORQUE_ENABLE, en ? 1 : 0, &Devices.err[id])) {
            if (DX2_ReadByteData (dvid, id, ADDRESS_X_TORQUE_ENABLE, &ren, &Devices.err[id])) {
              return (((ren == 1) ? true : false) == en);
            }
          }
          break;
        case devtPRO:
          if (DX2_WriteByteData (dvid, id, ADDRESS_PRO_TORQUE_ENABLE, en ? 1 : 0, &Devices.err[id])) {
            if (DX2_ReadByteData (dvid, id, ADDRESS_PRO_TORQUE_ENABLE, &ren, &Devices.err[id])) {
              return (((ren == 1) ? true : false) == en);
            }
          }
          break;
        case devtPROP:
          if (DX2_WriteByteData (dvid, id, ADDRESS_PROP_TORQUE_ENABLE, en ? 1 : 0, &Devices.err[id])) {
            if (DX2_ReadByteData (dvid, id, ADDRESS_PROP_TORQUE_ENABLE, &ren, &Devices.err[id])) {
              return (((ren == 1) ? true : false) == en);
            }
          }
          break;
        default:
          break;
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetTorqueEnables (TDeviceID dvid, const uint8_t *ids, const bool *ens, int num) {
#ifdef _MSC_VER
  __pragma (pack (push, 1))
  struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint8_t   TorqueEnable;
  } Bulk[num];
  __pragma (pack (pop))
#elif defined(__GNUC__)
  struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint8_t   TorqueEnable;
  } _PACKED_ Bulk[num];
#endif
  bool result = false;
  int size = 0;
  initarray();
  if (ens != NULL) {
    if (ids != NULL && ens != NULL) {
      for (int i = 0; i < num; i++) {
        uint8_t id = ids[i];
        if (id <= 252) {
          int idx = Devices.id2model[id];
          if (idx >= 2) {
            switch (ModelInfoList[idx].devtype) {
              case devtXL320:
                Bulk[size].id = id;
                Bulk[size].addr = 24;
                Bulk[size].size = 1;
                Bulk[size].TorqueEnable = ens[i];
                size++;
                break;
              case devtX:
                Bulk[size].id = id;
                Bulk[size].addr = ADDRESS_X_TORQUE_ENABLE;
                Bulk[size].size = 1;
                Bulk[size].TorqueEnable = ens[i];
                size++;
                break;
              case devtPRO:
                Bulk[size].id = id;
                Bulk[size].addr = ADDRESS_PRO_TORQUE_ENABLE;
                Bulk[size].size = 1;
                Bulk[size].TorqueEnable = ens[i];
                size++;
                break;
              case devtPROP:
                Bulk[size].id = id;
                Bulk[size].addr = ADDRESS_PROP_TORQUE_ENABLE;
                Bulk[size].size = 1;
                Bulk[size].TorqueEnable = ens[i];
                size++;
                break;
              default:
                break;
            }
          }
        }
      }
      if (size > 0) result = DX2_WriteBulkData (dvid, (uint8_t *)Bulk, 6 * size, NULL);
    }
  }
  return result;
}

DXAPIDLL bool DXL_SetTorqueEnablesEquival (TDeviceID dvid, const uint8_t *ids, int num, bool en) {
  bool ten[num];
  initarray();
  for (int i = 0; i < num; i++) ten[i] = en;
  return DXL_SetTorqueEnables (dvid, ids, ten, num);
}

//-------------------------------------------------
// トルクイネーブル取得
//-------------------------------------------------
DXAPIDLL bool DXL_GetTorqueEnable (TDeviceID dvid, uint8_t id, bool *en) {
  initarray();
  if (en != NULL) {
    if (id <= 254) {
      int idx = Devices.id2model[id];
      if (idx >= 2) {
        uint8_t ren;
        switch (ModelInfoList[idx].devtype) {
          case devtXL320:
            if (DX2_ReadByteData (dvid, id, 24, &ren, &Devices.err[id])) {
              *en = (ren == 1);
              return true;
            }
            break;
          case devtX:
            if (DX2_ReadByteData (dvid, id, ADDRESS_X_TORQUE_ENABLE, &ren, &Devices.err[id])) {
              *en = (ren == 1);
              return true;
            }
            break;
          case devtPRO:
            if (DX2_ReadByteData (dvid, id, ADDRESS_PRO_TORQUE_ENABLE, &ren, &Devices.err[id])) {
              *en = (ren == 1);
              return true;
            }
            break;
          case devtPROP:
            if (DX2_ReadByteData (dvid, id, ADDRESS_PROP_TORQUE_ENABLE, &ren, &Devices.err[id])) {
              *en = (ren == 1);
              return true;
            }
            break;
          default:
            break;
        }
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_GetTorqueEnables (TDeviceID dvid, const uint8_t *ids, bool *en, int num) {
  bool result = true;
  initarray();
  if (ids != NULL && en != NULL) {
    for (int i = 0; i < num; i++) {
      int idx = Devices.id2model[ids[i]];
      if (idx >= 2) {
        if (!DXL_GetTorqueEnable (dvid, ids[i], &en[i])) result = false;
      }
    }
  }
  return result;
}

//-------------------------------------------------
// 角度指令
//-------------------------------------------------
DXAPIDLL bool DXL_SetGoalAngle (TDeviceID dvid, uint8_t id, double angle) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      int32_t pmax, pmin;
      if (Devices.opmode[id] == 3) {
        pmax = ModelInfoList[idx].positionlimit.max;
        pmin = ModelInfoList[idx].positionlimit.min;
      } else {
        pmax = INT32_MAX;
        pmin = INT32_MIN;
      }
      int32_t pos = max (min (ang2pos (angle, &ModelInfoList[idx]), pmax), pmin);
      switch (ModelInfoList[idx].devtype) {
        case devtXL320:
          return DX2_WriteLongData (dvid, id, 30, pos, &Devices.err[id]);
          break;
        case devtX:
          return DX2_WriteLongData (dvid, id, ADDRESS_X_GOAL_POSITION, pos, &Devices.err[id]);
          break;
        case devtPRO:
          return DX2_WriteLongData (dvid, id, ADDRESS_PRO_GOAL_POSITION, pos, &Devices.err[id]);
          break;
        case devtPROP:
          return DX2_WriteLongData (dvid, id, ADDRESS_PROP_GOAL_POSITION, pos, &Devices.err[id]);
          break;
        default:
          break;
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetGoalAngles (TDeviceID dvid, const uint8_t *ids, const double *angles, int num) {
#ifdef _MSC_VER
  __pragma (pack (push, 1))
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint16_t  GoalPosition;
  } TBulkXL320;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
  } TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
  } TBulkPRO;
  __pragma (pack (pop))
#elif defined(__GNUC__)
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint16_t  GoalPosition;
  } _PACKED_ TBulkXL320;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
  } _PACKED_ TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
  } _PACKED_ TBulkPRO;
#endif
  uint8_t *data = (uint8_t *)malloc (sizeof (TBulkPRO) * num);
  bool result = false;
  int size = 0;
  initarray();
  if (ids != NULL && angles != NULL) {
    for (int i = 0; i < num; i++) {
      uint8_t id = ids[i];
      if (id <= 252) {
        int idx = Devices.id2model[id];
        if (idx >= 2) {
          int32_t pmax, pmin;
          if (Devices.opmode[id] == 3) {
            pmax = ModelInfoList[idx].positionlimit.max;
            pmin = ModelInfoList[idx].positionlimit.min;
          } else {
            pmax = INT32_MAX;
            pmin = INT32_MIN;
          }
          int32_t pos = max (min (ang2pos (angles[i], &ModelInfoList[idx]), pmax), pmin);
          switch (ModelInfoList[idx].devtype) {
            case devtXL320:
              ((TBulkXL320 *) (&data[size]))->id = id;
              ((TBulkXL320 *) (&data[size]))->addr = 30;
              ((TBulkXL320 *) (&data[size]))->size = 2;
              ((TBulkXL320 *) (&data[size]))->GoalPosition = pos;
              size += sizeof (TBulkXL320);
              break;
            case devtX:
              ((TBulkX *) (&data[size]))->id = id;
              ((TBulkX *) (&data[size]))->addr = ADDRESS_X_GOAL_POSITION;
              ((TBulkX *) (&data[size]))->size = 4;
              ((TBulkX *) (&data[size]))->GoalPosition = pos;
              size += sizeof (TBulkX);
              break;
            case devtPRO:
              ((TBulkPRO *) (&data[size]))->id = id;
              ((TBulkPRO *) (&data[size]))->addr = ADDRESS_PRO_GOAL_POSITION;
              ((TBulkPRO *) (&data[size]))->size = 4;
              ((TBulkPRO *) (&data[size]))->GoalPosition = pos;
              size += sizeof (TBulkPRO);
              break;
            case devtPROP:
              ((TBulkPRO *) (&data[size]))->id = id;
              ((TBulkPRO *) (&data[size]))->addr = ADDRESS_PROP_GOAL_POSITION;
              ((TBulkPRO *) (&data[size]))->size = 4;
              ((TBulkPRO *) (&data[size]))->GoalPosition = pos;
              size += sizeof (TBulkPRO);
              break;
            default:
              break;
          }
        }
      }
    }
    if (size > 0) result = DX2_WriteBulkData (dvid, data, size, NULL);
  }
  free (data);
  return result;
}

//-------------------------------------------------
// 角度取得
//-------------------------------------------------
DXAPIDLL bool DXL_GetPresentAngle (TDeviceID dvid, uint8_t id, double *angle) {
  initarray();
  if (angle != NULL) {
    if (id <= 252) {
      int idx = Devices.id2model[id];
      if (idx >= 2) {
        switch (ModelInfoList[idx].devtype) {
          case devtXL320: {
              int16_t pos;
              if (DX2_ReadWordData (dvid, id, 37, (uint16_t *)&pos, &Devices.err[id])) {
                *angle = pos2ang (pos, &ModelInfoList[idx]);
                return true;
              }
            }
            break;
          case devtX: {
              int32_t pos;
              if (DX2_ReadLongData (dvid, id, ADDRESS_X_PRESENT_POSITION, (uint32_t *)&pos, &Devices.err[id])) {
                *angle = pos2ang (pos, &ModelInfoList[idx]);
                return true;
              }
            }
            break;
          case devtPRO: {
              int32_t pos;
              if (DX2_ReadLongData (dvid, id, ADDRESS_PRO_PRESENT_POSITION, (uint32_t *)&pos, &Devices.err[id])) {
                *angle = pos2ang (pos, &ModelInfoList[idx]);
                return true;
              }
            }
            break;
          case devtPROP: {
              int32_t pos;
              if (DX2_ReadLongData (dvid, id, ADDRESS_PROP_PRESENT_POSITION, (uint32_t *)&pos, &Devices.err[id])) {
                *angle = pos2ang (pos, &ModelInfoList[idx]);
                return true;
              }
            }
            break;
          default:
            break;
        }
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_GetPresentAngles (TDeviceID dvid, const uint8_t *ids, double *angles, int num) {
  bool result = true;
  initarray();
  if (ids != NULL && angles != NULL) {
    for (int i = 0; i < num; i++) {
      int idx = Devices.id2model[ids[i]];
      if (idx >= 2) {
        if (!DXL_GetPresentAngle (dvid, ids[i], &angles[i])) result = false;
      }
    }
  }
  return result;
}

//-------------------------------------------------
// 現在位置で即時停止
//-------------------------------------------------
DXAPIDLL bool DXL_StandStillAngle (TDeviceID dvid, uint8_t id) {
  double pangle;
  initarray();
  if (DXL_GetPresentAngle (dvid, id, &pangle)) return DXL_SetGoalAngleAndVelocity (dvid, id, pangle, 0);
  return false;
}

DXAPIDLL bool DXL_StandStillAngles (TDeviceID dvid, const uint8_t *ids, int num) {
  double pangles[num];
  initarray();
#if 0
  if (DXL_GetPresentAngles (dvid, ids, pangles, num)) return DXL_SetGoalAngles (dvid, ids, pangles, num);
#else
  TAngleVelocity angvelo[num];
  memset (&angvelo, 0, sizeof (angvelo));
  if (DXL_GetPresentAngles (dvid, ids, pangles, num)) {
    for (int i = 0; i < num; i++) angvelo[i].angle = pangles[i];
    return DXL_SetGoalAnglesAndVelocities (dvid, ids, angvelo, num);
  }
#endif
  return false;
}

//-------------------------------------------------
// 角速度指令
//-------------------------------------------------
DXAPIDLL bool DXL_SetGoalVelocity (TDeviceID dvid, uint8_t id, double velocity) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      int32_t velo = max (min (velocity / ModelInfoList[idx].velocityratio, ModelInfoList[idx].velocitylimit.max), ModelInfoList[idx].velocitylimit.min);
      switch (ModelInfoList[idx].devtype) {
        case devtXL320:
          if (velo < 0) velo = abs (velo) | 0x400;
          return DX2_WriteWordData (dvid, id, 32, velo, &Devices.err[id]);
          break;
        case devtX:
          return DX2_WriteLongData (dvid, id, ADDRESS_X_GOAL_VELOCITY, velo, &Devices.err[id]);
          break;
        case devtPRO:
          return DX2_WriteLongData (dvid, id, ADDRESS_PRO_GOAL_VELOCITY, velo, &Devices.err[id]);
          break;
        case devtPROP:
          return DX2_WriteLongData (dvid, id, ADDRESS_PROP_GOAL_VELOCITY, velo, &Devices.err[id]);
          break;
        default:
          break;
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetGoalVelocities (TDeviceID dvid, const uint8_t *ids, const double *velocities, int num) {
#ifdef _MSC_VER
  __pragma (pack (push, 1))
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint16_t  MovingSpeed;
  } TBulkXL320;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalVelocity;
  } TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalVelocity;
  } TBulkPRO;
  __pragma (pack (pop))
#elif defined(__GNUC__)
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint16_t  MovingSpeed;
  } _PACKED_ TBulkXL320;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalVelocity;
  } _PACKED_ TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalVelocity;
  } _PACKED_ TBulkPRO;
#endif
  bool result = false;
  uint8_t *data = (uint8_t *)malloc (sizeof (TBulkPRO) * num);
  int size = 0;
  initarray();
  if (ids != NULL && velocities != NULL) {
    for (int i = 0; i < num; i++) {
      uint8_t id = ids[i];
      if (id <= 252) {
        int idx = Devices.id2model[id];
        if (idx >= 2) {
          int32_t velo = max (min (velocities[i] / ModelInfoList[idx].velocityratio, ModelInfoList[idx].velocitylimit.max), ModelInfoList[idx].velocitylimit.min);
          switch (ModelInfoList[idx].devtype) {
            case devtXL320:
              ((TBulkXL320 *) (&data[size]))->id = id;
              ((TBulkXL320 *) (&data[size]))->addr = 32;
              ((TBulkXL320 *) (&data[size]))->size = 2;
              if (velo < 0) velo = abs (velo) | 0x400;
              ((TBulkXL320 *) (&data[size]))->MovingSpeed = velo;
              size += sizeof (TBulkXL320);
              break;
            case devtX:
              ((TBulkX *) (&data[size]))->id = id;
              ((TBulkX *) (&data[size]))->addr = ADDRESS_X_GOAL_VELOCITY;
              ((TBulkX *) (&data[size]))->size = 4;
              ((TBulkX *) (&data[size]))->GoalVelocity = velo;
              size += sizeof (TBulkX);
              break;
            case devtPRO:
              ((TBulkPRO *) (&data[size]))->id = id;
              ((TBulkPRO *) (&data[size]))->addr = ADDRESS_PRO_GOAL_VELOCITY;
              ((TBulkPRO *) (&data[size]))->size = 4;
              ((TBulkPRO *) (&data[size]))->GoalVelocity = velo;
              size += sizeof (TBulkPRO);
              break;
            case devtPROP:
              ((TBulkPRO *) (&data[size]))->id = id;
              ((TBulkPRO *) (&data[size]))->addr = ADDRESS_PROP_GOAL_VELOCITY;
              ((TBulkPRO *) (&data[size]))->size = 4;
              ((TBulkPRO *) (&data[size]))->GoalVelocity = velo;
              size += sizeof (TBulkPRO);
              break;
            default:
              break;
          }
        }
      }
    }
    if (size > 0) result = DX2_WriteBulkData (dvid, data, size, NULL);
  }
  free (data);
  return result;
}

//-------------------------------------------------
// 角速度取得
//-------------------------------------------------
DXAPIDLL bool DXL_GetPresentVelocity (TDeviceID dvid, uint8_t id, double *velocity) {
  initarray();
  if (velocity != NULL) {
    if (id <= 252) {
      int idx = Devices.id2model[id];
      if (idx >= 2) {
        switch (ModelInfoList[idx].devtype) {
          case devtXL320: {
              int16_t velo;
              if (DX2_ReadWordData (dvid, id, 39, (uint16_t *)&velo, &Devices.err[id])) {
                *velocity = (double)velo * ModelInfoList[idx].velocityratio;
                return true;
              }
            }
            break;
          case devtX: {
              int32_t velo;
              if (DX2_ReadLongData (dvid, id, ADDRESS_X_PRESENT_VELOCITY, (uint32_t *)&velo, &Devices.err[id])) {
                *velocity = (double)velo * ModelInfoList[idx].velocityratio;
                return true;
              }
            }
            break;
          case devtPRO: {
              int32_t velo;
              if (DX2_ReadLongData (dvid, id, ADDRESS_PRO_PRESENT_VELOCITY, (uint32_t *)&velo, &Devices.err[id])) {
                *velocity = (double)velo * ModelInfoList[idx].velocityratio;
                return true;
              }
            }
            break;
          case devtPROP: {
              int32_t velo;
              if (DX2_ReadLongData (dvid, id, ADDRESS_PROP_PRESENT_VELOCITY, (uint32_t *)&velo, &Devices.err[id])) {
                *velocity = (double)velo * ModelInfoList[idx].velocityratio;
                return true;
              }
            }
            break;
          default:
            break;
        }
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_GetPresentVelocities (TDeviceID dvid, const uint8_t *ids, double *velocities, int num) {
  bool result = true;
  initarray();
  if (ids != NULL && velocities != NULL) {
    for (int i = 0; i < num; i++) {
      int idx = Devices.id2model[ids[i]];
      if (idx >= 2) {
        if (!DXL_GetPresentVelocity (dvid, ids[i], &velocities[i])) result = false;
      }
    }
  }
  return result;
}

//-------------------------------------------------
// 角度・角速度指令
//-------------------------------------------------
DXAPIDLL bool DXL_SetGoalAngleAndVelocity (TDeviceID dvid, uint8_t id, double angle, double velocity) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      int32_t pmax, pmin;
      if (Devices.opmode[id] == 3) {
        pmax = ModelInfoList[idx].positionlimit.max;
        pmin = ModelInfoList[idx].positionlimit.min;
      } else {
        pmax = INT32_MAX;
        pmin = INT32_MIN;
      }
      int32_t pos = max (min (ang2pos (angle, &ModelInfoList[idx]), pmax), pmin);
      int32_t velo = abs (max (min (velocity / ModelInfoList[idx].velocityratio, ModelInfoList[idx].velocitylimit.max), ModelInfoList[idx].velocitylimit.min));
      switch (ModelInfoList[idx].devtype) {
        case devtXL320: {
#ifdef _MSC_VER
            __pragma (pack (push, 1))
            struct {
              int16_t GoalPosition;
              int16_t MovingSpeed;
            } block;
            __pragma (pack (pop))
#elif defined(__GNUC__)
            struct {
              int16_t GoalPosition;
              int16_t MovingSpeed;
            } _PACKED_ block;
#endif
            block.GoalPosition = pos;
            if (velo < 0) velo = velo | 0x400;
            block.MovingSpeed = velo;
            return DX2_WriteBlockData (dvid, id, 30, (uint8_t *)&block, 4, &Devices.err[id]);
          }
          break;
        case devtX: {
#ifdef _MSC_VER
            __pragma (pack (push, 1))
            struct {
              int32_t ProfileVelocity;
              int32_t GoalPosition;
            } block;
            __pragma (pack (pop))
#elif defined(__GNUC__)
            struct {
              int32_t ProfileVelocity;
              int32_t GoalPosition;
            } _PACKED_ block;
#endif
            block.ProfileVelocity = velo;
            block.GoalPosition = pos;
            return DX2_WriteBlockData (dvid, id, ADDRESS_X_PROF_VELOCITY, (uint8_t *)&block, 8, &Devices.err[id]);
          }
          break;
        case devtPRO: {
#ifdef _MSC_VER
            __pragma (pack (push, 1))
            struct {
              int32_t GoalPosition;
              int32_t ProfileVelocity;
            } block;
            __pragma (pack (pop))
#elif defined(__GNUC__)
            struct {
              int32_t GoalPosition;
              int32_t ProfileVelocity;
            } _PACKED_ block;
#endif
            block.ProfileVelocity = velo;
            block.GoalPosition = pos;
            return DX2_WriteBlockData (dvid, id, ADDRESS_PRO_GOAL_POSITION, (uint8_t *)&block, 8, &Devices.err[id]);
          }
          break;
        case devtPROP: {
#ifdef _MSC_VER
            __pragma (pack (push, 1))
            struct {
              int32_t ProfileVelocity;
              int32_t GoalPosition;
            } block;
            __pragma (pack (pop))
#elif defined(__GNUC__)
            struct {
              int32_t ProfileVelocity;
              int32_t GoalPosition;
            } _PACKED_ block;
#endif
            block.ProfileVelocity = velo;
            block.GoalPosition = pos;
            return DX2_WriteBlockData (dvid, id, ADDRESS_PROP_PROF_VELOCITY, (uint8_t *)&block, 8, &Devices.err[id]);
          }
          break;
        default:
          break;
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetGoalAnglesAndVelocities (TDeviceID dvid, const uint8_t *ids, PAngleVelocity anglevelocity, int num) {
#ifdef _MSC_VER
  __pragma (pack (push, 1))
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint16_t  GoalPosition;
    uint16_t  MovingSpeed;
  } TBulkXL320;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
    uint32_t  GoalVelocity;
  } TBulkPRO;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } TBulkPROP;
  __pragma (pack (pop))
#elif defined(__GNUC__)
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint16_t  GoalPosition;
    uint16_t  MovingSpeed;
  } _PACKED_ TBulkXL320;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } _PACKED_ TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
    uint32_t  GoalVelocity;
  } _PACKED_ TBulkPRO;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } _PACKED_ TBulkPROP;
#endif
  bool result = false;
  uint8_t *data = (uint8_t *)malloc (sizeof (TBulkPROP) * num);
  int bytes = 0;
  initarray();
  if (ids != NULL && anglevelocity != NULL) {
    for (int i = 0; i < num; i++) {
      uint8_t id = ids[i];
      if (id <= 252) {
        int idx = Devices.id2model[id];
        if (idx >= 2) {
          int32_t pmax, pmin;
          if (Devices.opmode[id] == 3) {
            pmax = ModelInfoList[idx].positionlimit.max;
            pmin = ModelInfoList[idx].positionlimit.min;
          } else {
            pmax = INT32_MAX;
            pmin = INT32_MIN;
          }
          int32_t pos = max (min (ang2pos (anglevelocity[i].angle, &ModelInfoList[idx]), pmax), pmin);
          int32_t velo = abs (max (min (anglevelocity[i].velocity / ModelInfoList[idx].velocityratio, ModelInfoList[idx].velocitylimit.max), ModelInfoList[idx].velocitylimit.min));
          switch (ModelInfoList[idx].devtype) {
            case devtXL320:
              ((TBulkXL320 *) (&data[bytes]))->id = id;
              ((TBulkXL320 *) (&data[bytes]))->addr = 30;
              ((TBulkXL320 *) (&data[bytes]))->size = 4;
              ((TBulkXL320 *) (&data[bytes]))->GoalPosition = pos;
              if (velo < 0) velo = velo | 0x400;
              ((TBulkXL320 *) (&data[bytes]))->MovingSpeed = velo;
              bytes += sizeof (TBulkXL320);
              break;
            case devtX:
              ((TBulkX *) (&data[bytes]))->id = id;
              ((TBulkX *) (&data[bytes]))->addr = ADDRESS_X_PROF_VELOCITY;
              ((TBulkX *) (&data[bytes]))->size = 8;
              ((TBulkX *) (&data[bytes]))->ProfileVelocity = velo;
              ((TBulkX *) (&data[bytes]))->GoalPosition = pos;
              bytes += sizeof (TBulkX);
              break;
            case devtPRO:
              ((TBulkPRO *) (&data[bytes]))->id = id;
              ((TBulkPRO *) (&data[bytes]))->addr = ADDRESS_PRO_GOAL_POSITION;
              ((TBulkPRO *) (&data[bytes]))->size = 8;
              ((TBulkPRO *) (&data[bytes]))->GoalPosition = pos;
              ((TBulkPRO *) (&data[bytes]))->GoalVelocity = velo;
              bytes += sizeof (TBulkPRO);
              break;
            case devtPROP:
              ((TBulkPROP *) (&data[bytes]))->id = id;
              ((TBulkPROP *) (&data[bytes]))->addr = ADDRESS_PROP_PROF_VELOCITY;
              ((TBulkPROP *) (&data[bytes]))->size = 8;
              ((TBulkPROP *) (&data[bytes]))->ProfileVelocity = velo;
              ((TBulkPROP *) (&data[bytes]))->GoalPosition = pos;
              bytes += sizeof (TBulkPROP);
              break;
            default:
              break;
          }
        }
      }
    }
    if (bytes > 0) result = DX2_WriteBulkData (dvid, data, bytes, NULL);
  }
  free (data);
  return result;
}

//-------------------------------------------------
// 角度・時間指令(実際には速度指令)
//-------------------------------------------------
DXAPIDLL bool DXL_SetGoalAngleAndTime (TDeviceID dvid, uint8_t id, double angle, double sec) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      int32_t pmax, pmin;
      if (Devices.opmode[id] == 3) {
        pmax = ModelInfoList[idx].positionlimit.max;
        pmin = ModelInfoList[idx].positionlimit.min;
      } else {
        pmax = INT32_MAX;
        pmin = INT32_MIN;
      }
      double pangle;
      if (DXL_GetPresentAngle (dvid, id, &pangle) && ((Devices.drivemode[id] & 0x4) == 0)) {
        double velocity = fabs ((pangle - angle) / sec);
        int32_t pos = max (min (ang2pos (angle, &ModelInfoList[idx]), pmax), pmin);
        int32_t velo = abs (max (min (velocity / ModelInfoList[idx].velocityratio, ModelInfoList[idx].velocitylimit.max), ModelInfoList[idx].velocitylimit.min));
        switch (ModelInfoList[idx].devtype) {
          case devtXL320: {
#ifdef _MSC_VER
              __pragma (pack (push, 1))
              struct {
                int16_t GoalPosition;
                int16_t MovingSpeed;
              } block;
              __pragma (pack (pop))
#elif defined(__GNUC__)
              struct {
                int16_t GoalPosition;
                int16_t MovingSpeed;
              } _PACKED_ block;
#endif
              block.GoalPosition = pos;
              if (velo < 0) velo = velo | 0x400;
              block.MovingSpeed = velo;
              return DX2_WriteBlockData (dvid, id, 30, (uint8_t *)&block, 4, &Devices.err[id]);
            }
            break;
          case devtX: {
#ifdef _MSC_VER
              __pragma (pack (push, 1))
              struct {
                int32_t ProfileVelocity;
                int32_t GoalPosition;
              } block;
              __pragma (pack (pop))
#elif defined(__GNUC__)
              struct {
                int32_t ProfileVelocity;
                int32_t GoalPosition;
              } _PACKED_ block;
#endif
              block.ProfileVelocity = velo;
              block.GoalPosition = pos;
              return DX2_WriteBlockData (dvid, id, ADDRESS_X_PROF_VELOCITY, (uint8_t *)&block, 8, &Devices.err[id]);
            }
            break;
          case devtPRO: {
#ifdef _MSC_VER
              __pragma (pack (push, 1))
              struct {
                int32_t GoalPosition;
                int32_t GoalVelocity;
              } block;
              __pragma (pack (pop))
#elif defined(__GNUC__)
              struct {
                int32_t GoalPosition;
                int32_t GoalVelocity;
              } _PACKED_ block;
#endif
              block.GoalVelocity = abs (velo);
              block.GoalPosition = pos;
              return DX2_WriteBlockData (dvid, id, ADDRESS_PRO_GOAL_POSITION, (uint8_t *)&block, 8, &Devices.err[id]);
            }
            break;
          case devtPROP: {
#ifdef _MSC_VER
              __pragma (pack (push, 1))
              struct {
                int32_t ProfileVelocity;
                int32_t GoalPosition;
              } block;
              __pragma (pack (pop))
#elif defined(__GNUC__)
              struct {
                int32_t ProfileVelocity;
                int32_t GoalPosition;
              } _PACKED_ block;
#endif
              block.ProfileVelocity = abs (velo);
              block.GoalPosition = pos;
              return DX2_WriteBlockData (dvid, id, ADDRESS_PROP_PROF_VELOCITY, (uint8_t *)&block, 8, &Devices.err[id]);
            }
            break;
          default:
            break;
        }
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetGoalAnglesAndTime (TDeviceID dvid, const uint8_t *ids, const double *angles, int num, double sec) {
#ifdef _MSC_VER
  __pragma (pack (push, 1))
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint16_t  GoalPosition;
    uint16_t  MovingSpeed;
  } TBulkXL320;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
    uint32_t  GoalVelocity;
  } TBulkPRO;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } TBulkPROP;
  __pragma (pack (pop))
#elif defined(__GNUC__)
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint16_t  GoalPosition;
    uint16_t  MovingSpeed;
  } _PACKED_ TBulkXL320;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } _PACKED_ TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
    uint32_t  GoalVelocity;
  } _PACKED_ TBulkPRO;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } _PACKED_ TBulkPROP;
#endif
  bool result = false;
  uint8_t *data = (uint8_t *)malloc (sizeof (TBulkPRO) * num);
  int bytes = 0;
  initarray();
  if (ids != NULL && angles != NULL) {
    for (int i = 0; i < num; i++) {
      uint8_t id = ids[i];
      if (id <= 252) {
        int idx = Devices.id2model[id];
        if (idx >= 2) {
          int32_t pmax, pmin;
          if (Devices.opmode[id] == 3) {
            pmax = ModelInfoList[idx].positionlimit.max;
            pmin = ModelInfoList[idx].positionlimit.min;
          } else {
            pmax = INT32_MAX;
            pmin = INT32_MIN;
          }
          double pangle;
          if (DXL_GetPresentAngle (dvid, id, &pangle) && ((Devices.drivemode[id] & 0x4) == 0)) {
            double velocity = fabs ((pangle - angles[i]) / sec);
            int32_t pos = max (min (ang2pos (angles[i], &ModelInfoList[idx]), pmax), pmin);
            int32_t velo = abs (max (min (velocity / ModelInfoList[idx].velocityratio, ModelInfoList[idx].velocitylimit.max), ModelInfoList[idx].velocitylimit.min));
            switch (ModelInfoList[idx].devtype) {
              case devtXL320:
                ((TBulkXL320 *) (&data[bytes]))->id = id;
                ((TBulkXL320 *) (&data[bytes]))->addr = 30;
                ((TBulkXL320 *) (&data[bytes]))->size = 4;
                ((TBulkXL320 *) (&data[bytes]))->GoalPosition = pos;
                if (velo < 0) velo = velo | 0x400;
                ((TBulkXL320 *) (&data[bytes]))->MovingSpeed = velo;
                bytes += sizeof (TBulkXL320);
                break;
              case devtX:
                ((TBulkX *) (&data[bytes]))->id = id;
                ((TBulkX *) (&data[bytes]))->addr = ADDRESS_X_PROF_VELOCITY;
                ((TBulkX *) (&data[bytes]))->size = 8;
                ((TBulkX *) (&data[bytes]))->ProfileVelocity = velo;
                ((TBulkX *) (&data[bytes]))->GoalPosition = pos;
                bytes += sizeof (TBulkX);
                break;
              case devtPRO:
                ((TBulkPRO *) (&data[bytes]))->id = id;
                ((TBulkPRO *) (&data[bytes]))->addr = ADDRESS_PRO_GOAL_POSITION;
                ((TBulkPRO *) (&data[bytes]))->size = 8;
                ((TBulkPRO *) (&data[bytes]))->GoalPosition = pos;
                ((TBulkPRO *) (&data[bytes]))->GoalVelocity = velo;
                bytes += sizeof (TBulkPRO);
                break;
              case devtPROP:
                ((TBulkPROP *) (&data[bytes]))->id = id;
                ((TBulkPROP *) (&data[bytes]))->addr = ADDRESS_PROP_PROF_VELOCITY;
                ((TBulkPROP *) (&data[bytes]))->size = 8;
                ((TBulkPROP *) (&data[bytes]))->ProfileVelocity = velo;
                ((TBulkPROP *) (&data[bytes]))->GoalPosition = pos;
                bytes += sizeof (TBulkPROP);
                break;
              default:
                break;
            }
          }
          // OSによっては遅延が無いと正常動作しないので暫定挿入
          WaitForLittleBit;
        }
      }
    }
    if (bytes > 0) result = DX2_WriteBulkData (dvid, data, bytes, NULL);
  }
  free (data);
  return result;
}

//-------------------------------------------------
// 角度・時間指令2(時間指令)
//-------------------------------------------------
DXAPIDLL bool DXL_SetGoalAngleAndTime2 (TDeviceID dvid, uint8_t id, double angle, double sec) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      int32_t pmax, pmin;
      if (Devices.opmode[id] == 3) {
        pmax = ModelInfoList[idx].positionlimit.max;
        pmin = ModelInfoList[idx].positionlimit.min;
      } else {
        pmax = INT32_MAX;
        pmin = INT32_MIN;
      }
      if ((Devices.drivemode[id] & 0x4) != 0) {
        int32_t pos = max (min (ang2pos (angle, &ModelInfoList[idx]), pmax), pmin);
        int32_t profileacc = abs (max (min (sec * 1000.0, 32767), 0));
        switch (ModelInfoList[idx].devtype) {
          case devtX: {
#ifdef _MSC_VER
              __pragma (pack (push, 1))
              struct {
                int32_t ProfileVelocity;
                int32_t GoalPosition;
              } block;
              __pragma (pack (pop))
#elif defined(__GNUC__)
              struct {
                int32_t ProfileVelocity;
                int32_t GoalPosition;
              } _PACKED_ block;
#endif
              block.ProfileVelocity = profileacc;
              block.GoalPosition = pos;
              return DX2_WriteBlockData (dvid, id, ADDRESS_X_PROF_VELOCITY, (uint8_t *)&block, 8, &Devices.err[id]);
            }
            break;
          case devtPROP: {
#ifdef _MSC_VER
              __pragma (pack (push, 1))
              struct {
                int32_t ProfileVelocity;
                int32_t GoalPosition;
              } block;
              __pragma (pack (pop))
#elif defined(__GNUC__)
              struct {
                int32_t ProfileVelocity;
                int32_t GoalPosition;
              } _PACKED_ block;
#endif
              block.ProfileVelocity = profileacc;
              block.GoalPosition = pos;
              return DX2_WriteBlockData (dvid, id, ADDRESS_PROP_PROF_VELOCITY, (uint8_t *)&block, 8, &Devices.err[id]);
            }
            break;
          default:
            break;
        }
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetGoalAnglesAndTime2 (TDeviceID dvid, const uint8_t *ids, const double *angles, int num, double sec) {
#ifdef _MSC_VER
  __pragma (pack (push, 1))
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
    uint32_t  GoalVelocity;
  } TBulkPRO;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } TBulkPROP;
  __pragma (pack (pop))
#elif defined(__GNUC__)
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } _PACKED_ TBulkX;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  GoalPosition;
    uint32_t  GoalVelocity;
  } _PACKED_ TBulkPRO;
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    uint32_t  ProfileVelocity;
    uint32_t  GoalPosition;
  } _PACKED_ TBulkPROP;
#endif
  bool result = false;
  uint8_t *data = (uint8_t *)malloc (sizeof (TBulkPRO) * num);
  int bytes = 0;
  initarray();
  if (ids != NULL && angles != NULL) {
    for (int i = 0; i < num; i++) {
      uint8_t id = ids[i];
      if (id <= 252) {
        int idx = Devices.id2model[id];
        if (idx >= 2) {
          int32_t pmax, pmin;
          if (Devices.opmode[id] == 3) {
            pmax = ModelInfoList[idx].positionlimit.max;
            pmin = ModelInfoList[idx].positionlimit.min;
          } else {
            pmax = INT32_MAX;
            pmin = INT32_MIN;
          }
          if ((Devices.drivemode[id] & 0x4) != 0) {
            int32_t pos = max (min (ang2pos (angles[i], &ModelInfoList[idx]), pmax), pmin);
            int32_t profileacc = abs (max (min (sec * 1000.0, 32767), 0));
            switch (ModelInfoList[idx].devtype) {
              case devtX:
                ((TBulkX *) (&data[bytes]))->id = id;
                ((TBulkX *) (&data[bytes]))->addr = ADDRESS_X_PROF_VELOCITY;
                ((TBulkX *) (&data[bytes]))->size = 8;
                ((TBulkX *) (&data[bytes]))->ProfileVelocity = profileacc;
                ((TBulkX *) (&data[bytes]))->GoalPosition = pos;
                bytes += sizeof (TBulkX);
                break;
              case devtPROP:
                ((TBulkPROP *) (&data[bytes]))->id = id;
                ((TBulkPROP *) (&data[bytes]))->addr = ADDRESS_PROP_PROF_VELOCITY;
                ((TBulkPROP *) (&data[bytes]))->size = 8;
                ((TBulkPROP *) (&data[bytes]))->ProfileVelocity = profileacc;
                ((TBulkPROP *) (&data[bytes]))->GoalPosition = pos;
                bytes += sizeof (TBulkPROP);
                break;
              default:
                break;
            }
          } else break;
        }
      }
    }
    if (bytes > 0) result = DX2_WriteBulkData (dvid, data, bytes, NULL);
  }
  free (data);
  return result;
}

//-------------------------------------------------
// 電流指令
//-------------------------------------------------
DXAPIDLL bool DXL_SetGoalCurrent (TDeviceID dvid, uint8_t id, double current) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      switch (ModelInfoList[idx].devtype) {
        case devtX: {
            int16_t cur = current / ModelInfoList[idx].currentratio;
            return DX2_WriteWordData (dvid, id, ADDRESS_X_GOAL_CURRENT, cur, &Devices.err[id]);
          }
          break;
        case devtPRO: {
            int16_t cur = current / ModelInfoList[idx].currentratio;
            return DX2_WriteLongData (dvid, id, ADDRESS_PRO_GOAL_TORQUE, cur, &Devices.err[id]);
          }
          break;
        case devtPROP: {
            int16_t cur = current / ModelInfoList[idx].currentratio;
            return DX2_WriteWordData (dvid, id, ADDRESS_PROP_GOAL_CURRENT, cur, &Devices.err[id]);
          }
          break;
        default:
          break;
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetGoalCurrents (TDeviceID dvid, const uint8_t *ids, const double *currents, int num) {
#ifdef _MSC_VER
  __pragma (pack (push, 1))
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    int16_t   GoalCurrent;
  } TBulk;
  __pragma (pack (pop))
#elif defined(__GNUC__)
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    int16_t   GoalCurrent;
  } _PACKED_ TBulk;
#endif
  bool result = false;
  uint8_t *data = (uint8_t *)malloc (sizeof (TBulk) * num);
  int size = 0;
  initarray();
  if (ids != NULL && currents != NULL) {
    for (int i = 0; i < num; i++) {
      uint8_t id = ids[i];
      if (id <= 252) {
        int idx = Devices.id2model[id];
        if (idx >= 2) {
          int16_t cur = currents[i] / ModelInfoList[idx].currentratio;
          switch (ModelInfoList[idx].devtype) {
            case devtX:
              ((TBulk *) (&data[size]))->id = id;
              ((TBulk *) (&data[size]))->addr = ADDRESS_X_GOAL_CURRENT;
              ((TBulk *) (&data[size]))->size = 2;
              ((TBulk *) (&data[size]))->GoalCurrent = cur;
              size += sizeof (TBulk);
              break;
            case devtPRO:
              ((TBulk *) (&data[size]))->id = id;
              ((TBulk *) (&data[size]))->addr = ADDRESS_PRO_GOAL_TORQUE;
              ((TBulk *) (&data[size]))->size = 2;
              ((TBulk *) (&data[size]))->GoalCurrent = cur;
              size += sizeof (TBulk);
              break;
            case devtPROP:
              ((TBulk *) (&data[size]))->id = id;
              ((TBulk *) (&data[size]))->addr = ADDRESS_PROP_GOAL_CURRENT;
              ((TBulk *) (&data[size]))->size = 2;
              ((TBulk *) (&data[size]))->GoalCurrent = cur;
              size += sizeof (TBulk);
              break;
            default:
              break;
          }
        }
      }
    }
    if (size > 0) result = DX2_WriteBulkData (dvid, data, size, NULL);
  }
  free (data);
  return result;
}

//-------------------------------------------------
// 電流取得
//-------------------------------------------------
DXAPIDLL bool DXL_GetPresentCurrent (TDeviceID dvid, uint8_t id, double *current) {
  initarray();
  if (current != NULL) {
    if (id <= 252) {
      int idx = Devices.id2model[id];
      if (idx >= 2) {
        int16_t cur;
        switch (ModelInfoList[idx].devtype) {
          case devtXL320:
            if (DX2_ReadWordData (dvid, id, 41, (uint16_t *)&cur, &Devices.err[id])) {
              if (cur & 0x400) cur = - (cur & 0x3ff);
              *current = (double)cur * ModelInfoList[idx].currentratio;
              return true;
            }
            break;
          case devtX:
            if (DX2_ReadWordData (dvid, id, ADDRESS_X_PRESENT_CURRENT, (uint16_t *)&cur, &Devices.err[id])) {
              *current = (double)cur * ModelInfoList[idx].currentratio;
              return true;
            }
            break;
          case devtPRO:
            if (DX2_ReadWordData (dvid, id, ADDRESS_PRO_PRESENT_CURRENT, (uint16_t *)&cur, &Devices.err[id])) {
              *current = (double)cur * ModelInfoList[idx].currentratio;
              return true;
            }
            break;
          case devtPROP:
            if (DX2_ReadWordData (dvid, id, ADDRESS_PROP_PRESENT_CURRENT, (uint16_t *)&cur, &Devices.err[id])) {
              *current = (double)cur * ModelInfoList[idx].currentratio;
              return true;
            }
            break;
          default:
            break;
        }
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_GetPresentCurrents (TDeviceID dvid, const uint8_t *ids, double *currents, int num) {
  bool result = true;
  initarray();
  if (ids != NULL && currents != NULL) {
    for (int i = 0; i < num; i++) {
      int idx = Devices.id2model[ids[i]];
      if (idx >= 2) {
        if (!DXL_GetPresentCurrent (dvid, ids[i], &currents[i])) result = false;
      }
    }
  }
  return result;
}

//-------------------------------------------------
// PWM指令
//-------------------------------------------------
DXAPIDLL bool DXL_SetGoalPWM (TDeviceID dvid, uint8_t id, double pwm) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      switch (ModelInfoList[idx].devtype) {
        case devtX: {
            int32_t p = max (min (pwm / ModelInfoList[idx].pwmratio, ModelInfoList[idx].pwmlimit.max), ModelInfoList[idx].pwmlimit.min);
            return DX2_WriteWordData (dvid, id, ADDRESS_X_GOAL_PWM, p, &Devices.err[id]);
          }
          break;
        case devtPROP: {
            int32_t p = max (min (pwm / ModelInfoList[idx].pwmratio, ModelInfoList[idx].pwmlimit.max), ModelInfoList[idx].pwmlimit.min);
            return DX2_WriteWordData (dvid, id, ADDRESS_PROP_GOAL_PWM, p, &Devices.err[id]);
          }
          break;
        default:
          break;
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetGoalPWMs (TDeviceID dvid, const uint8_t *ids, const double *pwms, int num) {
#ifdef _MSC_VER
  __pragma (pack (push, 1))
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    int16_t   GoalPWM;
  } TBulk;
  __pragma (pack (pop))
#elif defined(__GNUC__)
  typedef struct {
    uint8_t   id;
    uint16_t  addr;
    uint16_t  size;
    int16_t   GoalPWM;
  } _PACKED_ TBulk;
#endif
  bool result = false;
  uint8_t *data = (uint8_t *)malloc (sizeof (TBulk) * num);
  int size = 0;
  initarray();
  if (ids != NULL && pwms != NULL) {
    for (int i = 0; i < num; i++) {
      uint8_t id = ids[i];
      if (id <= 252) {
        int idx = Devices.id2model[id];
        if (idx >= 2) {
          int32_t pwm = max (min (pwms[i] / ModelInfoList[idx].pwmratio, ModelInfoList[idx].pwmlimit.max), ModelInfoList[idx].pwmlimit.min);
          switch (ModelInfoList[idx].devtype) {
            case devtX:
              ((TBulk *) (&data[size]))->id = id;
              ((TBulk *) (&data[size]))->addr = ADDRESS_X_GOAL_PWM;
              ((TBulk *) (&data[size]))->size = 2;
              ((TBulk *) (&data[size]))->GoalPWM = pwm;
              size += sizeof (TBulk);
              break;
            case devtPROP:
              ((TBulk *) (&data[size]))->id = id;
              ((TBulk *) (&data[size]))->addr = ADDRESS_PROP_GOAL_PWM;
              ((TBulk *) (&data[size]))->size = 2;
              ((TBulk *) (&data[size]))->GoalPWM = pwm;
              size += sizeof (TBulk);
              break;
            default:
              break;
          }
        }
      }
    }
    if (size > 0) result = DX2_WriteBulkData (dvid, data, size, NULL);
  }
  free (data);
  return result;
}

//-------------------------------------------------
// PWM取得
//-------------------------------------------------
DXAPIDLL bool DXL_GetPresentPWM (TDeviceID dvid, uint8_t id, double *pwm) {
  initarray();
  if (pwm != NULL) {
    if (id <= 252) {
      int idx = Devices.id2model[id];
      if (idx >= 2) {
        int16_t p;
        switch (ModelInfoList[idx].devtype) {
          case devtX:
            if (DX2_ReadWordData (dvid, id, ADDRESS_X_PRESENT_PWM, (uint16_t *)&p, &Devices.err[id])) {
              *pwm = (double)p * ModelInfoList[idx].pwmratio;
              return true;
            }
            break;
          case devtPROP:
            if (DX2_ReadWordData (dvid, id, ADDRESS_PROP_PRESENT_PWM, (uint16_t *)&p, &Devices.err[id])) {
              *pwm = (double)p * ModelInfoList[idx].pwmratio;
              return true;
            }
            break;
          default:
            break;
        }
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_GetPresentPWMs (TDeviceID dvid, const uint8_t *ids, double *pwms, int num) {
  bool result = true;
  initarray();
  if (ids != NULL && pwms != NULL) {
    for (int i = 0; i < num; i++) {
      int idx = Devices.id2model[ids[i]];
      if (idx >= 2) {
        if (!DXL_GetPresentPWM (dvid, ids[i], &pwms[i])) result = false;
      }
    }
  }
  return result;
}

//-------------------------------------------------
// ドライブモード変更
//-------------------------------------------------
/*
   modeの値はモデル依存(以下はXシリーズの場合)
    bit3: Torque On by Goal Update
    bit2: Profile Config (0:Velocity-based, 1:Time-based)
    bit1: Master/Slave Mode (0:Master, 1:Slave)
    bit0: Normal/Reverse Mode (0:CCW Positive, 1:CW Positive)
 */
DXAPIDLL bool DXL_SetDriveMode (TDeviceID dvid, uint8_t id, uint8_t mode) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      uint8_t rmode;
      if (DXL_GetOperatingMode (dvid, id, &rmode)) {
        if (mode == Devices.drivemode[id]) {
          return true;
        } else {
          if (DXL_SetTorqueEnable (dvid, id, false)) {
            switch (ModelInfoList[idx].devtype) {
              case devtX:
              case devtPRO:
                if (DX2_WriteByteData (dvid, id, ADDRESS_X_DRIVE_MODE, mode, &Devices.err[id])) {
                  if (DXL_GetOperatingMode (dvid, id, &rmode)) {
                    return (mode == Devices.drivemode[id]);
                  }
                }
                break;
              case devtPROP:
                if (DX2_WriteByteData (dvid, id, ADDRESS_PROP_DRIVE_MODE, mode, &Devices.err[id])) {
                  if (DXL_GetOperatingMode (dvid, id, &rmode)) {
                    return (mode == Devices.drivemode[id]);
                  }
                }
                break;
              default:
                break;
            }
          }
        }
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetDriveModesEquival (TDeviceID dvid, const uint8_t *ids, int num, uint8_t mode) {
  bool result = true;
  initarray();
  if (ids != NULL) {
    for (int i = 0; i < num; i++) {
      int idx = Devices.id2model[ids[i]];
      if (idx >= 2) {
        if (!DXL_SetDriveMode (dvid, ids[i], mode)) result = false;
      }
    }
  }
  return result;
}

//-------------------------------------------------
// 動作モード変更
//-------------------------------------------------
/*
   modeの値はXシリーズ準拠
    0: cur
    1: velo
    3: pos
    4: expos(pos+multiturn)
    5: expos(pos+multiturn+cur)
    16:PWM
 */
DXAPIDLL bool DXL_SetOperatingMode (TDeviceID dvid, uint8_t id, uint8_t mode) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) {
      uint8_t rmode = 255;
      if (DXL_GetOperatingMode (dvid, id, &rmode)) {
        if (mode == rmode) {
          return true;
        } else {
          if (DXL_SetTorqueEnable (dvid, id, false)) {
            switch (ModelInfoList[idx].devtype) {
              case devtXL320:
                switch (mode) {
                  case 1: // velo
                    if (DX2_WriteByteData (dvid, id, ADDRESS_X_OPERATING_MODE, 1, &Devices.err[id])) {
                      if (DXL_GetOperatingMode (dvid, id, &rmode)) {
                        return (mode == rmode);
                      }
                    }
                    break;
                  case 3: // pos
                    if (DX2_WriteByteData (dvid, id, ADDRESS_X_OPERATING_MODE, 2, &Devices.err[id])) {
                      if (DXL_GetOperatingMode (dvid, id, &rmode)) {
                        return (mode == rmode);
                      }
                    }
                    break;
                }
                break;
              case devtX:
              case devtPRO:
                if (DX2_WriteByteData (dvid, id, ADDRESS_X_OPERATING_MODE, mode, &Devices.err[id])) {
                  if (DXL_GetOperatingMode (dvid, id, &rmode)) {
                    return (mode == rmode);
                  }
                }
                break;
              case devtPROP:
                if (DX2_WriteByteData (dvid, id, ADDRESS_PROP_OPERATING_MODE, mode, &Devices.err[id])) {
                  if (DXL_GetOperatingMode (dvid, id, &rmode)) {
                    return (mode == rmode);
                  }
                }
                break;
              default:
                break;
            }
          }
        }
      }
    }
  }
  return false;
}

DXAPIDLL bool DXL_SetOperatingModesEquival (TDeviceID dvid, const uint8_t *ids, int num, uint8_t mode) {
  bool result = true;
  initarray();
  if (ids != NULL) {
    for (int i = 0; i < num; i++) {
      int idx = Devices.id2model[ids[i]];
      if (idx >= 2) {
        if (!DXL_SetOperatingMode (dvid, ids[i], mode)) result = false;
      }
    }
  }
  return result;
}

//-------------------------------------------------
// 動作モード取得
//-------------------------------------------------
/*
   modeの値はXシリーズ準拠
    0: cur
    1: velo
    3: pos
    4: expos(pos+multiturn)
    5: expos(pos+multiturn+cur)
    16:PWM
 */
DXAPIDLL bool DXL_GetOperatingMode (TDeviceID dvid, uint8_t id, uint8_t *mode) {
  initarray();
  if (mode != NULL) {
    if (id <= 252) {
      int idx = Devices.id2model[id];
      uint8_t rmode = 255, rdmode = 0;
      if (idx >= 2) {
        switch (ModelInfoList[idx].devtype) {
          case devtXL320:
            if (DX2_ReadByteData (dvid, id, 11, &rmode, &Devices.err[id])) {
              switch (rmode) {
                case 1:
                  Devices.opmode[id] = (*mode = 1);
                  return true;
                  break;
                case 2:
                  Devices.opmode[id] = (*mode = 3);
                  return true;
                  break;
              }
              Devices.drivemode[id] = 0;
            }
            break;
          case devtX:
            if (DX2_ReadByteData (dvid, id, ADDRESS_X_OPERATING_MODE, &rmode, &Devices.err[id]) && DX2_ReadByteData (dvid, id, ADDRESS_X_DRIVE_MODE, &rdmode, &Devices.err[id])) {
              Devices.opmode[id] = (*mode = rmode);
              Devices.drivemode[id] = rdmode;
              return true;
            }
            break;
          case devtPRO:
            if (DX2_ReadByteData (dvid, id, ADDRESS_PRO_OPERATING_MODE, &rmode, &Devices.err[id])) {
              Devices.opmode[id] = (*mode = rmode);
              return true;
            }
            break;
          case devtPROP:
            if (DX2_ReadByteData (dvid, id, ADDRESS_PROP_OPERATING_MODE, &rmode, &Devices.err[id]) && DX2_ReadByteData (dvid, id, ADDRESS_PROP_DRIVE_MODE, &rdmode, &Devices.err[id])) {
              Devices.opmode[id] = (*mode = rmode);
              Devices.drivemode[id] = rdmode;
              return true;
            }
            break;
          default:
            break;
        }
      }
    }
  }
  return false;
}

//-------------------------------------------------
// 指定IDのハードウェアエラーを取得
//-------------------------------------------------
DXAPIDLL bool DXL_GetHWErrorCode (TDeviceID dvid, uint8_t id, uint8_t *hwerr) {
  initarray();
  if (hwerr != NULL) {
    if (id <= 252) {
      int idx = Devices.id2model[id];
      if (idx >= 2) {
        switch (ModelInfoList[idx].devtype) {
          case devtXL320:
            return DX2_ReadByteData (dvid, id, 50, hwerr, &Devices.err[id]);
            break;
          case devtX:
            return DX2_ReadByteData (dvid, id, ADDRESS_X_HARDWARE_ERROR_STATUS, hwerr, &Devices.err[id]);
            break;
          case devtPRO:
            return DX2_ReadByteData (dvid, id, ADDRESS_PRO_HARDWARE_ERROR_STATUS, hwerr, &Devices.err[id]);
            break;
          case devtPROP:
            return DX2_ReadByteData (dvid, id, ADDRESS_PROP_HARDWARE_ERROR_STATUS, hwerr, &Devices.err[id]);
            break;
          default:
            break;
        }
      }
    }
  }
  return false;
}

//-------------------------------------------------
// 指定IDの記録済みエラーコードを取得
//-------------------------------------------------
DXAPIDLL TErrorCode DXL_GetErrorCode (TDeviceID dvid, uint8_t id) {
  initarray();
  if (id <= 252) {
    int idx = Devices.id2model[id];
    if (idx >= 2) return Devices.err[id];
  }
  return 0xffff;
}

//-------------------------------------------------
// 指定IDのデバイズ情報を取得
//-------------------------------------------------
DXAPIDLL PDXL_ModelInfo DXL_GetModelInfo (TDeviceID dvid, uint8_t id) {
  uint16_t    model;
  uint8_t     mode, rdelay;

  initarray();
  if (DX2_ReadWordData (dvid, id, 0, &model, NULL)) {
    for (int i = 2; i < (sizeof (ModelInfoList) / sizeof (ModelInfoList[0])); i++) {
      if (ModelInfoList[i].modelno == model) {
        // デバイスのReturn Statusを2に、Return Delay Timeを0に変更
        if (Devices.id2model[id] == 0) {
          switch (ModelInfoList[i].devtype) {
            case devtXL320:
              DX2_WriteByteData (dvid, id, 17, 2, &Devices.err[id]);
              if (DX2_ReadByteData (dvid, id, 5, &rdelay, &Devices.err[id])) {
                if (rdelay != 0) {
                  DXL_SetTorqueEnable (dvid, id, false);
                  DX2_WriteByteData (dvid, id, 5, 0, &Devices.err[id]);
                }
              }
              break;
            case devtX:
              DX2_WriteByteData (dvid, id, ADDRESS_X_STATUS_RETURN_LEVEL, 2, &Devices.err[id]);
              if (DX2_ReadByteData (dvid, id, ADDRESS_X_RETURN_DELAY_TIME, &rdelay, &Devices.err[id])) {
                if (rdelay != 0) {
                  DXL_SetTorqueEnable (dvid, id, false);
                  DX2_WriteByteData (dvid, id, ADDRESS_X_RETURN_DELAY_TIME, 0, &Devices.err[id]);
                }
              }
              break;
            case devtPRO:
              DX2_WriteByteData (dvid, id, ADDRESS_PRO_STATUS_RETURN_LEVEL, 2, &Devices.err[id]);
              if (DX2_ReadByteData (dvid, id, ADDRESS_X_RETURN_DELAY_TIME, &rdelay, &Devices.err[id])) {
                if (rdelay != 0) {
                  DXL_SetTorqueEnable (dvid, id, false);
                  DX2_WriteByteData (dvid, id, ADDRESS_PRO_RETURN_DELAY_TIME, 0, &Devices.err[id]);
                }
              }
              break;
            case devtPROP:
              DX2_WriteByteData (dvid, id, ADDRESS_PROP_STATUS_RETURN_LEVEL, 2, &Devices.err[id]);
              if (DX2_ReadByteData (dvid, id, ADDRESS_PROP_RETURN_DELAY_TIME, &rdelay, &Devices.err[id])) {
                if (rdelay != 0) {
                  DXL_SetTorqueEnable (dvid, id, false);
                  DX2_WriteByteData (dvid, id, ADDRESS_PROP_RETURN_DELAY_TIME, 0, &Devices.err[id]);
                }
              }
              break;
            default:
              break;
          }
        }
        Devices.id2model[id] = i;
        DXL_GetOperatingMode (dvid, id, &mode);
        return (PDXL_ModelInfo)&ModelInfoList[i];
      }
    }
    Devices.id2model[id] = 1;
    return (PDXL_ModelInfo)&ModelInfoList[1];
  }
  Devices.id2model[id] = 0;
  return (PDXL_ModelInfo)&ModelInfoList[0];
}

//-------------------------------------------------
// 接続されたデバイスのリストを構成
//-------------------------------------------------
DXAPIDLL int DXL_ScanDevices (TDeviceID dvid, uint8_t *ids) {
  Devices.num = 0;
  initarray();
  for (int id = 0; id <= 252; id++) {
    Devices.id2model[id] = 0;
    Devices.err[id] = 0xffff;
    // デバイスのReturn Delay Timeを0に変更
    switch (DXL_GetModelInfo (dvid, id)->devtype) {
      case devtXL320:
      case devtX:
      case devtPRO:
      case devtPROP:
        if (ids != NULL) {
          *ids = id;
          ids++;
        }
        Devices.num++;
        break;
      default:
        break;
    }
  }
  return Devices.num;
}

//-------------------------------------------------
// スキャン済みデバイスリストに含まれるデバイスの一覧を表示
//-------------------------------------------------
DXAPIDLL bool DXL_PrintDevicesList (int (*pf) (const char *, ...)) {
  bool result = false;
  initarray();
  for (int id = 0; id <= 252; id++) {
    if (Devices.id2model[id] != 0) pf ("[%3d] %-15s($%04X)\n", id, ModelInfoList[Devices.id2model[id]].name, ModelInfoList[Devices.id2model[id]].modelno);
    result = true;
  }
  return result;
}

//-------------------------------------------------
// スキャン済みデバイスリストを初期状態に戻す
//-------------------------------------------------
DXAPIDLL void DXL_InitDevicesList (void) {
  Devices.num = 0;
  for (int i = 0; i < 256; i++) {
    Devices.id2model[i] = 0;
    Devices.err[i] = 0xffff;
    Devices.opmode[i] = 255;
  }
}
