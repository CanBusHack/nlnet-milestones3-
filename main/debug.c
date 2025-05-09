
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <esp_log.h>
#include <omnitrix/debug.h>

static const char* TAG = "omni_debug";

// primary UART used
void omni_debug_init(void) {
    ESP_LOGI(TAG, "Debug system initialized");
}

void omni_debug_log(const char* source, const char* format, ...) {
    char buffer[DEBUG_BUFFER_SIZE];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Format timestamp and source
    int len = snprintf(buffer, DEBUG_BUFFER_SIZE, "[%lld.%06ld][%s] ", 
                      (long long)tv.tv_sec, (long)tv.tv_usec, source);
    
    // Format message
    va_list args;
    va_start(args, format);
    vsnprintf(buffer + len, DEBUG_BUFFER_SIZE - len, format, args);
    va_end(args);

    // Use ESP logging system
    ESP_LOGI(TAG, "%s", buffer);
}