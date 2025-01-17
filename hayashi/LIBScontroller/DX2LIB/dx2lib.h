/*----------------------------------------------------------*
    dx2lib.h
    Dynamixel2プロトコル用API V2.9
                                       Last Edit '22 03/01
   Copyright (c) 2005, 2022 BestTechnology CO.,LTD.
 *----------------------------------------------------------*/

#ifndef _DX2LIB_H_INCLUDE
#define _DX2LIB_H_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifndef _MSC_VER
#include <stdbool.h>
#endif

#include <stdint.h>

#define BROADCASTING_ID     (0xfe)

#define DXMAXLENGTH         (10000)

#ifndef _WIN32
#ifdef __MAKE_LIB__
typedef int                             HANDLE;
typedef pthread_mutex_t                 CRITICAL_SECTION;

#define EnterCriticalSection(mtx)       pthread_mutex_lock(mtx)
#define LeaveCriticalSection(mtx)       pthread_mutex_unlock(mtx)
#define DeleteCriticalSection(mtx)      pthread_mutex_destroy(mtx)
#define InitializeCriticalSection(mtx)  pthread_mutex_init(mtx,NULL)
#define SetEvent(evt)                   pthread_cond_signal(evt)
#define INVALID_int_VALUE               -1
#define INVALID_HANDLE_VALUE            -1
#endif
#endif

// Device ID
#ifdef _WIN32
#ifdef _WIN64
typedef uint64_t TDeviceID;
#else
typedef uint32_t TDeviceID;
#endif
#define _PACKED_  __attribute__((gcc_struct,__packed__))
#else
#ifdef __APPLE__
typedef uint64_t TDeviceID;
#define _PACKED_  __attribute__((__packed__))
#else
#if defined(__x86_64__) | defined(__aarch64__ )
typedef uint64_t TDeviceID;
#if defined(__x86_64__)
#define _PACKED_  __attribute__((gcc_struct,__packed__))
#else
#define _PACKED_  __attribute__((__packed__))
#endif
#else
typedef uint32_t TDeviceID;
#if defined(__x86__)
#define _PACKED_  __attribute__((gcc_struct,__packed__))
#else
#define _PACKED_  __attribute__((__packed__))
#endif
#endif
#endif
#endif

// Kind of instruction
typedef enum {INST_PING = 1, INST_READ = 2, INST_WRITE = 3, INST_REG_WRITE = 4, INST_ACTION = 5, INST_RESET = 6, INST_REBOOT = 8, INST_CLEAR = 0x10, INST_BACKUP = 0x20, INST_STATUS = 0x55, INST_SYNC_READ = 0x82, INST_SYNC_WRITE = 0x83, INST_FAST_SYNC_READ = 0x8A, INST_BULK_READ = 0x92, INST_BULK_WRITE = 0x93, INST_FAST_BULK_READ = 0x9A} TInstruction;

// Error flags
typedef uint16_t TErrorCode;
#define ERR_INVALID_DEVID     (1 << 15)
#define ERR_INVALID_ID        (1 << 14)
#define ERR_DIFF_ID           (1 << 13)
#define ERR_ILLEGAL_SIZE      (1 << 12)
#define ERR_INVALID_PARAM     (1 << 11)
#define ERR_COMM              (1 << 10)
#define ERR_CHECKSUM          (1 << 9)
#define ERR_TIMEOUT           (1 << 8)
#define ERR_DX2_ALERT         (1 << 7)
#define ERR_DX2_ACCESS        (7)
#define ERR_DX2_DATALIMIT     (6)
#define ERR_DX2_DATALENGTH    (5)
#define ERR_DX2_DATARANGE     (4)
#define ERR_DX2_CRC           (3)
#define ERR_DX2_INSTRUCTION   (2)
#define ERR_DX2_RESULT        (1)
#define ERR_NON               (0)

// Alarm status
#ifdef _MSC_VER
__pragma (pack (push, 1))
typedef struct {
  uint8_t     id;
  TErrorCode  Status;
} TDx2AlarmStatus;

typedef struct {
  uint16_t addr;
  uint16_t length;
  uint8_t  ids[256];
} TSyncReadParam;

typedef struct {
  uint8_t  id;
  uint16_t addr;
  uint16_t length;
} TBulkReadParam;

typedef struct {
  uint16_t    size;
  uint8_t     id;
  TErrorCode  err;
  uint8_t     dat[];
} TBulkReadResult, *PBulkReadResult;
__pragma (pack (pop))
#elif defined(__GNUC__)
typedef struct {
  uint8_t     id;
  TErrorCode  Status;
} _PACKED_ TDx2AlarmStatus, *PDx2AlarmStatus;

typedef struct {
  uint16_t addr;
  uint16_t length;
  uint8_t  ids[256];
} _PACKED_ TSyncReadParam;

typedef struct {
  uint8_t  id;
  uint16_t addr;
  uint16_t length;
} _PACKED_ TBulkReadParam;

typedef struct {
  uint16_t    size;
  uint8_t     id;
  TErrorCode  err;
  uint8_t     dat[];
} _PACKED_ TBulkReadResult, *PBulkReadResult;
#endif

