#ifndef OMNITRIX_LIBISOTP_H_
#define OMNITRIX_LIBISOTP_H_

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stddef.h>
#include <stdint.h>

#include <omnitrix/libcan.h>

struct isotp_msg {
    uint32_t channel;
    size_t size;
    uint8_t data[256];
};

typedef void omni_libisotp_incoming_handler(struct isotp_msg* msg);
typedef void omni_libisotp_unmatched_handler(struct twai_message_timestamp* msg);

extern QueueHandle_t isotp_event_queue_handle;

void omni_libisotp_main(void);
void omni_libisotp_add_incoming_handler(omni_libisotp_incoming_handler* handler);
void omni_libisotp_add_unmatched_handler(omni_libisotp_unmatched_handler* handler);

#endif
