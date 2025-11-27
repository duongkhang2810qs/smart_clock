#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "display.h"

/* Tham số truyền cho vButtonTask từ app_main */
typedef struct
{
    gpio_num_t    xButtonGpio;
    QueueHandle_t xModeQueue;     /* queue chứa eDisplayScreen_t */
} ButtonTaskParams_t;

void vButtonInit( gpio_num_t xGpio );

/* pvParameters = (ButtonTaskParams_t *) */
void vButtonTask( void *pvParameters );

#ifdef __cplusplus
}
#endif
