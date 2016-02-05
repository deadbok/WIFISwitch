
/**
 * @file wifiswitch.c
 *
 * @brief Websocket wifiswitch protocol code.
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
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include "user_config.h"
#include "tools/strxtra.h"
#include "tools/jsmn.h"
#include "tools/json-gen.h"
#include "net/wifi.h"
#include "net/websocket.h"
#include "handlers/websocket/wifiswitch.h"

#define WS_PR_WIFISWITCH "wifiswitch"

/**
 * @brief The handler id the protocol is registered with.
 */
static ws_handler_id_t ws_wifiswitch_hid;

struct ws_handler ws_wifiswitch_handler = { WS_PR_WIFISWITCH, NULL, ws_wifiswitch_received, NULL, ws_wifiswitch_close, NULL, NULL};

/**
 * @brief Connection waiting for a list of ssids.
 */
static struct net_connection *response_connection = NULL;

bool ws_register_wifiswitch(void)
{
	debug("Registering wifiswitch WebSocket protocol handler.\n");
	ws_wifiswitch_hid = ws_register_handler(ws_wifiswitch_handler);
	if (ws_wifiswitch_hid < 0)
	{
		return(false);
	}
	return(true);
}

/**
 * @brief Add data to a message.
 *
 * @param msg Message to add to, created if NULL.
 * @param name Name of the value to add.
 * @param value Value.
 * @param quotes Add double quotes to the value.
 * @return The new message.
 */
static char *ws_wifiwsitch_add_data(char *msg, char *name, char *value, bool quotes)
{
	char *pair;
	
	debug("Adding WebSocket wifiswitch value %s	= %s to %s.\n", name, value, msg); 
	pair = json_create_pair(name, value, quotes);
	msg = json_add_to_object(msg, pair);
	db_free(pair);	
	
	return(msg);
}

/**
 * @brief Create response to a fw type request.
 * 
 * type: "fw".
 * -----------
 *
 * |  Name | Description | Values | Access |
 * | :---: | :---------- | :----: | :----: |
 * | mode | Network mode | "station" or "ap" | R/W |
 * | ver | Firmware version | "x.y.z" |  RO |
 * 
 * @return Pointer to JSON response.
 */
static char *ws_wifiswitch_fw_response(void)
{
	char *json_response = NULL;

	char *mode;
	
	debug("Creating JSON gpio response.\n");
	
	//Create type object.
	json_response = ws_wifiwsitch_add_data(json_response, "type", "fw", true);
	if (cfg->network_mode == WIFI_MODE_CLIENT)
	{
		mode = "\"station\"";
	}
	else
	{
		mode = "\"ap\"";
	}
	json_response = ws_wifiwsitch_add_data(json_response, "mode", mode, false);
	json_response = ws_wifiwsitch_add_data(json_response, "ver", "\"" VERSION "\"", false);	
	return(json_response);
}

/**
 * @brief Create response to a networks type request.
 * 
 * type: "networks".
 * ----------------
 * 
 * |  Name | Description | Values | Access |
 * | :---: | :---------- | :----: | :----: |
 * | ssids | Accessible networks | Array of ssids | R |
 * 
 * @param arg Pointer to a ESP8266 scaninfo struct, with an AP list.
 * @param status ESP8266 enum STATUS, telling how the scan went.
 */
