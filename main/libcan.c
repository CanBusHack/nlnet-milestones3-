#include <driver/gpio.h>
#include <driver/twai.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <omnitrix/libcan.h>

static const char tag[] = "omni_libcan";

static StackType_t can_reader_stack[4096];
static StaticTask_t can_reader_buffer;
static TaskHandle_t can_reader_handle;

static StackType_t can_dispatcher_stack[4096];
static StaticTask_t can_dispatcher_buffer;
static TaskHandle_t can_dispatcher_handle;

static uint8_t can_msg_queue_storage[sizeof(struct twai_message_timestamp) * 16];
static StaticQueue_t can_msg_queue_buffer;
static QueueHandle_t can_msg_queue_handle;

static uint32_t filter = 0xFFFFFFFF;
static uint32_t filters_or = 0;
static uint32_t mask = 0xFFFFFFFF;

static omni_libcan_incoming_handler* handlers[2] = { NULL, NULL };
static bool initialized = false;

static void can_dispatcher(void* ptr) {
    (void)ptr;
    for (;;) {
        struct twai_message_timestamp msg;
        if (xQueueReceive(can_msg_queue_handle, &msg, portMAX_DELAY) == pdTRUE) {
            if (handlers[0]) {
                handlers[0](&msg);
            }
            if (handlers[1]) {
                handlers[1](&msg);
            }
        }
    }
    vTaskDelete(NULL);
}

// for extreme debugging only, potentially a major performance hit
//#define CAN_LOGI(...) ESP_LOGI(__VA_ARGS__)
//#define CAN_LOGE(...) ESP_LOGE(__VA_ARGS__)
#define CAN_LOGI(...)
#define CAN_LOGE(...)

__attribute__((optimize("Ofast"))) static void can_reader(void* ptr) {
    (void)ptr;
    for (;;) {
        struct twai_message_timestamp msg;
        CAN_LOGI(tag, "waiting for next incoming frame...");
        esp_err_t result = twai_receive(&msg.msg, portMAX_DELAY);
        switch (result) {
        case ESP_OK: {
            if (msg.msg.extd) {
                CAN_LOGI(tag, "incoming frame received: ID=%08" PRIX32 ", DLC=%X, DATA=%02X%02X%02X%02X%02X%02X%02X%02X, EXTD=T", msg.msg.identifier, msg.msg.data_length_code, msg.msg.data[0], msg.msg.data[1], msg.msg.data[2], msg.msg.data[3], msg.msg.data[4], msg.msg.data[5], msg.msg.data[6], msg.msg.data[7]);
            } else {
                CAN_LOGI(tag, "incoming frame received: ID=%03" PRIX32 ", DLC=%X, DATA=%02X%02X%02X%02X%02X%02X%02X%02X, EXTD=F", msg.msg.identifier, msg.msg.data_length_code, msg.msg.data[0], msg.msg.data[1], msg.msg.data[2], msg.msg.data[3], msg.msg.data[4], msg.msg.data[5], msg.msg.data[6], msg.msg.data[7]);
            }
            uint32_t id = msg.msg.identifier | (msg.msg.extd ? 0x80000000 : 0);
            if ((id & mask) == filter) {
                CAN_LOGI(tag, "matched filter");
                gettimeofday(&msg.time, NULL);
                if (xQueueSend(can_msg_queue_handle, &msg, 0) == pdTRUE) {
                    CAN_LOGI(tag, "queued incoming frame event");
                } else {
                    CAN_LOGE(tag, "event queue error (full?)");
                }
            } else {
                CAN_LOGI(tag, "unmatched filter");
            }
            break;
        }
        case ESP_ERR_TIMEOUT:
            CAN_LOGE(tag, "frame read failed: timeout");
            break;
        case ESP_ERR_INVALID_ARG:
            CAN_LOGE(tag, "frame read failed: invalid argument");
            break;
        case ESP_ERR_INVALID_STATE:
            CAN_LOGE(tag, "frame read failed: driver is not running or installed");
            break;
        default:
            CAN_LOGE(tag, "frame read failed: unknown error");
            break;
        }
    }
    vTaskDelete(NULL);
}

static twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_34, TWAI_MODE_NORMAL);
static twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
static twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

static void reinstall(void) {
    vTaskDelete(can_reader_handle);
    twai_stop();
    twai_driver_uninstall();
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
    can_reader_handle = xTaskCreateStatic(
        can_reader,
        "can_reader",
        sizeof(can_reader_stack) / sizeof(can_reader_stack[0]),
        NULL,
        10,
        can_reader_stack,
        &can_reader_buffer);
}

void omni_libcan_main(void) {
    if (!initialized) {
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
        can_msg_queue_handle = xQueueCreateStatic(
            16,
            sizeof(struct twai_message_timestamp),
            can_msg_queue_storage,
            &can_msg_queue_buffer);
        can_reader_handle = xTaskCreateStatic(
            can_reader,
            "can_reader",
            sizeof(can_reader_stack) / sizeof(can_reader_stack[0]),
            NULL,
            10,
            can_reader_stack,
            &can_reader_buffer);
        can_dispatcher_handle = xTaskCreateStatic(
            can_dispatcher,
            "can_dispatcher",
            sizeof(can_dispatcher_stack) / sizeof(can_dispatcher_stack[0]),
            NULL,
            5,
            can_dispatcher_stack,
            &can_dispatcher_buffer);
        initialized = true;
    }
}

void omni_libcan_add_incoming_handler(omni_libcan_incoming_handler* handler) {
    if (!handlers[0]) {
        handlers[0] = handler;
    } else if (!handlers[1]) {
        handlers[1] = handler;
    }
}

void omni_libcan_add_filter(uint32_t id, bool extd) {
    filter &= id | (extd ? 0x80000000 : 0);
    filters_or |= id | (extd ? 0x80000000 : 0);
    mask = ~(filters_or & ~filter);
    ESP_LOGI(tag, "filter: %08" PRIX32 ", mask: %08" PRIX32, filter, mask);
}

void omni_libcan_clear_filter(void) {
    filter = 0xFFFFFFFF;
    filters_or = 0;
    mask = 0xFFFFFFFF;
}
