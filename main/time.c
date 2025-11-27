#include "time.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include <sys/time.h>   // dùng gettimeofday()

static const char *TAG = "Time";

/* Định nghĩa handle mutex (chỉ 1 nơi duy nhất). */
SemaphoreHandle_t xTimeMutex = NULL;

/* Thời gian giả lập. */
static uint8_t ucHour   = 12;
static uint8_t ucMinute = 0;
static uint8_t ucSecond = 0;

void vTimeSet( uint8_t ucH, uint8_t ucM, uint8_t ucS )
{
    if( xTimeMutex != NULL &&
        xSemaphoreTake( xTimeMutex, pdMS_TO_TICKS( 100 ) ) == pdTRUE )
    {
        ucHour   = ucH;
        ucMinute = ucM;
        ucSecond = ucS;

        ESP_LOGI(TAG, "vTimeSet -> %02u:%02u:%02u", ucHour, ucMinute, ucSecond);

        xSemaphoreGive( xTimeMutex );
    }
    else
    {
        /* Nếu chưa có mutex vẫn set, để khỏi crash */
        ucHour   = ucH;
        ucMinute = ucM;
        ucSecond = ucS;
        ESP_LOGW(TAG, "vTimeSet without mutex -> %02u:%02u:%02u",
                 ucHour, ucMinute, ucSecond);
    }
}

esp_err_t xTimeSyncFromNtp( const char *pcNtpServer,
                            TickType_t xMaxBlockTime )
{
    ESP_LOGI(TAG, "xTimeSyncFromNtp: server=\"%s\"", pcNtpServer);

    /* Cấu hình SNTP dùng esp_netif_sntp (IDF 5.x) */
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG( pcNtpServer );
    esp_netif_sntp_init( &config );

    /* Chờ sync – tối đa xMaxBlockTime */
    esp_err_t err = esp_netif_sntp_sync_wait( xMaxBlockTime );
    if( err != ESP_OK )
    {
        ESP_LOGE(TAG, "SNTP sync failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Đọc thời gian hệ thống (UTC), chuyển sang giờ địa phương đơn giản */
    struct timeval tv_now;
    gettimeofday( &tv_now, NULL );
    time_t t = tv_now.tv_sec;

    /* Nếu muốn dùng UTC+7: cộng offset 7 giờ (Viet Nam) */
    const int32_t lUtcOffsetSeconds = 7 * 3600;
    t += lUtcOffsetSeconds;

    int seconds_in_day = (int)( t % 86400 );
    if( seconds_in_day < 0 )
        seconds_in_day += 86400;

    uint8_t h = seconds_in_day / 3600;
    uint8_t m = (seconds_in_day % 3600) / 60;
    uint8_t s = seconds_in_day % 60;

    vTimeSet( h, m, s );

    ESP_LOGI(TAG, "Time synced from NTP -> %02u:%02u:%02u", h, m, s );

    return ESP_OK;
}

/* Task đếm thời gian – dùng xTimeMutex do app_main tạo. */
void vTimeTask( void *pvParameters )
{
    ( void ) pvParameters;

    const TickType_t xOneSecond = pdMS_TO_TICKS( 1000UL );
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for( ;; )
    {
        vTaskDelayUntil( &xLastWakeTime, xOneSecond );

        if( ( xTimeMutex != NULL ) &&
            ( xSemaphoreTake( xTimeMutex, portMAX_DELAY ) == pdTRUE ) )
        {
            ucSecond++;
            if( ucSecond >= 60 )
            {
                ucSecond = 0;
                ucMinute++;
                if( ucMinute >= 60 )
                {
                    ucMinute = 0;
                    ucHour++;
                    if( ucHour >= 24 )
                    {
                        ucHour = 0;
                    }
                }
            }

            xSemaphoreGive( xTimeMutex );
        }
    }
}

/* Lấy bản copy thời gian hiện tại. */
void vTimeGetCurrentTime( uint8_t *pucHour,
                          uint8_t *pucMinute,
                          uint8_t *pucSecond )
{
    if( ( pucHour == NULL ) || ( pucMinute == NULL ) || ( pucSecond == NULL ) )
    {
        return;
    }

    if( ( xTimeMutex != NULL ) &&
        ( xSemaphoreTake( xTimeMutex, pdMS_TO_TICKS( 20 ) ) == pdTRUE ) )
    {
        *pucHour   = ucHour;
        *pucMinute = ucMinute;
        *pucSecond = ucSecond;

        xSemaphoreGive( xTimeMutex );
    }
    else
    {
        *pucHour   = ucHour;
        *pucMinute = ucMinute;
        *pucSecond = ucSecond;
    }
}
