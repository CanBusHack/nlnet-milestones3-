#ifndef OMNITRIX_TEST_H_
#define OMNITRIX_TEST_H_

#include <esp_ota_ops.h>
#include <stdarg.h>
#include <stdio.h>

void ESP_LOGD(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("D: %s: ", tag);
    vprintf(format, args);
    putchar('\n');
    va_end(args);
}

void ESP_LOGI(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("I: %s: ", tag);
    vprintf(format, args);
    putchar('\n');
    va_end(args);
}

void ESP_LOGE(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("E: %s: ", tag);
    vprintf(format, args);
    putchar('\n');
    va_end(args);
}

void ESP_LOGW(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("W: %s: ", tag);
    vprintf(format, args);
    putchar('\n');
    va_end(args);
}

static const esp_partition_t* (*hook_esp_ota_get_running_partition)(void) = 0;
static int (*hook_esp_ota_get_state_partition)(const esp_partition_t* partition, esp_ota_img_states_t* ota_state);
void (*hook_esp_ota_mark_app_valid_cancel_rollback)(void);
const esp_partition_t* (*hook_esp_ota_get_boot_partition)(void);

const esp_partition_t* esp_ota_get_running_partition(void) {
    if (hook_esp_ota_get_running_partition) {
        return hook_esp_ota_get_running_partition();
    } else {
        fprintf(stderr, "Warning: %s called but there was no hook installed!\n", __func__);
        return 0;
    }
}

int esp_ota_get_state_partition(const esp_partition_t* partition, esp_ota_img_states_t* ota_state) {
    if (hook_esp_ota_get_state_partition) {
        return hook_esp_ota_get_state_partition(partition, ota_state);
    } else {
        fprintf(stderr, "Warning: %s called but there was no hook installed!\n", __func__);
        return 0;
    }
}

void esp_ota_mark_app_valid_cancel_rollback(void) {
    if (hook_esp_ota_mark_app_valid_cancel_rollback) {
        hook_esp_ota_mark_app_valid_cancel_rollback();
    } else {
        fprintf(stderr, "Warning: %s called but there was no hook installed!\n", __func__);
    }
}

const esp_partition_t* esp_ota_get_boot_partition(void) {
    if (hook_esp_ota_get_boot_partition) {
        return hook_esp_ota_get_boot_partition();
    } else {
        fprintf(stderr, "Warning: %s called but there was no hook installed!\n", __func__);
        return 0;
    }
}

#endif
