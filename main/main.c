#include <esp_err.h>
#include <esp_flash_partitions.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_gatt.h>
#include <omnitrix/ble.h>
#endif

#ifdef CONFIG_OMNITRIX_ENABLE_OTA
#include <omnitrix/ota.h>
#endif

#ifdef CONFIG_OMNITRIX_ENABLE_HELLO
#include <omnitrix/hello.h>
#endif

static const char tag[] = "omnitrix";

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
static const struct ble_gatt_svc_def* gatt_svr_svcs[] = {
#ifdef CONFIG_OMNITRIX_ENABLE_OTA
    omni_ota_gatt_svr_svcs,
#endif
#ifdef CONFIG_OMNITRIX_ENABLE_HELLO
    omni_hello_gatt_svr_svcs,
#endif
    NULL,
};
#endif

/**
 * Even if we're not including the OTA component, this firmware may have
 * been installed via OTA. In that case, we will need to finalize or
 * rollback the update. We'll include this functionality in the main
 * module, since it's always required.
 */
static void handle_ota_boot(void) {
    const esp_partition_t* configured = esp_ota_get_boot_partition();
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (configured != running) {
        ESP_LOGW(tag, "Not running from the configured partition!");
    }

    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // TODO: implement ability to rollback?
            ESP_LOGI(tag, "marking OTA as valid");
            esp_ota_mark_app_valid_cancel_rollback();
        } else {
            ESP_LOGI(tag, "no change to OTA state");
        }
    } else {
        ESP_LOGI(tag, "app is not OTA; factory app or other error");
    }
}

void app_main(void) {
    ESP_LOGI(tag, "main app started");
    handle_ota_boot();

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
    ESP_LOGI(tag, "starting BLE");
    omni_ble_main(gatt_svr_svcs);
#endif

#ifdef CONFIG_OMNITRIX_ENABLE_HELLO
    ESP_LOGI(tag, "starting Hello World");
    omni_hello_main();
#endif
}
