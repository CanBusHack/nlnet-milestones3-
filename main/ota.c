#include <sdkconfig.h>
#ifdef CONFIG_OMNITRIX_ENABLE_OTA

#include <assert.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stddef.h>
#include <stdint.h>

#include <esp_flash_partitions.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <host/ble_hs_mbuf.h>
#include <os/os_mbuf.h>

#include <omnitrix/libnvs.h>
#include <omnitrix/ota.h>
#include <omnitrix/uuid.gen.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_att.h>
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>
#endif

/** Logging tag (omni_ota) */
static const char tag[] = "omni_ota";

/** Global OTA handle */
static esp_ota_handle_t ota_handle;

/** Flag for currently running OTA */
static int ota_started = 0;

/** OTA update initializer */
static int ota_begin(void) {
    if (ota_started) {
        ESP_LOGI(tag, "abort current OTA!");
        esp_ota_abort(ota_handle);
        ota_started = 0;
    }
    ESP_LOGI(tag, "write firmware begin");

    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(tag, "no update partition available!");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err == ESP_OK) {
        ota_started = 1;
        return 0;
    } else {
        ESP_LOGE(tag, "begin error: %d", err);
    }
    return err;
}

/** OTA update callback */
static int ota_update(const void* data, size_t size, uint32_t offset) {
    ESP_LOGI(tag, "write firmware at 0x%08lx, size %zu", offset, size);
    if (ota_started) {
        esp_err_t err = esp_ota_write_with_offset(ota_handle, data, size, offset);
        if (err == ESP_OK) {
            return 0;
        } else {
            ESP_LOGE(tag, "write_with_offset error: %d", err);
        }
        return err;
    } else {
        ESP_LOGE(tag, "OTA not started yet!");
        return ESP_ERR_INVALID_ARG;
    }
}

/** OTA update finalizer */
static int ota_end(void) {
    ESP_LOGI(tag, "write firmware finished");
    if (ota_started) {
        esp_err_t err = esp_ota_end(ota_handle);
        ota_started = 0;
        if (err == ESP_OK) {
            return 0;
        } else {
            ESP_LOGE(tag, "end error: %d", err);
        }
        return err;
    } else {
        ESP_LOGE(tag, "OTA not started yet!");
        return ESP_ERR_INVALID_ARG;
    }
}

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
/** BLE GATT service UUID for OTA */
static const ble_uuid128_t gatt_svr_svc_uuid = CONFIG_OMNITRIX_OTA_SERVICE_UUID_INIT;

/** BLE GATT characteristic UUID for OTA */
static const ble_uuid128_t gatt_svr_chr_uuid = CONFIG_OMNITRIX_OTA_CHARACTERISTIC_UUID_INIT;

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
            if (om_len > sizeof(uint32_t) && om_len <= sizeof(buffer)) {
                uint16_t om_len_2;
                int rc = ble_hs_mbuf_to_flat(ctxt->om, buffer, sizeof(buffer), &om_len_2);
                assert(rc == 0);
                assert(om_len == om_len_2);

                uint32_t ptr = 0;
                for (int i = 0; i < sizeof(ptr); i++) {
                    ptr = (ptr << 8) | buffer[i];
                }
                switch (ptr) {
                case UINT32_MAX - 1:
                    rc = ota_begin();
                    break;
                case UINT32_MAX:
                    rc = ota_end();
                    break;
                default:
                    rc = ota_update(buffer + sizeof(ptr), om_len - sizeof(ptr), ptr);
                    break;
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

void omni_ota_main(void) {
    omni_libnvs_main();
}

#endif
