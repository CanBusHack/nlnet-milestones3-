#include <driver/gpio.h>
#include <driver/twai.h>
#include <esp_log.h>

#include <omnitrix/libcan.h>

static const char tag[] = "omni_libcan";

void omni_libcan_main(void) {
    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_34, TWAI_MODE_NORMAL);
    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    general_config.tx_queue_len = 255;
    general_config.rx_queue_len = 255;

    if (twai_driver_install(&general_config, &timing_config, &filter_config) == ESP_OK) {
        ESP_LOGI(tag, "driver installed");
    } else {
        ESP_LOGE(tag, "driver installation failed");
    }

    if (twai_start() == ESP_OK) {
        ESP_LOGI(tag, "driver started");
    } else {
        ESP_LOGE(tag, "driver start failed");
    }
}