// デバイスの種別
typedef enum {
  devtNONE, devtDX, devtAX, devtRX, devtEX, devtMX, devtXL320, devtPRO, devtPROP, devtX
} TDXL_DevType;

// 全シリーズモデル情報
#ifdef _MSC_VER
__pragma (pack (push, 1))
typedef struct {
  uint16_t  modelno;          // デバイス固有のモデル番号
  char      name[16];
  TDXL_DevType devtype;       // デバイスのグループ
  struct {                    // 位置の範囲
    int32_t max;
    int32_t min;
  }         positionlimit;
  struct {                    // 角度の範囲
    double  max;
    double  min;
  }         anglelimit;
  struct {                    // 速度の範囲
    int32_t max;
    int32_t min;
  }         velocitylimit;
  struct {                    // PWMの範囲
    int32_t max;
    int32_t min;
  }         pwmlimit;
  double    velocityratio;    // 角速度変換係数 [deg/sec]
  double    currentratio;     // 電流変換係数 [mA]
  double    pwmratio;         // PWM変換係数 [%]
} TDXL_ModelInfo, *PDXL_ModelInfo;
__pragma (pack (pop))
#elif defined(__GNUC__)
typedef struct {
  uint16_t  modelno;          // デバイス固有のモデル番号
  char      name[16];
  TDXL_DevType devtype;       // デバイスのグループ
  struct {                    // 位置の範囲
    int32_t max;
    int32_t min;
  } _PACKED_ positionlimit;
  struct {                    // 角度の範囲
    double  max;
    double  min;
  } _PACKED_ anglelimit;
  struct {                    // 速度の範囲
    int32_t max;
    int32_t min;
  } _PACKED_ velocitylimit;
  struct {                    // PWMの範囲
    int32_t max;
    int32_t min;
  } _PACKED_ pwmlimit;
  double    velocityratio;    // 角速度変換係数 [deg/sec]
  double    currentratio;     // 電流変換係数 [mA]
  double    pwmratio;         // PWM変換係数 [%]
} _PACKED_ TDXL_ModelInfo, *PDXL_ModelInfo;
#endif

//
typedef struct {
  double  angle;
  double  velocity;
} TAngleVelocity, *PAngleVelocity;

typedef struct {
  double  angle;
  double  sec;
} TAngleTime, *PAngleTime;

#ifdef _WIN32
#ifdef __cplusplus
#ifdef __MAKE_LIB__
#define DXAPIDLL extern "C" __declspec(dllexport)
#else
#define DXAPIDLL extern "C" __declspec(dllimport)
#endif
#else
#ifdef __MAKE_LIB__
#define DXAPIDLL __declspec(dllexport)
#else
#define DXAPIDLL __declspec(dllimport)
#endif
#endif
#else
#define DXAPIDLL
#define WINAPI
#endif

