#include "buzzer.h"
#include "esp_log.h"

static const char *TAGB = "buzzer";

static gpio_num_t    s_xBuzzerGpio = GPIO_NUM_NC;
static TimerHandle_t s_xBuzzerTimer = NULL;

/* callback tắt buzzer */
static void prvBuzzerTimerCallback( TimerHandle_t xTimer )
{
    (void)xTimer;
    if (s_xBuzzerGpio != GPIO_NUM_NC) {
        gpio_set_level(s_xBuzzerGpio, 0);
    }
}

TimerHandle_t xBuzzerCreateTimer( void )
{
    return xTimerCreate("Buzzer",
                        pdMS_TO_TICKS(50),
                        pdFALSE,
                        NULL,
                        prvBuzzerTimerCallback);
}

void vBuzzerInit( gpio_num_t xGpio, TimerHandle_t xTimer )
{
    s_xBuzzerGpio = xGpio;
    s_xBuzzerTimer = xTimer;

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << s_xBuzzerGpio,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(s_xBuzzerGpio, 0);

    ESP_LOGI(TAGB, "Buzzer init on GPIO %d", (int)s_xBuzzerGpio);
}

void vBuzzerOn( void )
{
    if (s_xBuzzerGpio != GPIO_NUM_NC)
        gpio_set_level(s_xBuzzerGpio, 1);
}

void vBuzzerOff( void )
{
    if (s_xBuzzerGpio != GPIO_NUM_NC)
        gpio_set_level(s_xBuzzerGpio, 0);
}

void vBuzzerBeep( TickType_t xDurationTicks )
{
    if (s_xBuzzerTimer == NULL || s_xBuzzerGpio == GPIO_NUM_NC)
        return;

    vBuzzerOn();
    xTimerStop(s_xBuzzerTimer, 0);
    xTimerChangePeriod(s_xBuzzerTimer, xDurationTicks, 0);
    xTimerStart(s_xBuzzerTimer, 0);
}
