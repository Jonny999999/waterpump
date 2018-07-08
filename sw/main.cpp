extern "C" {
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "bsp_io.h"
#include "a2d.h"
#include "modbus.h"
#include "time.h"

}

//#include "button.hpp"
//#include "InterruptTimer.hpp"


#undef FILEID
#define FILEID 0


int main(void)
{
  time_init();
  bsp_io_init();
  modbus_init();

  // LED test:
  
  LED0_ON;
  _delay_ms(500);
  LED1_ON;
  _delay_ms(500);
  LED2_ON;
  _delay_ms(500);
  LED0_OFF;
  _delay_ms(500);
  LED1_OFF;
  _delay_ms(500);
  LED2_OFF;
  _delay_ms(500);

  uint8_t count = 0;
  while(1)
  {
    count++;
    uint16_t potiVal = ReadChannel(APOTI);
    uint16_t druckVal = ReadChannel(ADRUCK);

    if(druckVal > potiVal )
    {
      //LED2_ON;

    }
    else
    {
      //LED2_OFF;

    }
    //while(potiVal--)
    //{
    //  _delay_ms(2);
    //}
    //LEDP_TOGGLE;
    //LED0_TOGGLE;

    _delay_ms(1000);


    uint8_t mode[4] = {0,0,0x01,0};
    int rc = modbus_readRegister(0x2000, mode, 2);

    uint8_t cmd = 0;
    if(rc ==0)
    {
        cmd = 0x02;
        LED2_ON;
        LED1_OFF;
      _delay_ms(100);
      uint16_t freq = potiVal *10;
      mode[2] = freq>>8;
      mode[3] = freq & 0xff;
      mode[1] = (mode[1] & (~0x03)) | cmd;
      //rc = modbus_writeRegister(0x2000, mode, 4);
      rc = modbus_writeRegister(0x2000, mode, 4);
    }

    //uint8_t readData[4] = {4,3,2,1};
    //int rc = modbus_readRegister(0x2102, readData, 4);
    //uint8_t expected[4] = {0x17, 0x70, 0x00, 0x00};
    //if(memcmp(readData, expected, 4) != 0)
    //{
    //  //printf("read failed: data missmatch\n");
    //  LED2_OFF;
    //  rc = -8;
    //}
    //else
    //{
    //  LED2_ON;
    //}
    //if(rc != 0)
    //{
    //  //printf("read failed: rc = %d\n", rc);
    //  LED1_OFF;
    //}
    //else
    //{
    //  LED1_ON;
    //  //printf("readReg OK\n");
    //}


  }

  return 0;
}
