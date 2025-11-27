#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "max7219.h"
#include "time_svc.h"
#include "dht_sensor.h"
#include "display.h"

static const char *TAG = "display";

/* 4 module MAX7219 8x8 ghép thành 8x32 */
static max7219_t s_xDisplayDev;

/* Nếu cần shift xuống một hàng cho hợp ma trận, hiện đang giữ nguyên như code cũ */
static inline uint8_t prvShiftDown( uint8_t ucByte )
{
    return ucByte;
}

/* --- Font số 0..9 (5x7) -> 8 byte (7 hàng + 1 trống) --- */
static void prvDigitToCols( uint8_t ucDigit, uint8_t pucOut8[ 8 ] )
{
    static const uint8_t ucFont[ 10 ][ 7 ] =
    {
        { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E }, /* 0 */
        { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }, /* 1 */
        { 0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F }, /* 2 */
        { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E }, /* 3 */
        { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 }, /* 4 */
        { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E }, /* 5 */
        { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E }, /* 6 */
        { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }, /* 7 */
        { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E }, /* 8 */
        { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C }  /* 9 */
    };

    ucDigit %= 10;

    for( int i = 0; i < 7; i++ )
    {
        pucOut8[ i ] = prvShiftDown( ucFont[ ucDigit ][ i ] );
    }

    pucOut8[ 7 ] = 0x00;
}

/* --- Font chữ A..Z “to” (5x7) -> 8 byte (giống kích thước số) --- */
static void prvLetterToCols( char c, uint8_t pucOut8[ 8 ] )
{
    /* 5x7, mỗi byte là 1 hàng, 5 bit thấp dùng, tự design đơn giản */
    static const uint8_t ucLetters[ 26 ][ 7 ] =
    {
        /* A */ { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 },
        /* B */ { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E },
        /* C */ { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E },
        /* D */ { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E },
        /* E */ { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F },
        /* F */ { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 },
        /* G */ { 0x0F, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0F },
        /* H */ { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 },
        /* I */ { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E },
        /* J */ { 0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E },
        /* K */ { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 },
        /* L */ { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F },
        /* M */ { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 },
        /* N */ { 0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11 },
        /* O */ { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E },
        /* P */ { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 },
        /* Q */ { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x13, 0x0F },
        /* R */ { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 },
        /* S */ { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E },
        /* T */ { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },
        /* U */ { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E },
        /* V */ { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 },
        /* W */ { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 },
        /* X */ { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 },
        /* Y */ { 0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04 },
        /* Z */ { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F }
    };

    int idx;

    if( c >= 'A' && c <= 'Z' )
    {
        idx = c - 'A';
    }
    else if( c >= 'a' && c <= 'z' )
    {
        idx = c - 'a';
    }
    else
    {
        memset( pucOut8, 0, 8 );
        return;
    }

    for( int i = 0; i < 7; i++ )
    {
        pucOut8[ i ] = prvShiftDown( ucLetters[ idx ][ i ] );
    }

    pucOut8[ 7 ] = 0x00;
}

/* --- Vẽ 32 “hàng” ra 4 module --- */
static void prvDrawCols8x32( max7219_t *pxDev,
                             const uint8_t pucCols[ 32 ] )
{
    for( int m = 0; m < 4; m++ )
    {
        uint8_t ucBlock[ 8 ];
        memcpy( ucBlock, &pucCols[ m * 8 ], 8 );
        max7219_draw_image_8x8( pxDev, m * 8, ucBlock );
    }
}

/* --- Các hàm vẽ cho từng mode --- */

/* HHMM */
static void prvDrawTimeHHMM( max7219_t *pxDev, int lHour, int lMinute )
{
    uint8_t ucCols[ 32 ] = { 0 };
    uint8_t ucTmp[ 8 ];

    prvDigitToCols( ( lHour / 10 ) % 10, ucTmp );
    memcpy( &ucCols[ 0 ], ucTmp, 8 );

    prvDigitToCols( lHour % 10, ucTmp );
    memcpy( &ucCols[ 8 ], ucTmp, 8 );

    prvDigitToCols( ( lMinute / 10 ) % 10, ucTmp );
    memcpy( &ucCols[ 16 ], ucTmp, 8 );

    prvDigitToCols( lMinute % 10, ucTmp );
    memcpy( &ucCols[ 24 ], ucTmp, 8 );

    prvDrawCols8x32( pxDev, ucCols );
}

