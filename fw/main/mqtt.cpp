/*
in this file custom functions for using mqtt are defined in c++.
since the mqtt code (mqtt.c) has to be compiled in c,
a separate .cpp file for the custom abstracted c++ functions makes it more clear
*/

extern "C"{
#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "mqtt.h"
}
#include <string>

#include "global.hpp"
#include "mqtt.hpp"


//===========================================
//===== functions for SUBSCRIBED TOPICS =====
//===========================================
//functions run at subscribed topics
//local function to get double value from received data string
int cStrToFloat(char *data, int len, double *result)
{
    ESP_LOGW("mqtt-sub", "received data '%s' len=%d, converting to float...", data, len);
    // copy data to zero terminated buffer
    char buffer[len + 1];
    memcpy(buffer, data, len);
    buffer[len] = '\0';
    // convert the string to a float
    char *endptr;
    double val = std::strtof(buffer, &endptr);
    // check if conversion failed
    if (endptr == buffer || *endptr != '\0')
    {
        ESP_LOGE("mqtt-sub", "failed to convert string to float");
        return 1;
    }
    else
    {
        *result = val;
        return 0;
    }
}

void handleTopicValveSetKp(char * data, int len) {
    double value;
    // convert data to float and assign to Kp if successfull
    if (!cStrToFloat(data, len, &value))
    valveControl.setKp(value);
}
void handleTopicValveSetKi(char * data, int len) {
    double value;
    // convert data to float and assign to Ki if successfull
    if (!cStrToFloat(data, len, &value))
    valveControl.setKi(value);
}
void handleTopicValveSetKd(char * data, int len) {
    double value;
    // convert data to float and assign to Kd if successfull
    if (!cStrToFloat(data, len, &value))
    valveControl.setKd(value);
}


//=================================
//======= subscribed topics =======
//=================================
// configure all subscribed topics
mqtt_subscribedTopic_t mqtt_subscribedTopics[] = {
    {
        "valve set Kp",
        "waterpump/valve/setKp",
        0,
        0,
        handleTopicValveSetKp //function from circulation.hpp
    },
    {
        "valve set Ki",
        "waterpump/valve/setKi",
        0,
        0,
        handleTopicValveSetKi //function from circulation.hpp
    },
    {
        "valve set Kd",
        "waterpump/valve/setKd",
        0,
        0,
        handleTopicValveSetKd //function from circulation.hpp
    },
};
size_t mqtt_subscribedTopics_count = sizeof(mqtt_subscribedTopics) / sizeof(mqtt_subscribedTopic_t);






//---------------------
//----- variables -----
//---------------------
static const char *TAG = "MQTT_cpp";



//---------------------
//----- functions -----
//---------------------
// general custom functions for using mqtt:

//--- publish string ---
void mqtt_publish(char *str, char *topic, uint8_t qos){
    esp_mqtt_client_publish(mqtt_client, topic, str, 0, qos, 0);
    ESP_LOGI(TAG, "Publishing value - topic:'%s' payload:'%s' qos=%d ...", topic, str, qos);
}


//--- publish int ---
void mqtt_publish(int value, char *topic, uint8_t qos){
    size_t maxLen = 32;
    char str[maxLen];
    snprintf(str, maxLen, "%d", value);
    ESP_LOGI(TAG, "Publishing value - topic:'%s' payload:'%s' qos=%d ...", topic, str, qos);
    esp_mqtt_client_publish(mqtt_client, topic, str, 0, qos, 0);
}


//--- publish float ---
void mqtt_publish(float value, char *topic, uint8_t qos){
    size_t maxLen = 32;
    char str[maxLen];
    //check if value fits in string with maxLen (e.g. very large number)
    if (snprintf(str, maxLen, "%.*f", MQTT_PUBLISH_DECIMAL_PLACES, value) >= maxLen){
        ESP_LOGE(TAG, "Number longer than %d places! Not publishing %f to '%s'", maxLen, value, topic);
    }
    else {
        ESP_LOGI(TAG, "Publishing value - topic:'%s' payload:'%s' qos=%d ...", topic, str, qos);
        esp_mqtt_client_publish(mqtt_client, topic, str, 0, qos, 0);
    }
}


//--- check if connected ---
bool mqtt_getConnectedState(){
    ESP_LOGD(TAG, "requested connected state is %s", mqtt_isConnected?"true":"false");
	return mqtt_isConnected;
}


//====================================
//===== CPP=>C wrapper functions =====
//====================================
//those functions can contain c++ code and can be called in c-code (mqtt.c)
//-> they are declared in mqtt.h file (common for mqtt.cpp, mqtt.c)

//subscribe to all configured topics
void mqtt_subscribeTopics() {
	for (int i=0; i<mqtt_subscribedTopics_count; i++) {
		ESP_LOGW(TAG, "subscribing topic '%s'... \t(name '%s')", 
				mqtt_subscribedTopics[i].mqtt_topic, 
				mqtt_subscribedTopics[i].name);
		esp_mqtt_client_subscribe(mqtt_client, mqtt_subscribedTopics[i].mqtt_topic, 0);
	}
}


//handle received data from subscribed topics
//runs corresponding handle function
void mqtt_handleData(char *topic, int topicLen, char *data, int dataLen) {
	//search corresponding configured topic an run its handle function
	for (int i=0; i<mqtt_subscribedTopics_count; i++) {
		if (
				(strlen(mqtt_subscribedTopics[i].mqtt_topic) == topicLen) &&			//length matches
				(strncmp(mqtt_subscribedTopics[i].mqtt_topic, topic,  topicLen) == 0)	//string matches
		   ){
			ESP_LOGI(TAG, "matched entry: '%s', running handleFunction...", mqtt_subscribedTopics[i].name);
			//run the configured function
			mqtt_subscribedTopics[i].handleFunction(data, dataLen);
			//search can be stopped after first match
			break;
		}
	}
}
