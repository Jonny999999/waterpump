#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "sensordeklaration.h"
#include "wasserpumpe.h"
#include "mqtt.h"

//=====CONFIG MQTT BROKER=======
const char *mqtt_uri = "mqtt://10.0.0.102";
uint32_t mqtt_port = 1883;
int temp_intervall_ms = 10000;
int analog_intervall_ms = 20000;
int mqtt_intervall_ms = 15000;
// IDEE VERWORFEN uin8 array?? -> wlan in make menuconfig
// const char *wlan_ssid = "Projektarbeit";
// const char *wlan_pwd = "asd";

//=====Analogsensoren=====
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
// const char name[16];
// const char mqtt_topic[64];
////sampleanzahl, delay?
// const adc1_channel_t channel1;
// static const adc_channnel_t channel;
// const adc_atten_t atten;        //spannungsbereich
// const float umrechnungsfaktor;
// float value;

int analogsensor_anzahl = 2;
struct ANALOG_Device Analogsensoren[] = {
    {
        .name = "Drucksensor", // 100psi drucksensor
        .mqtt_topic = "Wasserpumpe/druck",
        .channel1 = ADC1_CHANNEL_0,     // GPIO36
        .channel = ADC1_CHANNEL_0,      // GPIO36
        .atten = ADC_ATTEN_MAX,         // 0-3V
        .MODE = DC,                     // Mittelwert berechnen
        .umrechnung_faktor = 0.025 * 2, // 100psi drucksensor
        .umrechnung_offset = -12.5,
        .umrechnung_einheit = 0.068948, // psi in bar
        .value = 11,
        .timestamp = 0,
    },
    {
        .name = "Poti", // 100psi drucksensor
        .mqtt_topic = "Wasserpumpe/poti",
        .channel1 = ADC1_CHANNEL_6,     // GPIO34
        .channel = ADC1_CHANNEL_6,      // GPIO34
        .atten = ADC_ATTEN_MAX,         // 0-3V
        .MODE = DC,                     // Mittelwert berechnen
        .umrechnung_faktor = 0.025 * 2, // 100psi drucksensor
        .umrechnung_offset = -12.5,
        .umrechnung_einheit = 0.068948, // psi in bar
        .value = 11,
        .timestamp = 0,
    }};




//=======DIGITALSENSOREN=======
extern struct DIGITAL_Device Digitalsensoren[];
extern int digitalsensor_anzahl;
int digitalsensor_anzahl = 0;
struct DIGITAL_Device Digitalsensoren[] = {

    {
        .name = "Pumpe HK2 gpio25", // 5V digital input von sensorboard
        .mqtt_topic = "Sensordaten/Heizraum/Heizkreise/HK2/Pumpe",
        .gpio_pin = 25,    // 25,26,27,14, ,18,05
        .sample_count = 1, //(alle 1ms)
        .factor = 100,     // z.B. 1 fuer bool / 100 fuer duty%
        .inverted = true,
        .value = 0,
    }};




//======================MQTT subscribe / input======================
// Funktionen definieren die bei konfigurierten ankommenden topics aufgerufen werden koennen:
void relais_on(char *data, int len)
{
    printf("this is relais_on function in sensordeklaration.c - lendata: %d\n", len);
    // Data ausgeben:
    for (int i = 0; i < len; i++)
    {
        printf("%c", data[i]);
    };
    printf("\n");
    gpio_set_level(13, 1);
}

void relais_off(char *data, int len)
{
    printf("this is relais_off function in sensordeklaration.c - lendata: %d\n", len);
    // Data ausgeben:
    for (int i = 0; i < len; i++)
    {
        printf("%c", data[i]);
    };
    printf("\n");
    gpio_set_level(13, 0);
}

void piep_count(char *data, int len)
{
    printf("this is piepl function in sensordeklaration.c - lendata: %d\n", len);
    uint8_t gpio_pin = 12;
    uint8_t delayOn_ms = 20;
    uint8_t delayOff_ms = 60;

    char string[len];
    for (int i = 0; i < len; i++)
    {
        printf("%c", data[i]);
        string[i] = data[i];
    };
    int count = atoi(string);

    for (uint8_t i = 0; i < count; i++)
    {
        gpio_set_level(gpio_pin, 1);
        vTaskDelay(delayOn_ms / portTICK_PERIOD_MS);
        gpio_set_level(gpio_pin, 0);
        vTaskDelay(delayOff_ms / portTICK_PERIOD_MS);
    }
    // gpio_set_level(12, 1);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    // gpio_set_level(12, 0);
}

