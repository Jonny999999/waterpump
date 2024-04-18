#include "credentials.h"

// mqtt:
#define MQTT_BROKER_URL     "mqtt://10.0.0.102:1883"
#define MQTT_LOGLEVEL       ESP_LOG_INFO

// wifi:
// wifi ssid and password are configured in credentials.h
#define STATIC_IP_ADDR      "10.0.0.88"
#define STATIC_NETMASK_ADDR "255.255.0.0"
#define STATIC_GW_ADDR      "10.0.0.1"
#define MAIN_DNS_SERVER     "10.0.0.1"
#define BACKUP_DNS_SERVER   "8.8.8.8"