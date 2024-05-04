// include config section not tracked in repo
// show message when file is not manually created yet
#if __has_include("credentials.h")
#include "credentials.h"
#else
#error [=== FILE 'credentials.h' MISSING! ===] => Copy and customize 'credentials.default.h' first!
#endif

// mqtt:
#define MQTT_BROKER_URL     "mqtt://10.0.0.102:1883"
#define MQTT_LOGLEVEL       ESP_LOG_WARN

// wifi:
// wifi ssid and password are configured in credentials.h
#define STATIC_IP_ADDR      "10.0.0.88"
#define STATIC_NETMASK_ADDR "255.255.0.0"
#define STATIC_GW_ADDR      "10.0.0.1"
#define MAIN_DNS_SERVER     "10.0.0.1"
#define BACKUP_DNS_SERVER   "8.8.8.8"
#define WIFI_LOGLEVEL ESP_LOG_INFO


//--------------------------
//----- display config -----
//--------------------------
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
#define HOST    HSPI_HOST
#else
#define HOST    SPI2_HOST
#endif
#define DISPLAY_PIN_NUM_MOSI GPIO_NUM_23
#define DISPLAY_PIN_NUM_CLK GPIO_NUM_22
#define DISPLAY_PIN_NUM_CS GPIO_NUM_21
#define DISPLAY_DELAY 2000
#define DISPLAY_BRIGHTNESS 2 // 0 - 15
#define DISPLAY_MODULE_COUNT_CONNECTED_IN_SERIES 3