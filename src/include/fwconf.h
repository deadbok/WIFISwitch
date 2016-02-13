/** @file fwconf.h
 *
 * @brief Hardwired firmware configuration.
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
#ifndef FWCONF_H
#define FWCONF_H

//Project name.
//#define PROJECT_NAME "wifiswitch"

/************************ Version ************************/
/**
 * @brief Firmware version.
 */
#ifndef PROJECT_VERSION
	#define PROJECT_VERSION "unknown"
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
 * @brief Fallback password for the configuration AP.
 */
#define SOFTAP_PASSWORD "0123456789"

/**
 * @brief How often to check the status in milliseconds.
 */
#define CHECK_TIME 5000

/**
 * @brief WebSocket connection time out in milliseconds.
 */
#define WS_WIFISWITCH_TIMEOUT 240000

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
	#define BAUD_RATE 115200
#endif

/************************ Flash configuration. ************************/
/**
 * @brief Signature to tell if the configuration data sound.
 */
//#define ESP_CONFIG_SIG 0xCF60BEEF

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
 * @brief Tell some files to use the ESP8266 part of their code.
 */
#define DB_ESP8266 1

#endif //FWCONF_H
