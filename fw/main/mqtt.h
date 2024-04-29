/*
function declarations from the esp-idf mqtt example
(only needed for starting the mqtt-client in main.cpp and in mqtt.cpp for futher abstraction)
*/

#pragma once
#include "mqtt_client.h"


//global client configuration
extern esp_mqtt_client_handle_t mqtt_client;
extern bool mqtt_isConnected;

//configure and start mqtt client and registers event handler
void mqtt_app_start(void);

//also see event handler function in mqtt.c


//wrapper functions that can be run from c-code in mqtt.c
//but are defined with c++ code in mqtt.cpp
void mqtt_subscribeTopics();
void mqtt_handleData(char * topic, int topicLen, char * data, int dataLen);