static void ws_wifiswitch_networks_response(void *arg, STATUS status)
{
	struct bss_info *current_ap_info;
	struct bss_info *scn_info;
	size_t ssid_size;
	size_t size;
	char *pair;
	char *ssid_array = NULL;
	char *json_ssid;
	char *json_ssid_pos = NULL;
	char *json_response = NULL;
	
	debug("Creating networks message.\n");
	if (status == OK)
	{
		debug(" Scanning went OK (%p).\n", arg);
		
		//Create initial part of JSON response.
		json_response = ws_wifiwsitch_add_data(json_response, "type", "networks", true);
		
		scn_info = (struct bss_info *)arg;
		debug(" Processing AP list (%p, %p).\n", scn_info, scn_info->next.stqe_next);
		debug(" AP names:\n");
		//Skip first according to docs.
		for (current_ap_info = scn_info->next.stqe_next;
			 current_ap_info != NULL;
			 current_ap_info = current_ap_info->next.stqe_next)
		{
			debug("  %s.\n", current_ap_info->ssid);
			//SSID cannot be longer than 32 char.
			ssid_size = strlen((char *)(current_ap_info->ssid));
			if (ssid_size > 32)
			{
				warn("SSID to long.\n");
				//\" + \" + \0, limit size.
				size = 35;
			}
			else
			{
				//\" + \" + \0
				size = ssid_size + 3;
			}
			json_ssid = db_malloc(size, "scan_done_sb json_ssid");
			json_ssid_pos = json_ssid;
			*json_ssid_pos++ = '\"';
			if (ssid_size > 32)
			{
				os_memcpy(json_ssid_pos, current_ap_info->ssid, 32);
				json_ssid_pos += 32;
			}
			else
			{
				os_memcpy(json_ssid_pos, current_ap_info->ssid, ssid_size);
				json_ssid_pos += ssid_size;
			}
			os_memcpy(json_ssid_pos, "\"\0", 2);
			ssid_array = json_add_to_array(ssid_array, json_ssid);
			db_free(json_ssid);
		}
		if (ssid_array)
		{
			pair = json_create_pair("ssids", ssid_array, false);
		}
		else
		{
			pair = json_create_pair("ssids", "", true);
		}
		json_response = json_add_to_object(json_response, pair);
		db_free(pair);	
		ws_send_text(json_response, response_connection);
		response_connection = NULL;
		db_free(json_response);
	}
	else
	{
		error("Scanning AP's.\n");
	}
}

/**
 * @brief Start scanning for WIFI networks.
 * 
 * @return NULL.
 */
static char *ws_wifiswitch_networks_scan(void)
{
	debug("Start network names scan.\n");
	
	debug(" Starting scan.\n");
	if (wifi_station_scan(NULL, &ws_wifiswitch_networks_response))
	{
		debug(" Scanning for AP's.\n");
	}
	else
	{
		error(" Could not scan AP's.\n");
	}	
	return(NULL);
}

/**
 * @brief Add hostname to message.
 * 
 * @param msg Message to add to or NULL to create a new one.
 * @return Pointer to new message.
 */
static char *ws_wifiswitch_add_hostname(char *msg)
{
	char *value = NULL;
	
	value = wifi_station_get_hostname();
	if (!value)
	{
		warn("Could net get hostname.\n");
		return(NULL);
	}	
	msg = ws_wifiwsitch_add_data(msg, "hostname", value, true);
	
	return(msg);
}

/**
 * @brief Add IP address to message.
 * 
 * @param msg Message to add to or NULL to create new.
 * @param if_idx AP/station interface, SOFTAP_IF/STATION_IF.
 * @return New message.
 */
static char *ws_wifiswitch_add_ip(char *msg, uint8_t if_idx) 
{
	char *value;
	struct ip_info ip;
	
	if (!wifi_get_ip_info(if_idx, &ip))
	{
		warn("Could get IP info.\n");
		return(NULL);
	}
	value = db_zalloc(13, "ws_wifiswitch_add_ip value");
	os_sprintf(value, IPSTR, IP2STR(&ip.ip));
	msg = ws_wifiwsitch_add_data(msg, "ip", value, true);
	db_free(value);	
	
	return(msg);
}

/**
 * @brief Create response to a station type request.
 * 
 * type: "station".
 * ----------------
 * 
 * |  Name | Description | Values | Access |
 * | :---: | :---------- | :----: | :----: |
 * | ssid | Network name | "name" | R/W |
 * | passwd | Network password | "password" | W |
 * | hostname | Switch hostname | hostname | R/W |
 * | ip | IP address | ip | RO |
 */
