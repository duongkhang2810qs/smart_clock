#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "max7219.h"
#include "time.h"
#include "display.h"

static const char *TAG = "display";

/* Chân SPI – giống dự án cũ, chỉnh lại nếu bạn dùng khác */
#define PIN_MOSI   GPIO_NUM_7
#define PIN_SCLK   GPIO_NUM_6
#define PIN_CS     GPIO_NUM_5

/* 4 module MAX7219 8x8 ghép thành 8x32 */
static max7219_t s_dev;

/* --- Các hàm vẽ ma trận lấy từ code cũ --- */

static uint8_t prvShiftDown(uint8_t b)
{
    return b;   // giữ nguyên như thiết kế cũ
}

/* chuyển số [0..9] thành 8 byte cột (5x7 font) */
static void prvDigitToCols(uint8_t digit, uint8_t out8[8])
{
    static const uint8_t F[10][7] = {
        {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},   // 0
        {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},   // 1
        {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},   // 2
        {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},   // 3
        {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},   // 4
        {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},   // 5
        {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},   // 6
        {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},   // 7
        {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},   // 8
        {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},   // 9
    };

    for (int i = 0; i < 7; i++)
        out8[i] = prvShiftDown(F[digit][i]);

    out8[7] = 0x00;
}

/* vẽ 32 cột thành 4 module (8x8) */
static void prvDrawCols8x32(max7219_t *dev, const uint8_t cols[32])
{
    for (int m = 0; m < 4; m++) {
        uint8_t block[8];
        memcpy(block, &cols[m * 8], 8);
        max7219_draw_image_8x8(dev, m * 8, block);
    }
}

/* vẽ HH:MM full 8x32 */
static void prvDrawTimeHHMM(max7219_t *dev, int hour, int minute)
{
    uint8_t cols[32] = {0};
    uint8_t tmp[8];

    prvDigitToCols(hour / 10, tmp);
    memcpy(&cols[0], tmp, 8);

    prvDigitToCols(hour % 10, tmp);
    memcpy(&cols[8], tmp, 8);

    prvDigitToCols(minute / 10, tmp);
    memcpy(&cols[16], tmp, 8);

    prvDigitToCols(minute % 10, tmp);
    memcpy(&cols[24], tmp, 8);

    prvDrawCols8x32(dev, cols);
}

/* --- Public API --- */

void vDisplayHardwareInit( void )
{
    ESP_LOGI(TAG, "Init SPI bus for 8x32 matrix...");

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    memset(&s_dev, 0, sizeof(s_dev));
    ESP_ERROR_CHECK(max7219_init_desc(&s_dev, SPI2_HOST, 1000000, PIN_CS));

    /* Cấu hình giống code cũ: 4 module, 32 digit */
    s_dev.cascade_size = 4;
    s_dev.digits       = 32;
    s_dev.mirrored     = false;

    ESP_ERROR_CHECK(max7219_init(&s_dev));
    ESP_ERROR_CHECK(max7219_set_brightness(&s_dev, 8));
    ESP_LOGI(TAG, "Display init done (8x32 matrix)");
}

/* Task hiển thị: luôn vẽ HHMM từ time */
void vDisplayTask( void *pvParameters )
{
    (void) pvParameters;

    ESP_LOGI(TAG, "vDisplayTask started");

    for (;;)
    {
        uint8_t h, m, s;
        vTimeGetCurrentTime(&h, &m, &s);

        prvDrawTimeHHMM(&s_dev, h, m);

        ESP_LOGD(TAG, "Draw time: %02u:%02u", h, m);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
