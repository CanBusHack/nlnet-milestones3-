#ifndef OMNITRIX_LIBCAN_H_
#define OMNITRIX_LIBCAN_H_

#include <driver/twai.h>
#include <sys/time.h>

struct twai_message_timestamp {
    twai_message_t msg;
    struct timeval time;
};

typedef void omni_libcan_incoming_handler(struct twai_message_timestamp* msg);

void omni_libcan_main(void);
void omni_libcan_add_incoming_handler(omni_libcan_incoming_handler* handler);

#endif
