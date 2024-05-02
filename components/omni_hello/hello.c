#include <assert.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <driver/gpio.h>
#include <driver/twai.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/projdefs.h>
#include <hal/twai_types.h>
#include <host/ble_uuid.h>
#include <os/os_mbuf.h>

#include <omnitrix/hello.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_att.h>
#include <host/ble_gatt.h>
#endif

static const char tag[] = "omni_hello";

static char vin[17] = { 0 };
static SemaphoreHandle_t semaphore;

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
            if (semaphore != NULL) {
                if (xSemaphoreTake(semaphore, 0) == pdTRUE) {
                    if (vin[16]) {
                        int rc = os_mbuf_append(ctxt->om, vin, sizeof(vin));
                        xSemaphoreGive(semaphore);
                        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
                    }
                    xSemaphoreGive(semaphore);
                }
                return BLE_ATT_ERR_READ_NOT_PERMITTED;
            }
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

static twai_message_t message = { 0 };

static twai_message_t generic_continuation = {
    .identifier = 0x7e0,
    .data_length_code = 8,
    .data = { 0x30, 0x00, 0x00, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc },
};

static void debug_log_message(const char* text, const twai_message_t* message) {
    static char buf[139];
    snprintf(
        buf,
        sizeof(buf),
        "{ .identifier = 0x%08lx, .data_length_code = %d, .flags = 0x%08lx, .data = {0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x} }",
        message->identifier,
        message->data_length_code,
        message->flags,
        message->data[0],
        message->data[1],
        message->data[2],
        message->data[3],
        message->data[4],
        message->data[5],
        message->data[6],
        message->data[7]);
    ESP_LOGD(tag, "%s: %s", text, buf);
}

#define can_receive(message) (twai_receive(message, pdMS_TO_TICKS(1000)) == ESP_OK)
#define can_transmit(message) (twai_transmit(message, pdMS_TO_TICKS(1000)) == ESP_OK)

static BaseType_t generic_match(const twai_message_t* pattern, int data_len) {
    for (int i = 0; can_receive(&message) && i < 100; i++) {
        ESP_LOGI(tag, "received CAN frame");
        debug_log_message("receive", &message);
        debug_log_message("pattern", pattern);
        int match = pattern->identifier == message.identifier;
        match = match || pattern->data_length_code == message.data_length_code;
        match = match || pattern->flags == message.flags;
        for (int i = 0; match && i < data_len; i++) {
            match = match || pattern->data[i];
        }
        if (match) {
            ESP_LOGI(tag, "match successful");
            return pdTRUE;
        }
        ESP_LOGE(tag, "match failed");
        continue;
    }
    ESP_LOGE(tag, "receive failed");
    return pdFALSE;
}

static BaseType_t generic_read_continuation(int data_len) {
    static twai_message_t pattern = {
        .identifier = 0x7e8,
        .data_length_code = 8,
        .data = { 0 },
    };
    pattern.data[0] = 0x21;
    while (data_len > 0 && generic_match(&pattern, 1)) {
        ESP_LOGI(tag, "matched continuation");
        int sz = (data_len < 7) ? data_len : 7;
        memcpy(vin + (17 - data_len), pattern.data + 1, sz);
        data_len -= sz;
        pattern.data[0] = 0x20 | ((pattern.data[0] + 1) & 0xF);
    }
    if (!data_len) {
        ESP_LOGE(tag, "finished reading VIN");
        return pdTRUE;
    }
    ESP_LOGE(tag, "failed to match continuation");
    return pdFALSE;
}

static BaseType_t read_vin_09(void) {
    // request is 09 02
    // response is 49 02 01 31 32 33 34 ...
    ESP_LOGI(tag, "reading VIN with service 09");
    twai_clear_receive_queue();
    twai_clear_transmit_queue();
    static const twai_message_t request = {
        .identifier = 0x7e0,
        .data_length_code = 8,
        .data = { 0x02, 0x09, 0x02, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc },
    };
    debug_log_message("transmit", &request);
    if (can_transmit(&request)) {
        ESP_LOGI(tag, "sent request");
        static const twai_message_t ff_pattern = {
            .identifier = 0x7e8,
            .data_length_code = 8,
            .data = { 0x10, 0x14, 0x49, 0x02, 0x01 },
        };
        if (generic_match(&ff_pattern, 5)) {
            ESP_LOGI(tag, "successful response");
            memcpy(vin, message.data + 5, 3);
            debug_log_message("transmit", &generic_continuation);
            if (can_transmit(&generic_continuation)) {
                ESP_LOGI(tag, "sent continuation");
                return generic_read_continuation(14);
            }
            ESP_LOGE(tag, "failed to send continuation");
            return pdFALSE;
        }
        ESP_LOGE(tag, "failed to receive response");
        return pdFALSE;
    }
    ESP_LOGE(tag, "failed to send request");
    return pdFALSE;
}

