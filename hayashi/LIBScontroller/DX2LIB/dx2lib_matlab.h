#ifndef _DXLIB2P_MATLAB_H_INCLUDE
#define _DXLIB2P_MATLAB_H_INCLUDE

#include <windows.h>

#define BROADCASTING_ID     (0xfe)
#define DXMAXLENGTH         (143)

typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;
typedef long long  int64_t;
typedef unsigned long long   uint64_t;

// デバイスID
#ifdef _WIN64
typedef uint64_t TDeviceID;
#else
typedef uint32_t TDeviceID;
#endif

// インストラクション
typedef enum {INST_PING = 1, INST_READ = 2, INST_WRITE = 3, INST_REG_WRITE = 4, INST_ACTION = 5, INST_RESET = 6, INST_REBOOT = 8, INST_STATUS = 0x55, INST_SYNC_READ = 0x82, INST_SYNC_WRITE = 0x83, INST_BULK_READ = 0x92, INST_BULK_WRITE = 0x93} TInstruction;

// エラーフラグ
typedef uint16_t TErrorCode;
#define ERR_INVALID_DEVID     (1 << 15)
#define ERR_INVALID_ID        (1 << 14)
#define ERR_DIFF_ID           (1 << 13)
#define ERR_ILLEGAL_SIZE      (1 << 12)
#define ERR_INVALID_PARAM     (1 << 11)
#define ERR_COMM              (1 << 10)
#define ERR_CHECKSUM          (1 << 9)
#define ERR_TIMEOUT           (1 << 8)
#define ERR_DX2_ALERT        (1 << 7)
#define ERR_DX2_LENGTHLONG   (6)
#define ERR_DX2_LENGTHSHORT  (5)
#define ERR_DX2_OVERRANGE    (4)
#define ERR_DX2_CRCERROR     (3)
#define ERR_DX2_UNDEFINST    (2)
#define ERR_DX2_INSTERROR    (1)
#define ERR_NON               (0)

// アラームステータス
typedef struct {
  uint8_t       id;
  TErrorCode  Status;
} __attribute__ ((packed)) TDx2PAlarmStatus;

TDeviceID __declspec (dllimport) WINAPI DX2_OpenPort (char *name, uint32_t baud);
bool __declspec (dllimport) WINAPI DX2_ClosePort (TDeviceID dvid);
bool __declspec (dllimport) WINAPI DX2_SetBaudrate (TDeviceID dvid, uint32_t baud);
bool __declspec (dllimport) WINAPI DX2_Active (TDeviceID dvid);
void __declspec (dllimport) WINAPI DX2_SetTimeOutOffset (TDeviceID dvid, uint32_t t);
double __declspec (dllimport) WINAPI GetQueryPerformanceCounter (void);
bool __declspec (dllimport) WINAPI DX2_TxPacket (TDeviceID dvid, uint8_t id, TInstruction inst, uint8_t *param, uint32_t len, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_RxPacket (TDeviceID dvid, uint8_t *rdata, uint32_t rdatasize, uint32_t *rlen, uint32_t timeout, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_ReadByteData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_WriteByteData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t dat, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_ReadWordData (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t *rdata, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_WriteWordData (TDeviceID dvid, uint8_t id, uint16_t adr, uint16_t dat, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_ReadLongData (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t *rdata, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_WriteLongData (TDeviceID dvid, uint8_t id, uint16_t adr, uint32_t dat, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_ReadBlockData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *rdata, uint32_t len, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_WriteBlockData (TDeviceID dvid, uint8_t id, uint16_t adr, uint8_t *dat, uint32_t len, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_Ping (TDeviceID dvid, uint8_t id, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_Ping2 (TDeviceID dvid, uint32_t *num, TDx2PAlarmStatus *AlarmStatus, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_ReadSyncData (TDeviceID dvid, const uint8_t *param, uint32_t *num, uint8_t *dat, TErrorCode *err)
bool __declspec (dllimport) WINAPI DX2_WriteSyncData (TDeviceID dvid, uint8_t *dat, uint32_t size, TErrorCode *err);
bool __declspec (dllimport) WINAPI DX2_Reset (TDeviceID dvid, uint8_t id, TErrorCode *err);

#endif //_DXLIB2P_H_INCLUDE