// stepper 1
void st1_toggle(char *data, int len)
{
    if (wp.st1 == false)
    {
        wp.st1 = true;
        gpio_set_level(15, 1);
    }
    else
    {
        wp.st1 = false;
        gpio_set_level(15, 0);
    }
    printf("st1 toggled");
    mqtt_publish_bool(wp.st1, "wasserpumpe/st1/state");
}

// stepper 2
void st2_toggle(char *data, int len)
{
    if (wp.st2 == false)
    {
        wp.st2 = true;
        gpio_set_level(2, 1);
    }
    else
    {
        wp.st2 = false;
        gpio_set_level(2, 0);
    }
    printf("st2 toggled");
    mqtt_publish_bool(wp.st2, "wasserpumpe/st2/state");
}

// stepper 3
void st3_toggle(char *data, int len)
{
    if (wp.st3 == false)
    {
        wp.st3 = true;
        gpio_set_level(16, 1);
    }
    else
    {
        wp.st3 = false;
        gpio_set_level(16, 0);
    }
    printf("st3 toggled");
    mqtt_publish_bool(wp.st3, "wasserpumpe/st3/state");
}

// stepper 4
void st4_toggle(char *data, int len)
{
    if (wp.st4 == false)
    {
        wp.st4 = true;
        gpio_set_level(4, 1);
    }
    else
    {
        wp.st4 = false;
        gpio_set_level(4, 0);
    }
    printf("st4 toggled");
    mqtt_publish_bool(wp.st4, "wasserpumpe/st4/state");
}

void fu_setlevel(char *data, int len)
{
    char string[len];
    for (int i = 0; i < len; i++)
    {
        printf("%c", data[i]);
        string[i] = data[i];
    };
    int level = atoi(string);
    wp_FuSetLevel(level);
}

void set_pressure(char *data, int len)
{
    char string[len];
    for (int i = 0; i < len; i++)
    {
        printf("%c", data[i]);
        string[i] = data[i];
    };
    float pressure = atof(string);
    wp.druck_soll = pressure;
}

// Informationen - struct fuer MQTT subscribed topics
int MQTT_SubscribedTopics_anzahl = 9;
struct MQTT_SubscribedTopic MQTT_SubscribedTopics[] = {
    {.name = "Relais anschalten",
     .mqtt_topic = "wasserpumpe/relais/on",
     .timestamp = 0,
     .value = 0,
     .handleFunction = relais_on},
    {.name = "Relais ausschalten",
     .mqtt_topic = "wasserpumpe/relais/off",
     .timestamp = 0,
     .value = 0,
     .handleFunction = relais_off},
    {.name = "count Piepsen oder blinken",
     .mqtt_topic = "wasserpumpe/buzzer/count",
     .timestamp = 0,
     .value = 0,
     .handleFunction = piep_count},

    {.name = "ST1 toggle",
     .mqtt_topic = "wasserpumpe/st1/toggle",
     .timestamp = 0,
     .value = 0,
     .handleFunction = st1_toggle},
    {.name = "ST2 toggle",
     .mqtt_topic = "wasserpumpe/st2/toggle",
     .timestamp = 0,
     .value = 0,
     .handleFunction = st2_toggle},
    {.name = "ST3 toggle",
     .mqtt_topic = "wasserpumpe/st3/toggle",
     .timestamp = 0,
     .value = 0,
     .handleFunction = st3_toggle},
    {.name = "ST4 toggle",
     .mqtt_topic = "wasserpumpe/st4/toggle",
     .timestamp = 0,
     .value = 0,
     .handleFunction = st4_toggle},

    {.name = "FU set level",
     .mqtt_topic = "wasserpumpe/fu/level/set",
     .timestamp = 0,
     .value = 0,
     .handleFunction = fu_setlevel},

    {.name = "solldruck",
     .mqtt_topic = "wasserpumpe/druck/set",
     .timestamp = 0,
     .value = 0,
     .handleFunction = set_pressure}

};
