#ifndef OMNI_HELLO_ISOTP_H_
#define OMNI_HELLO_ISOTP_H_

#include <stddef.h>
#include <stdint.h>

#define ISOTP_MAX_PAIRS 2

enum isotp_event_type {
    EVENT_RECONFIGURE_PAIRS,
    EVENT_RECONFIGURE_BS_STMIN,
    EVENT_WRITE_MSG,
    EVENT_INCOMING_CAN,
    EVENT_SHUTDOWN,
};

struct isotp_event {
    enum isotp_event_type type;
    union {
        struct {
            size_t size;
            uint8_t data[252];
        } pairs;
        struct {
            uint8_t data[2];
        } bs_stmin;
        struct {
            size_t size;
            uint8_t data[256];
        } msg;
        struct {
            uint32_t id;
            uint8_t dlc;
            uint8_t data[8];
            uint8_t frame[72]; // big enough to hold a CAN-FD frame on Linux
        } can;
    };
};

typedef void isotp_event_cb(struct isotp_event*);
typedef void isotp_unmatched_frame(const uint8_t* frame);
typedef void isotp_write_frame(uint32_t id, uint8_t dlc, const uint8_t* data);
typedef void isotp_read_message_cb(const uint8_t* data, size_t size);

void isotp_event_loop(isotp_event_cb* get_next_event, isotp_unmatched_frame* unmatched_frame, isotp_write_frame* write_frame, isotp_read_message_cb* read_message_cb);

#endif
