extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "driver/adc.h"
}

#include <stdio.h>
#include "vfd.hpp"
#include "mode.hpp"
#include "servo.hpp"
#include "pressureSensor.hpp"
#include "pumpControl.hpp"

//tag for logging
static const char * TAG = "main";

extern "C" void app_main(void)
{
    // set loglevel
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("VFD", ESP_LOG_DEBUG);
    esp_log_level_set("servo", ESP_LOG_INFO);
    esp_log_level_set("control", ESP_LOG_INFO);
    esp_log_level_set("regulateValve", ESP_LOG_DEBUG);
    esp_log_level_set("reulateMotor", ESP_LOG_DEBUG);

    //enable 5V volage regulator (needed for pressure sensor and flow meter)
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_17, 1);

    // create motor object
    Vfd4DigitalPins motor(GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_2, GPIO_NUM_15, true);

    // create servo object
    servoConfig_t servoConfig {
        .gpioPwmSignal = 27,
        .gpioEnablePower = 13, // onboard realy
        .powerEnableRequired = true, // require enable() call to turn above pin on
        .ratedAngle = 180,
        // Coupling V1: 17 to 87 (no play)
        // Coupling V2: 11 to 89 deg 
        .minAllowedAngle = 17, // valve completely closed
        .maxAllowedAngle = 87, // valve completely open
        .invertDirection = true
    };
    ServoMotor servo(servoConfig);
    servo.enable(); // turn servo power supply on (onboard relay)

    // create control object
    controlConfig_t controlConfig{
        .defaultMode = IDLE,
        .gpioSetButton = GPIO_NUM_11,
        .gpioStatusLed = GPIO_NUM_10
    };
    SystemModeController control(controlConfig);

    // create pressure sensor on gpio 36
    AnalogPressureSensor pressureSensor(ADC1_CHANNEL_0, 0.25, 2.5, 0, 30);

    // create controlled valve object that regulates the valve position
    ControlledValve valveControl(&servo);

// configure adc for poti
#define ADC_POTI ADC1_CHANNEL_6 //gpio34
    adc1_config_width(ADC_WIDTH_BIT_12);                  //=> max resolution 4096
    adc1_config_channel_atten(ADC_POTI, ADC_ATTEN_DB_11); //max voltage

    // create control task
    // TODO: is this task necessary?
    //xTaskCreate(&task_control, "task_control", 4096, &control, 5, NULL);

    //TODO add tasks "regulate-pressure", "mqtt", ...

    //===== TESTING =====

//--- test vfd ---
//#define VFD_TEST
#ifdef VFD_TEST
    // test on/off
    motor.turnOn();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    motor.turnOff();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    motor.turnOn();
    // test speed levels
    while (1)
    {
        // vary speed level time based
        // motor.setSpeedLevel(0);
        // motor.turnOn();
        // vTaskDelay(2000 / portTICK_PERIOD_MS);
        // for (int i = 0; i < 10; i++)
        //{
        //     motor.setSpeedLevel(i);
        //     vTaskDelay(2000 / portTICK_PERIOD_MS);
        // }
        // motor.turnOff();
        // vTaskDelay(5000 / portTICK_PERIOD_MS);

        // set motor speed using poti
        int potiRaw = adc1_get_raw(ADC_POTI);
        float potiPercent = (float)potiRaw / 4095 * 100;
        motor.setSpeedLevel(potiPercent * 4 / 100);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
#endif




// --- test poti, servo, pressure-sensor ---
//#define SERVO_TEST
#ifdef SERVO_TEST
while(1){
    // test pressure sensor
    pressureSensor.readBar();

    // read poti
    int potiRaw = adc1_get_raw(ADC_POTI);
    float potiPercent = (float)potiRaw/4095*100;
    ESP_LOGI(TAG, "poti adc=%d per=%f", potiRaw, potiPercent);

    // apply poti to servo
    servo.setPercentage(potiPercent);
    //servo.setAngle(180*potiPercent/100);

    vTaskDelay(100 / portTICK_PERIOD_MS);
}
#endif

//--- test pressure regulation ---
#define REGULATION_TEST
#ifdef REGULATION_TEST
#define MAX_PRESSURE 8
    // turn on at startup
    motor.turnOn();
    while (1)
    {
        // TODO add timeouts
        //  read poti
        int potiRaw = adc1_get_raw(ADC_POTI);
        float potiPercent = (float)potiRaw / 4095 * 100;
        // define target pressure
        float pressureTarget = potiPercent * MAX_PRESSURE / 100;
        // read pressure
        float pressureNow = pressureSensor.readBar();
        // calcuate difference (positive = too low,  negative = too high)
        float pressureDiff = pressureTarget - pressureNow;

        ESP_LOGI(TAG, "poti=%d, pTarget=%.2fbar, pNow=%.2fbar, diff=%.2f",
                 potiRaw, pressureTarget, pressureNow, pressureDiff);

        // regulate valve pos
        valveControl.compute(pressureDiff);

        // regulate motor speed
        regulateMotor(pressureDiff, &servo, &motor);

        vTaskDelay(250 / portTICK_PERIOD_MS);
}
#endif


    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }
}
