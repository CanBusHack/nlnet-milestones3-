#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "isotp.h"

static void test_send_sf_no_configure(void) {
    uint32_t read_id = 0;
    uint8_t read_dlc = 0;
    uint8_t read_data[8] = { 0 };

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_WRITE_MSG,
                .msg = {
                    .size = 6,
                    .data = { 0x00, 0x00, 0x07, 0xdf, 0x09, 0x02 },
                },
            },
            {
                .type = EVENT_SHUTDOWN,
            },
        };
        static int index = 0;
        assert(index < (sizeof(events) / sizeof(events[0])));
        memcpy(evt, &events[index++], sizeof(*evt));
    }

    void write_frame(uint32_t id, uint8_t dlc, const uint8_t* data) {
        static int count = 0;
        assert(count < 1);
        count++;
        read_id = id;
        read_dlc = dlc;
        memcpy(read_data, data, 8);
    }

    isotp_event_loop(get_next_event, NULL, write_frame, NULL);
    assert(read_id == 0x7df);
    assert(read_dlc == 8);
    assert(read_data[0] == 2);
    assert(read_data[1] == 9);
    assert(read_data[2] == 2);
    assert(read_data[3] == 0xCC);
    assert(read_data[4] == 0xCC);
    assert(read_data[5] == 0xCC);
    assert(read_data[6] == 0xCC);
    assert(read_data[7] == 0xCC);
}

static void test_read_unmatched(void) {
    uintptr_t read_frame = 0;

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0x10, 0x14, 0x49, 0x02, 0x01, 0x41, 0x42, 0x43 },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_SHUTDOWN,
            },
        };
        static int index = 0;
        assert(index < (sizeof(events) / sizeof(events[0])));
        memcpy(evt, &events[index++], sizeof(*evt));
    }

    void unmatched_frame(void* frame) {
        static int count = 0;
        assert(count < 1);
        count++;
        read_frame = (uintptr_t)frame;
    }

    isotp_event_loop(get_next_event, unmatched_frame, NULL, NULL);
    assert(read_frame == 0xDEADBEEF);
}

static void test_read_multi(void) {
    uint8_t read_data[256] = { 0 };
    size_t read_size;

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_RECONFIGURE_PAIRS,
                .pairs = {
                    .size = 12,
                    .data = { 0x20, 0x00, 0x07, 0xE0, 0x00, 0xCC, 0x20, 0x00, 0x07, 0xE8, 0x00, 0xCC },
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0x10, 0x14, 0x49, 0x02, 0x01, 0x41, 0x42, 0x43 },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0x21, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0x22, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51 },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_SHUTDOWN,
            },
        };
        static int index = 0;
        assert(index < (sizeof(events) / sizeof(events[0])));
        memcpy(evt, &events[index++], sizeof(*evt));
    }

    void read_message_cb(const uint8_t* data, size_t size) {
        assert(size <= 256);
        memcpy(read_data, data, size);
        read_size = size;
    }

    isotp_event_loop(get_next_event, NULL, NULL, read_message_cb);
    assert(read_size == 24);
    uint8_t expected_data[] = "\x00\x00\x07\xE8\x49\x02\x01"
                              "ABCDEFGHIJKLMNOPQ";
    assert(!memcmp(read_data, expected_data, 24));
}

int main(int argc, char** argv) {
    test_send_sf_no_configure();
    test_read_unmatched();
    test_read_multi();
}
