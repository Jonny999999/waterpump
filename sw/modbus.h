#pragma once
#include <inttypes.h>
void modbus_init();
void modbus_writeMessage(uint8_t * data, uint16_t len);
int modbus_readMessage(uint8_t * data, uint16_t * len, uint16_t timeoutMs);
