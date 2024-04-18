/*
custom abstract functions for using mqtt in c++
those are planned to be used most of the time in the entire project
*/

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <cstddef>


//----------------------
//------- config -------
//----------------------
#define MQTT_PUBLISH_DECIMAL_PLACES 4 //accuracy float values are published


/* qos:
  0: at most once
  1: at least once
  2: exactly once
*/

// task implemented in mqtt.cpp that repeatedly publishes certain data
void task_mqtt( void * pvParameters );

//--- publish string ---
//FIXME: string const too? calling the function without a variable (e.g. "asdasd") results in warning: C++ forbids converting a string constant to 'char*'
//TODO: add return value if successful?
void mqtt_publish(char *string, char *topic, uint8_t qos = 0);

//--- publish int ---
void mqtt_publish(int value, char *topic, uint8_t qos = 0);

//--- publish float ---
void mqtt_publish(float value, char *topic, uint8_t qos = 0);

//--- check if connected ---
bool mqtt_getConnectedState();

//struct for mqtt actions / subscribed topics
typedef struct mqtt_subscribedTopic_t
{
    const char name[32];
    const char mqtt_topic[64];
    uint32_t timestamp;
    float value;
    void (*handleFunction)(char * data, int len);
} mqtt_subscribedTopic_t;

//global array of configured mqtt actions / subscribed topics
//actually defined in firmware/board_XXX/config/mqttActions.cpp
extern struct mqtt_subscribedTopic_t mqtt_subscribedTopics[];
extern size_t mqtt_subscribedTopics_count;
