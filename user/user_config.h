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

//Print extra debug info on the serial port
#define DEBUG
#define DEBUG_MEM

//Macro for debugging. Prints the messag if debugging is enabled.
#ifdef DEBUG
#include "tools/missing_dec.h"
#include "osapi.h"
#define debug(...)     os_printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#ifdef DEBUG_MEM
#include "user_interface.h"
#include "mem.h"
int n_alloc;

#define DEBUG_ALLOC     n_alloc++;\
                        os_printf("Allocs: %d.\n", n_alloc);
#define DEBUG_DEALLOC   n_alloc--;\
                        os_printf("Allocs: %d.\n", n_alloc);
#define DEBUG_HEAP_SIZE(ARG) os_printf("Free heap (" ARG "): %d.\n", system_get_free_heap_size())
#define db_malloc(ARG)  os_malloc(ARG);\
                        DEBUG_ALLOC\
                        DEBUG_HEAP_SIZE("malloc")
                        
#define db_free(ARG)    os_free(ARG);\
                        DEBUG_DEALLOC\
                        DEBUG_HEAP_SIZE("free")
                        
#define db_zalloc(ARG)  os_zalloc(ARG);\
                        DEBUG_ALLOC\
                        DEBUG_HEAP_SIZE("zalloc")
#else
#define db_malloc      os_malloc
#define db_free        os_free
#define db_zalloc      os_zalloc
#endif

#endif //USER_CONFIG_H
