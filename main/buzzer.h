#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

/* Tạo timer one-shot cho buzzer (callback bên trong buzzer.c) */
TimerHandle_t xBuzzerCreateTimer( void );

/* Khởi tạo buzzer với GPIO & timer đã tạo ở app_main */
void vBuzzerInit( gpio_num_t xGpio, TimerHandle_t xTimer );

/* Bật / tắt thủ công nếu cần */
void vBuzzerOn( void );
void vBuzzerOff( void );

/* Beep trong xDurationTicks rồi tự tắt bằng timer */
void vBuzzerBeep( TickType_t xDurationTicks );

#ifdef __cplusplus
}
#endif
