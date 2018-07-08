#pragma once
#include <inttypes.h>
void modbus_init();
void modbus_writeMessage(uint8_t * data, uint16_t len);
int modbus_readMessage(uint8_t * data, uint16_t * len, uint16_t timeoutMs);

int modbus_writeRegister(uint16_t regAddr, const uint8_t * data, uint8_t len);
