/* 
the code in this file is mostly copied from the esp-idf mqtt tcp example:
'/opt/esp-idf_github/examples/protocols/mqtt/tcp/main/app_main.c'
with some adjustments:
    - global mqtt_client variable (where mqtt.h is included)
        extern esp_mqtt_client_handle_t mqtt_client;
    - renamed some variables and macros
    - loglevel simplified to one config option
    - define mqtt_app_start function instead of app_main
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"

#include "mqtt.h"
#include "config.h"



//--------------------
//------ config ------
//--------------------
//from config.h:
//#define MQTT_BROKER_URL     "mqtt://10.0.0.102:1883"
//#define MQTT_LOGLEVEL       ESP_LOG_INFO //ESP_LOG_VERBOSE



//---------------------
//----- variables -----
//---------------------
static const char *TAG = "MQTT_c";
//global mqtt_client - defined when starting mqtt client
esp_mqtt_client_handle_t mqtt_client;
//variables for logging connect event to mqtt
uint32_t timestampConnected = 0;
uint32_t timestampDisconnected = 0;
bool mqtt_isConnected = false;
    


//---------------------
//----- functions -----
//---------------------
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t mqtt_client = event->client;
	//int msg_id;
	switch ((esp_mqtt_event_id_t)event_id) {
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			//--- subscribe all configured topics ---
			mqtt_subscribeTopics(); //from mqtt.cpp
			//msg_id = esp_mqtt_client_subscribe(mqtt_client, "/topic/qos1", 1);
			//ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
			//msg_id = esp_mqtt_client_unsubscribe(mqtt_client, "/topic/qos1");
			//ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
			ESP_LOGW(TAG, "(re)connected");
			mqtt_isConnected = true;
			break;
		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			if (mqtt_isConnected){ //DISCONNECT even happens every 15s when not able to reconnect
				ESP_LOGE(TAG, "disconnected");
				timestampDisconnected = esp_log_timestamp();
				mqtt_isConnected = false;
			}
			break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        //msg_id = esp_mqtt_client_publish(mqtt_client, "/topic/qos0", "data", 0, 0, 0);
        //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
		//--- handle received data ---
		//run corresponding configured function by topic
		mqtt_handleData(event->topic, event->topic_len, event->data, event->data_len); //from mqtt.cpp
        //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        //printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


void mqtt_app_start(void)
{
    //set loglevel
    esp_log_level_set("MQTT_CLIENT", MQTT_LOGLEVEL);
    esp_log_level_set("MQTT", MQTT_LOGLEVEL);
    esp_log_level_set("TRANSPORT_BASE", MQTT_LOGLEVEL);
    esp_log_level_set("esp-tls", MQTT_LOGLEVEL);
    esp_log_level_set("TRANSPORT", MQTT_LOGLEVEL);
    esp_log_level_set("OUTBOX", MQTT_LOGLEVEL);

    //already done when starting wifi:
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_mqtt_client_config_t mqtt_cfg = { };
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URL;

    //declared globally in the variable section
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

