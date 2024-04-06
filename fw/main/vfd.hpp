#pragma once
// driver for VFD (variable frequency drive)
extern "C" {
#include "driver/gpio.h"
#include "esp_log.h"
}


// class for controlling a VFD connected via 4 digital pins (3x speed, 1x start)
class Vfd4DigitalPins
{
public:
    Vfd4DigitalPins(
        gpio_num_t gpioSpeedBit0, // vfd speed input 3
        gpio_num_t gpioSpeedBit1, // vfd speed input 2
        gpio_num_t gpioSpeedBit2, // vfd speed input 1
        gpio_num_t gpioTurnOn,
        bool invertSpeedBits = false);
    void turnOn();                   // turn on at last speed level
    void turnOn(uint8_t speedLevel); // turn on at certain speed level
    void turnOff();
    void setSpeedLevel(uint8_t speedLevel); // change speed level 0-7

private:
    // methods
    void initGpios();

    // config
    gpio_num_t mGpioBit0;
    gpio_num_t mGpioBit1;
    gpio_num_t mGpioBit2;
    gpio_num_t mGpioOn;
    bool mInvertSpeedBits = false;

    // variables
    uint8_t mSpeedLevel = 0;
};