#ifndef _DYNAMICLOAD
// When static DLL calls is used the following prototype declaration
DXAPIDLL TDeviceID WINAPI DX2_OpenPort (char *name, uint32_t baud);
DXAPIDLL bool WINAPI DX2_ClosePort (TDeviceID dvid);
DXAPIDLL bool WINAPI DX2_SetBaudrate (TDeviceID dvid, uint32_t baud);
DXAPIDLL bool WINAPI DX2_Active (TDeviceID dvid);
DXAPIDLL void WINAPI DX2_SetTimeOutOffset (TDeviceID dvid, uint32_t t);
DXAPIDLL double WINAPI GetQueryPerformanceCounter (void);
DXAPIDLL bool WINAPI DX2_TxPacket (TDeviceID dvid, uint8_t id, TInstruction inst, const uint8_t *param, uint32_t len, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_RxPacket (TDeviceID dvid, uint8_t *rdata, uint32_t rdatasize, uint32_t *rlen, uint32_t timeout, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_ReadByteData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_WriteByteData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t dat, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_ReadWordData (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t *rdata, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_WriteWordData (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t dat, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_ReadLongData (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t *rdata, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_WriteLongData (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t dat, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_ReadBlockData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, uint32_t len, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_WriteBlockData (TDeviceID dvid, uint8_t id, uint16_t adr, const uint8_t *dat, uint32_t len, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_Ping (TDeviceID dvid, uint8_t id, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_Ping2 (TDeviceID dvid, uint32_t *num, TDx2AlarmStatus *AlarmStatus, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_ReadSyncData (TDeviceID dvid, const TSyncReadParam *param, uint32_t *num, uint8_t *dat, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_WriteSyncData (TDeviceID dvid, uint8_t *dat, uint32_t size, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_ReadBulkData (TDeviceID dvid, const TBulkReadParam *param, uint32_t *num, uint8_t *dat, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_WriteBulkData (TDeviceID dvid, uint8_t *dat, uint32_t size, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_Reset (TDeviceID dvid, uint8_t id, TErrorCode *err);
DXAPIDLL bool WINAPI DX2_Reboot (TDeviceID dvid, uint8_t id, TErrorCode *err);

DXAPIDLL bool WINAPI DXL_SetLED (TDeviceID dvid, uint8_t id, bool en);
DXAPIDLL bool WINAPI DXL_SetTorqueEnable (TDeviceID dvid, uint8_t id, bool en);
DXAPIDLL bool WINAPI DXL_SetTorqueEnables (TDeviceID dvid, const uint8_t *ids, const bool *ens, int num);
DXAPIDLL bool WINAPI DXL_SetTorqueEnablesEquival (TDeviceID dvid, const uint8_t *ids, int num, bool en);
DXAPIDLL bool WINAPI DXL_GetTorqueEnable (TDeviceID dvid, uint8_t id, bool *en);
DXAPIDLL bool WINAPI DXL_GetTorqueEnables (TDeviceID dvid, const uint8_t *ids, bool *en, int num);
DXAPIDLL bool WINAPI DXL_SetGoalAngle (TDeviceID dvid, uint8_t id, double angle);
DXAPIDLL bool WINAPI DXL_SetGoalAngles (TDeviceID dvid, const uint8_t *ids, const double *angles, int num);
DXAPIDLL bool WINAPI DXL_GetPresentAngle (TDeviceID dvid, uint8_t id, double *angle);
DXAPIDLL bool WINAPI DXL_GetPresentAngles (TDeviceID dvid, const uint8_t *ids, double *angles, int num);
DXAPIDLL bool WINAPI DXL_StandStillAngle (TDeviceID dvid, uint8_t id);
DXAPIDLL bool WINAPI DXL_StandStillAngles (TDeviceID dvid, const uint8_t *ids, int num);
DXAPIDLL bool WINAPI DXL_SetGoalVelocity (TDeviceID dvid, uint8_t id, double velocity);
DXAPIDLL bool WINAPI DXL_SetGoalVelocities (TDeviceID dvid, const uint8_t *ids, const double *velocities, int num);
DXAPIDLL bool WINAPI DXL_GetPresentVelocity (TDeviceID dvid, uint8_t id, double *velocity);
DXAPIDLL bool WINAPI DXL_GetPresentVelocities (TDeviceID dvid, const uint8_t *ids, double *velocities, int num);
DXAPIDLL bool WINAPI DXL_SetGoalAngleAndVelocity (TDeviceID dvid, uint8_t id, double angle, double velocity);
DXAPIDLL bool WINAPI DXL_SetGoalAnglesAndVelocities (TDeviceID dvid, const uint8_t *ids, PAngleVelocity anglevelocity, int num);
DXAPIDLL bool WINAPI DXL_SetGoalAngleAndTime (TDeviceID dvid, uint8_t id, double angle, double sec);
DXAPIDLL bool WINAPI DXL_SetGoalAnglesAndTime (TDeviceID dvid, const uint8_t *ids, const double *angles, int num, double sec);
DXAPIDLL bool WINAPI DXL_SetGoalAngleAndTime2 (TDeviceID dvid, uint8_t id, double angle, double sec);
DXAPIDLL bool WINAPI DXL_SetGoalAnglesAndTime2 (TDeviceID dvid, const uint8_t *ids, const double *angles, int num, double sec);
DXAPIDLL bool WINAPI DXL_SetGoalCurrent (TDeviceID dvid, uint8_t id, double current);
DXAPIDLL bool WINAPI DXL_SetGoalCurrents (TDeviceID dvid, const uint8_t *ids, const double *currents, int num);
DXAPIDLL bool WINAPI DXL_GetPresentCurrent (TDeviceID dvid, uint8_t id, double *current);
DXAPIDLL bool WINAPI DXL_GetPresentCurrents (TDeviceID dvid, const uint8_t *ids, double *currents, int num);
DXAPIDLL bool WINAPI DXL_SetGoalPWM (TDeviceID dvid, uint8_t id, double pwm);
DXAPIDLL bool WINAPI DXL_SetGoalPWMs (TDeviceID dvid, const uint8_t *ids, const double *pwms, int num);
DXAPIDLL bool WINAPI DXL_GetPresentPWM (TDeviceID dvid, uint8_t id, double *pwm);
DXAPIDLL bool WINAPI DXL_GetPresentPWMs (TDeviceID dvid, const uint8_t *ids, double *pwms, int num);
DXAPIDLL bool WINAPI DXL_SetDriveMode (TDeviceID dvid, uint8_t id, uint8_t mode);
DXAPIDLL bool WINAPI DXL_SetDriveModesEquival (TDeviceID dvid, const uint8_t *ids, int num, uint8_t mode);
DXAPIDLL bool WINAPI DXL_SetOperatingMode (TDeviceID dvid, uint8_t id, uint8_t mode);
DXAPIDLL bool WINAPI DXL_SetOperatingModesEquival (TDeviceID dvid, const uint8_t *ids, int num, uint8_t mode);
DXAPIDLL bool WINAPI DXL_GetOperatingMode (TDeviceID dvid, uint8_t id, uint8_t *mode);
DXAPIDLL bool WINAPI DXL_GetHWErrorCode (TDeviceID dvid, uint8_t id, uint8_t *hwerr);
DXAPIDLL TErrorCode WINAPI DXL_GetErrorCode (TDeviceID dvid, uint8_t id);
DXAPIDLL PDXL_ModelInfo WINAPI DXL_GetModelInfo (TDeviceID dvid, uint8_t id);
DXAPIDLL int WINAPI DXL_ScanDevices (TDeviceID dvid, uint8_t *ids);
DXAPIDLL bool WINAPI DXL_PrintDevicesList (int (*pf) (const char *, ...));
DXAPIDLL void WINAPI DXL_InitDevicesList (void);

#else

#ifdef _WIN32
// When the dynamic DLL calls is used the following prototype declaration
TDeviceID (WINAPI *DX2_OpenPort) (char *name, uint32_t baud) = NULL;
bool (WINAPI *DX2_ClosePort) (TDeviceID dvid) = NULL;
bool (WINAPI *DX2_SetBaudrate) (TDeviceID dvid, uint32_t baud) = NULL;
bool (WINAPI *DX2_Active) (TDeviceID dvid) = NULL;
void (WINAPI *DX2_SetTimeOutOffset) (TDeviceID dvid, uint32_t offsettime) = NULL;
double (WINAPI *GetQueryPerformanceCounter) (void) = NULL;
bool (WINAPI *DX2_TxPacket) (TDeviceID dvid, uint8_t id, TInstruction inst, const uint8_t *param, uint32_t len, TErrorCode *err) = NULL;
bool (WINAPI *DX2_RxPacket) (TDeviceID dvid, uint8_t *rdata, uint32_t rdatasize, uint32_t *rlen, uint32_t timeout, TErrorCode *err) = NULL;
bool (WINAPI *DX2_ReadByteData) (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, TErrorCode *err) = NULL;
bool (WINAPI *DX2_WriteByteData) (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t dat, TErrorCode *err) = NULL;
bool (WINAPI *DX2_ReadWordData) (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t *rdata, TErrorCode *err) = NULL;
bool (WINAPI *DX2_WriteWordData) (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t dat, TErrorCode *err) = NULL;
bool (WINAPI *DX2_ReadLongData) (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t *rdata, TErrorCode *err) = NULL;
bool (WINAPI *DX2_WriteLongData) (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t dat, TErrorCode *err) = NULL;
bool (WINAPI *DX2_ReadBlockData) (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, uint32_t len, TErrorCode *err) = NULL;
bool (WINAPI *DX2_WriteBlockData) (TDeviceID dvid, uint8_t id, uint16_t adr, const uint8_t *dat, uint32_t len, TErrorCode *err) = NULL;
bool (WINAPI *DX2_Ping) (TDeviceID dvid, uint8_t id, TErrorCode *err) = NULL;
bool (WINAPI *DX2_Ping2) (TDeviceID dvid, uint32_t *num, TDx2AlarmStatus *AlarmStatus, TErrorCode *err) = NULL;
bool (WINAPI *DX2_ReadSyncData) (TDeviceID dvid, const TSyncReadParam *param, uint32_t *num, uint8_t *dat, TErrorCode *err) = NULL;
bool (WINAPI *DX2_WriteSyncData) (TDeviceID dvid, uint8_t *dat, uint32_t size, TErrorCode *err) = NULL;
bool (WINAPI *DX2_ReadBulkData) (TDeviceID dvid, const TBulkReadParam *param, uint32_t *num, uint8_t *dat, TErrorCode *err) = NULL;
bool (WINAPI *DX2_WriteBulkData) (TDeviceID dvid, uint8_t *dat, uint32_t size, TErrorCode *err) = NULL;
bool (WINAPI *DX2_Reset) (TDeviceID dvid, uint8_t id, TErrorCode *err) = NULL;
bool (WINAPI *DX2_Reboot) (TDeviceID dvid, uint8_t id, TErrorCode *err) = NULL;

bool (WINAPI *DXL_SetLED) (TDeviceID dvid, uint8_t id, bool en) = NULL;
bool (WINAPI *DXL_SetTorqueEnable) (TDeviceID dvid, uint8_t id, bool en) = NULL;
bool (WINAPI *DXL_SetTorqueEnables) (TDeviceID dvid, const uint8_t *ids, const bool *ens, int num) = NULL;
bool (WINAPI *DXL_SetTorqueEnablesEquival) (TDeviceID dvid, const uint8_t *ids, int num, bool en) = NULL;
bool (WINAPI *DXL_GetTorqueEnable) (TDeviceID dvid, uint8_t id, bool *en) = NULL;
bool (WINAPI *DXL_GetTorqueEnables) (TDeviceID dvid, const uint8_t *ids, bool *en, int num) = NULL;
bool (WINAPI *DXL_SetGoalAngle) (TDeviceID dvid, uint8_t id, double angle) = NULL;
bool (WINAPI *DXL_SetGoalAngles) (TDeviceID dvid, const uint8_t *ids, const double *angles, int num) = NULL;
bool (WINAPI *DXL_GetPresentAngle) (TDeviceID dvid, uint8_t id, double *angle) = NULL;
bool (WINAPI *DXL_GetPresentAngles) (TDeviceID dvid, const uint8_t *ids, double *angles, int num) = NULL;
bool (WINAPI *DXL_StandStillAngle) (TDeviceID dvid, uint8_t id);
bool (WINAPI *DXL_StandStillAngles) (TDeviceID dvid, const uint8_t *ids, int num);
bool (WINAPI *DXL_SetGoalVelocity) (TDeviceID dvid, uint8_t id, double velocity) = NULL;
bool (WINAPI *DXL_SetGoalVelocities) (TDeviceID dvid, const uint8_t *ids, const double *velocities, int num) = NULL;
bool (WINAPI *DXL_GetPresentVelocity) (TDeviceID dvid, uint8_t id, double *velocity) = NULL;
bool (WINAPI *DXL_GetPresentVelocities) (TDeviceID dvid, const uint8_t *ids, double *velocities, int num) = NULL;
bool (WINAPI *DXL_SetGoalAngleAndVelocity) (TDeviceID dvid, uint8_t id, double angle, double velocity) = NULL;
bool (WINAPI *DXL_SetGoalAnglesAndVelocities) (TDeviceID dvid, const uint8_t *ids, PAngleVelocity anglevelocity, int num) = NULL;
bool (WINAPI *DXL_SetGoalAngleAndTime) (TDeviceID dvid, uint8_t id, double angle, double sec) = NULL;
bool (WINAPI *DXL_SetGoalAnglesAndTime) (TDeviceID dvid, const uint8_t *ids, const double *angles, int num, double sec) = NULL;
bool (WINAPI *DXL_SetGoalAngleAndTime2) (TDeviceID dvid, uint8_t id, double angle, double sec) = NULL;
bool (WINAPI *DXL_SetGoalAnglesAndTime2) (TDeviceID dvid, const uint8_t *ids, const double *angles, int num, double sec) = NULL;
bool (WINAPI *DXL_SetGoalCurrent) (TDeviceID dvid, uint8_t id, double current) = NULL;
bool (WINAPI *DXL_SetGoalCurrents) (TDeviceID dvid, const uint8_t *ids, const double *currents, int num) = NULL;
bool (WINAPI *DXL_GetPresentCurrent) (TDeviceID dvid, uint8_t id, double *current) = NULL;
bool (WINAPI *DXL_GetPresentCurrents) (TDeviceID dvid, const uint8_t *ids, double *currents, int num) = NULL;
bool (WINAPI *DXL_SetGoalPWM) (TDeviceID dvid, uint8_t id, double pwm) = NULL;
bool (WINAPI *DXL_SetGoalPWMs) (TDeviceID dvid, const uint8_t *ids, const double *pwms, int num) = NULL;
bool (WINAPI *DXL_GetPresentPWM) (TDeviceID dvid, uint8_t id, double *pwm) = NULL;
bool (WINAPI *DXL_GetPresentPWMs) (TDeviceID dvid, const uint8_t *ids, double *pwms, int num) = NULL;
bool (WINAPI *DXL_SetDriveMode) (TDeviceID dvid, uint8_t id, uint8_t mode) = NULL;
bool (WINAPI *DXL_SetDriveModesEquival) (TDeviceID dvid, const uint8_t *ids, int num, uint8_t mode) = NULL;
bool (WINAPI *DXL_SetOperatingMode) (TDeviceID dvid, uint8_t id, uint8_t mode) = NULL;
bool (WINAPI *DXL_SetOperatingModesEquival) (TDeviceID dvid, const uint8_t *ids, int num, uint8_t mode) = NULL;
bool (WINAPI *DXL_GetOperatingMode) (TDeviceID dvid, uint8_t id, uint8_t *mode) = NULL;
bool (WINAPI *DXL_GetHWErrorCode) (TDeviceID dvid, uint8_t id, uint8_t *hwerr) = NULL;
TErrorCode (WINAPI *DXL_GetErrorCode) (TDeviceID dvid, uint8_t id) = NULL;
PDXL_ModelInfo (WINAPI *DXL_GetModelInfo) (TDeviceID dvid, uint8_t id) = NULL;
int (WINAPI *DXL_ScanDevices) (TDeviceID dvid, uint8_t *ids) = NULL;
bool (WINAPI *DXL_PrintDevicesList) (int (*pf) (const char *, ...)) = NULL;
void (WINAPI *DXL_InitDevicesList) (void) = NULL;

static HMODULE hModule = NULL;

#ifdef _WIN32
#ifdef _WIN64
#ifdef _MSC_VER
#define DLLNAME   L"dx2lib_x64.dll"
#else
#define DLLNAME   "dx2lib_x64.dll"
#endif
#else
#ifdef _MSC_VER
#define DLLNAME   L"dx2lib_x32.dll"
#else
#define DLLNAME   "dx2lib_x32.dll"
#endif
#endif
#else
#ifdef __x86_64__
#define DLLNAME   "dx2lib_x64.dll"
#else
#define DLLNAME   "dx2lib_x32.dll"
#endif
#endif

// Load the DLL
bool LoadDLL (void) {
  hModule = LoadLibrary (DLLNAME);
  if (hModule == NULL) {
    return false;
  }

  * (FARPROC *)&DX2_OpenPort               = GetProcAddress (hModule, "DX2_OpenPort");
  * (FARPROC *)&DX2_ClosePort              = GetProcAddress (hModule, "DX2_ClosePort");
  * (FARPROC *)&DX2_SetBaudrate            = GetProcAddress (hModule, "DX2_SetBaudrate");
  * (FARPROC *)&DX2_Active                 = GetProcAddress (hModule, "DX2_Active");
  * (FARPROC *)&DX2_SetTimeOutOffset       = GetProcAddress (hModule, "DX2_SetTimeOutOffset");
  * (FARPROC *)&GetQueryPerformanceCounter = GetProcAddress (hModule, "GetQueryPerformanceCounter");
  * (FARPROC *)&DX2_TxPacket               = GetProcAddress (hModule, "DX2_TxPacket");
  * (FARPROC *)&DX2_RxPacket               = GetProcAddress (hModule, "DX2_RxPacket");
  * (FARPROC *)&DX2_ReadByteData           = GetProcAddress (hModule, "DX2_ReadByteData");
  * (FARPROC *)&DX2_WriteByteData          = GetProcAddress (hModule, "DX2_WriteByteData");
  * (FARPROC *)&DX2_ReadWordData           = GetProcAddress (hModule, "DX2_ReadWordData");
  * (FARPROC *)&DX2_WriteWordData          = GetProcAddress (hModule, "DX2_WriteWordData");
  * (FARPROC *)&DX2_ReadLongData           = GetProcAddress (hModule, "DX2_ReadLongData");
  * (FARPROC *)&DX2_WriteLongData          = GetProcAddress (hModule, "DX2_WriteLongData");
  * (FARPROC *)&DX2_ReadBlockData          = GetProcAddress (hModule, "DX2_ReadBlockData");
  * (FARPROC *)&DX2_WriteBlockData         = GetProcAddress (hModule, "DX2_WriteBlockData");
  * (FARPROC *)&DX2_ReadSyncData           = GetProcAddress (hModule, "DX2_ReadSyncData");
  * (FARPROC *)&DX2_WriteSyncData          = GetProcAddress (hModule, "DX2_WriteSyncData");
  * (FARPROC *)&DX2_ReadBulkData           = GetProcAddress (hModule, "DX2_ReadBulkData");
  * (FARPROC *)&DX2_WriteBulkData          = GetProcAddress (hModule, "DX2_WriteBulkData");
  * (FARPROC *)&DX2_Ping                   = GetProcAddress (hModule, "DX2_Ping");
  * (FARPROC *)&DX2_Ping2                  = GetProcAddress (hModule, "DX2_Ping2");
  * (FARPROC *)&DX2_Reset                  = GetProcAddress (hModule, "DX2_Reset");
  * (FARPROC *)&DX2_Reboot                 = GetProcAddress (hModule, "DX2_Reboot");

  * (FARPROC *)&DXL_SetTorqueEnable           = GetProcAddress (hModule, "DXL_SetTorqueEnable");
  * (FARPROC *)&DXL_SetTorqueEnables          = GetProcAddress (hModule, "DXL_SetTorqueEnables");
  * (FARPROC *)&DXL_SetTorqueEnablesEquival   = GetProcAddress (hModule, "DXL_SetTorqueEnablesEquival");
  * (FARPROC *)&DXL_GetTorqueEnable           = GetProcAddress (hModule, "DXL_GetTorqueEnable");
  * (FARPROC *)&DXL_GetTorqueEnables          = GetProcAddress (hModule, "DXL_GetTorqueEnables");
  * (FARPROC *)&DXL_SetLED                    = GetProcAddress (hModule, "DXL_SetLED");
  * (FARPROC *)&DXL_SetGoalAngle              = GetProcAddress (hModule, "DXL_SetGoalAngle");
  * (FARPROC *)&DXL_SetGoalAngles             = GetProcAddress (hModule, "DXL_SetGoalAngles");
  * (FARPROC *)&DXL_GetPresentAngle           = GetProcAddress (hModule, "DXL_GetPresentAngle");
  * (FARPROC *)&DXL_GetPresentAngles          = GetProcAddress (hModule, "DXL_GetPresentAngles");
  * (FARPROC *)&DXL_StandStillAngle           = GetProcAddress (hModule, "DXL_StandStillAngle");
  * (FARPROC *)&DXL_StandStillAngles          = GetProcAddress (hModule, "DXL_StandStillAngles");
  * (FARPROC *)&DXL_SetGoalVelocity           = GetProcAddress (hModule, "DXL_SetGoalVelocity");
  * (FARPROC *)&DXL_SetGoalVelocities         = GetProcAddress (hModule, "DXL_SetGoalVelocities");
  * (FARPROC *)&DXL_GetPresentVelocity        = GetProcAddress (hModule, "DXL_GetPresentVelocity");
  * (FARPROC *)&DXL_GetPresentVelocities      = GetProcAddress (hModule, "DXL_GetPresentVelocities");
  * (FARPROC *)&DXL_SetGoalAngleAndVelocity   = GetProcAddress (hModule, "DXL_SetGoalAngleAndVelocity");
  * (FARPROC *)&DXL_SetGoalAnglesAndVelocities = GetProcAddress (hModule, "DXL_SetGoalAnglesAndVelocities");
  * (FARPROC *)&DXL_SetGoalAngleAndTime       = GetProcAddress (hModule, "DXL_SetGoalAngleAndTime");
  * (FARPROC *)&DXL_SetGoalAnglesAndTime      = GetProcAddress (hModule, "DXL_SetGoalAnglesAndTime");
  * (FARPROC *)&DXL_SetGoalAngleAndTime2      = GetProcAddress (hModule, "DXL_SetGoalAngleAndTime2");
  * (FARPROC *)&DXL_SetGoalAnglesAndTime2     = GetProcAddress (hModule, "DXL_SetGoalAnglesAndTime2");
  * (FARPROC *)&DXL_SetGoalPWM                = GetProcAddress (hModule, "DXL_SetGoalPWM");
  * (FARPROC *)&DXL_SetGoalPWMs               = GetProcAddress (hModule, "DXL_SetGoalPWMs");
  * (FARPROC *)&DXL_GetPresentPWM             = GetProcAddress (hModule, "DXL_GetPresentPWM");
  * (FARPROC *)&DXL_GetPresentPWMs            = GetProcAddress (hModule, "DXL_GetPresentPWMs");
  * (FARPROC *)&DXL_SetGoalCurrent            = GetProcAddress (hModule, "DXL_SetGoalCurrent");
  * (FARPROC *)&DXL_SetGoalCurrents           = GetProcAddress (hModule, "DXL_SetGoalCurrents");
  * (FARPROC *)&DXL_GetPresentCurrent         = GetProcAddress (hModule, "DXL_GetPresentCurrent");
  * (FARPROC *)&DXL_GetPresentCurrents        = GetProcAddress (hModule, "DXL_GetPresentCurrents");
  * (FARPROC *)&DXL_SetDriveMode              = GetProcAddress (hModule, "DXL_SetDriveMode");
  * (FARPROC *)&DXL_SetDriveModesEquival      = GetProcAddress (hModule, "DXL_SetDriveModesEquival");
  * (FARPROC *)&DXL_SetOperatingMode          = GetProcAddress (hModule, "DXL_SetOperatingMode");
  * (FARPROC *)&DXL_SetOperatingModesEquival  = GetProcAddress (hModule, "DXL_SetOperatingModesEquival");
  * (FARPROC *)&DXL_GetOperatingMode          = GetProcAddress (hModule, "DXL_GetOperatingMode");
  * (FARPROC *)&DXL_GetHWErrorCode            = GetProcAddress (hModule, "DXL_GetHWErrorCode");
  * (FARPROC *)&DXL_GetErrorCode              = GetProcAddress (hModule, "DXL_GetErrorCode");
  * (FARPROC *)&DXL_GetModelInfo              = GetProcAddress (hModule, "DXL_GetModelInfo");
  * (FARPROC *)&DXL_ScanDevices               = GetProcAddress (hModule, "DXL_ScanDevices");
  * (FARPROC *)&DXL_PrintDevicesList          = GetProcAddress (hModule, "DXL_PrintDevicesList");
  * (FARPROC *)&DXL_InitDevicesList           = GetProcAddress (hModule, "DXL_InitDevicesList");

  return
    (DX2_OpenPort               != NULL) &&
    (DX2_ClosePort              != NULL) &&
    (DX2_SetBaudrate            != NULL) &&
    (DX2_Active                 != NULL) &&
    (DX2_SetTimeOutOffset       != NULL) &&
    (GetQueryPerformanceCounter != NULL) &&
    (DX2_TxPacket               != NULL) &&
    (DX2_RxPacket               != NULL) &&
    (DX2_ReadByteData           != NULL) &&
    (DX2_WriteByteData          != NULL) &&
    (DX2_ReadWordData           != NULL) &&
    (DX2_WriteWordData          != NULL) &&
    (DX2_ReadLongData           != NULL) &&
    (DX2_WriteLongData          != NULL) &&
    (DX2_ReadBlockData          != NULL) &&
    (DX2_WriteBlockData         != NULL) &&
    (DX2_ReadSyncData           != NULL) &&
    (DX2_WriteSyncData          != NULL) &&
    (DX2_ReadBulkData           != NULL) &&
    (DX2_WriteBulkData          != NULL) &&
    (DX2_Ping                   != NULL) &&
    (DX2_Ping2                  != NULL) &&
    (DX2_Reset                  != NULL) &&
    (DX2_Reboot                 != NULL) &&

    (DXL_SetTorqueEnable           != NULL) &&
    (DXL_SetTorqueEnables          != NULL) &&
    (DXL_SetTorqueEnablesEquival   != NULL) &&
    (DXL_GetTorqueEnable           != NULL) &&
    (DXL_GetTorqueEnables          != NULL) &&
    (DXL_SetLED                    != NULL) &&
    (DXL_SetGoalAngle              != NULL) &&
    (DXL_SetGoalAngles             != NULL) &&
    (DXL_GetPresentAngle           != NULL) &&
    (DXL_GetPresentAngles          != NULL) &&
    (DXL_StandStillAngle           != NULL) &&
    (DXL_StandStillAngles          != NULL) &&
    (DXL_SetGoalVelocity           != NULL) &&
    (DXL_SetGoalVelocities         != NULL) &&
    (DXL_GetPresentVelocity        != NULL) &&
    (DXL_GetPresentVelocities      != NULL) &&
    (DXL_SetGoalAngleAndVelocity   != NULL) &&
    (DXL_SetGoalAnglesAndVelocities != NULL) &&
    (DXL_SetGoalAngleAndTime       != NULL) &&
    (DXL_SetGoalAnglesAndTime      != NULL) &&
    (DXL_SetGoalAngleAndTime2      != NULL) &&
    (DXL_SetGoalAnglesAndTime2     != NULL) &&
    (DXL_SetGoalCurrent            != NULL) &&
    (DXL_SetGoalCurrents           != NULL) &&
    (DXL_GetPresentCurrent         != NULL) &&
    (DXL_GetPresentCurrents        != NULL) &&
    (DXL_SetGoalPWM                != NULL) &&
    (DXL_SetGoalPWMs               != NULL) &&
    (DXL_GetPresentPWM             != NULL) &&
    (DXL_GetPresentPWMs            != NULL) &&
    (DXL_SetDriveMode              != NULL) &&
    (DXL_SetDriveModesEquival      != NULL) &&
    (DXL_SetOperatingMode          != NULL) &&
    (DXL_SetOperatingModesEquival  != NULL) &&
    (DXL_GetOperatingMode          != NULL) &&
    (DXL_GetHWErrorCode            != NULL) &&
    (DXL_GetErrorCode              != NULL) &&
    (DXL_GetModelInfo              != NULL) &&
    (DXL_ScanDevices               != NULL) &&
    (DXL_PrintDevicesList          != NULL) &&
    (DXL_InitDevicesList           != NULL);
}

// Unload the DLL
void UnloadDLL (void) {
  if (hModule != NULL) {
    FreeLibrary (hModule);
    DX2_OpenPort               = NULL;
    DX2_ClosePort              = NULL;
    DX2_SetBaudrate            = NULL;
    DX2_Active                 = NULL;
    DX2_SetTimeOutOffset       = NULL;
    GetQueryPerformanceCounter = NULL;
    DX2_TxPacket               = NULL;
    DX2_RxPacket               = NULL;
    DX2_ReadByteData           = NULL;
    DX2_WriteByteData          = NULL;
    DX2_ReadWordData           = NULL;
    DX2_WriteWordData          = NULL;
    DX2_ReadLongData           = NULL;
    DX2_WriteLongData          = NULL;
    DX2_ReadBlockData          = NULL;
    DX2_WriteBlockData         = NULL;
    DX2_ReadSyncData           = NULL;
    DX2_WriteSyncData          = NULL;
    DX2_ReadBulkData           = NULL;
    DX2_WriteBulkData          = NULL;
    DX2_Ping                   = NULL;
    DX2_Ping2                  = NULL;
    DX2_Reset                  = NULL;
    DX2_Reboot                 = NULL;

    DXL_SetTorqueEnable           = NULL;
    DXL_SetTorqueEnables          = NULL;
    DXL_SetTorqueEnablesEquival   = NULL;
    DXL_GetTorqueEnable           = NULL;
    DXL_GetTorqueEnables          = NULL;
    DXL_SetLED                    = NULL;
    DXL_SetGoalAngle              = NULL;
    DXL_SetGoalAngles             = NULL;
    DXL_GetPresentAngle           = NULL;
    DXL_GetPresentAngles          = NULL;
    DXL_StandStillAngle           = NULL;
    DXL_StandStillAngles          = NULL;
    DXL_SetGoalVelocity           = NULL;
    DXL_SetGoalVelocities         = NULL;
    DXL_GetPresentVelocity        = NULL;
    DXL_GetPresentVelocities      = NULL;
    DXL_SetGoalAngleAndVelocity   = NULL;
    DXL_SetGoalAnglesAndVelocities = NULL;
    DXL_SetGoalAngleAndTime       = NULL;
    DXL_SetGoalAnglesAndTime      = NULL;
    DXL_SetGoalAngleAndTime2      = NULL;
    DXL_SetGoalAnglesAndTime2     = NULL;
    DXL_SetGoalCurrent            = NULL;
    DXL_SetGoalCurrents           = NULL;
    DXL_GetPresentCurrent         = NULL;
    DXL_GetPresentCurrents        = NULL;
    DXL_SetGoalPWM                = NULL;
    DXL_SetGoalPWMs               = NULL;
    DXL_GetPresentPWM             = NULL;
    DXL_GetPresentPWMs            = NULL;
    DXL_SetDriveMode              = NULL;
    DXL_SetDriveModesEquival      = NULL;
    DXL_SetOperatingMode          = NULL;
    DXL_SetOperatingModesEquival  = NULL;
    DXL_GetOperatingMode          = NULL;
    DXL_GetHWErrorCode            = NULL;
    DXL_GetErrorCode              = NULL;
    DXL_GetModelInfo              = NULL;
    DXL_ScanDevices               = NULL;
    DXL_PrintDevicesList          = NULL;
    DXL_InitDevicesList           = NULL;
  }
}
#else
#error Not support dynamic load!!
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif //_DX2LIB2_H_INCLUDE
