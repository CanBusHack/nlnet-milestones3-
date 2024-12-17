#include <driver/twai.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <nvs_flash.h>
#include <unity.h>
#include <unity_test_runner.h>

static void ble_host_task(void* param) {
    unity_run_all_tests();
    nimble_port_freertos_deinit();
}

void setUp(void) {
    TEST_ASSERT_EQUAL(0, nimble_port_init());
}

void tearDown(void) {
    TEST_ASSERT_EQUAL(0, nimble_port_deinit());
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_34, TWAI_MODE_NORMAL);
    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    general_config.tx_queue_len = 255;
    general_config.rx_queue_len = 255;

    if (twai_driver_install(&general_config, &timing_config, &filter_config) == ESP_OK) {
        ESP_LOGI("omnitest", "driver installed");
    } else {
        ESP_LOGE("omnitest", "driver installation failed");
        return;
    }

    if (twai_start() == ESP_OK) {
        ESP_LOGI("omnitest", "driver started");
    } else {
        ESP_LOGE("omnitest", "driver start failed");
        return;
    }

    nimble_port_freertos_init(ble_host_task);
}
