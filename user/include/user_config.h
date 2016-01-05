/** @file user_config.h
 *
 * @brief Hard wired firmware configuration.
 * 
 * @copyright
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
 * 
 * @license
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */ 
#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include <stdint.h>
/**
 * @brief SDK GPIO API needs this.
 */
#define uint32 uint32_t

#include <mem.h>
#include <gpio.h>
#include <osapi.h>
#include <user_interface.h>
#include "tools/missing_dec.h"
#include "config/config.h"

/************************ Version ************************/
/**
 * @brief Firmware version.
 */
#ifndef VERSION
	#define VERSION "unknown"
#endif

#ifndef GIT_VERSION
	#define GIT_VERSION "none"
#endif

/************************ Network ************************/
/**
 * @brief Network WIFI connection time-out.
 * 
 * Seconds before the firmware stops trying to connect to the configured WIFI
 * network, and switches to network configuration mode.
 */
#define CONNECT_DELAY_SEC 10

/**
 * @brief Password for the configuration AP.
 */
#define SOFTAP_PASSWORD "0123456789"

/**
 * @brief How often to check the status in milliseconds.
 */
#define CHECK_TIME 5000

/**
 * @brief WebSocket connection time out in milliseconds.
 */
#define WS_WIFISWITCH_TIMEOUT 30000

/************************ Physical pin functions. ************************/
/**
 * @brief Multiplexer pin name for the switch button.
 */
#define SWITCH_KEY_NAME PERIPHS_IO_MUX_GPIO4_U
/**
 * @brief Multiplexer pin function for the switch button.
 */
#define SWITCH_KEY_FUNC FUNC_GPIO4
/**
 * @brief Multiplexer GPIO number for the switch button.
 */
#define SWITCH_KEY_NUM 4

/**
 * @brief Multiplexer pin name for the relay.
 */
#define RELAY_NAME PERIPHS_IO_MUX_GPIO5_U
/**
 * @brief Multiplexer pin function for the relay.
 */
#define RELAY_FUNC FUNC_GPIO05


/**
 * @brief Bit mask of which GPIO pins the REST interface can control.
 */
#define REST_GPIO_ENABLED 0x30
/**
 * @brief Bit mask of which GPIO pins the WebSocket API can control.
 */
#define WS_WIFISWITCH_GPIO_ENABLED 0x30

/**
 * @brief Baud rate of the serial console.
 */
#ifndef BAUD_RATE
	#define BAUD_RATE 230400
#endif

/**
 * @brief Address in flash of configuration data.
 * 
 * *Default value for a 512kb Flash.*
 */
#ifndef CONFIG_FLASH_ADDR
	#define CONFIG_FLASH_ADDR 0x3c
#endif

/* Enable any of the following to have different levels of debug output on the
 * serial port. 
 */
/**
 * @brief Print warnings on the serial port.
 */
#define WARNINGS
/**
 * @brief Print debug info on the serial port.
 */
//#define DEBUG

#ifdef DEBUG
/**
 * @brief Print memory allocation info.
 */
#define DEBUG_MEM
#endif
/**
 * @brief List memory allocations.
 */
//#define DEBUG_MEM_LIST
/**
 * @brief Mostly shut up output from the SDK.
 */
//#define SDK_DEBUG


/************************ Config values end ***************************/

/**
 * @brief Customised print function.
 * 
 * If #SDK_DEBUG is TRUE, the regular os_printf function stops doing anything,
 * but ets_prinf still does it's job.
 */
#ifdef SDK_DEBUG
#define db_printf(...) 	os_printf(__VA_ARGS__)
#else
#define db_printf(...) 	ets_printf(__VA_ARGS__)
#endif

/**
 * @brief Print an error message.
 */
#define error(...)     db_printf("ERROR: " __VA_ARGS__ )

//Macro for debugging. Prints the message if warnings are enabled.
#ifdef WARNINGS
#define warn(...)     db_printf("WARN: " __VA_ARGS__)
#else
#define warn(...)
#endif

//Macro for debugging. Prints the message if debugging is enabled.
#ifdef DEBUG
#define debug(...)     db_printf(__VA_ARGS__)

//Hexdump some memory.
extern void db_hexdump(void *mem, unsigned int len);
#else
#define debug(...)
#define db_hexdump(mem, len)
#endif

//Debug memory de-/allocation if enabled.
#ifdef DEBUG_MEM
/**
 * @brief Maximum number of memory blocks to keep track of in debug mode.
 */
#define DBG_MEM_MAX_INFOS	200
#define db_malloc(ARG, INFO)  db_alloc(ARG, false, INFO)
#define db_zalloc(ARG, INFO)  db_alloc(ARG, true, INFO)
#define db_realloc(PTR, SIZE, INFO)  db_realloc(PTR, SIZE, INFO)
#define db_free(ARG)    db_dealloc(ARG)             

extern void *db_alloc(size_t size, bool zero, char *info);
extern void *db_realloc(void *ptr, size_t size, char *info);
extern void db_dealloc(void *ptr);
extern void db_mem_list(void);
                        
#else
#define db_malloc(ARG, INFO) os_malloc(ARG)
#define db_realloc(PTR, SIZE, INFO) os_realloc(PTR, SIZE)
#define db_free(ARG) os_free(ARG)
#define db_zalloc(ARG, INFO) os_zalloc(ARG)
#define db_mem_list(ARG)
#endif

/**
 * @brief Firmware configuration, loaded from flash.
 */
extern struct config *cfg;

/**
 * @brief Signal to de a system reset.
 */
extern os_signal_t signal_reset;

#define IRAM_ATTR __attribute__((section(".iram.text")))

#endif //USER_CONFIG_H
