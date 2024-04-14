extern "C" {
#include "esp_log.h"
#include "esp_rom_gpio.h"
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
    esp_rom_gpio_pad_select_gpio(mGpioBit0);
    gpio_set_direction(mGpioBit0, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(mGpioBit1);
    gpio_set_direction(mGpioBit1, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(mGpioBit2);
    gpio_set_direction(mGpioBit2, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(mGpioOn);
    gpio_set_direction(mGpioOn, GPIO_MODE_OUTPUT);
    turnOff();
    setSpeedLevel(mSpeedLevel);
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
    // if (newSpeedLevel > 0b111)
    if (newSpeedLevel > 3) //currently only 3 levels available due to hardware issue
    {
        //mSpeedLevel = 0b111;
        mSpeedLevel = 3;
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

//FIXME debug vfd for all 7 levels to work
// current workaround use those that work:
// note: currently assumes output is inverted in constructor
    switch (mSpeedLevel)
    {
    default:
    case 0:
        level = 0b000; //fixme currently 0Hz
        break;
    case 1:            //"secton speed 6" on page11
        level = 0b111; // or: 011, 110
        break;
    case 2:            //"secton speed 4" on page11
        level = 0b101; // or: 001, 100
        break;
    case 3: //"secton speed 2" on page11
        level = 0b010;
        break;
    }

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