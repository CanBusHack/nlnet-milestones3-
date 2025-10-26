#pragma once

#include <esp_err.h>
#include "sdkconfig.h"

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_gatt.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LED state enumeration
 */
typedef enum {
    LED_STATE_POWERUP,      // Initial bootup sequence--RED,blue,green cycle
    LED_STATE_ACTIVE,       // Normal operation (dim green)
    LED_STATE_BLE_ADV,      // BLE advertising (breathing blue)
    LED_STATE_BLE_CONN,     // BLE connected (solid blue)
    LED_STATE_ERROR_CRITICAL,   // Fast red flash
    LED_STATE_ERROR_WARNING,    // Slow red pulse
    LED_STATE_ERROR_FAULT,      // Solid red
    LED_STATE_OTA_PROGRESS,     // Purple breathing
    LED_STATE_DATA_TRANSFER,    // White flash overlay
    LED_STATE_CUSTOM        // App-controlled color

} led_state_t;

/**
 * Initialize the LED subsystem
 */
esp_err_t omni_led_init(void);

/**
 * Set LED state
 */
void omni_led_set_state(led_state_t state);

/**
 * Get current LED state
 */
led_state_t omni_led_get_state(void);

/**
 * Set LED to indicate debug mode
 */
void omni_led_set_debug(void);

/**
 * Set LED to indicate error state
 */
void omni_led_set_error(void);

/**
 * Set LED to indicate active/normal operation
 */
void omni_led_set_active(void);

/**
 * Set LED brightness (0-100)
 */
void omni_led_set_brightness(uint8_t brightness);

/**
 * Set custom LED color
 */
void omni_led_set_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * Initialize LED component
 */
void omni_led_main(void);

/**
 * Run power-up sequence
 */
void omni_led_run_powerup(void);

void omni_led_set_state(led_state_t new_state);
void omni_led_set_ota_progress(uint8_t progress);
void omni_led_data_transfer_start(void);
void omni_led_data_transfer_stop(void);

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
/**
 * BLE service definition for LED control
 */
extern const struct ble_gatt_svc_def omni_led_gatt_svr_svcs[];

/**
 * Handle BLE connection state changes
 */
void omni_led_handle_ble_state(bool connected);
#endif

#ifdef __cplusplus
}
#endif