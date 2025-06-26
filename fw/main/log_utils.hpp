#pragma once
#include "esp_log.h"
#include "mqttTopics.h"
#include "mqtt.hpp"  // or wherever `mqtt_publish()` is declared

// publish topic (defined in mqttTopics.h)
#ifndef MQTT_TOPIC__LOG_ERROR
#define MQTT_TOPIC__LOG_ERROR "waterpump/log/error"
#endif

// optional compile-time toggle for MQTT logging
#ifndef LOG_MQTT_ENABLED
#define LOG_MQTT_ENABLED 1
#endif



// macro for logging error to both terminal (ESP_LOGE) and MQTT
#define LOGE_TERM_AND_MQTT(tag, fmt, ...) do {                                    \
    ESP_LOGE(tag, fmt, ##__VA_ARGS__);                                            \
    if (LOG_MQTT_ENABLED) {                                                       \
        char buf[256];                                                            \
        snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__);                           \
        mqtt_publish(buf, MQTT_TOPIC__LOG_ERROR);                                 \
    }                                                                             \
} while (0)
