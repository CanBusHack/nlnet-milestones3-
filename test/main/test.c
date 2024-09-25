#include <esp_err.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <nvs_flash.h>
#include <unity_test_runner.h>

static void ble_host_task(void* param) {
    unity_run_all_tests();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(nimble_port_init());
    nimble_port_freertos_init(ble_host_task);
}
