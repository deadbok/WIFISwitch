#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "wifi_connect.h"

/* Main entry point.
 */ 
void ICACHE_FLASH_ATTR user_init()
{
    char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;
    
    // Set baud rate of debug port
    uart_div_modify(0,UART_CLK_FREQ / 115200);

    //Set station mode
    wifi_set_opmode( STATION_MODE );

    //Set ap settings
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);
    wifi_station_set_config(&stationConf);
    
    system_print_meminfo();
    os_printf("Free heap %u\n", system_get_free_heap_size());
    
    if (wifi_station_get_connect_status() != STATION_GOT_IP)
    {
        no_ap();
    }
}
