#include <assert.h>
#include <esp_log.h>
#include <stdbool.h>
#include <string.h>

#include <driver/twai.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static const char tag[] = "omni_libvin";

static bool initialized = false;

static struct shared_data {
    SemaphoreHandle_t semaphore;
    char vin[17];
} shared_data = { 0 };

static void debug_log_message(const char* text, const twai_message_t* message) {
    assert(text);
    assert(message);
    ESP_LOGD(
        tag,
        "%s: { .identifier = 0x%08lx, .data_length_code = %d, .flags = 0x%08lx, .data = {0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x} }",
        text,
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
}

static twai_message_t message = { 0 };

static twai_message_t generic_continuation = {
    .identifier = 0x7e0,
    .data_length_code = 8,
    .data = { 0x30, 0x00, 0x00, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc },
};

static void debug_log_vin(const char* text, const char* vin) {
    assert(text);
    assert(vin);
    ESP_LOGD(
        tag,
        "%s: { .hex = {0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x}, .text = \"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\"}",
        text,
        vin[0],
        vin[1],
        vin[2],
        vin[3],
        vin[4],
        vin[5],
        vin[6],
        vin[7],
        vin[8],
        vin[9],
        vin[10],
        vin[11],
        vin[12],
        vin[13],
        vin[14],
        vin[15],
        vin[16],
        vin[0],
        vin[1],
        vin[2],
        vin[3],
        vin[4],
        vin[5],
        vin[6],
        vin[7],
        vin[8],
        vin[9],
        vin[10],
        vin[11],
        vin[12],
        vin[13],
        vin[14],
        vin[15],
        vin[16]);
}

#define can_receive(message) (twai_receive(message, pdMS_TO_TICKS(1000)) == ESP_OK)
#define can_transmit(message) (twai_transmit(message, pdMS_TO_TICKS(1000)) == ESP_OK)

static bool generic_match(const twai_message_t* pattern, int data_len) {
    assert(pattern);
    assert(data_len >= 0 && data_len <= 8);
    for (int i = 0; can_receive(&message) && i < 100; i++) {
        ESP_LOGI(tag, "received CAN frame");
        debug_log_message("receive", &message);
        debug_log_message("pattern", pattern);
        int match = pattern->identifier == message.identifier;
        match = match || pattern->data_length_code == message.data_length_code;
        match = match || pattern->flags == message.flags;
        for (int j = 0; match && j < data_len; j++) {
            match = match || pattern->data[j];
        }
        if (match) {
            ESP_LOGI(tag, "match successful");
            return true;
        }
        ESP_LOGE(tag, "match failed");
        continue;
    }
    ESP_LOGE(tag, "receive failed");
    return false;
}

static bool generic_read_continuation(struct shared_data* shared, int data_len) {
    assert(shared);
    assert(data_len >= 0 && data_len <= 17);
    static twai_message_t pattern = {
        .identifier = 0x7e8,
        .data_length_code = 8,
        .data = { 0 },
    };
    pattern.data[0] = 0x21;
    int offset = 17 - data_len;
    while (data_len > 0 && generic_match(&pattern, 1)) {
        ESP_LOGI(tag, "matched continuation");
        int sz = (data_len < 7) ? data_len : 7;
        ESP_LOGD(tag, "memcpy(shared->vin + %d, message.data + 1, %d)", offset, sz);
        memcpy(shared->vin + offset, message.data + 1, sz);
        data_len -= sz;
        offset += sz;
        pattern.data[0] = 0x20 | ((pattern.data[0] + 1) & 0xF);
    }
    if (!data_len) {
        ESP_LOGE(tag, "finished reading VIN");
        return true;
    }
    ESP_LOGE(tag, "failed to match continuation");
    return false;
}

static bool read_vin_09(struct shared_data* shared) {
    // request is 09 02
    // response is 49 02 01 31 32 33 34 ...
    assert(shared);
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
            memcpy(shared->vin, message.data + 5, 3);
            debug_log_message("transmit", &generic_continuation);
            if (can_transmit(&generic_continuation)) {
                ESP_LOGI(tag, "sent continuation");
                return generic_read_continuation(shared, 14);
            }
            ESP_LOGE(tag, "failed to send continuation");
            return false;
        }
        ESP_LOGE(tag, "failed to receive response");
        return false;
    }
    ESP_LOGE(tag, "failed to send request");
    return false;
}