static BaseType_t read_vin_22(void) {
    // request is 22 F1 90
    // response is 62 F1 90 31 32 33 34 ...
    ESP_LOGI(tag, "reading VIN with service 22");
    twai_clear_receive_queue();
    twai_clear_transmit_queue();
    static const twai_message_t request = {
        .identifier = 0x7e0,
        .data_length_code = 8,
        .data = { 0x03, 0x22, 0xf1, 0x90, 0xcc, 0xcc, 0xcc, 0xcc },
    };
    debug_log_message("transmit", &request);
    if (can_transmit(&request)) {
        ESP_LOGI(tag, "sent request");
        static const twai_message_t ff_pattern = {
            .identifier = 0x7e8,
            .data_length_code = 8,
            .data = { 0x10, 0x14, 0x62, 0xF1, 0x90 },
        };
        if (generic_match(&ff_pattern, 5)) {
            ESP_LOGI(tag, "successful response");
            memcpy(vin, message.data + 5, 3);
            debug_log_message("transmit", &generic_continuation);
            if (can_transmit(&generic_continuation)) {
                ESP_LOGI(tag, "sent continuation");
                return generic_read_continuation(14);
            }
            ESP_LOGE(tag, "failed to send continuation");
            return pdFALSE;
        }
        ESP_LOGE(tag, "failed to receive response");
        return pdFALSE;
    }
    ESP_LOGE(tag, "failed to send request");
    return pdFALSE;
}

static BaseType_t read_vin_1a(void) {
    // request is 1A 90
    // response is 5A 31 32 33 34 ...
    ESP_LOGI(tag, "reading VIN with service 1A");
    twai_clear_receive_queue();
    twai_clear_transmit_queue();
    static const twai_message_t request = {
        .identifier = 0x7e0,
        .data_length_code = 8,
        .data = { 0x02, 0x1a, 0x90, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc },
    };
    debug_log_message("transmit", &request);
    if (can_transmit(&request)) {
        ESP_LOGI(tag, "sent request");
        static const twai_message_t ff_pattern = {
            .identifier = 0x7e8,
            .data_length_code = 8,
            .data = { 0x10, 0x12, 0x5a },
        };
        if (generic_match(&ff_pattern, 3)) {
            ESP_LOGI(tag, "successful response");
            memcpy(vin, message.data + 3, 5);
            debug_log_message("transmit", &generic_continuation);
            if (can_transmit(&generic_continuation)) {
                ESP_LOGI(tag, "sent continuation");
                return generic_read_continuation(12);
            }
            ESP_LOGE(tag, "failed to send continuation");
            return pdFALSE;
        }
        ESP_LOGE(tag, "failed to receive response");
        return pdFALSE;
    }
    ESP_LOGE(tag, "failed to send request");
    return pdFALSE;
}

static void try_read_vin(void* arg) {
    ESP_LOGI(tag, "try_read_vin task entered");
    SemaphoreHandle_t local = (SemaphoreHandle_t)arg;
    BaseType_t bt = xSemaphoreTake(semaphore, portMAX_DELAY);
    assert(bt == pdTRUE);
    ESP_LOGI(tag, "mutex taken");

    for (;;) {
        if (read_vin_09() == pdTRUE) {
            break;
        }
        if (read_vin_22() == pdTRUE) {
            break;
        }
        if (read_vin_1a() == pdTRUE) {
            break;
        }
        ESP_LOGE(tag, "failed to read VIN, trying again in 3 seconds");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    xSemaphoreGive(semaphore);
    ESP_LOGI(tag, "mutex given");
    ESP_LOGI(tag, "try_read_vin task finished");
    vTaskDelete(NULL);
    ESP_LOGE(tag, "task failed to delete!");
    assert(0);
    for (;;) { }
}

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

    static StaticSemaphore_t semaphoreBuffer;
    semaphore = xSemaphoreCreateMutexStatic(&semaphoreBuffer);
    assert(semaphore);

    static StackType_t stackBuffer[2048];
    static StaticTask_t taskBuffer;
    (void)xTaskCreateStatic(try_read_vin, "try_read_vin", sizeof(stackBuffer) / sizeof(*stackBuffer), semaphore, 5, stackBuffer, &taskBuffer);
}
