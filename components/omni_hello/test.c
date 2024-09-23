#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "isotp.h"

static void no_unmatched_frame(void* frame) {
    assert(0);
}

static void no_write_frame(uint32_t id, uint8_t dlc, const uint8_t* data) {
    assert(0);
}

static void no_read_message_cb(const uint8_t* data, size_t size) {
    assert(0);
}

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
                    .data = { 0x00, 0x00, 0x07, 0xDF, 0x09, 0x02 },
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

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, no_read_message_cb);
    assert(read_id == 0x7DF);
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
    uint8_t read_frame[4] = { 0 };

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0x10, 0x14, 0x49, 0x02, 0x01, 0x41, 0x42, 0x43 },
                    .frame = { 0xDE, 0xAD, 0xBE, 0xEF },
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

    void unmatched_frame(const uint8_t* frame) {
        static int count = 0;
        assert(count < 1);
        count++;
        memcpy(read_frame, frame, sizeof(read_frame));
    }

    isotp_event_loop(get_next_event, unmatched_frame, no_write_frame, no_read_message_cb);
    assert(read_frame[0] == 0xDE);
    assert(read_frame[1] == 0xAD);
    assert(read_frame[2] == 0xBE);
    assert(read_frame[3] == 0xEF);
}

static void test_read_multi_normal(void) {
    uint8_t read_data[256] = { 0 };
    size_t read_size = 0;

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

    void write_frame(uint32_t id, uint8_t dlc, const uint8_t* data) {
        static int count = 0;
        assert(count < 1);
        count++;
        assert(id == 0x7E0);
        assert(dlc == 8);
        assert(data[0] == 0x30);
        assert(data[1] == 0);
        assert(data[2] == 0);
        assert(data[3] == 0xCC);
        assert(data[4] == 0xCC);
        assert(data[5] == 0xCC);
        assert(data[6] == 0xCC);
        assert(data[7] == 0xCC);
    }

    void read_message_cb(const uint8_t* data, size_t size) {
        static int count = 0;
        assert(count < 1);
        count++;
        assert(size <= 256);
        memcpy(read_data, data, size);
        read_size = size;
    }

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, read_message_cb);
    assert(read_size == 24);
    uint8_t expected_data[] = "\x00\x00\x07\xE8\x49\x02\x01"
                              "ABCDEFGHIJKLMNOPQ";
    assert(!memcmp(read_data, expected_data, 24));
}

static void test_read_multi_extended(void) {
    uint8_t read_data[256] = { 0 };
    size_t read_size = 0;

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_RECONFIGURE_PAIRS,
                .pairs = {
                    .size = 12,
                    .data = { 0x60, 0x00, 0x07, 0xE0, 0xFE, 0x42, 0x60, 0x00, 0x07, 0xE8, 0xEF, 0x24 },
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0xEF, 0x10, 0x14, 0x49, 0x02, 0x01, 0x41, 0x42 },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0xEF, 0x21, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48 },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0xEF, 0x22, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0xEF, 0x23, 0x4F, 0x50, 0x51, 0x24, 0x24, 0x24 },
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

    void write_frame(uint32_t id, uint8_t dlc, const uint8_t* data) {
        static int count = 0;
        assert(count < 1);
        count++;
        assert(id == 0x7E0);
        assert(dlc == 8);
        assert(data[0] == 0xFE);
        assert(data[1] == 0x30);
        assert(data[2] == 0);
        assert(data[3] == 0);
        assert(data[4] == 0x42);
        assert(data[5] == 0x42);
        assert(data[6] == 0x42);
        assert(data[7] == 0x42);
    }

    void read_message_cb(const uint8_t* data, size_t size) {
        static int count = 0;
        assert(count < 1);
        count++;
        assert(size <= 256);
        memcpy(read_data, data, size);
        read_size = size;
    }

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, read_message_cb);
    assert(read_size == 25);
    uint8_t expected_data[] = "\x00\x00\x07\xE8\xEF\x49\x02\x01"
                              "ABCDEFGHIJKLMNOPQ";
    assert(!memcmp(read_data, expected_data, 25));
}

