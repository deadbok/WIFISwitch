#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "missing_dec.h"
#include "wifi_connect.h"
#include "tcp.h"

//This functions is called when a WIFI connection has been established.
void connected_cb(void)
{
    init_tcp(80);
}

// Main entry point and init code.
void ICACHE_FLASH_ATTR user_init()
{
    //Turn off auto connect.
    wifi_station_set_auto_connect(false);
    
    // Set baud rate of debug port
    uart_div_modify(0,UART_CLK_FREQ / 115200);
    
    os_delay_us(1000);
    
    //Print banner
    os_printf("\nWIFISwitch version %s.\n", STRING_VERSION);
    system_print_meminfo();
    os_printf("Free heap %u\n\n", system_get_free_heap_size());    
           
    //flash_dump(0x07600, 65536);
    
    wifi_connect(connected_cb);
    
    os_printf("\nLeaving user_init...\n");
}
