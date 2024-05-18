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

#include <cJSON.h>


//---------------------
//----- variables -----
//---------------------
//tag for logging
static const char *TAG = "mqtt-cpp";


//=====================
//===== task_mqtt =====
//=====================
// repeatedly publish variables when connected
#define PUBLISH_INTERVAL_RUNNING 500
#define PUBLISH_INTERVAL_IDLE 3000
void task_mqtt(void *pvParameters)
{
    static const char *TAG = "mqtt-task";
    while (1)
    {
        // debug: log memory stats
        ESP_LOGD(TAG, "free Heap:%ld,%d", esp_get_free_heap_size(), heap_caps_get_free_size(MALLOC_CAP_8BIT));

        // dont do anything when not connected
        if (!mqtt_getConnectedState())
        {
            ESP_LOGW(TAG, "not connected to broker, waiting...");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGV(TAG, "publishing values");

        //publish current pressure
        mqtt_publish(pressureSensor.readBar(), "waterpump/pressure_bar", 0);

        // get current stats from object
        float pressureDiff, targetPressure, p, i, d, valve;
        uint32_t time;
        valveControl.getCurrentStats(&time, &pressureDiff, &targetPressure, &p, &i, &d, &valve);

        // publish motor speed
        mqtt_publish(motor.getSpeedLevel() ,"waterpump/status/motorLevel", 0);

        //publish valve regulation stats
        mqtt_publish(pressureDiff ,"waterpump/valve/pidStats/pressureDiff", 0);
        mqtt_publish(targetPressure ,"waterpump/valve/pidStats/targetPressure", 0);
        mqtt_publish(valve,"waterpump/valve/valvePer", 0);
        mqtt_publish(p,"waterpump/valve/pidStats/p", 0);
        mqtt_publish(i,"waterpump/valve/pidStats/i", 0);
        mqtt_publish(d,"waterpump/valve/pidStats/d", 0);

        // publish flow sensor
        mqtt_publish((float)(flowSensor.getFlowRate_literPerSecond()*60), "waterpump/flowRate_literPerMinute", 0);
        mqtt_publish(flowSensor.getVolume_liter(), "waterpump/volume_liter", 0);

       // publish less frequently when in IDLE mode
        if (control.getMode() == controlMode_t::IDLE)
        {
            vTaskDelay(PUBLISH_INTERVAL_IDLE / portTICK_PERIOD_MS);
        }
        else
        {
            vTaskDelay(PUBLISH_INTERVAL_RUNNING / portTICK_PERIOD_MS);
        }
    }
}

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
void handleTopicValveSetOffset(char * data, int len) {
    double value;
    // convert data to float and assign to Kd if successfull
    if (!cStrToFloat(data, len, &value))
    valveControl.setOffset(value);
}
void handleTopicValveReset(char * data, int len) {
    valveControl.reset();
}

void handleTopicSetTargetPressure(char * data, int len) {
    ESP_LOGW(TAG, "recieved target pressure");
    double value;
    // convert data to float and assign to Kd if successfull
    if (!cStrToFloat(data, len, &value))
    valveControl.setTargetPressure(value);
}

void handleTopicSetAcceptableDiff(char * data, int len) {
    ESP_LOGW(TAG, "recieved acceptable diff");
    double value;
    // convert data to float and update valve config if successfull
    if (!cStrToFloat(data, len, &value))
    valveControl.setAcceptableDiff(value);
}

void handleTopicStart(char * data, int len) {
    ESP_LOGW(TAG, "start via mqtt");
    control.changeMode(REGULATE_PRESSURE);
}

void handleTopicStop(char * data, int len) {
    ESP_LOGW(TAG, "stop via mqtt");
    control.changeMode(IDLE);
}

void handleTopicRequestParameters(char * data, int len) {
    ESP_LOGW(TAG, "publish current parameters as requested");
    double p, i, d, offset;
    float acceptableDiff;
    valveControl.getCurrentSettings(&p, &i, &d, &offset, &acceptableDiff);
    mqtt_publish((float)p, "waterpump/valve/getSettings/Kp", 0);
    mqtt_publish((float)i, "waterpump/valve/getSettings/Ki", 0);
    mqtt_publish((float)d, "waterpump/valve/getSettings/Kd", 0);
    mqtt_publish((float)offset, "waterpump/valve/getSettings/offset", 0);
    mqtt_publish(acceptableDiff, "waterpump/valve/getSettings/acceptableDiff", 0);
}

//=================================
//======= subscribed topics =======
//=================================
// configure all subscribed topics
mqtt_subscribedTopic_t mqtt_subscribedTopics[] = {
    //==== configuration ====
    {
        "valve set Kp",
        "waterpump/valve/setKp",
        0,
        0,
        handleTopicValveSetKp
    },
    {
        "valve set Ki",
        "waterpump/valve/setKi",
        0,
        0,
        handleTopicValveSetKi
    },
    {
        "valve set Kd",
        "waterpump/valve/setKd",
        0,
        0,
        handleTopicValveSetKd
    },
    {
        "valve set Offset",
        "waterpump/valve/setOffset",
        0,
        0,
        handleTopicValveSetOffset
    },
    {
        "set acceptable pressure diff",
        "waterpump/valve/setAcceptableDiff",
        0,
        0,
        handleTopicSetAcceptableDiff
    },

    //==== actions ====
    {
        "valve reset",
        "waterpump/valve/reset",
        0,
        0,
        handleTopicValveReset
    },
    {
        "start pump",
        "waterpump/start",
        0,
        0,
        handleTopicStart
    },
    {
        "stop pump",
        "waterpump/stop",
        0,
        0,
        handleTopicStop
    },
    {
        "set target pressure",
        "waterpump/setTargetPressure",
        0,
        0,
        handleTopicSetTargetPressure
    },
    {
        "request parameters",
        "waterpump/requestParameters",
        0,
        0,
        handleTopicRequestParameters
    },
};
size_t mqtt_subscribedTopics_count = sizeof(mqtt_subscribedTopics) / sizeof(mqtt_subscribedTopic_t);








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
