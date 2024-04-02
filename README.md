Firmware for controlling a custom built water pump using an esp32 microcontroller

# Features
### Planned
- measure pressure
- control VFD
- control servo motor
- regulate pressure
    - motor RPM (VFD)
    - return valve position (servo)
- control interface
    - set target pressure poti
    - status leds
    - display
- MQTT
    - remote control
    - publish current status


# Install
Currently using ESP-IDF version **v5.2.1**
```bash
git clone -b v5.2.1 --recursive https://github.com/espressif/esp-idf.git /opt/esp-idf-v5.2.1
/opt/esp-idf-v5.2.1/install.sh
```

# Compile
### setup
run once per terminal:
```
. /opt/esp-idf-v5.2.1/export.sh
```
### build
```
idf.py build
```
