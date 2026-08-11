idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES 
                    #nvs_storage
                    #lfs_storage
                    #utilities
                    #led_service
                    esp32-led-service
                    storage_manager
                    ethernet_w5500
                    wifi_manager
                    mqtt_service
                    )
