#ifndef ESP_OTA_OPS_H_
#define ESP_OTA_OPS_H_

#include <esp_err.h>
#include <esp_partition.h>
#include <stddef.h>
#include <stdint.h>

const esp_partition_t* esp_ota_get_boot_partition(void);
const esp_partition_t* esp_ota_get_running_partition(void);
const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t* current);

typedef enum _esp_ota_img_states {
    ESP_OTA_IMG_PENDING_VERIFY
} esp_ota_img_states_t;

int esp_ota_get_state_partition(const esp_partition_t* partition, esp_ota_img_states_t* ota_state);
void esp_ota_mark_app_valid_cancel_rollback(void);

typedef void* esp_ota_handle_t;

esp_err_t esp_ota_end(esp_ota_handle_t handle);
esp_err_t esp_ota_write_with_offset(esp_ota_handle_t handle, const void *data, size_t size, uint32_t offset);
esp_err_t esp_ota_abort(esp_ota_handle_t handle);
esp_err_t esp_ota_begin(const esp_partition_t *partition, size_t image_size, esp_ota_handle_t *out_handle);

enum {
    OTA_SIZE_UNKNOWN
};

#endif
