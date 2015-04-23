#ifndef __USER_CONFIG_H
#define __USER_CONFIG_H

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

//Macro for debugging. Prints the messag if debugging is enabled.
#ifdef DEBUG
#include "osapi.h"
#define debug(...)     os_printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#endif
