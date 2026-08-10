idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES 
                    #nvs_storage
                    #lfs_storage
                    #utilities
                    #led_service
                    Asy508__esp32-led-service
                    storage_manager
                    )