static void test_read_multi_normal_nopad(void) {
    uint8_t read_data[256] = { 0 };
    size_t read_size = 0;

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_RECONFIGURE_PAIRS,
                .pairs = {
                    .size = 12,
                    .data = { 0x00, 0x00, 0x07, 0xE0, 0x00, 0xCC, 0x00, 0x00, 0x07, 0xE8, 0x00, 0xCC },
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

    void write_frame(uint32_t id, uint8_t dlc, const uint8_t* data) {
        static int count = 0;
        assert(count < 1);
        count++;
        assert(id == 0x7E0);
        assert(dlc == 3);
        assert(data[0] == 0x30);
        assert(data[1] == 0);
        assert(data[2] == 0);
    }

    void read_message_cb(const uint8_t* data, size_t size) {
        static int count = 0;
        assert(count < 1);
        count++;
        assert(size <= 256);
        memcpy(read_data, data, size);
        read_size = size;
    }

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, read_message_cb);
    assert(read_size == 24);
    uint8_t expected_data[] = "\x00\x00\x07\xE8\x49\x02\x01"
                              "ABCDEFGHIJKLMNOPQ";
    assert(!memcmp(read_data, expected_data, 24));
}

static void test_read_multi_extended_nopad(void) {
    uint8_t read_data[256] = { 0 };
    size_t read_size = 0;

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_RECONFIGURE_PAIRS,
                .pairs = {
                    .size = 12,
                    .data = { 0x40, 0x00, 0x07, 0xE0, 0xFE, 0x42, 0x40, 0x00, 0x07, 0xE8, 0xEF, 0x24 },
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0xEF, 0x10, 0x14, 0x49, 0x02, 0x01, 0x41, 0x42 },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0xEF, 0x21, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48 },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 8,
                    .data = { 0xEF, 0x22, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x7e8,
                    .dlc = 5,
                    .data = { 0xEF, 0x23, 0x4F, 0x50, 0x51 },
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

    void write_frame(uint32_t id, uint8_t dlc, const uint8_t* data) {
        static int count = 0;
        assert(count < 1);
        count++;
        assert(id == 0x7E0);
        assert(dlc == 4);
        assert(data[0] == 0xFE);
        assert(data[1] == 0x30);
        assert(data[2] == 0);
        assert(data[3] == 0);
    }

    void read_message_cb(const uint8_t* data, size_t size) {
        static int count = 0;
        assert(count < 1);
        count++;
        assert(size <= 256);
        memcpy(read_data, data, size);
        read_size = size;
    }

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, read_message_cb);
    assert(read_size == 25);
    uint8_t expected_data[] = "\x00\x00\x07\xE8\xEF\x49\x02\x01"
                              "ABCDEFGHIJKLMNOPQ";
    assert(!memcmp(read_data, expected_data, 25));
}

void test_send_sf_normal(void) {
    uint32_t read_id = 0;
    uint8_t read_dlc = 0;
    uint8_t read_data[8] = { 0 };

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_RECONFIGURE_PAIRS,
                .pairs = {
                    .size = 12,
                    .data = { 0x20, 0x00, 0x07, 0xE0, 0x00, 0x42, 0x20, 0x00, 0x07, 0xE8, 0x00, 0x24 },
                },
            },
            {
                .type = EVENT_WRITE_MSG,
                .msg = {
                    .size = 6,
                    .data = { 0x00, 0x00, 0x07, 0xE0, 0x09, 0x02 },
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

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, no_read_message_cb);
    assert(read_id == 0x7E0);
    assert(read_dlc == 8);
    assert(read_data[0] == 2);
    assert(read_data[1] == 9);
    assert(read_data[2] == 2);
    assert(read_data[3] == 0x42);
    assert(read_data[4] == 0x42);
    assert(read_data[5] == 0x42);
    assert(read_data[6] == 0x42);
    assert(read_data[7] == 0x42);
}

void test_send_sf_normal_nopad(void) {
    uint32_t read_id = 0;
    uint8_t read_dlc = 0;
    uint8_t read_data[8] = { 0 };

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_RECONFIGURE_PAIRS,
                .pairs = {
                    .size = 12,
                    .data = { 0x00, 0x00, 0x07, 0xE0, 0x00, 0x42, 0x00, 0x00, 0x07, 0xE8, 0x00, 0x24 },
                },
            },
            {
                .type = EVENT_WRITE_MSG,
                .msg = {
                    .size = 6,
                    .data = { 0x00, 0x00, 0x07, 0xE0, 0x09, 0x02 },
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

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, no_read_message_cb);
    assert(read_id == 0x7E0);
    assert(read_dlc == 3);
    assert(read_data[0] == 2);
    assert(read_data[1] == 9);
    assert(read_data[2] == 2);
}

