#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
#include "nvs_storage.h"
#include "lfs_storage.h"
#include "utilities.h"
*/

#include "storage_manager.h"

extern "C" void app_main(void)
{
    auto& storageManager = StorageManager::getInstance();
    storageManager.init();
    
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
