/*
  ID・ボーレート・動作モードの変更と動作確認
*/

#ifdef _WIN32
#define _DYNAMICLOAD
#endif

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <stdbool.h>
#include  <ctype.h>
#ifdef _WIN32
#include  <conio.h>
#include  <io.h>
#include  <time.h>
#else
#include  "kbhit.h"
#endif
#include  "dx2lib.h"
#include  "dx2memmap.h"
#include  "dxmisc.h"

#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

#define _ADDR_ID        (7)
#define _ADDR_BAUDRATE  (8)

const uint32_t baudlist[7] = { 9600, 57600, 115200, 1000000, 2000000, 3000000, 4000000 };
const char *opmode[] = {
  "cur", "velo", "", "pos", "multiturn", "multiturn+cur",
  "","","","","","","","","","",
  "PWM"
};

#ifdef _WIN32
bool en_vterm_proc (FILE *stream) {
  HANDLE handle = (HANDLE)_get_osfhandle(_fileno(stream));
  DWORD mode = 0;
  if (!GetConsoleMode(handle, &mode)) return false;
  if (!SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) return false;
  return true;
}
#endif

int Getc (void) {
#ifdef _WIN32
  return _getch ();
#else
  return getchar ();
#endif
}

int Putc (int ch) {
#ifdef _WIN32
  return putch (ch);
#else
  return putchar (ch);
#endif
}

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

bool YorN (void) {
  char ch;
  do {
    ch = Getc ();
    ch = toupper( ch );
  } while (ch == 0);
  return (ch == 'Y');
}

bool EditComAndBaudAndTarget (char *comport, uint32_t *baud, uint8_t *id) {
  bool result = true;
  int32_t n;
  char s[100];

  printf ("Input device name (default %s) = ", _COMPORT);
  if (InputStr (s)) strcpy (comport, s);
  else { strcpy (comport, _COMPORT); result = false; }

  printf ("Input baudrate (default %dbps) = ", _BAUDRATE);
  if (!InputNum (&n)) { n = _BAUDRATE; result = false; }
  if (n >= 9600 && n <= 4000000) *baud = n; else *baud = _BAUDRATE;

  printf ("Input target id (default %d) = ", _TARGETID);
  if (!InputNum (&n)) { n = _TARGETID; result = false; }
  if (n >= 1 && n < 0xfd) *id = n; else *id = _TARGETID;

  return result;
}

bool DeviceExists (TDeviceID dev, uint8_t id, uint32_t baud, uint32_t prevbaud) {
  bool detected = false;
  if (DX2_SetBaudrate (dev, baud)) {
    for (int i = 0; i < 5; i++) {
      if ((detected = DX2_Ping (dev, id, NULL))) break;
    }
  }
  DX2_SetBaudrate (dev, prevbaud);
  return detected;
}

void Prompt (uint8_t id, uint32_t baud) {
  time_t t = time(NULL);
  struct tm *local = localtime(&t);
  printf ("\r\33[1m%02d:%02d:%02d \33[0m\033[45mid:%d baud:%d\033[49m ", local->tm_hour, local->tm_min, local->tm_sec, id , baud);
  fflush(stdout);
}