/* SUN / MON / ... */
static void prvDrawWeekday( max7219_t *pxDev, int lWday )
{
    static const char *pcWd[ 7 ] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
    const char *pcStr;

    if( lWday >= 0 && lWday <= 6 )
    {
        pcStr = pcWd[ lWday ];
    }
    else
    {
        pcStr = "SUN";
    }

    uint8_t ucCols[ 32 ] = { 0 };
    uint8_t ucL[ 8 ];

    prvLetterToCols( pcStr[ 0 ], ucL );
    memcpy( &ucCols[ 0 ], ucL, 8 );

    prvLetterToCols( pcStr[ 1 ], ucL );
    memcpy( &ucCols[ 8 ], ucL, 8 );

    prvLetterToCols( pcStr[ 2 ], ucL );
    memcpy( &ucCols[ 16 ], ucL, 8 );

    /* module thứ 4 để trống */

    prvDrawCols8x32( pxDev, ucCols );
}

/* DD-MM */
static void prvDrawDateDDMM( max7219_t *pxDev, int lDay, int lMonth )
{
    uint8_t ucCols[ 32 ] = { 0 };
    uint8_t ucD[ 8 ];

    if( lDay < 1 )   lDay   = 1;
    if( lDay > 31 )  lDay   = 31;
    if( lMonth < 1 ) lMonth = 1;
    if( lMonth > 12 )lMonth = 12;

    prvDigitToCols( ( lDay / 10 ) % 10, ucD );
    memcpy( &ucCols[ 0 ], ucD, 8 );

    prvDigitToCols( lDay % 10, ucD );
    memcpy( &ucCols[ 8 ], ucD, 8 );

    prvDigitToCols( ( lMonth / 10 ) % 10, ucD );
    memcpy( &ucCols[ 16 ], ucD, 8 );

    prvDigitToCols( lMonth % 10, ucD );
    memcpy( &ucCols[ 24 ], ucD, 8 );

    prvDrawCols8x32( pxDev, ucCols );
}

/* YYYY */
static void prvDrawYearYYYY( max7219_t *pxDev, int lYear )
{
    uint8_t ucCols[ 32 ] = { 0 };
    uint8_t ucD[ 8 ];

    if( lYear < 0 )      lYear = 0;
    if( lYear > 9999 )   lYear = 9999;

    prvDigitToCols( ( lYear / 1000 ) % 10, ucD );
    memcpy( &ucCols[ 0 ], ucD, 8 );

    prvDigitToCols( ( lYear / 100 ) % 10, ucD );
    memcpy( &ucCols[ 8 ], ucD, 8 );

    prvDigitToCols( ( lYear / 10 ) % 10, ucD );
    memcpy( &ucCols[ 16 ], ucD, 8 );

    prvDigitToCols( lYear % 10, ucD );
    memcpy( &ucCols[ 24 ], ucD, 8 );

    prvDrawCols8x32( pxDev, ucCols );
}

/* TT HH – nhiệt độ, độ ẩm (lấy phần nguyên, 0..99) */
static void prvDrawTempHumid( max7219_t *pxDev, float fTemp, float fHumid )
{
    int lT = ( int ) ( fTemp + 0.5f );
    int lH = ( int ) ( fHumid + 0.5f );

    if( lT < 0 )   lT = 0;
    if( lT > 99 )  lT = 99;
    if( lH < 0 )   lH = 0;
    if( lH > 99 )  lH = 99;

    uint8_t ucCols[ 32 ] = { 0 };
    uint8_t ucD[ 8 ];

    prvDigitToCols( ( lT / 10 ) % 10, ucD );
    memcpy( &ucCols[ 0 ], ucD, 8 );

    prvDigitToCols( lT % 10, ucD );
    memcpy( &ucCols[ 8 ], ucD, 8 );

    prvDigitToCols( ( lH / 10 ) % 10, ucD );
    memcpy( &ucCols[ 16 ], ucD, 8 );

    prvDigitToCols( lH % 10, ucD );
    memcpy( &ucCols[ 24 ], ucD, 8 );

    prvDrawCols8x32( pxDev, ucCols );
}

