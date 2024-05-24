#include <assert.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdint.h>

#include <driver/gpio.h>
#include <driver/twai.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/projdefs.h>
#include <hal/twai_types.h>
#include <host/ble_uuid.h>
#include <os/os_mbuf.h>

#include <omnitrix/hello.h>
#include <omnitrix/libvin.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_att.h>
#include <host/ble_gatt.h>
#endif

static const char tag[] = "omni_hello";

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
static const ble_uuid128_t gatt_svr_svc_uuid = BLE_UUID128_INIT(0x46, 0x9a, 0x1b, 0xa2, 0xe8, 0xb6, 0xf6, 0x93, 0x33, 0x43, 0x3d, 0x4e, 0xa8, 0x1b, 0x94, 0x49);
static const ble_uuid128_t gatt_svr_chr_hello_uuid = BLE_UUID128_INIT(0x5b, 0x36, 0x94, 0x23, 0x4e, 0xae, 0x48, 0x9f, 0xe9, 0x43, 0x52, 0xca, 0x7a, 0xf3, 0xce, 0x4c);
static const ble_uuid128_t gatt_svr_chr_vin_uuid = BLE_UUID128_INIT(0xb4, 0x8f, 0xd5, 0xba, 0x66, 0x75, 0x3b, 0x90, 0xb1, 0x4c, 0xec, 0x9b, 0x43, 0x07, 0xf7, 0x83);
static uint16_t gatt_svr_chr_hello_val_handle;
static uint16_t gatt_svr_chr_vin_val_handle;

static int gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        if (attr_handle == gatt_svr_chr_hello_val_handle) {
            ESP_LOGI(tag, "read hello characteristic");
            static const char hello[] = "Hello from Omnitrix!";
            int rc = os_mbuf_append(ctxt->om, hello, sizeof(hello) - 1);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        if (attr_handle == gatt_svr_chr_vin_val_handle) {
            ESP_LOGI(tag, "read vin characteristic");
            char vin[17];
            if (omni_libvin_get_vin(vin)) {
                ESP_LOGD(tag, "vin read complete");
                int rc = os_mbuf_append(ctxt->om, vin, sizeof(vin));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            ESP_LOGD(tag, "vin read incomplete");
            return BLE_ATT_ERR_UNLIKELY;
        }
        /* fallthrough */

    default:
        ESP_LOGI(tag, "unsupported operation: %d", ctxt->op);
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }
}

const struct ble_gatt_svc_def omni_hello_gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_svr_chr_hello_uuid.u,
                .access_cb = gatt_svc_access,
                .flags = BLE_GATT_CHR_F_READ,
                .val_handle = &gatt_svr_chr_hello_val_handle,
            },
            {
                .uuid = &gatt_svr_chr_vin_uuid.u,
                .access_cb = gatt_svc_access,
                .flags = BLE_GATT_CHR_F_READ,
                .val_handle = &gatt_svr_chr_vin_val_handle,
            },
            {
                0,
            },
        },
    },
    {
        0,
    },
};
#endif

void omni_hello_main(void) {
    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_34, TWAI_MODE_NORMAL);
    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter_config = {
        .acceptance_code = (0x7E8U << 21),
        .acceptance_mask = ~(0x7FFU << 21),
        .single_filter = true,
    };

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

    omni_libvin_main();
}
