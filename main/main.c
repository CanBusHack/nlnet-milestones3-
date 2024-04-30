#include <esp_log.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_gatt.h>
#include <omnitrix/ble.h>
#endif

static const char tag[] = "omnitrix";

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
static const struct ble_gatt_svc_def* gatt_svr_svcs[] = {
    NULL
};
#endif

void app_main(void) {
    ESP_LOGI(tag, "main app started");

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
    ESP_LOGI(tag, "starting BLE");
    omni_ble_main(gatt_svr_svcs);
#endif
}
