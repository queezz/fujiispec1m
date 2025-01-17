#ifndef MY_NAME
#define MY_NAME  "dx2lib"
#endif

#ifndef MESSAGE_OUT
#ifdef __DEBUG__
 #define MESSAGE_OUT(fmt,...)  {\
   printf(fmt,##__VA_ARGS__);\
}
 #define MESSAGE_OUT_D(name,fmt,...)  MESSAGE_OUT(fmt,##__VA_ARGS__)
 #define MESSAGE_OUT_I(name,fmt,...)  MESSAGE_OUT(fmt,##__VA_ARGS__)
 #define MESSAGE_OUT_W(name,fmt,...)  MESSAGE_OUT(fmt,##__VA_ARGS__)
 #define MESSAGE_OUT_E(name,fmt,...)  MESSAGE_OUT(fmt,##__VA_ARGS__)
#else
 #ifdef _WIN32
  #define MESSAGE_OUT(lv,name,fmt,...)  {\
    char *msg = malloc (10000);\
    HANDLE _lcevh = RegisterEventSource (NULL, name);\
    sprintf (msg, fmt, ##__VA_ARGS__);\
    ReportEvent (_lcevh, lv, 0, 0, NULL, 1, 0, (const char **)&msg, NULL);\
    DeregisterEventSource(_lcevh);\
    free (msg);\
}
  #define MESSAGE_OUT_D(name,fmt,...)
  #define MESSAGE_OUT_I(name,fmt,...)  MESSAGE_OUT(EVENTLOG_INFORMATION_TYPE,name,fmt,##__VA_ARGS__)
  #define MESSAGE_OUT_W(name,fmt,...)  MESSAGE_OUT(EVENTLOG_WARNING_TYPE,name,fmt,##__VA_ARGS__)
  #define MESSAGE_OUT_E(name,fmt,...)  MESSAGE_OUT(EVENTLOG_ERROR_TYPE,name,fmt,##__VA_ARGS__)
 #else
  #define MESSAGE_OUT(lv,name,fmt,...)  {\
    openlog (name, 0, LOG_LOCAL0);\
    syslog (lv, fmt, ##__VA_ARGS__);\
    closelog ();\
}
  #define MESSAGE_OUT_D(name,fmt,...)
  #define MESSAGE_OUT_I(name,fmt,...)  MESSAGE_OUT(LOG_INFO,name,fmt,##__VA_ARGS__)
  #define MESSAGE_OUT_W(name,fmt,...)  MESSAGE_OUT(LOG_WAT,name,fmt,##__VA_ARGS__)
  #define MESSAGE_OUT_E(name,fmt,...)  MESSAGE_OUT(LOG_ERR,name,fmt,##__VA_ARGS__)
 #endif
#endif

#define DEBUG_OUT(fmt,...)    MESSAGE_OUT_D(MY_NAME,fmt,##__VA_ARGS__)
#define INFO_OUT(fmt,...)     MESSAGE_OUT_I(MY_NAME,fmt,##__VA_ARGS__)
#define WARNING_OUT(fmt,...)  MESSAGE_OUT_W(MY_NAME,fmt,##__VA_ARGS__)
#define ERROR_OUT(fmt,...)    MESSAGE_OUT_E(MY_NAME,fmt,##__VA_ARGS__)

#endif

