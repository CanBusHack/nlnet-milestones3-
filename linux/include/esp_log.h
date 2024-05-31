#ifndef ESP_LOG_H_
#define ESP_LOG_H_

void ESP_LOGD(const char* tag, const char* format, ...);
void ESP_LOGI(const char* tag, const char* format, ...);
void ESP_LOGE(const char* tag, const char* format, ...);
void ESP_LOGW(const char* tag, const char* format, ...);

#endif
