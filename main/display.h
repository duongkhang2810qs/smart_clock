#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"

/* Các chế độ hiển thị trên ma trận 8x32 */
typedef enum
{
    eDisplayScreenTime = 0,       /* HH:MM */
    eDisplayScreenWeekday,        /* Thứ: SUN/MON/... */
    eDisplayScreenDate,           /* DD-MM */
    eDisplayScreenYear,           /* YYYY */
    eDisplayScreenTempHumid,      /* TT HH (nhiệt độ – ẩm) */
    eDisplayScreenCount
} eDisplayScreen_t;

/* Init phần cứng (SPI + MAX7219) */
void vDisplayHardwareInit( gpio_num_t xGpioMosi, gpio_num_t xGpioSlck, gpio_num_t xGpioCs );

/* Task hiển thị, nhận lệnh đổi mode qua Queue (pvParameters = QueueHandle_t) */
void vDisplayTask( void *pvParameters );

#ifdef __cplusplus
}
#endif
