#include <sdkconfig.h>
#ifdef CONFIG_OMNITRIX_ENABLE_HELLO

#include <assert.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <driver/gpio.h>
#include <driver/twai.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/projdefs.h>
#include <hal/twai_types.h>
#include <host/ble_hs_mbuf.h>
#include <host/ble_uuid.h>
#include <os/os_mbuf.h>

#include <omnitrix/hello.h>
#include <omnitrix/libcan.h>
#include <omnitrix/libisotp.h>
#include <omnitrix/libvin.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_att.h>
#include <host/ble_gatt.h>
#endif

#include "isotp.h"

static const char tag[] = "omni_hello";

static uint8_t isotp_unmatched_frame_queue_storage[sizeof(twai_message_t) * 4];
static StaticQueue_t isotp_unmatched_frame_queue_buffer;
static QueueHandle_t isotp_unmatched_frame_queue_handle;

static uint8_t isotp_msg_queue_storage[sizeof(struct isotp_msg) * 4];
static StaticQueue_t isotp_msg_queue_buffer;
static QueueHandle_t isotp_msg_queue_handle;

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
static const ble_uuid128_t gatt_svr_svc_uuid = BLE_UUID128_INIT(0x46, 0x9a, 0x1b, 0xa2, 0xe8, 0xb6, 0xf6, 0x93, 0x33, 0x43, 0x3d, 0x4e, 0xa8, 0x1b, 0x94, 0x49);
static const ble_uuid128_t gatt_svr_chr_hello_uuid = BLE_UUID128_INIT(0x5b, 0x36, 0x94, 0x23, 0x4e, 0xae, 0x48, 0x9f, 0xe9, 0x43, 0x52, 0xca, 0x7a, 0xf3, 0xce, 0x4c);
static const ble_uuid128_t gatt_svr_chr_vin_uuid = BLE_UUID128_INIT(0xb4, 0x8f, 0xd5, 0xba, 0x66, 0x75, 0x3b, 0x90, 0xb1, 0x4c, 0xec, 0x9b, 0x43, 0x07, 0xf7, 0x83);
static const ble_uuid128_t gatt_svr_chr_can_uuid = BLE_UUID128_INIT(0x43, 0x17, 0x96, 0x20, 0x7c, 0xf2, 0x2c, 0x90, 0xce, 0x4b, 0xb0, 0x8a, 0x25, 0x9b, 0x59, 0x0b);
static const ble_uuid128_t gatt_svr_chr_isotp_pairs_uuid = BLE_UUID128_INIT(0x39, 0x9d, 0x8e, 0x6a, 0x12, 0x6e, 0x8b, 0xb1, 0x57, 0x4d, 0xd7, 0xdf, 0x1d, 0x80, 0x31, 0x7e);
static const ble_uuid128_t gatt_svr_chr_isotp_bs_stmin_uuid = BLE_UUID128_INIT(0xfc, 0xe2, 0x52, 0x84, 0xed, 0x1d, 0x22, 0x8d, 0xb4, 0x4e, 0xdb, 0x76, 0xfa, 0x17, 0x49, 0x27);
static const ble_uuid128_t gatt_svr_chr_isotp_msg_uuid = BLE_UUID128_INIT(0x1e, 0x9a, 0x7a, 0x3f, 0x3f, 0x9e, 0x6f, 0x87, 0x3a, 0x42, 0x2b, 0xb9, 0xe1, 0xd4, 0x13, 0x28);
static uint16_t gatt_svr_chr_hello_val_handle;
static uint16_t gatt_svr_chr_vin_val_handle;
static uint16_t gatt_svr_chr_can_val_handle;
static uint16_t gatt_svr_chr_isotp_pairs_val_handle;
static uint16_t gatt_svr_chr_isotp_bs_stmin_val_handle;
static uint16_t gatt_svr_chr_isotp_msg_val_handle;

