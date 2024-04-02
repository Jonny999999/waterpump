#pragma once
#include "driver/adc.h"

//=====CONFIG MQTT BROKER======
extern const char *mqtt_uri;
extern uint32_t mqtt_port;
extern int temp_intervall_ms;
extern int analog_intervall_ms;
extern int mqtt_intervall_ms;
// Todo: Idee verworfen, uin8 array? -> wlan in make menuconfig
// extern const char *wlan_ssid;
// extern const char *wlan_pwd;

//=======struct fuer Temperatursensoren======
typedef enum
{
    DS1 = 1,
    DS2
} DS_SELECT;



typedef struct OWB_Sensor
{
    const char name[32];
    const char mqtt_topic[64];
    const char rom_code[18];
    int device_id;
    DS_SELECT dsPin; // wird beim scan festgelegt
    uint32_t timestamp;
    float value;
    // Todo: max samplerate? (unwichtige Sensoren)
} OWB_Sensor;

extern struct OWB_Sensor Temperatursensoren_DS[];
extern int temperatursensor_anzahl;

//=======struct fuer Sensoren an Analogeingaengen=======
//
/*  enum adc1_channel_t:        enum adc_atten_t:

    ADC1_CHANNEL_0 = 0              ADC_ATTEN_DB_0 = 0
    ADC1 channel 0 is GPIO36        The input voltage of ADC will be reduced to about 1/1
    ADC1_CHANNEL_1                  ADC_ATTEN_DB_2_5 = 1
    ADC1 channel 1 is GPIO37        The input voltage of ADC will be reduced to about 1/1.34
    ADC1_CHANNEL_2                  ADC_ATTEN_DB_6 = 2
    ADC1 channel 2 is GPIO38        The input voltage of ADC will be reduced to about 1/2
    ADC1_CHANNEL_3                  ADC_ATTEN_DB_11 = 3
    ADC1 channel 3 is GPIO39        The input voltage of ADC will be reduced to about 1/3.6
    ADC1_CHANNEL_4                  ADC_ATTEN_MAX
    ADC1 channel 4 is GPIO32        ??
    ADC1_CHANNEL_5
    ADC1 channel 5 is GPIO33
    ADC1_CHANNEL_6
    ADC1 channel 6 is GPIO34
    ADC1_CHANNEL_7
    ADC1 channel 7 is GPIO35
    */


typedef enum
{
    AC,
    DC
} AS_MODE;



typedef struct ANALOG_Device
{
    const char name[32];
    const char mqtt_topic[64];
    // Todo sampleanzahl, delay?
    const adc1_channel_t channel1;
    const adc_channel_t channel;
    const adc_atten_t atten;       // Spannungsbereich
    const AS_MODE MODE;            // Modus AC: effektivwert von Mittelwert aus berechnen / DC: Mittelwert berechnen
    const float umrechnung_faktor; // ausgehend von Millivolt
    const float umrechnung_offset;
    const float umrechnung_einheit;
    float value;
    uint32_t timestamp;
} ANALOG_Device;
extern struct ANALOG_Device Analogsensoren[];
extern int analogsensor_anzahl;



// struct fuer Sensoren/Signale an Digitaleingaengen
typedef struct DIGITAL_Device
{
    const char name[32];
    const char mqtt_topic[64];
    const int sample_count; // samples (alle 1ms)
    const gpio_num_t gpio_pin;
    const int factor; // z.B. 100 fuer duty%
    const bool inverted;
    float value;
} DIGITAL_Device;
extern struct DIGITAL_Device Digitalsensoren[];
extern int digitalsensor_anzahl;



// struct fuer MQTT subscribed topics
typedef struct MQTT_SubscribedTopic
{
    const char name[32];
    const char mqtt_topic[64];
    uint32_t timestamp;
    float value;
    void (*handleFunction)(char *data, int len);
} MQTT_SubscribedTopic;



extern struct MQTT_SubscribedTopic MQTT_SubscribedTopics[];
extern int MQTT_SubscribedTopics_anzahl;

void MQTT_SubscribedTopicsActions(char *topic, int topic_len, char *data, int data_len);
