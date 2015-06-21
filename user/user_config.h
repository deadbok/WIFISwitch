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

#include "mem.h"
#include "tools/missing_dec.h"
#include "osapi.h"
#include "user_interface.h"
#include "sm_types.h"

/**
 * @brief Firmware version.
 */
#define STRING_VERSION "0.0.2"

/**
 * @brief Name of the default network to try and connect to.
 */
#define SSID "default"
/**
 * @brief Password for the default network.
 */
#define SSID_PASSWORD "password"

/**
 * @brief Password for the configuration AP.
 */
#define SOFTAP_PASSWORD "0123456789"

/**
 * @brief Time between event dispatches.
 */
#define DISPATCH_TIME 100
/**
 * @brief Network connection time-out.
 */
#define CONNECT_DELAY_SEC   10

/***
 * @brief Mostly shut off output from the SDK.
 */
#define SDK_DEBUG

/**
 * @brief Customised print function.
 * 
 * This is an ugly but simple way, to shut up most of the ESP8266 SDK output.
 * Simple only turn the os_printf function on, when we are using it.
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

/**
 * @brief Print warnings on the serial port.
 */
#define WARNINGS
/**
 * @brief Print debug info on the serial port.
 */
#define DEBUG
/**
 * @brief Print memory allocation info.
 */
#define DEBUG_MEM
/**
 * @brief List memory allocations.
 */
//#define DEBUG_MEM_LIST

//Macro for debugging. Prints the message if warnings is enabled.
#ifdef WARNINGS
#define warn(...)     db_printf("WARN: " __VA_ARGS__)
#else
#define warn(...)
#endif

//Macro for debugging. Prints the messag if debugging is enabled.
#ifdef DEBUG
#define debug(...)     db_printf(__VA_ARGS__)
#else
#define debug(...)
#endif

//Debug memory de-/allocation if enabled.
#ifdef DEBUG_MEM
/**
 * @brief Maximum number of memory blocks to keep track of in debug mode.
 */
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

extern state_t state;

extern void db_mem_list(void);
extern void handle_events(void);

#endif //USER_CONFIG_H
