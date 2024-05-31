#ifndef ESP_OTA_OPS_H_
#define ESP_OTA_OPS_H_

#include <esp_partition.h>

const esp_partition_t* esp_ota_get_boot_partition(void);
const esp_partition_t* esp_ota_get_running_partition(void);

typedef enum _esp_ota_img_states {
    ESP_OTA_IMG_PENDING_VERIFY
} esp_ota_img_states_t;

int esp_ota_get_state_partition(const esp_partition_t* partition, esp_ota_img_states_t* ota_state);
void esp_ota_mark_app_valid_cancel_rollback(void);

#endif