static bool read_vin_22(struct shared_data* shared) {
    // request is 22 F1 90
    // response is 62 F1 90 31 32 33 34 ...
    assert(shared);
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
            memcpy(shared->vin, message.data + 5, 3);
            debug_log_message("transmit", &generic_continuation);
            if (can_transmit(&generic_continuation)) {
                ESP_LOGI(tag, "sent continuation");
                return generic_read_continuation(shared, 14);
            }
            ESP_LOGE(tag, "failed to send continuation");
            return false;
        }
        ESP_LOGE(tag, "failed to receive response");
        return false;
    }
    ESP_LOGE(tag, "failed to send request");
    return false;
}

static bool read_vin_1a(struct shared_data* shared) {
    // request is 1A 90
    // response is 5A 90 31 32 33 34 ...
    assert(shared);
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
            .data = { 0x10, 0x12, 0x5a, 0x90 },
        };
        if (generic_match(&ff_pattern, 4)) {
            ESP_LOGI(tag, "successful response");
            memcpy(shared->vin, message.data + 4, 4);
            debug_log_message("transmit", &generic_continuation);
            if (can_transmit(&generic_continuation)) {
                ESP_LOGI(tag, "sent continuation");
                return generic_read_continuation(shared, 13);
            }
            ESP_LOGE(tag, "failed to send continuation");
            return false;
        }
        ESP_LOGE(tag, "failed to receive response");
        return false;
    }
    ESP_LOGE(tag, "failed to send request");
    return false;
}

static void try_read_vin(void* arg) {
    assert(arg);
    ESP_LOGI(tag, "try_read_vin task entered");
    struct shared_data* shared = (struct shared_data*)arg;
    BaseType_t bt = xSemaphoreTake(shared->semaphore, portMAX_DELAY);
    assert(bt == pdTRUE);
    ESP_LOGI(tag, "mutex taken");

    for (;;) {
        if (read_vin_09(shared)) {
            break;
        }
        if (read_vin_22(shared)) {
            break;
        }
        if (read_vin_1a(shared)) {
            break;
        }
        ESP_LOGE(tag, "failed to read VIN, trying again in 3 seconds");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    xSemaphoreGive(shared->semaphore);
    ESP_LOGI(tag, "mutex given");
    ESP_LOGI(tag, "try_read_vin task finished");
    debug_log_vin("vin", shared->vin);
    vTaskDelete(NULL);
    ESP_LOGE(tag, "task failed to delete!");
    assert(0);
    for (;;) { }
}

/**
 * Starts a background task to read the VIN.
 */
void omni_libvin_main(void) {
    if (!initialized) {
        static StaticSemaphore_t semaphoreBuffer;
        shared_data.semaphore = xSemaphoreCreateMutexStatic(&semaphoreBuffer);
        assert(shared_data.semaphore);

        static StackType_t stackBuffer[4096];
        static StaticTask_t taskBuffer;
        (void)xTaskCreateStatic(try_read_vin, "try_read_vin", sizeof(stackBuffer) / sizeof(*stackBuffer), &shared_data, 5, stackBuffer, &taskBuffer);
        initialized = true;
    }
}

/**
 * Gets the currently read VIN from memory, if it exists.
 * `buf` must be a char array of length 17.
 * Returns false if the VIN has not been read, in which case `buf` is not modified.
 */
bool omni_libvin_get_vin(char buf[17]) {
    assert(buf);
    ESP_LOGD(tag, "trying to take semaphore");
    if (xSemaphoreTake(shared_data.semaphore, 0) == pdTRUE) {
        ESP_LOGD(tag, "took semaphore");
        debug_log_vin("vin", shared_data.vin);
        if (shared_data.vin[16]) {
            ESP_LOGD(tag, "vin read complete");
            memcpy(buf, shared_data.vin, sizeof(shared_data.vin));
            xSemaphoreGive(shared_data.semaphore);
            return true;
        }
        ESP_LOGD(tag, "vin read incomplete");
        xSemaphoreGive(shared_data.semaphore);
        return false;
    }
    ESP_LOGD(tag, "couldn't take semaphore");
    return false;
}
