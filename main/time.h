#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_err.h"

extern SemaphoreHandle_t xTimeMutex;

void vTimeTask( void *pvParameters );
void vTimeGetCurrentTime( uint8_t *pucHour,
                          uint8_t *pucMinute,
                          uint8_t *pucSecond );

/* Set lại thời gian (giờ/phút/giây) – sẽ được gọi sau khi sync NTP */
void vTimeSet( uint8_t ucHour, uint8_t ucMinute, uint8_t ucSecond );

/* Gọi một lần sau khi WiFi đã có IP: sync từ NTP -> vTimeSet() */
esp_err_t xTimeSyncFromNtp( const char *pcNtpServer,
                            TickType_t xMaxBlockTime );