/* Lấy thời gian hệ thống (đã sync NTP) về struct tm local (UTC+7) */
static void prvGetLocalDateTime( struct tm *pxOutTm )
{
    struct timeval xTv;
    gettimeofday( &xTv, NULL );

    /* Nếu chưa set TZ thì cộng tay UTC+7 */
    time_t xT = xTv.tv_sec + 7 * 3600;
    gmtime_r( &xT, pxOutTm );
}

/* --- Public API --- */

void vDisplayHardwareInit( gpio_num_t xGpioMosi, gpio_num_t xGpioSlck, gpio_num_t xGpioCs )
{
    ESP_LOGI(TAG, "Init SPI bus for 8x32 matrix...");

    spi_bus_config_t buscfg = {
        .mosi_io_num = xGpioMosi,
        .miso_io_num = -1,
        .sclk_io_num = xGpioSlck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    memset(&s_xDisplayDev, 0, sizeof(s_xDisplayDev));
    ESP_ERROR_CHECK(max7219_init_desc(&s_xDisplayDev, SPI2_HOST, 1000000, xGpioCs));

    s_xDisplayDev.cascade_size = 4;
    s_xDisplayDev.digits       = 32;
    s_xDisplayDev.mirrored     = false;

    ESP_ERROR_CHECK(max7219_init(&s_xDisplayDev));
    ESP_ERROR_CHECK(max7219_set_brightness(&s_xDisplayDev, 8));

    ESP_LOGI(TAG, "Display init done (8x32 matrix)");
}

/* pvParameters: QueueHandle_t xModeQueue (chứa eDisplayScreen_t) */
void vDisplayTask( void *pvParameters )
{
    QueueHandle_t xModeQueue = (QueueHandle_t) pvParameters;
    eDisplayScreen_t eCurrent = eDisplayScreenTime;

    ESP_LOGI(TAG, "vDisplayTask started");

    for (;;)
    {
        /* 1) Nhận lệnh đổi mode nếu có */
        eDisplayScreen_t eNew;
        if (xModeQueue != NULL &&
            xQueueReceive(xModeQueue, &eNew, pdMS_TO_TICKS(10)) == pdPASS)
        {
            if (eNew < eDisplayScreenCount)
            {
                eCurrent = eNew;
                ESP_LOGI(TAG, "Change screen -> %d", eCurrent);
            }
        }

        /* 2) Đọc thời gian / nhiệt độ / ẩm tuỳ mode */
        uint8_t h, m, s;
        vTimeGetCurrentTime(&h, &m, &s);

        struct tm tm_local = {0};
        prvGetLocalDateTime(&tm_local);

        float temp = 0.0f, humid = 0.0f;
        BaseType_t xHasDht = xSensorDhtGetLatest(&temp, &humid);

        /* 3) Vẽ theo eCurrent */
        switch (eCurrent)
        {
        case eDisplayScreenTime:
            prvDrawTimeHHMM(&s_xDisplayDev, h, m);
            break;

        case eDisplayScreenWeekday:
            prvDrawWeekday(&s_xDisplayDev, tm_local.tm_wday);
            break;

        case eDisplayScreenDate:
            prvDrawDateDDMM(&s_xDisplayDev, tm_local.tm_mday, tm_local.tm_mon + 1);
            break;

        case eDisplayScreenYear:
            prvDrawYearYYYY(&s_xDisplayDev, tm_local.tm_year + 1900);
            break;

        case eDisplayScreenTempHumid:
            if (xHasDht == pdTRUE)
                prvDrawTempHumid(&s_xDisplayDev, temp, humid);
            else
                prvDrawTempHumid(&s_xDisplayDev, 0, 0); /* chưa có dữ liệu thì hiển thị 00 00 */
            break;

        default:
            prvDrawTimeHHMM(&s_xDisplayDev, h, m);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
