#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string.h>

#include <omnitrix/libcan.h>
#include <omnitrix/libisotp.h>
#include <omnitrix/debug.h>
#include <omnitrix/led.h>

#include "isotp.h"

static const char tag[] = "omni_isotp";

static StackType_t isotp_task_stack[4096];
static StaticTask_t isotp_task_buffer;
static TaskHandle_t isotp_task_handle;

static uint8_t isotp_event_queue_storage[sizeof(struct isotp_event) * 4];
static StaticQueue_t isotp_event_queue_buffer;
QueueHandle_t isotp_event_queue_handle;

static uint8_t isotp_unmatched_frame_queue_storage[sizeof(struct twai_message_timestamp) * 4];
static StaticQueue_t isotp_unmatched_frame_queue_buffer;
static QueueHandle_t isotp_unmatched_frame_queue_handle;

static uint8_t isotp_msg_queue_storage[sizeof(struct isotp_msg) * 4];
static StaticQueue_t isotp_msg_queue_buffer;
static QueueHandle_t isotp_msg_queue_handle;

static StackType_t isotp_dispatch_stack[4096];
static StaticTask_t isotp_dispatch_buffer;
static TaskHandle_t isotp_dispatch_handle;

static StackType_t isotp_dispatch_u_stack[4096];
static StaticTask_t isotp_dispatch_u_buffer;
static TaskHandle_t isotp_dispatch_u_handle;

static omni_libisotp_incoming_handler* handlers[2] = { NULL, NULL };
static omni_libisotp_unmatched_handler* u_handlers[2] = { NULL, NULL };
static bool initialized = false;

static void get_next_event(struct isotp_event* evt) {
    assert(evt);
    BaseType_t ret = xQueueReceive(isotp_event_queue_handle, evt, portMAX_DELAY);
    assert(ret == pdTRUE);
}

static void unmatched_frame(const uint8_t* frame) {
    assert(frame);
    // NOTE: if this queue is full the frame will be dropped!
    xQueueSend(isotp_unmatched_frame_queue_handle, frame, 0);
}

