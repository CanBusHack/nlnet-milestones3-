#ifndef DRIVER_TWAI_H_
#define DRIVER_TWAI_H_

#include <esp_err.h>
#include <freertos/FreeRTOS.h>

typedef struct {
    int identifier;
    int flags;
    int data_length_code;
    unsigned char data[8];
} twai_message_t;

esp_err_t twai_transmit(const twai_message_t *message, TickType_t ticks_to_wait);
esp_err_t twai_receive(twai_message_t *message, TickType_t ticks_to_wait);
esp_err_t twai_clear_transmit_queue(void);
esp_err_t twai_clear_receive_queue(void);

#endif
