
#include <sdkconfig.h>
#ifdef CONFIG_OMNITRIX_ENABLE_LED

#include <assert.h>
#include <esp_log.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <host/ble_att.h>
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>
#include <host/ble_hs_mbuf.h>
#include <os/os_mbuf.h>

#include <omnitrix/led.h>
#include <omnitrix/uuid.gen.h>
#include <omnitrix/debug.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE

/** BLE GATT service UUID for LED */
static const ble_uuid128_t gatt_svr_svc_uuid = CONFIG_OMNITRIX_LED_SERVICE_UUID_INIT;

/** BLE GATT characteristic UUID for LED control */
static const ble_uuid128_t gatt_svr_chr_control_uuid = CONFIG_OMNITRIX_LED_CHARACTERISTIC_UUID_INIT;

static uint16_t gatt_svr_chr_control_val_handle;

/** BLE GATT access callback for LED control */
static int gatt_svr_access(uint16_t conn_handle, uint16_t attr_handle, 
                          struct ble_gatt_access_ctxt* ctxt, void* arg) {
    omni_debug_log("LED", "GATT operation: %d", ctxt->op);

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR: {
            if (attr_handle == gatt_svr_chr_control_val_handle) {
                omni_debug_log("LED", "LED control characteristic write");

                static uint8_t cmd[4];  // [R,G,B,Brightness]
                uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);

                if (om_len == sizeof(cmd)) {
                    uint16_t len;
                    int rc = ble_hs_mbuf_to_flat(ctxt->om, cmd, sizeof(cmd), &len);
                    assert(rc == 0);
                    assert(len == sizeof(cmd));

                    omni_debug_log("LED", "Setting R:%u G:%u B:%u Brightness:%u%%",
                                 cmd[0], cmd[1], cmd[2], cmd[3]);

                    // Set state to custom when receiving color commands
                    omni_led_set_state(LED_STATE_CUSTOM);

                    // Set brightness first
                    omni_led_set_brightness(cmd[3]);
                    // Then set color
                    omni_led_set_color(cmd[0], cmd[1], cmd[2]);
                    return 0;
                }

                omni_debug_log("LED", "Invalid message length: %u", om_len);
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            /* fallthrough */
        }
        default:
            omni_debug_log("LED", "Unsupported GATT operation: %d", ctxt->op);
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

/** BLE GATT services for LED control */
const struct ble_gatt_svc_def omni_led_gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_svr_chr_control_uuid.u,
                .access_cb = gatt_svr_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &gatt_svr_chr_control_val_handle,
            },
            {
                0,  // End of characteristics array
            },
        },
    },
    {
        0,  // End of services array
    },
};

#endif // CONFIG_OMNITRIX_ENABLE_BLE

// void omni_led_main(void) {
//     omni_debug_log("LED", "Initializing LED subsystem");
//     ESP_ERROR_CHECK(omni_led_init());
//     omni_led_set_brightness(15);  // Default 25% brightness
//     omni_led_set_active();        // Set to green to indicate active state
//     omni_debug_log("LED", "LED initialization complete");
// }
void omni_led_main(void) {
    omni_debug_log("LED", "Initializing LED subsystem");
    ESP_ERROR_CHECK(omni_led_init());
    
    // Run power up sequence
    omni_led_run_powerup();
    
    // Set initial state for BLE advertising
    omni_led_set_state(LED_STATE_BLE_ADV);
    
    omni_debug_log("LED", "LED initialization complete");
}
#endif // CONFIG_OMNITRIX_ENABLE_LED