static char *ws_wifiswitch_station_response(void)
{
    struct station_config st_config;
	char *json_response = NULL;
	size_t size;
	char *value;
	
	debug("Creating JSON station response.\n");
	
	//Create type object.
	json_response = ws_wifiwsitch_add_data(json_response, "type", "station", true);

    //Get station SSID from flash.
    if (!wifi_station_get_config_default(&st_config))
    {
        error("Cannot get default station configuration.\n");
        return(NULL);
	}
	size = strnlen((char *)st_config.ssid, 32); 
	//Wastes one byte if \0 character is already in ssid.
	value = db_malloc(size + 1, "ws_wifiswitch_station_response value");
	os_memcpy(value, st_config.ssid, size);
	value[size] = '\0';

	json_response = ws_wifiwsitch_add_data(json_response, "ssid", value, true);
	db_free(value);
	
	json_response = ws_wifiswitch_add_hostname(json_response);
	if (!json_response)
	{
		warn("Could net get hostname.\n");
		return(NULL);
	}
	
	//IP.
	json_response = ws_wifiswitch_add_ip(json_response, STATION_IF);
	
	return(json_response);
}

/**
 * @brief Create response to a ap type request.
 * 
 * type: "ap".
 * -----------
 *
 * |  Name | Description | Values | Access |
 * | :---: | :---------- | :----: | :----: |
 * | ssid | Network name | "name" | R/W |
 * | passwd | Network password | "password" | W |
 * | channel | Network channel | channel | R/W |
 * | hostname | Switch hostname | hostname | R/W |
 * | ip | IP address | ip | RO | 
 */
static char *ws_wifiswitch_ap_response(void)
{
    struct softap_config ap_config;
	char *json_response = NULL;
	size_t size;
	char *value;
	
	debug("Creating JSON ap response.\n");
	
	//Create type object.
	json_response = ws_wifiwsitch_add_data(json_response, "type", "ap", true);

    //Get ap SSID from flash.
    if (!wifi_softap_get_config_default(&ap_config))
    {
        error("Cannot get default ap configuration.\n");
        return(NULL);
	}
	size = strnlen((char *)ap_config.ssid, 32); 
	//Wastes one byte if \0 character is already in ssid.
	value = db_malloc(size + 1, "ws_wifiswitch_ap_response value");
	os_memcpy(value, ap_config.ssid, size);
	value[size] = '\0';
	json_response = ws_wifiwsitch_add_data(json_response, "ssid", value, true);
	db_free(value);
	
	//Channel.
	//Two digits and zero byte.
	value = db_malloc(3, "ws_wifiswitch_ap_response value");
	itoa(ap_config.channel, value, 10);
	json_response = ws_wifiwsitch_add_data(json_response, "channel", value, true);
	db_free(value);
	
	//Hostname.
	json_response = ws_wifiswitch_add_hostname(json_response);
	if (!json_response)
	{
		warn("Could net get hostname.\n");
		return(NULL);
	}
	
	//IP.
	json_response = ws_wifiswitch_add_ip(json_response, SOFTAP_IF);	
	
	return(json_response);
}

/**
 * @brief Create response to a gpio type request.
 * 
 * @return Pointer to JSON response.
 */
