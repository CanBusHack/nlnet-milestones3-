
#include <driver/gpio.h>
#include <driver/rmt.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include <omnitrix/led.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <freertos/timers.h>

static const char* TAG = "omni:led";

// Single definition of LED_PIN
#ifdef CONFIG_OMNITRIX_ENABLE_LED
    #define LED_PIN CONFIG_OMNITRIX_LED_GPIO
#else
    #error "LED component requires CONFIG_OMNITRIX_ENABLE_LED to be enabled"
#endif

#define RMT_TX_CHANNEL RMT_CHANNEL_0

// WS2812B timing values
#define T0H  14  // 0 bit high time (350 ns @ 25 ns resolution)
#define T0L  34  // 0 bit low time (850 ns @ 25 ns resolution)
#define T1H  34  // 1 bit high time (850 ns @ 25 ns resolution)
#define T1L  14  // 1 bit low time (350 ns @ 25 ns resolution)


// Add new state variables
static led_state_t current_state = LED_STATE_POWERUP;
static TimerHandle_t error_timer = NULL;
static bool led_animation_running = false;
static TaskHandle_t led_task_handle = NULL;
static uint8_t ota_progress = 0;

// static const uint8_t COLOR_RED[] = {255, 0, 0};
// static const uint8_t COLOR_GREEN[] = {0, 255, 0};
// static const uint8_t COLOR_BLUE[] = {0, 0, 255};
// static const uint8_t COLOR_PURPLE[] = {128, 0, 128};
// static const uint8_t COLOR_WHITE[] = {255, 255, 255};

static uint8_t current_brightness = 50;  // Default 50% brightness

static void IRAM_ATTR ws2812_rmt_adapter(const void* src, rmt_item32_t* dest, size_t src_size,
                                       size_t wanted_num, size_t* translated_size, size_t* item_num)
{
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }
    const rmt_item32_t bit0 = {{{ T0H, 1, T0L, 0 }}};
    const rmt_item32_t bit1 = {{{ T1H, 1, T1L, 0 }}};
    size_t size = 0;
    size_t num = 0;
    const uint8_t *psrc = (const uint8_t *)src;
    rmt_item32_t* pdest = dest;
    while (size < src_size && num < wanted_num) {
        for (int i = 0; i < 8; i++) {
            if (*psrc & (1 << (7 - i))) {
                pdest->val = bit1.val;
            } else {
                pdest->val = bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}


esp_err_t omni_led_init(void)
{
    ESP_LOGI(TAG, "Initializing LED on GPIO %d", LED_PIN);
    
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_PIN, RMT_TX_CHANNEL);
    config.clk_div = 2;  // Set counter clock to 40MHz (25 ns resolution)

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    ESP_ERROR_CHECK(rmt_translator_init(config.channel, ws2812_rmt_adapter));

    // Initialize with LED off
    uint8_t led_off[3] = {0, 0, 0};  // GRB format
    rmt_write_sample(config.channel, led_off, 3, true);
    
    ESP_LOGI(TAG, "LED initialization complete");
    return ESP_OK;
}

static void set_led_raw(uint8_t red, uint8_t green, uint8_t blue)
{
    // Apply brightness adjustment and prepare data in GRB format
    uint8_t led_data[3] = {
        (green * current_brightness) / 100,
        (red * current_brightness) / 100,
        (blue * current_brightness) / 100
    };
    rmt_write_sample(RMT_TX_CHANNEL, led_data, 3, true);
}

void omni_led_set_debug(void)
{
    ESP_LOGD(TAG, "Setting LED to debug mode");
    set_led_raw(0, 0, 255);  // Blue
}

void omni_led_set_error(void)
{
    ESP_LOGD(TAG, "Setting LED to error state");
    set_led_raw(255, 0, 0);  // Red
}

void omni_led_set_active(void)
{
    ESP_LOGD(TAG, "Setting LED to active state");
    set_led_raw(0, 255, 0);  // Green
}

void omni_led_set_brightness(uint8_t brightness)
{
    current_brightness = (brightness > 100) ? 100 : brightness;
    ESP_LOGD(TAG, "LED brightness set to %u%%", current_brightness);
}

void omni_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    ESP_LOGD(TAG, "Setting LED color to R:%u G:%u B:%u", red, green, blue);
    set_led_raw(red, green, blue);
}


