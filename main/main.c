#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "wifi.h"
#include "time_svc.h"
#include "display.h"
#include "button.h"
#include "dht_sensor.h"
#include "buzzer.h"

static const char *TAG = "Smart_Clock";

/* ====== CẤU HÌNH ====== */
#define WIFI_SSID        "WillSun"
#define WIFI_PASSWORD    "abcabcabc"
#define NTP_SERVER_NAME  "pool.ntp.org"
#define NTP_SYNC_TIMEOUT_MS   15000

#define DISPLAY_PIN_MOSI   GPIO_NUM_7
#define DISPLAY_PIN_SCLK   GPIO_NUM_6
#define DISPLAY_PIN_CS     GPIO_NUM_5

#define BUTTON_GPIO     GPIO_NUM_9
#define BUZZER_GPIO     GPIO_NUM_1
#define DHT_GPIO        GPIO_NUM_3
/* ===================================== */

/* Mutex dùng cho time_svc – được định nghĩa trong time_svc.c là biến global */
SemaphoreHandle_t xTimeMutex = NULL;

/* Queue điều khiển mode hiển thị */
static QueueHandle_t xDisplayModeQueue = NULL;

/* Timer điều khiển thời gian beep của buzzer */
static TimerHandle_t xBuzzerTimer = NULL;

void app_main( void )
{
    ESP_LOGI(TAG, "===== app_main START =====");

    xTimeMutex = xSemaphoreCreateMutex();
    configASSERT(xTimeMutex != NULL);

    xDisplayModeQueue = xQueueCreate(4, sizeof(eDisplayScreen_t));
    configASSERT(xDisplayModeQueue != NULL);

    xBuzzerTimer = xBuzzerCreateTimer();
    configASSERT(xBuzzerTimer != NULL);

    vWiFiInit( WIFI_SSID, WIFI_PASSWORD );
    if( xWiFiWaitForIP( pdMS_TO_TICKS( 10000 ) ) != pdTRUE )
    {
        ESP_LOGE(TAG, "WiFi not connected -> skip NTP, dùng giờ mặc định");
    }
    else
    {
        esp_err_t err = xTimeSyncFromNtp(
                NTP_SERVER_NAME,
                pdMS_TO_TICKS( NTP_SYNC_TIMEOUT_MS ) );

        if( err != ESP_OK )
        {
            ESP_LOGE(TAG, "xTimeSyncFromNtp failed, dùng giờ mặc định");
        }
    }

    vDisplayHardwareInit(DISPLAY_PIN_MOSI, DISPLAY_PIN_SCLK, DISPLAY_PIN_CS);
    vSensorDhtInit(DHT_GPIO);
    vButtonInit(BUTTON_GPIO);
    vBuzzerInit(BUZZER_GPIO, xBuzzerTimer);

    /* Task thời gian (phần trước anh đã có) */
    xTaskCreate(vTimeTask, "TimeTask",
                2048, NULL, 5, NULL);

    /* Task hiển thị, nhận queue để đổi mode */
    xTaskCreate(vDisplayTask, "DisplayTask",
                2048, (void *)xDisplayModeQueue, 4, NULL);

    /* Task DHT */
    xTaskCreate(vSensorDhtTask, "DhtTask",
                2048, NULL, 3, NULL);

    /* Task Button (truyền tham số gồm GPIO + Queue) */
    static ButtonTaskParams_t xBtnParams;
    xBtnParams.xButtonGpio = BUTTON_GPIO;
    xBtnParams.xModeQueue  = xDisplayModeQueue;

    xTaskCreate(vButtonTask, "ButtonTask",
                2048, &xBtnParams, 4, NULL);



    // while (1) {
    //     ESP_LOGI(TAG, "Hello world!");
    //     vTaskDelay(pdMS_TO_TICKS(5000));
    // }
}


