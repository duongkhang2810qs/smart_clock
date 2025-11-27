#include "button.h"
#include "buzzer.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAGBTN = "button";

static gpio_num_t s_xButtonGpio = GPIO_NUM_NC;

void vButtonInit( gpio_num_t xGpio )
{
    s_xButtonGpio = xGpio;

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << s_xButtonGpio,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,   // nút kéo về GND
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io);

    ESP_LOGI(TAGBTN, "Button init on GPIO %d", (int)s_xButtonGpio);
}

void vButtonTask( void *pvParameters )
{
    ButtonTaskParams_t *pxParams = (ButtonTaskParams_t *)pvParameters;
    if (pxParams == NULL || pxParams->xModeQueue == NULL) {
        ESP_LOGE(TAGBTN, "ButtonTask params invalid");
        vTaskDelete(NULL);
        return;
    }

    eDisplayScreen_t eCurrent = eDisplayScreenTime;
    int last_level = gpio_get_level(pxParams->xButtonGpio);

    ESP_LOGI(TAGBTN, "vButtonTask started");

    for (;;)
    {
        int level = gpio_get_level(pxParams->xButtonGpio);

        /* Active low: phát hiện cạnh xuống (1 -> 0) */
        if (last_level == 1 && level == 0)
        {
            /* Đổi mode */
            eCurrent = (eDisplayScreen_t)((eCurrent + 1) % eDisplayScreenCount);

            BaseType_t xOk = xQueueSend(pxParams->xModeQueue, &eCurrent, 0);
            if (xOk == pdPASS) {
                ESP_LOGI(TAGBTN, "Button pressed -> mode %d", eCurrent);
            }

            /* Beep 1 cái */
            vBuzzerBeep(pdMS_TO_TICKS(80));
        }

        last_level = level;
        vTaskDelay(pdMS_TO_TICKS(20));   // debounce ~20 ms
    }
}
