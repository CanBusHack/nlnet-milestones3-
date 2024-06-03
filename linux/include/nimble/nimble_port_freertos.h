#ifndef NIMBLE_NIMBLE_PORT_FREERTOS_H_
#define NIMBLE_NIMBLE_PORT_FREERTOS_H_

void nimble_port_freertos_init(void (*task)(void*));
void nimble_port_freertos_deinit(void);

#endif
