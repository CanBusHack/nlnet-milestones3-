#include <esp_log.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <omnitrix/omni_ble.h>
#endif

static const char tag[] = "omnitrix";

void app_main(void) {
    ESP_LOGI(tag, "main app started");

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
    ESP_LOGI(tag, "starting BLE");
    omni_ble_main();
#endif
}