static void write_frame(uint32_t id, uint8_t dlc, const uint8_t* data) {
    assert(data);
    omni_debug_log("ISOTP", "Writing frame - ID: 0x%lx, DLC: %d", (unsigned long)id, dlc);
    omni_led_data_transfer_start();  // Start LED indication
    twai_message_t msg = {
        .extd = (id & 0x80000000) != 0,
        .identifier = id & 0x1FFFFFFF,
        .data_length_code = dlc,
    };
    memcpy(msg.data, data, (dlc > 8) ? 8 : dlc);
    if (msg.extd) {
        ESP_LOGI(tag, "about to write frame: ID=%08" PRIX32 ", DLC=%X, DATA=%02X%02X%02X%02X%02X%02X%02X%02X, EXTD=T", msg.identifier, msg.data_length_code, msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
    } else {
        ESP_LOGI(tag, "about to write frame: ID=%03" PRIX32 ", DLC=%X, DATA=%02X%02X%02X%02X%02X%02X%02X%02X, EXTD=F", msg.identifier, msg.data_length_code, msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
    }
    esp_err_t result = twai_transmit(&msg, 0);
    switch (result) {
    case ESP_OK:
        ESP_LOGI(tag, "frame write successful");
        break;
    case ESP_ERR_INVALID_ARG:
        ESP_LOGE(tag, "frame write failed: invalid argument");
        break;
    case ESP_ERR_TIMEOUT:
        ESP_LOGE(tag, "frame write failed: timeout");
        break;
    case ESP_FAIL:
        ESP_LOGE(tag, "frame write failed: TX queue disabled and another message is currently transmitting");
        break;
    case ESP_ERR_INVALID_STATE:
        ESP_LOGE(tag, "frame write failed: driver is not running or installed");
        break;
    case ESP_ERR_NOT_SUPPORTED:
        ESP_LOGE(tag, "frame write failed: listen only mode does not support transmissions");
        break;
    default:
        ESP_LOGE(tag, "frame write failed: unknown error");
        break;
    }
    omni_led_data_transfer_stop();  // Stop LED indication
}

static void read_message_cb(const uint8_t* data, size_t size) {
    assert(data);
    assert(size <= 256);
    struct isotp_msg msg = {
        .size = size,
    };
    memcpy(msg.data, data, size);
    // NOTE: if this queue is full the message will be dropped!
    xQueueSend(isotp_msg_queue_handle, &msg, 0);
}

static void isotp_task(void* ptr) {
    (void)ptr;
    isotp_event_loop(get_next_event, unmatched_frame, write_frame, read_message_cb);
    vTaskDelete(NULL);
}

static void isotp_read_handler(struct twai_message_timestamp* msg) {
    struct isotp_event event = {
        .type = EVENT_INCOMING_CAN,
        .can = {
            .id = msg->msg.identifier | (msg->msg.extd << 31),
            .dlc = msg->msg.data_length_code,
        },
    };
    memcpy(event.can.data, msg->msg.data, (msg->msg.data_length_code < 8) ? msg->msg.data_length_code : 8);
    memcpy(event.can.frame, msg, sizeof(*msg));
    if (xQueueSend(isotp_event_queue_handle, &event, 0) == pdTRUE) {
        ESP_LOGI(tag, "queued incoming frame event");
    } else {
        ESP_LOGE(tag, "event queue error (full?)");
    }
}

static void isotp_dispatch(void* ptr) {
    (void)ptr;
    for (;;) {
        struct isotp_msg msg;
        if (xQueueReceive(isotp_msg_queue_handle, &msg, portMAX_DELAY) == pdTRUE) {
            omni_led_data_transfer_start();  // Start LED indication
            if (handlers[0]) {
                handlers[0](&msg);
            }
            if (handlers[1]) {
                handlers[1](&msg);
            }
            omni_led_data_transfer_stop();  // Stop LED indication
        
        }
    }
    vTaskDelete(NULL);
}

static void isotp_dispatch_u(void* ptr) {
    (void)ptr;
    for (;;) {
        struct twai_message_timestamp msg;
        if (xQueueReceive(isotp_unmatched_frame_queue_handle, &msg, portMAX_DELAY) == pdTRUE) {
            omni_led_data_transfer_start();  // Start LED indication
            if (u_handlers[0]) {
                u_handlers[0](&msg);
            }
            if (u_handlers[1]) {
                u_handlers[1](&msg);
            }
            omni_led_data_transfer_start();  // Start LED indication
        }
    }
    vTaskDelete(NULL);
}

void omni_libisotp_main(void) {
    if (!initialized) {
        omni_libcan_main();
        omni_libcan_add_incoming_handler(isotp_read_handler);
        isotp_event_queue_handle = xQueueCreateStatic(4, sizeof(struct isotp_event), isotp_event_queue_storage, &isotp_event_queue_buffer);
        isotp_unmatched_frame_queue_handle = xQueueCreateStatic(4, sizeof(struct twai_message_timestamp), isotp_unmatched_frame_queue_storage, &isotp_unmatched_frame_queue_buffer);
        isotp_msg_queue_handle = xQueueCreateStatic(4, sizeof(struct isotp_msg), isotp_msg_queue_storage, &isotp_msg_queue_buffer);
        isotp_task_handle = xTaskCreateStatic(isotp_task, "isotp_task", sizeof(isotp_task_stack) / sizeof(isotp_task_stack[0]), NULL, 9, isotp_task_stack, &isotp_task_buffer);
        isotp_dispatch_handle = xTaskCreateStatic(isotp_dispatch, "isotp_dispatch", sizeof(isotp_dispatch_stack) / sizeof(isotp_dispatch_stack[0]), NULL, 5, isotp_dispatch_stack, &isotp_dispatch_buffer);
        isotp_dispatch_handle = xTaskCreateStatic(isotp_dispatch_u, "isotp_dispatch_u", sizeof(isotp_dispatch_u_stack) / sizeof(isotp_dispatch_u_stack[0]), NULL, 5, isotp_dispatch_u_stack, &isotp_dispatch_u_buffer);
        initialized = true;
    }
}

void omni_libisotp_add_incoming_handler(omni_libisotp_incoming_handler* handler) {
    if (!handlers[0]) {
        handlers[0] = handler;
    } else if (!handlers[1]) {
        handlers[1] = handler;
    }
}

void omni_libisotp_add_unmatched_handler(omni_libisotp_unmatched_handler* handler) {
    if (!u_handlers[0]) {
        u_handlers[0] = handler;
    } else if (!u_handlers[1]) {
        u_handlers[1] = handler;
    }
}