static char *ws_wifiswitch_gpio_response(void)
{
	char json_value[3];
	char *json_response = NULL;
	char *pair;
	char *json_gpio_en = NULL;
	
	debug("Creating JSON gpio response.\n");
	
	//Create type object.
	json_response = ws_wifiwsitch_add_data(json_response, "type", "gpio", true);
	//Run through all GPIO's.
	for (unsigned int i = 0; i < WS_WIFISWITCH_GPIO_PINS; i++)
	{
		if (((WS_WIFISWITCH_GPIO_ENABLED >> i) & 1) == 1)
		{
			//Create an array of the enabled ones.
			debug(" GPIO%d is enabled.\n", i);
			if (!itoa(i, json_value, 10))
			{
				warn("Could not convert GPIO pin number to string.\n");
			}
			//Starts of with json_gpio_en = NULL, to create a new array.
			json_gpio_en = json_add_to_array(json_gpio_en, json_value);
		}
	}
	json_response = ws_wifiwsitch_add_data(json_response, "gpios", json_gpio_en, false);
	db_free(json_gpio_en);
	//Run through all GPIO's, again. Ended up deciding it was the nicest way.
	for (unsigned int i = 0; i < WS_WIFISWITCH_GPIO_PINS; i++)
	{
		if (((WS_WIFISWITCH_GPIO_ENABLED >> i) & 1) == 1)
		{
			//Create an array of the enabled ones.
			debug(" GPIO%d is enabled.\n", i);
			if (!itoa(i, json_value, 10))
			{
				warn("Could not convert GPIO pin number to string.\n");
			}
			//Create an array of objects with GPIO state of enabled pins.
			if (GPIO_INPUT_GET(i))
			{
				debug(" GPIO is on.\n");
				pair = json_create_pair(json_value, "1", false);
			}
			else
			{
				debug(" GPIO is off.\n");
				pair = json_create_pair(json_value, "0", false);
			}				
			json_response = json_add_to_object(json_response, pair);
			db_free(pair);
			
		}
	}
	return(json_response);
}

/**
 * @brief Parse a firmware request.
 * 
 * @param request Raw request string.
 * @param parser Pointer to a jsmn JSON parser.
 * @param token Pointer to jsmn
 */
static void ws_wifiswitch_fw_parse(char *request, jsmn_parser *parser, jsmntok_t *tokens, int n_tokens)
{
	int i;
	uint16_t value;
	char *token;

	debug("Parsing wifiswitch fw request.\n");	
	//Assumes the top level token is an object.
	for (i = 1; i < n_tokens; i++)
	{
		debug(" JSON token %d.\n", i);
		token = request + tokens[i].start;
		if (tokens[i].type == JSMN_STRING)
		{
			debug(" JSON token is a string (%s).\n", token);
			/* The first 2 bytes of type are unique by using them as an
			 * 16 bit uint, we save a string comparison.
			 * The only R/W property is "mode"
			 */
			value = (request[tokens[i].start] << 8 | request[tokens[i].start + 1]);
			debug("First bytes 0x%.2x.\n", value);
			if ( value== 0x6d6f)
			{
				debug(" Mode token.\n");
				//Next token is supposed to be the value.
				i++;
				//Convert first to characters to int.
				value = request[tokens[i].start] << 8 |
						request[tokens[i].start + 1];
				switch (value)
				{
					//station
					case 0x7374:
						debug(" Mode: station.\n");
						cfg->network_mode = WIFI_MODE_CLIENT;
						write_cfg_flash(*cfg);
						break;
					//ap
					case 0x6170:
						debug(" Mode: AP.\n");
						cfg->network_mode = WIFI_MODE_AP;
						write_cfg_flash(*cfg);	
						break;
					default:
						warn("Wrong mode value in firmware message.\n");
				}
			}
		}
		else
		{
			debug(" Unexpected token.\n");
		}
	}
}

/**
 * @brief Parse a station request.
 * 
 * @param request Raw request string.
 * @param parser Pointer to a jsmn JSON parser.
 * @param token Pointer to jsmn
 */
