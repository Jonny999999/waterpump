extern "C" {
#include "esp_log.h"
}
#include "vfd.hpp"

//tag for logging
static const char * TAG = "VFD";


//=======================
//===== Constructor =====
//=======================
Vfd4DigitalPins::Vfd4DigitalPins(
    gpio_num_t speedBit0, // Speed input 3
    gpio_num_t speedBit1, // Speed input 2
    gpio_num_t speedBit2, // Speed input 1
    gpio_num_t gpioTurnOn,
    bool invertSpeedBits)
{
    ESP_LOGW(TAG, "Initializing VFD...");
    // copy configuration
    mGpioBit0 = speedBit0;
    mGpioBit1 = speedBit1;
    mGpioBit2 = speedBit2;
    mGpioOn = gpioTurnOn;
    mInvertSpeedBits = invertSpeedBits;
    // init gpios
    initGpios();
    // set default state
    setSpeedLevel(0);
    turnOff();
}


//======================
//======== init ========
//======================
void Vfd4DigitalPins::initGpios()
{
    ESP_LOGI(TAG, "configuring gpio pins...");
    gpio_set_direction(mGpioBit0, GPIO_MODE_OUTPUT);
    gpio_set_direction(mGpioBit1, GPIO_MODE_OUTPUT);
    gpio_set_direction(mGpioBit2, GPIO_MODE_OUTPUT);
    gpio_set_direction(mGpioOn, GPIO_MODE_OUTPUT);
}


//======================
//======= turnOn =======
//======================
void Vfd4DigitalPins::turnOn()
{
    gpio_set_level(mGpioOn, 1);
    ESP_LOGI(TAG, "Turned ON VFD, current level=%d", mSpeedLevel);
}
void Vfd4DigitalPins::turnOn(uint8_t speedLevel)
{
    setSpeedLevel(speedLevel);
    turnOn();
}


//=======================
//======= turnOff =======
//=======================
void Vfd4DigitalPins::turnOff()
{
    gpio_set_level(mGpioOn, 0);
    ESP_LOGI(TAG, "Turned OFF VFD");
}


//=========================
//===== setSpeedLevel =====
//=========================
//TODO 0 is off?
void Vfd4DigitalPins::setSpeedLevel(uint8_t newSpeedLevel)
{
    // limit to max range 0-7 (3 bits)
    if (newSpeedLevel > 0b111)
    {
        mSpeedLevel = 0b111;
        ESP_LOGE(TAG, "target speed level '%d' exceeds limit -> setting to '%d' (max)", newSpeedLevel, 0b111);
    }
    else
        mSpeedLevel = newSpeedLevel;

    // invert level bits for inverted output if configured
    uint8_t level;
    if (mInvertSpeedBits)
        level = ~mSpeedLevel;
    else
        level = mSpeedLevel;

    // apply 3 bits of level to gpio pins
    gpio_set_level(mGpioBit0, level & 1 << 0);
    gpio_set_level(mGpioBit1, level & 1 << 1);
    gpio_set_level(mGpioBit2, level & 1 << 2);
    // log current pin state
    ESP_LOGD(TAG, "set speed level to %d, outputs: %d%d%d",
             mSpeedLevel,
             (bool)(level & 1 << 2),
             (bool)(level & 1 << 1),
             (bool)(level & 1 << 0));
}