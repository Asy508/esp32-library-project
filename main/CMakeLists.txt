idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES 
                    #nvs_storage
                    #lfs_storage
                    #utilities
                    storage_manager
                    )
