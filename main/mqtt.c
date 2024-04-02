#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"

#include "sensordeklaration.h"
#include "mqtt.h"

#include "mqtt_client.h"
#include "wasserpumpe.h"

// Globale Variable fuer gestarteten mqtt client
esp_mqtt_client_handle_t client;

static const char *TAG = "MQTT_EXAMPLE";
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)



{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_SubscribeTopics(); // Topics abonieren
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("MQTT data recieved! id: %d\n", event->event_id);
        for (int i = 0; i < MQTT_SubscribedTopics_anzahl; i++)
        { // alle deklarierten outputs durchgehen
            if (strncmp(MQTT_SubscribedTopics[i].mqtt_topic, event->topic, event->topic_len) == 0)
            {                                                                          // deklariertes topic mit ankommenden topic vergleichen
                MQTT_SubscribedTopics[i].handleFunction(event->data, event->data_len); // funktion des outputs ausfuehren
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}



void mqtt_app_start(void)
{
    printf("\n starting mqtt task!\n");

    //====MQTT client deklarieren und starten====
    esp_mqtt_client_config_t mqtt_cfg = {
        //.uri = CONFIG_BROKER_URL,
        .uri = mqtt_uri, // aus sensordeklaration.c
        .event_handle = mqtt_event_handler,
        .port = mqtt_port // aus sensordeklaration.c
    };
    // esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_start(client);
}



void mqtt_SubscribeTopics(void)
{
    // alle deklarierten Outputs durchgehen und jedem topic subscriben
    int msg;
    for (int i = 0; i < MQTT_SubscribedTopics_anzahl; i++)
    { // alle deklarierten outputs durchgehen
        msg = esp_mqtt_client_subscribe(client, MQTT_SubscribedTopics[i].mqtt_topic, 1);
        // int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char *topic, int qos);
        printf("subscribed to topic: %s - name: %s\n", MQTT_SubscribedTopics[i].mqtt_topic, MQTT_SubscribedTopics[i].name);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg);
    }
}



void mqtt_PublishAnalogsensors(struct ANALOG_Device *f_sensoren, int numSensor) // fixxme: pfusch OBVsensor struct // analogsensor struct...
{
    printf("=====> Publishing ANALOG");
    for (size_t i = 0; i < numSensor; i++)
    {
        // convert float to string:
        char *pointer_text;
        char text[32];
        sprintf(text, "%.2f", f_sensoren[i].value);
        pointer_text = text;
        printf("MQTT: publishing sensor: %s, topic: %s, value: %s\n", f_sensoren[i].name, f_sensoren[i].mqtt_topic, pointer_text);
        int msg_id = esp_mqtt_client_publish(client, f_sensoren[i].mqtt_topic, pointer_text, 0, 0, 0);
    }
}



void mqtt_PublishDigitalsensors(struct DIGITAL_Device *f_sensoren, int numSensor) // fixxme: pfusch OBVsensor struct // analogsensor struct...
{
    printf("=====> Publishing DIGITAL");
    for (size_t i = 0; i < numSensor; i++)
    {
        // convert float to string:
        char *pointer_text;
        char text[32];
        sprintf(text, "%.2f", f_sensoren[i].value);
        pointer_text = text;
        printf("MQTT: publishing sensor: %s, topic: %s, value: %s\n", f_sensoren[i].name, f_sensoren[i].mqtt_topic, pointer_text);
        int msg_id = esp_mqtt_client_publish(client, f_sensoren[i].mqtt_topic, pointer_text, 0, 0, 0);
    }
}



void mqtt_publish_float(float value, char *topic)
{
    char *pointer_text;
    char text[32];
    sprintf(text, "%.2f", value);
    pointer_text = text;
    printf("MQTT: publishing topic: %s, value: %s\n", topic, pointer_text);
    int msg_id = esp_mqtt_client_publish(client, topic, pointer_text, 0, 0, 0);
}



void mqtt_publish_bool(bool value, char *topic)
{
    char *pointer_text;
    char text[32];
    sprintf(text, "%d", value);
    pointer_text = text;
    printf("MQTT: publishing topic: %s, value: %s\n", topic, pointer_text);
    int msg_id = esp_mqtt_client_publish(client, topic, pointer_text, 0, 0, 0);
}



void mqtt_PublishWasserpumpe(struct wasserpumpe_t wp)
{
    printf("===> Publishing Wasserpumpe\n");
    mqtt_publish_float(wp.druck_soll, "wasserpumpe/druck/soll");
    mqtt_publish_float(wp.druck, "wasserpumpe/druck/ist/");
    mqtt_publish_float(wp.druck_max, "wasserpumpe/druck/ist/max");
    mqtt_publish_float(wp.druck_min, "wasserpumpe/druck/ist/min");
    mqtt_publish_float(wp.fu_level, "wasserpumpe/fu/level/current");

    mqtt_publish_bool(wp.st1, "wasserpumpe/st1/state");
    mqtt_publish_bool(wp.st2, "wasserpumpe/st2/state");
    mqtt_publish_bool(wp.st3, "wasserpumpe/st3/state");
    mqtt_publish_bool(wp.st4, "wasserpumpe/st4/state");
}