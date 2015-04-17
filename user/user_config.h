#ifndef __USER_CONFIG_H
#define __USER_CONFIG_H

#define SSID "esp8266"
#define SSID_PASSWORD "1234567890"

#define DEBUG

#ifdef DEBUG
#include "osapi.h"
#define debug(...)     os_printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#endif
