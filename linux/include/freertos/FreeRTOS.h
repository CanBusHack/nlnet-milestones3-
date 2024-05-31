#ifndef FREERTOS_FREERTOS_H_
#define FREERTOS_FREERTOS_H_

#include <stdint.h>

typedef unsigned int UBaseType_t;
typedef int BaseType_t;
typedef int TickType_t;

typedef void TaskFunction_t(void*);
typedef unsigned char StackType_t;
typedef unsigned char StaticTask_t;
typedef void* TaskHandle_t;

BaseType_t xTaskCreateStatic(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t ulStackDepth, void *const pvParameters, UBaseType_t uxPriority, StackType_t *const puxStackBuffer, StaticTask_t *const pxTaskBuffer);
void vTaskDelete(TaskHandle_t xTaskToDelete);
void vTaskDelay(TickType_t delay);

enum {
    pdFALSE,
    pdTRUE,
};

typedef unsigned char StaticSemaphore_t;
typedef void* SemaphoreHandle_t;

SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t* semaphore);
BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore);
BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, TickType_t delay);

enum {
    portMAX_DELAY = 0x7FFFFFFF,
};

#define pdMS_TO_TICKS(x) (x)

#endif
