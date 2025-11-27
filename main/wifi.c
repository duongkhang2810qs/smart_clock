#include "wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "WiFi";

/* Event group để báo đã có IP */
static EventGroupHandle_t s_xWiFiEventGroup = NULL;

#define WIFI_CONNECTED_BIT  BIT0

/* Event handler cho WiFi + IP */
static void prvWiFiEventHandler( void *arg,
                                 esp_event_base_t event_base,
                                 int32_t event_id,
                                 void *event_data )
{
    if( event_base == WIFI_EVENT )
    {
        switch( event_id )
        {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START -> esp_wifi_connect()");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "STA_DISCONNECTED -> reconnect");
                esp_wifi_connect();
                break;

            default:
                break;
        }
    }
    else if( event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP )
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits( s_xWiFiEventGroup, WIFI_CONNECTED_BIT );
    }
}

static void prvNvsInit( void )
{
    esp_err_t ret = nvs_flash_init();

    if( ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND )
    {
        // NVS full hoặc version cũ -> erase rồi init lại
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK( ret );
}


void vWiFiInit( const char *pcSsid, const char *pcPassword )
{
    ESP_LOGI(TAG, "vWiFiInit: SSID=\"%s\"", pcSsid);

    /* 0) Init NVS trước khi đụng WiFi driver */
    prvNvsInit();

    s_xWiFiEventGroup = xEventGroupCreate();

    /* 1) init TCP/IP + event loop + default STA netif */
    ESP_ERROR_CHECK( esp_netif_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    esp_netif_create_default_wifi_sta();

    /* 2) init WiFi driver */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );

    /* 3) đăng ký event handler */
    ESP_ERROR_CHECK( esp_event_handler_instance_register(
                         WIFI_EVENT,
                         ESP_EVENT_ANY_ID,
                         &prvWiFiEventHandler,
                         NULL,
                         NULL ) );

    ESP_ERROR_CHECK( esp_event_handler_instance_register(
                         IP_EVENT,
                         IP_EVENT_STA_GOT_IP,
                         &prvWiFiEventHandler,
                         NULL,
                         NULL ) );

    /* 4) cấu hình station */
    wifi_config_t wifi_config = { 0 };
    strlcpy( (char *) wifi_config.sta.ssid,
             pcSsid,
             sizeof( wifi_config.sta.ssid ) );
    strlcpy( (char *) wifi_config.sta.password,
             pcPassword,
             sizeof( wifi_config.sta.password ) );
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_STA, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_LOGI(TAG, "WiFi init done, waiting for IP...");
}

BaseType_t xWiFiWaitForIP( TickType_t xTicksToWait )
{
    EventBits_t bits = xEventGroupWaitBits(
            s_xWiFiEventGroup,
            WIFI_CONNECTED_BIT,
            pdFALSE,      /* không xóa flag */
            pdFALSE,      /* chỉ cần 1 bit */
            xTicksToWait  /* timeout */
    );

    if( ( bits & WIFI_CONNECTED_BIT ) != 0 )
    {
        ESP_LOGI(TAG, "WiFi connected & got IP");
        return pdTRUE;
    }
    else
    {
        ESP_LOGE(TAG, "Timeout waiting for IP");
        return pdFALSE;
    }
}
