#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "missing_dec.h"
#include "wifi_connect.h"
#include "int_flash.h"

#define INTERNAL_FLASH_START_ADDRESS    0x40200000

extern long _irom0_text_end;
static __attribute__ ((used))	
__attribute__((section(".firmware_end_marker"))) uint32_t flash_ends_here;

/* Main entry point.
 */ 
void ICACHE_FLASH_ATTR user_init()
{
    unsigned char   connection_status;
    struct station_config station_conf = { "default", "password", 0, "000000"};

    wifi_set_opmode(STATION_MODE);
    //Turn on auto connect.
    wifi_station_set_auto_connect(true);
    
    // Set baud rate of debug port
    uart_div_modify(0,UART_CLK_FREQ / 115200);
    
    os_delay_us(1000);
    
    //Print banner
    os_printf("\nWIFISwitch version %s.\n", STRING_VERSION);
    system_print_meminfo();
    os_printf("Free heap %u\n", system_get_free_heap_size());    
    
    os_printf("\n\n\n\n");
    
    //flash_dump(0x07600, 65536);
    
    //connection_status = connect_ap(SSID, SSID_PASSWORD);
    if (!wifi_station_set_config(&station_conf))
    {
//        ETS_UART_INTR_ENABLE();
        os_printf("Failed to set station configuration.\n");
    }
        
    if (!wifi_station_connect())
        {
            os_printf("Main: Failed to connect to AP.\n");
        }
        
    /*if (connection_status != STATION_GOT_IP)
    {
        debug("Connection status: %d\n", connection_status);
        scan_ap();
        print_ap_list();
        no_ap();
    }
    
    connection_status = connect_ap(SSID, SSID_PASSWORD);*/
}
