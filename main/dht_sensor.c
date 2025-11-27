#include "dht_sensor.h"
#include "dht.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAGDHT = "dht";

static gpio_num_t        s_xDhtGpio = GPIO_NUM_NC;
static float             s_fTemp = 0.0f;
static float             s_fHumid = 0.0f;
static BaseType_t        s_xHasData = pdFALSE;
static SemaphoreHandle_t s_xDhtMutex = NULL;

void vSensorDhtInit( gpio_num_t xGpio )
{
    s_xDhtGpio = xGpio;

    if (s_xDhtMutex == NULL)
        s_xDhtMutex = xSemaphoreCreateMutex();

    ESP_LOGI(TAGDHT, "DHT init on GPIO %d", (int)s_xDhtGpio);
}

void vSensorDhtTask( void *pvParameters )
{
    (void)pvParameters;

    if (s_xDhtGpio == GPIO_NUM_NC) {
        ESP_LOGE(TAGDHT, "DHT GPIO not set");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAGDHT, "vSensorDhtTask started");

    for (;;)
    {
        float h = 0.0f, t = 0.0f;
        esp_err_t err = dht_read_float_data(DHT_TYPE_DHT11, s_xDhtGpio, &h, &t);

        if (err == ESP_OK) {
            if (xSemaphoreTake(s_xDhtMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                s_fTemp = t;
                s_fHumid = h;
                s_xHasData = pdTRUE;
                xSemaphoreGive(s_xDhtMutex);
            }
            ESP_LOGI(TAGDHT, "Temperature: %.1f Humidity: %.1f", t, h);
        } else {
            ESP_LOGW(TAGDHT, "DHT read error: 0x%x", err);
        }

        vTaskDelay(pdMS_TO_TICKS(180000)); // 180s/lần
    }
}

BaseType_t xSensorDhtGetLatest( float *pfTemp, float *pfHumid )
{
    if (s_xDhtMutex == NULL || s_xHasData == pdFALSE)
        return pdFALSE;

    if (xSemaphoreTake(s_xDhtMutex, pdMS_TO_TICKS(10)) != pdTRUE)
        return pdFALSE;

    if (pfTemp)  *pfTemp  = s_fTemp;
    if (pfHumid) *pfHumid = s_fHumid;

    xSemaphoreGive(s_xDhtMutex);
    return pdTRUE;
}
