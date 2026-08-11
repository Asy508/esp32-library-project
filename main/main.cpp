#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
/*
#include "nvs_storage.h"
#include "lfs_storage.h"
#include "utilities.h"
*/
#include "led_service.h"
#include "storage_manager.h"
#include "ethernet_w5500.h"
#include "wifi_manager.h"
#include "mqtt_service.h"

const char *TAG = "Main";

extern "C" void app_main(void)
{
    StorageManager& storageManager = StorageManager::getInstance();
    storageManager.init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    auto& wifiManager = WiFiManager::getInstance();

    if (wifiManager.init()) {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char captive_ssid[32];
        snprintf(captive_ssid, sizeof(captive_ssid), "Hybrid Module-%02X%02X%02X", mac[3], mac[4], mac[5]);
        printf("WiFi initialized\n");

        wifiManager.configSTA("AA_AP", "aaap2018");
        wifiManager.configAP(captive_ssid, "");

        wifiManager.setMode(WIFI_MODE_APSTA);
        wifiManager.start();

        wifiManager.scan();

        vTaskDelay(pdMS_TO_TICKS(20000));

        const auto& aps = wifiManager.getScanResults();

        for(const auto& ap : aps)
        {
            printf("%s (%d)\n",
                ap.ssid.c_str(),
                ap.rssi);
        }

    } else {
        printf("WiFi initialization failed\n");
    }
    
    auto& w5500 = W5500::getInstance();
    if (w5500.init()) {
        printf("W5500 init start\n");
        w5500.start();
    } else {
        printf("W5500 init failed\n");
    }
    
    auto& mqttClient = MQTTClient::getInstance();
    if (mqttClient.init("mqtt://mqtt.armscloud.com")) {
        printf("MQTT initialized\n");
        
        mqttClient.onConnection([&mqttClient](bool connected) {
            if (connected) {
                printf("MQTT connected\n");
                uint8_t rawData[4] = {0xAA,0x01, 0x02,0xBB};
                mqttClient.subscribe("test/topic", 0);
                mqttClient.publish("test/device","Hello from ESP32-C5",1,false);
                mqttClient.publish("test/device", rawData, sizeof(rawData), 1, false);
            } else {
                printf("MQTT disconnected\n");
            }
        });

        mqttClient.onMessage([&mqttClient](const char* topic, const uint8_t* data, size_t length) {
            printf("MQTT message received on topic: %s\n", topic);
            printf("Message: %.*s\n", (int)length, data);
        });

        mqttClient.start();
    } else {
        printf("MQTT initialization failed\n");
    }

    

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
    
    if(w5500.isConnected()) {
        printf("Ethernet is connected\n");
    } else {
        printf("Ethernet is not connected\n");
    }

    if(wifiManager.isConnected()) {
        printf("WiFi is connected\n");
    } else {
        printf("WiFi is not connected\n");
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ledService.last_uart_time = xTaskGetTickCount();
        ledService.last_mqtt_time = xTaskGetTickCount();

    }
}
