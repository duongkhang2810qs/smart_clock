#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

/* Init DHT (chỉ lưu GPIO, chuẩn bị cho task đọc) */
void vSensorDhtInit( gpio_num_t xGpio );

/* Task đọc DHT định kỳ, lưu giá trị mới nhất */
void vSensorDhtTask( void *pvParameters );

/* Lấy giá trị nhiệt độ/độ ẩm mới nhất (pdTRUE nếu đã có) */
BaseType_t xSensorDhtGetLatest( float *pfTemp, float *pfHumid );

#ifdef __cplusplus
}
#endif
