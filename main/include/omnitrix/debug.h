#ifndef OMNITRIX_DEBUG_H_
#define OMNITRIX_DEBUG_H_

#include <stdint.h>
#include <driver/uart.h>
#include <sys/time.h>

// Debug UART configurations
#define DEBUG_UART_NUM UART_NUM_1
#define DEBUG_TXD_PIN 22
#define DEBUG_RXD_PIN 23
#define DEBUG_UART_BAUD_RATE 115200
#define DEBUG_BUFFER_SIZE 1024

// Initialize debug UART interface
void omni_debug_init(void);

// Log debug message with timestamp and source
void omni_debug_log(const char* source, const char* format, ...);

// Log BLE specific events
void omni_debug_ble_event(const char* event_type, const char* format, ...);

// Log CAN specific events
void omni_debug_can_event(const char* event_type, uint32_t id, const uint8_t* data, size_t len);

// Log ISOTP specific events
void omni_debug_isotp_event(const char* event_type, const char* format, ...);

#endif