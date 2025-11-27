#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "nvs_flash.h"
#include "nvs.h" 
#include "esp_log.h"

#include "wifi.h"
#include "display.h"
#include "time.h"

static const char *TAG = "Smart_Lock";

/* ====== CẤU HÌNH – CHỈ SỬA Ở ĐÂY ====== */
#define WIFI_SSID        "WillSun"
#define WIFI_PASSWORD    "abcabcabc"
#define NTP_SERVER_NAME  "pool.ntp.org"
#define NTP_SYNC_TIMEOUT_MS   15000
/* ===================================== */

void app_main( void )
{
    ESP_LOGI(TAG, "===== app_main START =====");

    /* 1) Tạo mutex cho module time (hiển thị rõ ở main) */
    xTimeMutex = xSemaphoreCreateMutex();
    if( xTimeMutex == NULL )
    {
        ESP_LOGE(TAG, "Failed to create xTimeMutex");
        return;
    }

    /* 2) Khởi tạo & kết nối WiFi */
    vWiFiInit( WIFI_SSID, WIFI_PASSWORD );

    if( xWiFiWaitForIP( pdMS_TO_TICKS( 10000 ) ) != pdTRUE )
    {
        ESP_LOGE(TAG, "WiFi not connected -> skip NTP, dùng giờ mặc định");
    }
    else
    {
        /* 3) Sync thời gian từ NTP */
        esp_err_t err = xTimeSyncFromNtp(
                NTP_SERVER_NAME,
                pdMS_TO_TICKS( NTP_SYNC_TIMEOUT_MS ) );

        if( err != ESP_OK )
        {
            ESP_LOGE(TAG, "xTimeSyncFromNtp failed, dùng giờ mặc định");
        }
    }

    /* 4) Khởi tạo phần cứng display */
    vDisplayHardwareInit();

    /* 5) Tạo task time + display */
    xTaskCreate( vTimeTask,    "TimeTask",    2048, NULL, 3, NULL );
    xTaskCreate( vDisplayTask, "DisplayTask", 4096, NULL, 2, NULL );


    while (1) {
        ESP_LOGI(TAG, "Hello world!");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


