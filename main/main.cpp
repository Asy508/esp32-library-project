#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
#include "nvs_storage.h"
#include "lfs_storage.h"
#include "utilities.h"
*/
#include "led_service.h"
#include "storage_manager.h"

extern "C" void app_main(void)
{
    auto& storageManager = StorageManager::getInstance();
    storageManager.init();
    
    auto& ledService = LEDService::getInstance();
    ledService.init();

    ledService.ota_running = true;
    vTaskDelay(pdMS_TO_TICKS(5000));
    ledService.ota_running = false;

    ledService.wifi_ap_mode = true;
    vTaskDelay(pdMS_TO_TICKS(5000));
    ledService.wifi_ap_mode = false;
    
    ledService.wifi_reconnecting = true;
    vTaskDelay(pdMS_TO_TICKS(5000));
    ledService.wifi_reconnecting = false;
    
    ledService.wifi_connected = true;
    vTaskDelay(pdMS_TO_TICKS(5000));    
    ledService.wifi_connected = false;

    ledService.lan_connected = true;
    vTaskDelay(pdMS_TO_TICKS(5000));
    ledService.lan_connected = false;

    vTaskDelay(pdMS_TO_TICKS(5000));
    ledService.wifi_connected = true;
    ledService.lan_connected = true;
    ledService.mqtt_connected = true; 

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ledService.last_uart_time = xTaskGetTickCount();
        ledService.last_mqtt_time = xTaskGetTickCount();

    }
}