static void ws_wifiswitch_station_parse(char *request, jsmn_parser *parser, jsmntok_t *tokens, int n_tokens)
{
	struct station_config st_config;
	uint16_t value;
	char *token;
	size_t size;
	bool ret;
	int i;

	debug("Parsing wifiswitch station request.\n");	
	//Get a pointer the configuration data.
	ret = wifi_station_get_config_default(&st_config);
	if (!ret)
	{
		error("Cannot get station configuration.");
		return;
	}
	//Assumes the top level token is an object.
	for (i = 1; i < n_tokens; i++)
	{
		debug(" JSON token %d.\n", i);
		token = request + tokens[i].start;
		if (tokens[i].type == JSMN_STRING)
		{
			debug(" JSON token is a string (%s).\n", token);
			/* The first 2 bytes of type are unique by using them as an
			 * 16 bit uint, we save a string comparison.
			 */
			value = (request[tokens[i].start] << 8 | request[tokens[i].start + 1]);
			debug("First bytes 0x%.2x.\n", value);
			if ( value== 0x7373)
			{
				debug(" SSID token.\n");
				//Next token is supposed to be the SSID.
				i++;
				
				size = tokens[i].end - tokens[i].start;
				if (size > 32)
				{
					size = 32;
				}
				os_memcpy(st_config.ssid, request + tokens[i].start, size);
			}
			if ( value== 0x7061)
			{
				debug(" password token.\n");
				//Next token is supposed to be the password.
				i++;
				
				size = tokens[i].end - tokens[i].start;
				if (size > 64)
				{
					size = 64;
				}
				os_memcpy(st_config.password, request + tokens[i].start, size);
			}
			if ( value== 0x686f)
			{
				debug(" hostname token.\n");
				//Next token is supposed to be the hostname.
				i++;
				
				size = tokens[i].end - tokens[i].start;
				if (size > 32)
				{
					size = 32;
				}
				os_memset(cfg->hostname, 0, 33);
				os_memcpy(cfg->hostname, request + tokens[i].start, size);
				debug(" Setting hostname: %s-.\n", cfg->hostname);
				write_cfg_flash(*cfg);

				ret = wifi_station_set_hostname((char *)cfg->hostname);
				if (!ret)
				{
					error("Failed to set host name.");
					return;
				}
			}
		}
		else
		{
			debug(" Unexpected token.\n");
		}
	}
	
	//Set connections config.
	ret = wifi_station_set_config(&st_config);
	if (!ret)
	{
		error("Failed to set station configuration.");
		return;
	}
}

static void ws_wifiswitch_gpio_parse(char *request, jsmn_parser *parser, jsmntok_t *tokens, int n_tokens)
{
	int i;
	char gpio = -1;
	char *token;
	char *token_end;

	debug("Parsing wifiswitch gpio request.\n");	
	//Assumes the top level token is an object.
	for (i = 1; i < n_tokens; i++)
	{
		debug(" JSON token %d.\n", i);
		token = request + tokens[i].start;
		//Assumes GPIO state messages.
		if (tokens[i].type == JSMN_STRING)
		{
			token_end = token;
			debug(" JSON token is a string \"%s\".\n", token);
			gpio = strtol(token, &token_end, 10);
			if (token == token_end)
			{
				debug(" Unexpected token.\n");
				gpio = -1;
			}
		}
		if (tokens[i].type == JSMN_PRIMITIVE)
		{
			debug(" JSON token is a primitive.\n");
			if (gpio > -1)
			{
				debug(" Have GPIO%d, setting state %c.\n", gpio, *token);
				if ((*token == '0') || (*token == '1'))
				{
					GPIO_OUTPUT_SET(gpio, *token - 48);
				}
				else
				{
					warn("Unsupported GPIO state %c.\n", *token);
				}
				gpio = -1;
				
			}
		}
	}
}

