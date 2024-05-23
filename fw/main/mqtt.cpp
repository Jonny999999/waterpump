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

// all MQTT_TOPIC__ definitions:
#include "mqttTopics.h"


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

        // get current stats from object
        float pressureDiff, targetPressure, p, i, d, valve;
        uint32_t time;
        valveControl.getCurrentStats(&time, &pressureDiff, &targetPressure, &p, &i, &d, &valve);

        // publish motor speed
        mqtt_publish(motor.getSpeedLevel(), MQTT_TOPIC__MOTOR_LEVEL, 0);

        //publish current pressure
        mqtt_publish(pressureSensor.readBar(), MQTT_TOPIC__PRESSURE_BAR, 0);

        //publish valve regulation stats
        mqtt_publish(pressureDiff,    MQTT_TOPIC__PRESSURE_DIFF, 0);
        mqtt_publish(targetPressure,  MQTT_TOPIC__TARGET_PRESSURE, 0);
        mqtt_publish(valve, MQTT_TOPIC__VALVE_PER, 0);
        mqtt_publish(p,     MQTT_TOPIC__P_VALUE, 0);
        mqtt_publish(i,     MQTT_TOPIC__I_VALUE, 0);
        mqtt_publish(d,     MQTT_TOPIC__D_VALUE, 0);

        // publish flow sensor
        mqtt_publish((float)(flowSensor.getFlowRate_literPerSecond()*60), MQTT_TOPIC__FLOW_LITER_PER_MINUTE, 0);
        mqtt_publish(flowSensor.getVolume_liter(), MQTT_TOPIC__VOLUME_LITER, 0);

       // publish less frequently when in IDLE mode
        if (control.getMode() == controlMode_t::IDLE)
            vTaskDelay(PUBLISH_INTERVAL_IDLE / portTICK_PERIOD_MS);
        else
            vTaskDelay(PUBLISH_INTERVAL_RUNNING / portTICK_PERIOD_MS);
    } // end while
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

//--- set config options ---
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
void handleTopicSetAcceptableDiff(char * data, int len) {
    ESP_LOGW(TAG, "recieved acceptable diff");
    double value;
    // convert data to float and update valve config if successfull
    if (!cStrToFloat(data, len, &value))
    valveControl.setAcceptableDiff(value);
}

//--- control pump ---
void handleTopicSetTargetPressure(char * data, int len) {
    ESP_LOGW(TAG, "recieved target pressure");
    double value;
    // convert data to float and assign to Kd if successfull
    if (!cStrToFloat(data, len, &value))
    valveControl.setTargetPressure(value);
}
void handleTopicStart(char * data, int len) {
    ESP_LOGW(TAG, "start via mqtt");
    control.changeMode(REGULATE_PRESSURE);
    // immediately publish changed motor level
    vTaskDelay(50 / portTICK_PERIOD_MS);
    mqtt_publish(motor.getSpeedLevel(), MQTT_TOPIC__MOTOR_LEVEL, 0);
}
void handleTopicStop(char * data, int len) {
    ESP_LOGW(TAG, "stop via mqtt");
    control.changeMode(IDLE);
    // immediately publish changed motor level
    vTaskDelay(50 / portTICK_PERIOD_MS);
    mqtt_publish(motor.getSpeedLevel(), MQTT_TOPIC__MOTOR_LEVEL, 0);
}

//--- other requests ---
void handleTopicRequestParameters(char * data, int len) {
    ESP_LOGW(TAG, "publish current parameters as requested");
    double p, i, d, offset;
    float acceptableDiff;
    valveControl.getCurrentSettings(&p, &i, &d, &offset, &acceptableDiff);
    mqtt_publish((float)p, MQTT_TOPIC__KP, 0);
    mqtt_publish((float)i, MQTT_TOPIC__KI, 0);
    mqtt_publish((float)d, MQTT_TOPIC__KD, 0);
    mqtt_publish((float)offset, MQTT_TOPIC__OFFSET, 0);
    mqtt_publish(acceptableDiff, MQTT_TOPIC__ACCEPTABLE_DIFF, 0);
}

//=================================
//======= subscribed topics =======
//=================================
// configure all subscribed topics
mqtt_subscribedTopic_t mqtt_subscribedTopics[] = {
    //==== configuration ====
    {
        "valve set Kp",
        MQTT_TOPIC__SET_KP,
        0,
        0,
        handleTopicValveSetKp
    },
    {
        "valve set Ki",
        MQTT_TOPIC__SET_KI,
        0,
        0,
        handleTopicValveSetKi
    },
    {
        "valve set Kd",
        MQTT_TOPIC__SET_KD,
        0,
        0,
        handleTopicValveSetKd
    },
    {
        "valve set Offset",
        MQTT_TOPIC__SET_OFFSET,
        0,
        0,
        handleTopicValveSetOffset
    },
    {
        "set acceptable pressure diff",
        MQTT_TOPIC__SET_ACCEPTABLE_DIFF,
        0,
        0,
        handleTopicSetAcceptableDiff
    },

    //==== actions ====
    {
        "valve reset",
        MQTT_TOPIC__RESET,
        0,
        0,
        handleTopicValveReset
    },
    {
        "start pump",
        MQTT_TOPIC__START,
        0,
        0,
        handleTopicStart
    },
    {
        "stop pump",
        MQTT_TOPIC__STOP,
        0,
        0,
        handleTopicStop
    },
    {
        "set target pressure",
        MQTT_TOPIC__SET_TARGET_PRESSURE,
        0,
        0,
        handleTopicSetTargetPressure
    },
    {
        "request parameters",
        MQTT_TOPIC__REQUEST_PARAMMETERS,
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
void mqtt_publish(char *str, const char *topic, uint8_t qos){
    esp_mqtt_client_publish(mqtt_client, topic, str, 0, qos, 0);
    ESP_LOGI(TAG, "Publishing value - topic:'%s' payload:'%s' qos=%d ...", topic, str, qos);
}


//--- publish int ---
void mqtt_publish(int value, const char *topic, uint8_t qos){
    size_t maxLen = 32;
    char str[maxLen];
    snprintf(str, maxLen, "%d", value);
    ESP_LOGI(TAG, "Publishing value - topic:'%s' payload:'%s' qos=%d ...", topic, str, qos);
    esp_mqtt_client_publish(mqtt_client, topic, str, 0, qos, 0);
}


//--- publish float ---
void mqtt_publish(float value, const char *topic, uint8_t qos){
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