static int gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    static struct isotp_event event;
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
        if (attr_handle == gatt_svr_chr_can_val_handle) {
            ESP_LOGI(tag, "read can characteristic");
            static twai_message_t message;
            if (xQueueReceive(isotp_unmatched_frame_queue_handle, &message, 0) == pdTRUE) {
                ESP_LOGD(tag, "can read complete");
                int rc = os_mbuf_append(ctxt->om, &message, sizeof(message));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            ESP_LOGD(tag, "no can frames available");
            return BLE_ATT_ERR_UNLIKELY;
        }
        if (attr_handle == gatt_svr_chr_isotp_msg_val_handle) {
            ESP_LOGI(tag, "read isotp msg characteristic");
            static struct isotp_msg message;
            if (xQueueReceive(isotp_msg_queue_handle, &message, 0) == pdTRUE) {
                assert(message.size <= 256);
                ESP_LOGD(tag, "msg read complete");
                int rc = os_mbuf_append(ctxt->om, message.data, message.size);
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            ESP_LOGD(tag, "no msgs available");
            return BLE_ATT_ERR_UNLIKELY;
        }
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (attr_handle == gatt_svr_chr_can_val_handle) {
            ESP_LOGI(tag, "write can characteristic");
            static twai_message_t message;
            uint16_t len;
            if (ble_hs_mbuf_to_flat(ctxt->om, &message, sizeof(message), &len) == 0) {
                ESP_LOGD(tag, "mbuf_to_flat ok");
                if (twai_transmit(&message, 0) == ESP_OK) {
                    ESP_LOGD(tag, "can write complete");
                    int rc = os_mbuf_append(ctxt->om, &message, sizeof(message));
                    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
                }
                ESP_LOGD(tag, "write buffer full");
                return BLE_ATT_ERR_UNLIKELY;
            }
            ESP_LOGD(tag, "mbuf_to_flat error");
            return BLE_ATT_ERR_UNLIKELY;
        }
        if (attr_handle == gatt_svr_chr_isotp_pairs_val_handle) {
            ESP_LOGI(tag, "write isotp pairs characteristic");
            static unsigned char buf[252];
            uint16_t len;
            if (ble_hs_mbuf_to_flat(ctxt->om, &buf, sizeof(buf), &len) == 0) {
                ESP_LOGD(tag, "mbuf_to_flat ok");
                if (len % 12 == 0) {
                    event.type = EVENT_RECONFIGURE_PAIRS;
                    event.pairs.size = len;
                    memcpy(event.pairs.data, buf, len);
                    if (xQueueSend(isotp_event_queue_handle, &event, 0) == pdTRUE) {
                        ESP_LOGD(tag, "queued reconfigure pairs");
                        return 0;
                    }
                    ESP_LOGD(tag, "event queue error (full?)");
                    return BLE_ATT_ERR_UNLIKELY;
                }
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            ESP_LOGD(tag, "mbuf_to_flat error");
            return BLE_ATT_ERR_UNLIKELY;
        }
        if (attr_handle == gatt_svr_chr_isotp_bs_stmin_val_handle) {
            ESP_LOGI(tag, "write isotp bs stmin characteristic");
            static unsigned char buf[2];
            uint16_t len;
            if (ble_hs_mbuf_to_flat(ctxt->om, &buf, sizeof(buf), &len) == 0) {
                ESP_LOGD(tag, "mbuf_to_flat ok");
                if (len == 2) {
                    event.type = EVENT_RECONFIGURE_BS_STMIN;
                    event.bs_stmin.data[0] = buf[0];
                    event.bs_stmin.data[1] = buf[1];
                    if (xQueueSend(isotp_event_queue_handle, &event, 0) == pdTRUE) {
                        ESP_LOGD(tag, "queued reconfigure bs/stmin");
                        return 0;
                    }
                    ESP_LOGD(tag, "event queue error (full?)");
                    return BLE_ATT_ERR_UNLIKELY;
                }
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            ESP_LOGD(tag, "mbuf_to_flat error");
            return BLE_ATT_ERR_UNLIKELY;
        }
        if (attr_handle == gatt_svr_chr_isotp_msg_val_handle) {
            ESP_LOGI(tag, "write isotp msg characteristic");
            static unsigned char buf[256];
            uint16_t len;
            if (ble_hs_mbuf_to_flat(ctxt->om, &buf, sizeof(buf), &len) == 0) {
                ESP_LOGD(tag, "mbuf_to_flat ok");
                if (len >= 4) {
                    event.type = EVENT_WRITE_MSG;
                    event.msg.size = len;
                    memcpy(event.msg.data, buf, len);
                    if (xQueueSend(isotp_event_queue_handle, &event, 0) == pdTRUE) {
                        ESP_LOGD(tag, "queued msg send");
                        return 0;
                    }
                    ESP_LOGD(tag, "event queue error (full?)");
                    return BLE_ATT_ERR_UNLIKELY;
                }
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            ESP_LOGD(tag, "mbuf_to_flat error");
            return BLE_ATT_ERR_UNLIKELY;
        }
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;

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
                .uuid = &gatt_svr_chr_can_uuid.u,
                .access_cb = gatt_svc_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &gatt_svr_chr_can_val_handle,
            },
            {
                .uuid = &gatt_svr_chr_isotp_pairs_uuid.u,
                .access_cb = gatt_svc_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle = &gatt_svr_chr_isotp_pairs_val_handle,
            },
            {
                .uuid = &gatt_svr_chr_isotp_bs_stmin_uuid.u,
                .access_cb = gatt_svc_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle = &gatt_svr_chr_isotp_bs_stmin_val_handle,
            },
            {
                .uuid = &gatt_svr_chr_isotp_msg_uuid.u,
                .access_cb = gatt_svc_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                .val_handle = &gatt_svr_chr_isotp_msg_val_handle,
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

static void isotp_read_handler(struct isotp_msg* msg) {
    // TODO: remove queues; notify instead
    xQueueSend(isotp_msg_queue_handle, msg, 0);
}

static void isotp_unmatched_handler(struct twai_message_timestamp* msg) {
    // TODO: remove queues; notify instead
    xQueueSend(isotp_unmatched_frame_queue_handle, msg, 0);
}

void omni_hello_main(void) {
    omni_libisotp_main();
    omni_libisotp_add_incoming_handler(isotp_read_handler);
    omni_libisotp_add_unmatched_handler(isotp_unmatched_handler);
    isotp_unmatched_frame_queue_handle = xQueueCreateStatic(4, sizeof(twai_message_t), isotp_unmatched_frame_queue_storage, &isotp_unmatched_frame_queue_buffer);
    isotp_msg_queue_handle = xQueueCreateStatic(4, sizeof(struct isotp_msg), isotp_msg_queue_storage, &isotp_msg_queue_buffer);
}

#endif
