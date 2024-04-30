#include <assert.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdint.h>

#include <esp_flash_partitions.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <host/ble_hs_mbuf.h>
#include <os/os_mbuf.h>

#include <omnitrix/ota.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_att.h>
#include <host/ble_gatt.h>
#endif

/** Logging tag (omni_ota) */
static const char tag[] = "omni_ota";

/** OTA update callback */
static int ota_update(void* address, size_t size) {
    ESP_LOGI(tag, "write firmware at %p, size %zu", address, size);
    return 0;
}

/** OTA update finalizer */
static int ota_end(void) {
    ESP_LOGI(tag, "write firmware finished");
    return 0;
}

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
/** BLE GATT service UUID for OTA (0ac32576-eeb0-4be5-8cc3-01cfd3a6c233) */
static const ble_uuid128_t gatt_svr_svc_uuid = BLE_UUID128_INIT(0x33, 0xc2, 0xa6, 0xd3, 0xcf, 0x01, 0xc3, 0x8c, 0xe5, 0x4b, 0xb0, 0xee, 0x76, 0x25, 0xc3, 0x0a);

/** BLE GATT characteristic UUID for OTA (324278c9-c3da-4c81-aaa0-b55c1211c4cc) */
static const ble_uuid128_t gatt_svr_chr_uuid = BLE_UUID128_INIT(0xcc, 0xc4, 0x11, 0x12, 0x5c, 0xb5, 0xa0, 0xaa, 0x81, 0x4c, 0xda, 0xc3, 0xc9, 0x78, 0x42, 0x32);

static uint16_t gatt_svr_chr_val_handle;

/** BLE GATT access callback for OTA */
static int gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    ESP_LOGI(tag, "GATT op: %d", ctxt->op);

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (attr_handle == gatt_svr_chr_val_handle) {
            ESP_LOGI(tag, "write OTA characteristic");

            static uint8_t buffer[256];
            uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
            if (om_len > sizeof(uintptr_t) && om_len < sizeof(buffer)) {
                uint16_t om_len_2;
                int rc = ble_hs_mbuf_to_flat(ctxt->om, buffer, sizeof(buffer), &om_len_2);
                assert(rc == 0);

                uintptr_t ptr = 0;
                for (int i = 0; i < sizeof(uintptr_t); i++) {
                    ptr = (ptr << 8) | buffer[i];
                }
                if (ptr == UINTPTR_MAX) {
                    rc = ota_end();
                } else {
                    rc = ota_update((void*)ptr, om_len - sizeof(uintptr_t));
                }
                if (rc != 0) {
                    ESP_LOGE(tag, "error writing OTA: %d", rc);
                    return BLE_ATT_ERR_INVALID_OFFSET;
                }

                return 0;
            }

            ESP_LOGE(tag, "invalid length: %u", om_len);
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        /* fallthrough */
    default:
        ESP_LOGI(tag, "unsupported operation: %d", ctxt->op);
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }
}

/** BLE GATT services for OTA */
const struct ble_gatt_svc_def omni_ota_gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_svr_chr_uuid.u,
                .access_cb = gatt_svc_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle = &gatt_svr_chr_val_handle,
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