// Add breathing animation function
static void breathing_animation_task(void *pvParameters) {
    const int BREATH_CYCLE_MS = 2000;  // 2 second cycle
    
    while (led_animation_running) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        float phase = (float)(now % BREATH_CYCLE_MS) / BREATH_CYCLE_MS;
        float brightness = (1.0f - cosf(2.0f * M_PI * phase)) * 0.5f;
        
        if (current_state == LED_STATE_BLE_ADV) {
            // Blue breathing for BLE advertising
            uint8_t intensity = (uint8_t)(brightness * 255.0f);
            set_led_raw(0, 0, intensity);
        }
        else if (current_state == LED_STATE_OTA_PROGRESS) {
            // Purple breathing for OTA, intensity based on progress
            float progress_factor = ota_progress / 100.0f;
            uint8_t intensity = (uint8_t)(brightness * 255.0f * progress_factor);
            set_led_raw(intensity, 0, intensity);  // Purple = Red + Blue
        }
        
        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz update
    }
    
    led_task_handle = NULL;
    vTaskDelete(NULL);
}

// Add powerup sequence function
static void run_powerup_sequence(void) {
    // Red
    set_led_raw(255, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Blue
    set_led_raw(0, 0, 255);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Green
    set_led_raw(0, 255, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
}

// static void run_powerup_sequence(void) {
//     // Step 1: Pulsing Effect (Startup Indication)
//     for (int cycle = 0; cycle < 3; cycle++) { // Repeat pulse 3 times
//         // Brighten
//         for (int brightness = 0; brightness <= 255; brightness += 5) {
//             set_led_raw(0, brightness, 0); // Increase green brightness
//             vTaskDelay(pdMS_TO_TICKS(10));
//         }
//         // Dim
//         for (int brightness = 255; brightness >= 0; brightness -= 5) {
//             set_led_raw(0, brightness, 0); // Decrease green brightness
//             vTaskDelay(pdMS_TO_TICKS(10));
//         }
//     }

//     // Step 2: Solid Green (Ready State)
//     set_led_raw(0, 255, 0); // Full green brightness
// }




static void error_timer_callback(TimerHandle_t xTimer) {
    static bool state = false;
    state = !state;
    
    switch (current_state) {
        case LED_STATE_ERROR_CRITICAL:
            set_led_raw(state ? 255 : 0, 0, 0);  // Fast red flash
            break;
        case LED_STATE_ERROR_WARNING:
            // Smooth pulsing for warning
            uint8_t intensity = state ? 255 : 64;
            set_led_raw(intensity, 0, 0);
            break;
        default:
            break;
    }
}

// Add state management
void omni_led_set_state(led_state_t new_state) {
    if (new_state == current_state) return;
    
    ESP_LOGD(TAG, "LED state change: %d -> %d", current_state, new_state);
    
    // Stop any running animations
    if (led_animation_running) {
        led_animation_running = false;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    // Stop error timer if running
    if (error_timer != NULL) {
        xTimerStop(error_timer, 0);
    }
    
    current_state = new_state;
    
    switch (new_state) {
        case LED_STATE_POWERUP:
            // Red
            set_led_raw(255, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            // Blue
            set_led_raw(0, 0, 255);
            vTaskDelay(pdMS_TO_TICKS(500));
            // Green
            set_led_raw(0, 255, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            // Transition to BLE advertising
            omni_led_set_state(LED_STATE_BLE_ADV);
            break;
            
        case LED_STATE_ACTIVE:
            set_led_raw(0, 64, 0);  // Dim green (25%)
            break;
            
        case LED_STATE_BLE_ADV:
            led_animation_running = true;
            xTaskCreate(breathing_animation_task, "led_breath", 2048, NULL, 5, &led_task_handle);
            break;
            
        case LED_STATE_BLE_CONN:
            set_led_raw(0, 0, 255);  // Solid blue
            break;
            
        case LED_STATE_ERROR_CRITICAL:
            if (error_timer == NULL) {
                error_timer = xTimerCreate("error_timer", pdMS_TO_TICKS(250),
                                         pdTRUE, NULL, error_timer_callback);
            }
            xTimerStart(error_timer, 0);
            break;
            
        case LED_STATE_ERROR_WARNING:
            if (error_timer == NULL) {
                error_timer = xTimerCreate("error_timer", pdMS_TO_TICKS(1000),
                                         pdTRUE, NULL, error_timer_callback);
            }
            xTimerStart(error_timer, 0);
            break;
            
        case LED_STATE_ERROR_FAULT:
            set_led_raw(255, 0, 0);  // Solid red
            break;
            
        case LED_STATE_OTA_PROGRESS:
            led_animation_running = true;
            xTaskCreate(breathing_animation_task, "led_breath", 2048, NULL, 5, &led_task_handle);
            break;
            
        case LED_STATE_DATA_TRANSFER:
            set_led_raw(255, 255, 255);  // White flash
            break;
            
        case LED_STATE_CUSTOM:
            // Color already set by app
            break;
    }
}

led_state_t omni_led_get_state(void) {
    return current_state;
}


void omni_led_set_ota_progress(uint8_t progress) {
    ota_progress = progress;
    if (current_state != LED_STATE_OTA_PROGRESS) {
        omni_led_set_state(LED_STATE_OTA_PROGRESS);
    }
}

void omni_led_data_transfer_start(void) {
    omni_led_set_state(LED_STATE_DATA_TRANSFER);
}

void omni_led_data_transfer_stop(void) {
    omni_led_set_state(LED_STATE_ACTIVE);  // Return to normal operation
}

void omni_led_run_powerup(void) {
    ESP_LOGI(TAG, "Running power-up sequence");
    // Red
    set_led_raw(255, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Blue
    set_led_raw(0, 0, 255);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Green
    set_led_raw(0, 255, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
}


#ifdef CONFIG_OMNITRIX_ENABLE_BLE
void omni_led_handle_ble_state(bool connected) {
    if (connected) {
        omni_led_set_state(LED_STATE_BLE_CONN);
    } else if (current_state == LED_STATE_BLE_CONN) {
        omni_led_set_state(LED_STATE_BLE_ADV);
    }
}
#endif


// esp_err_t omni_led_init(void)
// {
//     ESP_LOGI(TAG, "Initializing LED on GPIO %d", LED_PIN);
    
//     rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_PIN, RMT_TX_CHANNEL);
//     config.clk_div = 2;  // Set counter clock to 40MHz (25 ns resolution)

//     ESP_ERROR_CHECK(rmt_config(&config));
//     ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
//     ESP_ERROR_CHECK(rmt_translator_init(config.channel, ws2812_rmt_adapter));

//     // Initialize with LED off
//     uint8_t led_off[3] = {0, 0, 0};  // GRB format
//     rmt_write_sample(config.channel, led_off, 3, true);

//     return ESP_OK;
// }

// esp_err_t omni_led_init(void)
// {
//     ESP_LOGI(TAG, "Initializing LED on GPIO %d", LED_PIN);
    
//     rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_PIN, RMT_TX_CHANNEL);
//     config.clk_div = 2;  // Set counter clock to 40MHz (25 ns resolution)

//     ESP_ERROR_CHECK(rmt_config(&config));
//     ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
//     ESP_ERROR_CHECK(rmt_translator_init(config.channel, ws2812_rmt_adapter));

//     // Run a quick test pattern to verify LED functionality
//     ESP_LOGI(TAG, "Running LED test pattern");
    
//     // Red
//     uint8_t led_data[3] = {0, 50, 0};  // GRB format, 20% brightness
//     rmt_write_sample(config.channel, led_data, 3, true);
//     vTaskDelay(pdMS_TO_TICKS(500));
    
//     // Green
//     led_data[0] = 50;  // G
//     led_data[1] = 0;   // R
//     led_data[2] = 0;   // B
//     rmt_write_sample(config.channel, led_data, 3, true);
//     vTaskDelay(pdMS_TO_TICKS(500));
    
//     // Blue
//     led_data[0] = 0;   // G
//     led_data[1] = 0;   // R
//     led_data[2] = 50;  // B
//     rmt_write_sample(config.channel, led_data, 3, true);
//     vTaskDelay(pdMS_TO_TICKS(500));

//     // Set initial state (dim green to indicate active)
//     led_data[0] = 10;  // G
//     led_data[1] = 0;   // R
//     led_data[2] = 0;   // B
//     rmt_write_sample(config.channel, led_data, 3, true);
    
//     ESP_LOGI(TAG, "LED initialization complete");
//     return ESP_OK;
// }