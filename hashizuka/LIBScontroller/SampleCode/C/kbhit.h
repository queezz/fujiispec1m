/*
  kbhit.h
   Linux/macOS上でエコー無しの直接入力とkbhitを提供
   Windowsでは使用不可
*/

#ifndef _KBHIT_H_INCLUDE
#define _KBHIT_H_INCLUDE

#ifdef _WIN32
#error Not support Windows
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <sys/ioctl.h>
#include <termios.h>

void enable_raw_mode (void) {
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
    tcsetattr(0, TCSANOW, &term);
}

void disable_raw_mode (void) {
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &term);
}

bool kbhit (void) {
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    return byteswaiting > 0;
}

void tty_flush (void) {
    tcflush(0, TCIFLUSH);
}


#ifdef __cplusplus
}
#endif

#endif //_KBHIT_H_INCLUDE
