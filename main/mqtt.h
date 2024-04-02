#pragma once

#include "mqtt_client.h"
#include "sensordeklaration.h"
#include "wasserpumpe.h"

//void mqtt_app_start(void);
extern esp_mqtt_client_handle_t client;
void mqtt_app_start(void);

//void mqtt_SubscribeTopics(void);
void mqtt_SubscribeTopics(void);

void mqtt_publishtest();

void mqtt_PublishAnalogsensors(struct ANALOG_Device * f_sensoren, int numSensor);

void mqtt_PublishDigitalsensors(struct DIGITAL_Device * f_sensoren, int numSensor);




void mqtt_publish_float(float value, char * topic);

void mqtt_publish_bool(bool value, char * topic);

void mqtt_PublishWasserpumpe(struct wasserpumpe_t wp);

