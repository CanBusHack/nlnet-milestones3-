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

esp_err_t twai_transmit(const twai_message_t* message, TickType_t ticks_to_wait);
esp_err_t twai_receive(twai_message_t* message, TickType_t ticks_to_wait);
esp_err_t twai_clear_transmit_queue(void);
esp_err_t twai_clear_receive_queue(void);

typedef struct {
    int rx;
    int tx;
    int mode;
} twai_general_config_t;

typedef struct {
    int baud;
} twai_timing_config_t;

typedef struct {
    int acceptance_code;
    int acceptance_mask;
    int single_filter;
} twai_filter_config_t;

twai_general_config_t TWAI_GENERAL_CONFIG_DEFAULT(int rx, int tx, int mode);
twai_timing_config_t TWAI_TIMING_CONFIG_500KBITS(void);

esp_err_t twai_driver_install(const twai_general_config_t* g_config, const twai_timing_config_t* t_config, const twai_filter_config_t* f_config);
esp_err_t twai_start(void);

enum {
    GPIO_NUM_33,
    GPIO_NUM_34,
    TWAI_MODE_NORMAL,
};

#endif