void test_send_sf_extended(void) {
    uint32_t read_id = 0;
    uint8_t read_dlc = 0;
    uint8_t read_data[8] = { 0 };

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_RECONFIGURE_PAIRS,
                .pairs = {
                    .size = 12,
                    .data = { 0x60, 0x00, 0x07, 0xE0, 0x21, 0x42, 0x60, 0x00, 0x07, 0xE8, 0x12, 0x24 },
                },
            },
            {
                .type = EVENT_WRITE_MSG,
                .msg = {
                    .size = 7,
                    .data = { 0x00, 0x00, 0x07, 0xE0, 0x21, 0x09, 0x02 },
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

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, no_read_message_cb);
    assert(read_id == 0x7E0);
    assert(read_dlc == 8);
    assert(read_data[0] == 0x21);
    assert(read_data[1] == 2);
    assert(read_data[2] == 9);
    assert(read_data[3] == 2);
    assert(read_data[4] == 0x42);
    assert(read_data[5] == 0x42);
    assert(read_data[6] == 0x42);
    assert(read_data[7] == 0x42);
}

void test_send_sf_extended_nopad(void) {
    uint32_t read_id = 0;
    uint8_t read_dlc = 0;
    uint8_t read_data[8] = { 0 };

    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_RECONFIGURE_PAIRS,
                .pairs = {
                    .size = 12,
                    .data = { 0x40, 0x00, 0x07, 0xE0, 0x21, 0x42, 0x40, 0x00, 0x07, 0xE8, 0x12, 0x24 },
                },
            },
            {
                .type = EVENT_WRITE_MSG,
                .msg = {
                    .size = 7,
                    .data = { 0x00, 0x00, 0x07, 0xE0, 0x21, 0x09, 0x02 },
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

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, no_read_message_cb);
    assert(read_id == 0x7E0);
    assert(read_dlc == 4);
    assert(read_data[0] == 0x21);
    assert(read_data[1] == 2);
    assert(read_data[2] == 9);
    assert(read_data[3] == 2);
}

void test_can_log(void) {
    void get_next_event(struct isotp_event* evt) {
        static const struct isotp_event events[] = {
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x9FFFFFFF,
                    .dlc = 2,
                    .data = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                    .frame = (void*)(uintptr_t)0xDEADBEEF,
                },
            },
            {
                .type = EVENT_WRITE_MSG,
                .msg = {
                    .size = 6,
                    .data = { 0x00, 0x00, 0x07, 0xE0, 0x09, 0x02 },
                },
            },
            {
                .type = EVENT_INCOMING_CAN,
                .can = {
                    .id = 0x9FFFFFFF,
                    .dlc = 2,
                    .data = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
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

    void write_frame(uint32_t id, uint8_t dlc, const uint8_t* data) {
        static int count = 0;
        switch (count) {
        case 0:
            assert(id == 0x9FFFFFFE);
            assert(dlc == 1);
            assert(data[0] == 1);
            break;
        case 1:
            assert(id == 0x7E0);
            assert(dlc == 8);
            assert(data[0] == 0x02);
            assert(data[1] == 0x09);
            assert(data[2] == 0x02);
            assert(data[3] == 0xCC);
            assert(data[4] == 0xCC);
            assert(data[5] == 0xCC);
            assert(data[6] == 0xCC);
            assert(data[7] == 0xCC);
            break;
        case 2:
            assert(id == 0x9FFFFFFE);
            assert(dlc == 1);
            assert(data[0] == 0);
            break;
        default:
            assert(count < 3);
            break;
        }
        count++;
    }

    isotp_event_loop(get_next_event, no_unmatched_frame, write_frame, no_read_message_cb);
}

int main(int argc, char** argv) {
    test_send_sf_no_configure();
    test_read_unmatched();
    test_read_multi_normal();
    test_read_multi_extended();
    test_read_multi_normal_nopad();
    test_read_multi_extended_nopad();
    test_send_sf_normal();
    test_send_sf_normal_nopad();
    test_send_sf_extended();
    test_send_sf_extended_nopad();
    test_can_log();
}
