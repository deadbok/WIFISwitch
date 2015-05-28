#ifndef USER_CONFIG_H
#define USER_CONFIG_H

//Version string
#define STRING_VERSION "0.0.1"

//Values for default AP to try
#define SSID "default"
#define SSID_PASSWORD "password"

//Password for the configuration AP
#define SOFTAP_PASSWORD "0123456789"

//Try to connect for X seconds.
#define CONNECT_DELAY_SEC   30

#define HLT os_printf("Halt execution.\n");\
            while(1);

#define error(...)     os_printf("ERROR: " __VA_ARGS__ )

//Print warnings on the serial port.
#define WARNINGS
//Print extra debug info on the serial port
#define DEBUG
#define DEBUG_MEM

//Macro for debugging. Prints the message if warnings is enabled.
#ifdef WARNIGS
#include "tools/missing_dec.h"
#include "osapi.h"
#define warn(...)     os_printf("WARN: " __VA_ARGS__)
#else
#define warn(...)
#endif

//Macro for debugging. Prints the messag if debugging is enabled.
#ifdef DEBUG
#include "tools/missing_dec.h"
#include "osapi.h"
#define debug(...)     os_printf(__VA_ARGS__)
#else
#define debug(...)
#endif

//Debug memory de-/allocation if enabled.
#ifdef DEBUG_MEM
#include "mem.h"

#define DBG_MEM_MAX_INFOS	500
#define db_malloc(ARG, INFO)  db_alloc(ARG, false, INFO)
#define db_zalloc(ARG, INFO)  db_alloc(ARG, true, INFO)
#define db_free(ARG)    db_dealloc(ARG)             

extern void *db_alloc(size_t size, bool zero, char *info);
extern void db_dealloc(void *ptr);
                        
#else
#define db_malloc(ARG, INFO)      os_malloc(ARG)
#define db_free(ARG)        os_free(ARG)
#define db_zalloc(ARG, INFO)      os_zalloc(ARG)
#endif

#endif //USER_CONFIG_H