int main (void) {
  TDeviceID  dev;
  TErrorCode err;
  int result = 0;

  char comname[20];
  uint32_t baud;
  uint8_t id, /*btyeval, */prevbyteval;
  bool blval/*, prevblbal*/;
  double dblval, dblval_flg;
  int32_t d;
//  uint8_t dum[1000];

  EditComAndBaudAndTarget (comname, &baud, &id);
  printf("\nCOM=%s, Baudrate=%d, id=%d\n", comname, baud, id);

#ifdef _WIN32
  en_vterm_proc (stdout);

  if (LoadDLL ()) {
#else
  enable_raw_mode ();
#endif

    if ((dev = DX2_OpenPort (comname, baud))) {
      Prompt (id, baud);
      while (1) {
        char ch;
        while (kbhit ()) {
          Putc (ch = Getc ());
          switch (ch) {
            case 's': // scan
              printf("\nStat scan\n");
              blval = true;
              for (int b = 0; b < 7 && blval; b++) {
                printf("\rhost baudrate:%dbps\n", baudlist[b]);
                DX2_SetBaudrate (dev, baudlist[b]);
                for (uint8_t i = 0; i <= 252 && blval; i++) {
                  printf (" ping to %d\r",i);
                  fflush (stdout);
                  if (kbhit ()) {
                    Getc ();
                    blval = false;
                  }
                  if (DX2_Ping (dev, i, NULL)) {
                    printf ("\r id:%3d, device name:%s \n", i, DXL_GetModelInfo (dev, i)->name);
                  }
                }
              }
              DX2_SetBaudrate (dev, baud);
              break;

            case 'p': // ping
              DXL_GetModelInfo (dev, id);
              if (!DXL_GetOperatingMode (dev, id, &prevbyteval)) prevbyteval = 255;
              printf("\nPing\nstat:%s, device name:%s, op mode:%s\n", DX2_Ping (dev, id, NULL) ? "ok" : "ng", DXL_GetModelInfo (dev, id)->name, (prevbyteval != 255) ? opmode[prevbyteval] : "?");
              break;

            case 'R': // factory reset device
              if (DX2_Ping (dev, id, NULL)) {
                printf("\nFactory reset device\nAre you sure ?(Y/N)");
                if (YorN ()) {
                  printf("y\n");
                  if (!DeviceExists (dev, 1, 57600, baud) || (id == 1 && baud ==57600)) {
                    if (DX2_Reset (dev, id, &err)) {
                      printf("OK\n");
                      id = 1;
                      if (DX2_SetBaudrate (dev, 57600)) baud = 57600;
                    } else
                      printf("NG: $%04x\n", err);
                  } else
                    printf("NG: That setting conflicts\n");
                } else
                  printf("n\n");
              } else
                printf("\nCannot find device\n");
              break;

            case 'b': // change host baudrate
              printf("\nChange host baud\nInput baudrate index\n (0:9600, 1:57600, 2:115200, 3:1M, 4:2M, 5:3M, 6:4M[bps]) = ");
              if (InputNum (&d)) {
                d = MAX (MIN (d, 6), 0);
                if (DX2_SetBaudrate (dev, baudlist[d])) baud = baudlist[d];
              }
              break;
            case 'B': // change device baudrate
              if (DX2_Ping (dev, id, NULL)) {
                printf("\nChange device baud\nInput baudrate index\n (0:9600, 1:57600, 2:115200, 3:1M, 4:2M, 5:3M, 6:4M[bps]) = ");
                if (InputNum (&d)) {
                  d = MAX (MIN (d, 6), 0);

                  if (!DeviceExists (dev, id, baudlist[d], baud)) {
                    if (DX2_WriteByteData (dev, id, _ADDR_BAUDRATE , d, &err)) {
                      printf("OK\n");
                      if (DX2_SetBaudrate (dev, baudlist[d])) baud = baudlist[d];
                    } else
                      printf("NG: $%04x\n",err);
                  } else
                    printf("NG: That setting conflicts\n");
                }
              } else
                printf("\nCannot find devicet\n");
              break;

            case 'i': // change host id
              printf("\nChange host id\nInput id (0..252) = ");
              if (InputNum (&d)) {
                id =  MAX (MIN (d, 252), 0);
              }
              break;
            case 'I': // change device id
              if (DX2_Ping (dev, id, NULL)) {
                printf("\nChange device id\nInput id (0..252) = ");
                if (InputNum (&d)) {
                  d = MAX (MIN (d, 252), 0);
                  if (!DeviceExists (dev, d, baud, baud)) {
                    if (DX2_WriteByteData (dev, id, _ADDR_ID , d, &err)) {
                      printf("OK\n");
                      id = d;
                    } else
                      printf("NG: $%04x\n", err);
                  } else
                    printf("NG: That setting conflicts\n");
                }
              } else
                printf("\nCannot find device\n");
              break;

            case 'O': // change device operating mode
              if (DX2_Ping (dev, id, NULL)) {
                DXL_InitDevicesList ();
                DXL_GetModelInfo (dev, id);
                if (DXL_GetOperatingMode (dev, id, &prevbyteval)) {
                  printf("\nChange device op mode (current op mode = %d:%s)\nInput id (0:cur 1:velo 3:pos 4:multiturn 5:multi+cur 16:PWM) = ", prevbyteval, opmode[prevbyteval]);
                  if (InputNum (&d)) {
                    d = MAX (MIN (d, 16), 0);
                    if (DXL_SetOperatingMode (dev, id, d)) {
                      DX2_Reboot (dev, id, NULL); // 各設定値が残るのを嫌ってリブート
                      printf("OK\n");
                    } else
                      printf("NG: $%04x\n", err);
                  }
                }
              } else
                printf("\nCannot find device\n");
              break;

            case 'E': // change device torque enable
              printf ("\n");
              if (DX2_Ping (dev, id, NULL)) {
                DXL_InitDevicesList ();
                DXL_GetModelInfo (dev, id);
                DXL_GetTorqueEnable (dev, id, &blval);
                blval = !blval;
                printf("Change device torqule %s:%s\n", blval ? "enable" : "disable", DXL_SetTorqueEnable (dev, id, blval) ? "OK" : "NG");
              } else
                printf("Cannot find device\n");
              break;

            case 'A':
              printf ("\n");
              if (DX2_Ping (dev, id, NULL)) {
                DXL_InitDevicesList ();
                DXL_GetModelInfo (dev, id);
                if (DXL_GetOperatingMode (dev, id, &prevbyteval)) {
                  if ((prevbyteval == 3) || (prevbyteval == 4) || (prevbyteval == 5)) {
                    DXL_GetPresentAngle (dev, id, &dblval);
                    dblval_flg = 1;
                    while (!kbhit ()) {
                      dblval += (dblval_flg * 0.1);
                      if (dblval > 720.0) { dblval_flg = -1; dblval = 720.0; }
                      else if (dblval < -720.0) { dblval_flg = 1; dblval = -720.0; }
                      printf ("goal angle:%6.1f[deg]\r", dblval);
                      DXL_SetGoalAngle (dev, id, dblval);
                      Sleep (1);
                    }
                    Getc ();
                    printf ("\n");
                  } else
                    printf("NG: op mode is different\n");
                }
              } else
                printf("Cannot find device\n");
              break;

            case 'V':
              printf ("\n");
              if (DX2_Ping (dev, id, NULL)) {
                DXL_InitDevicesList ();
                DXL_GetModelInfo (dev, id);
                if (DXL_GetOperatingMode (dev, id, &prevbyteval)) {
                  if (prevbyteval == 1) {
                    DXL_GetPresentVelocity (dev, id, &dblval);
                    dblval_flg = 1;
                    while (!kbhit ()) {
                      dblval += (dblval_flg * 0.1);
                      if (dblval > 720.0) { dblval_flg = -1; dblval = 720.0; }
                      else if (dblval < -720.0) { dblval_flg = 1; dblval = -720.0; }
                      printf ("goal velocity:%7.1f[deg/sec]\r",dblval);
                      DXL_SetGoalVelocity (dev, id, dblval);
                      Sleep (1);
                    }
                    Getc ();
                    printf ("\n");
                  } else
                    printf("NG: op mode is different\n");
                }
              } else
                printf("Cannot find device\n");
              break;

            case 'C':
              printf ("\n");
              if (DX2_Ping (dev, id, NULL)) {
                DXL_InitDevicesList ();
                DXL_GetModelInfo (dev, id);
                if (DXL_GetOperatingMode (dev, id, &prevbyteval)) {
                  if (prevbyteval == 0) {
                    DXL_GetPresentCurrent (dev, id, &dblval);
                    dblval_flg = 1;
                    while (!kbhit ()) {
                      dblval += (dblval_flg);
                      if (dblval > 2000.0) { dblval_flg = -1; dblval = 2000.0; }
                      else if (dblval < -2000.0) { dblval_flg = 1; dblval = -2000.0; }
                      printf (" goal current=%6.1f[mA]\r",dblval);
                      DXL_SetGoalCurrent (dev, id, dblval);
                      Sleep (1);
                    }
                    Getc ();
                    printf ("\n");
                  } else
                    printf("NG: op mode is different\n");
                }
              } else
                printf("\nCannot find device\n");
              break;

            default:
              printf("\n[s]scan [p]ping [R]factory reset device\n"
                "[b]change host baudrate [B]change device baudrate\n"
                "[i]change host id [I]change device id\n"
                "[O]change device operating mode\n"
                "[E]change device torque enable\n"
                "[A]test angle control\n"
                "[V]test velocity control\n"
                "[C]test current control\n"
              );
              break;

            case 'q':
            case 'Q':
              goto fin;
              break;
          }
          Prompt (id, baud);
        }
        Sleep (100);
      }
fin:
      DX2_ClosePort (dev);

    } else {
      printf ("Failed to open %s\n", comname);
      result = 1;
    }
#ifdef _WIN32
    UnloadDLL();
  }
#else
  disable_raw_mode ();
#endif

  printf ("\nFin\n");
  return result;
}
