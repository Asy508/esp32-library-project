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
#include "httpClient_service.h"
#include "httpServer_service.h"
#include "captive_service.h"
#include "time_service.h"
#include "ping_service.h"
#include "sse_service.h"
#include "uart_service.h"
#include "ota_service.h"

#define UART_PORT UART_NUM_1
#define TXD_PIN   GPIO_NUM_5
#define RXD_PIN   GPIO_NUM_4

const char *TAG = "Main";

static esp_err_t rootHandler(httpd_req_t* req) {
    auto& server = HttpServer::getInstance();

    server.send(req,"Hello from ESP32!","text/plain");

    return ESP_OK;
}

static esp_err_t statusHandler(httpd_req_t* req) {
    auto& server = HttpServer::getInstance();

    server.sendJson(req,"{\"status\":\"ok\",\"device\":\"ESP32-C5\"}");

    return ESP_OK;
}

static esp_err_t postHandler(httpd_req_t* req) {
    auto& server = HttpServer::getInstance();

    char buffer[256];

    int length = server.receive(req, buffer, sizeof(buffer));

    if (length < 0) {
        server.sendStatus(req, "400 Bad Request", "Invalid request"
        );

        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "POST data: %s", buffer);

    server.sendJson(req, "{\"received\":true}");

    return ESP_OK;
}

extern "C" void app_main(void)
{
    //----------------------Storage Manager
    StorageManager& storageManager = StorageManager::getInstance();
    storageManager.init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    //----------------------Wifi Manager
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
        wifiManager.getIP_info();
        wifiManager.scan();

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
    
    //----------------------W5500
    auto& w5500 = W5500::getInstance();
    if (w5500.init()) {
        printf("W5500 init start\n");
        w5500.start();
    } else {
        printf("W5500 init failed\n");
    }

    //----------------------MQTT Service
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

    //----------------------HTTP Server
    auto& server = HttpServer::getInstance();

    if (!server.start(80)) {
        ESP_LOGE(TAG, "HTTP server failed to start");
        return;
    }

    server.registerGet("/", rootHandler);

    server.registerGet("/api/status",statusHandler);

    server.registerPost("/api/message",postHandler);


    ESP_LOGI(TAG, "HTTP server ready");

    if (!server.start(80)) {
        ESP_LOGE(TAG, "HTTP server failed");
        return;
    }

    //----------------------SSE service
    auto& sse = SSEService::getInstance();

    if (!sse.init(server, "/events")) {
        ESP_LOGE(TAG, "SSE initialization failed");
    }
    /*
        sse.sendEvent("wifiRSSI", "82");
        sse.sendEvent("mqtt", "connected");
        sse.sendEvent("ethernet", "connected");
    */

    //----------------------Captive Service
    auto& captive = CaptivePortal::getInstance();

    captive.setPortalUrl("http://192.168.4.1/captive");

    if (!captive.start(
            server,
            wifiManager.getAPNetif()))
    {
        ESP_LOGE(
            TAG,
            "Captive portal failed"
        );
    }

    //----------------------Ping Service
    auto& ping = PingService::getInstance();

    ping.start(
        "8.8.8.8",
        [](bool online, uint32_t latency) {
            if (online) {
                ESP_LOGI("MAIN", "Ping OK - %lu ms", (unsigned long)latency);
            } else {
                ESP_LOGW("MAIN", "Ping FAILED");
            }
        },
        5000,
        1000
    );

    //----------------------Time Service
    auto& timeService = TimeService::getInstance();
    timeService.setTimezone("MYT-8");
    timeService.init();
    timeService.start();

    while (!timeService.isSynced())
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "Time synchronized");

    //----------------------LED Service
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

    //----------------------HTTPClient Service
    auto& http = HttpClient::getInstance();

    http.init();

    http.get("http://example.com/api/status");

    http.post(
        "http://example.com/api/message",
        "{\"hello\":\"world\"}"
    );
    
    http.uploadFile(
        "http://example.com/api/upload",
        "/littlefs/data.csv",
        "csv_file",
        "text/csv"
    );

    //----------------------UART Service
    auto& uart = UARTService::getInstance();

    if (uart.init(UART_PORT, TXD_PIN, RXD_PIN, 115200)) {
        uart.onReceive([](const uint8_t* data, size_t length) {
            printf("UART RX: %u bytes\n", static_cast<unsigned>(length));
            ESP_LOG_BUFFER_HEX("UART", data, length);
            //Application queue
        });

        uart.start();
    }

    uint8_t packet[] = {0xAA, 0x01, 0x02, 0xBB};
    uart.write(packet, sizeof(packet));

    uart.write("Hello UART\n"); 

    //----------------------OTA Service
    auto& ota = OTAService::getInstance();

    ota.init();

    //ota.performOTA("http://aasupport.supa.com.my/IOT/ESP_C5.bin");

    //----------------------Main loop
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ledService.last_uart_time = xTaskGetTickCount();
        ledService.last_mqtt_time = xTaskGetTickCount();

        time_t timestamp = timeService.getTimestamp();

        int64_t timestampMs = timeService.getTimestampMs();

        char timeString[64];

        if (timeService.getFormattedTime(timeString, sizeof(timeString))) {
            ESP_LOGI(TAG, "Time: %s", timeString);
        }

        ESP_LOGI(TAG, "Timestamp: %lld", (long long)timestamp);

        ESP_LOGI(TAG, "Timestamp ms: %lld", (long long)timestampMs);
    }
}