signed long int ws_wifiswitch_received(struct ws_frame *frame, struct net_connection *connection)
{
	uint32_t value;
	char *response = NULL;
	signed long int ret = 0;
	jsmn_parser parser;
	jsmntok_t tokens[10];
	int n_tokens;
	
	debug("Wifiswitch WebSocket data received.\n");
	//Set timeout.
	connection->timeout = WS_WIFISWITCH_TIMEOUT;
	db_hexdump(frame->data, frame->payload_len);
	if (frame->opcode != WS_OPCODE_TEXT)
	{
		error(" I only understand text data.\n");
		return(WS_ERROR);
	}
	
	jsmn_init(&parser);
	n_tokens  = jsmn_parse(&parser, frame->data, frame->payload_len, tokens, 10);
	
	debug(" %d JSON tokens received.\n", n_tokens);
	if ( (n_tokens < 3) || tokens[0].type != JSMN_OBJECT)
	{
		warn("Could not parse JSON request.\n");
		return(WS_ERROR);
	}
	//Assumes the top level token is an object.
	for (unsigned int i = 1; i < n_tokens; i++)
	{
		debug(" JSON token %d.\n", i);
		if (tokens[i].type == JSMN_STRING)
		{
			value = frame->data[tokens[i].start] << 24 |
					frame->data[tokens[i].start + 1] << 16 |
					frame->data[tokens[i].start + 2] << 8 |
					frame->data[tokens[i].start + 3];
			debug(" JSON token start with a string (0x%.4x).\n", value);
			//0x74797065 is ASCII values for "type".
			if (value == 0x74797065)
			{
				i++;
				if (tokens[i].type == JSMN_STRING)
				{
					debug(" JSON string comes next.\n");			
					/* The first 2 bytes of type are unique by using them as an
					 * 16 bit uint, we save a string comparison.
					 */
		 			value = frame->data[tokens[i].start] << 8 |
							frame->data[tokens[i].start + 1];
					switch (value)
					{
						case 0x6677:
							debug(" fw request.\n");
							if (n_tokens > 3)
							{
								debug(" Full fw message.\n");
								ws_wifiswitch_fw_parse(frame->data, &parser, tokens, n_tokens);
							}
							response = ws_wifiswitch_fw_response();
							break;
						case 0x6e65:
							debug(" network request.\n");
							if (response_connection)
							{
								warn("Scan waiting.\n");
								return(0);
							}
							response_connection = connection;
							ws_wifiswitch_networks_scan();
							break;
						case 0x7374:
							debug(" station request.\n");
							if (n_tokens > 3)
							{
								debug(" Full station message.\n");
								ws_wifiswitch_station_parse(frame->data, &parser, tokens, n_tokens);
							}
							response = ws_wifiswitch_station_response();
							break;
						case 0x6170:
							debug(" ap request.\n");
							if (n_tokens > 3)
							{
								debug(" Full ap message.\n");
							}
							response = ws_wifiswitch_ap_response();
							break;
						case 0x6770:
							debug(" gpio message.\n");
							//If there are only three tokens, the only valid request is one with only type.
							if (n_tokens > 3)
							{
								debug(" Full GPIO request.\n");
								ws_wifiswitch_gpio_parse(frame->data, &parser, tokens, n_tokens);
							}
							response = ws_wifiswitch_gpio_response();
							break;
						default:
							warn("Unknown wifiswitch request (0x%.2x).\n", value);
					}
					if (response)
					{
						ret = os_strlen(response);
						ws_send_text(response, connection);
						db_free(response);
					}
	
					return(ret);
				}
				else
				{
					warn("Unexpected JSON type in wifiswitch request.\n");
				}
			}
			else
			{
				debug("Not a type token, ignoring.\n");
			}
		}
	}

	return(WS_ERROR);
}

void ws_wifiswitch_send_gpio_status(void)
{
	struct net_connection *connection = tcp_get_connections();
	struct ws_connection *ws_conn;
	char *response = NULL;
	
	debug("Sending GPIO status to WebSocket clients.\n");
	response = ws_wifiswitch_gpio_response();
	while (connection)
	{
		//Find WebSocket connections.
		if (connection->type == NET_CT_WS)
		{
			ws_conn = connection->user;
			if (ws_conn)
			{
				if (ws_conn->handler)
				{
					//Check protocol.
					if (strncmp(ws_conn->handler->protocol, WS_PR_WIFISWITCH, strlen(WS_PR_WIFISWITCH)) == 0);
					{
						debug(" Sending to %p.\n", connection);
						ws_send_text(response, connection);
					}
				}
			}
		}			
		connection = connection->next;
	}
	db_free(response);
}

signed long int ws_wifiswitch_close(struct ws_frame *frame, struct net_connection *connection)
{
	signed long int ret = 0;
	
	debug("Wifiswitch WebSocket close received.\n");
	
	return(ret);
}
