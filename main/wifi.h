#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"

/* Khởi tạo WiFi STA + đăng ký event, connect tới AP (SSID/PASS truyền từ main) */
void vWiFiInit( const char *pcSsid, const char *pcPassword );

/* Chờ đến khi đã có IP (return pdTRUE nếu ok, pdFALSE nếu timeout) */
BaseType_t xWiFiWaitForIP( TickType_t xTicksToWait );

#ifdef __cplusplus
}
#endif
