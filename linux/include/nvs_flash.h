#ifndef NVS_FLASH_H_
#define NVS_FLASH_H_

#include <esp_err.h>

esp_err_t nvs_flash_init(void);
esp_err_t nvs_flash_erase(void);
void ESP_ERROR_CHECK(esp_err_t err);

#endif
