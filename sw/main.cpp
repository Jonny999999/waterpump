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

template <uint16_t n>
class MovingAverage
{
  public:
  MovingAverage(uint16_t initVal = 0)
  {
    for(uint16_t i =0; i<n; i++)
    {
      store[i] = initVal;
    }
  }

  void add(uint16_t value)
  {
    store[stop] = value;
    stop++;
    if(stop >= n)
    {
      stop = 0;
    }
  }

  uint16_t getAverage()
  {
    uint32_t sum = 0;
    for(uint16_t i =0; i<n; i++)
    {
      sum += store[i];
    }
    uint32_t result = sum / n;
    return result;
  }

  private:
    uint16_t store[n];
    uint16_t stop = 0;
};

int vfd_setMode(bool running)
{
  uint8_t buf[2];
  buf[0] = 0;
  if(running)
  {
    buf[1] = 0x02;
  }
  else
  {
    buf[1] = 0x01;
  }
  int rc = modbus_writeRegister(0x2000, buf, 2);
  return rc;

}

int vfd_setFrequency(uint16_t centiHz)
{
  uint16_t freq = centiHz;
  uint8_t buf[2];
  if(freq > 6000)
  {
    freq = 6000;
  }
  if(freq < 400)
  {
    //cmd = 0x02;
    freq = 400;
  }
  buf[0] = freq>>8;
  buf[1] = freq & 0xff;
  //buf[1] = (buf[1] & (~0x03)) | cmd;
  int rc = modbus_writeRegister(0x2001, buf, 2);
  return rc;
}

const uint16_t targetBarDivider = 2;

const int16_t maxFreq = 5500;

const int32_t P = 5;
const int32_t I = 5;

int32_t i = 0;
void regulateFrequency(MovingAverage<50> & druckAdc, uint16_t targetVal)
{
  targetVal = targetVal/targetBarDivider;

  //int32_t e = targetVal - druckAdc.getAverage();
  int32_t target = ReadChannel(APOTI);
  target = target / 2;
  int32_t current = ReadChannel(ADRUCK);
  //int32_t e = targetVal - ReadChannel(ADRUCK);
  
  int32_t e = target-current;

  if(e>0)
  {
    LED2_ON;
  }
  else
  {
    LED2_OFF;
  }
  i += e;
  if(i>maxFreq*I)
  {
    i = maxFreq*I;
  }
  if(i < -maxFreq*I)
  {
    i = -maxFreq*I;
  }

  int32_t frequency = P * e + i*I;

  uint16_t realFreq = 0;
  if(frequency < 0)
  {
    realFreq = 0;
  }else
  {
    if(frequency > maxFreq)
    {
      realFreq = maxFreq;
    }
    else
    {
      realFreq = frequency;
    }
  }

  vfd_setFrequency(realFreq);
}

bool isOff = true;
void setFrequencyByPoti(uint16_t potiVal)
{
  uint16_t freq = potiVal * 6;

  if(freq > 6000)
  {
    freq = 6000;
  }
  if(freq < 400)
  {
    freq = 400;
    vfd_setMode(false);
    isOff = true;
  }
  else
  {
    if(isOff)
    {
      vfd_setMode(true);
      isOff = false;
    }
  }
  vfd_setFrequency(freq);
}

int main(void)
{
  time_init();
  bsp_io_init();
  modbus_init();

  // LED test:

  LED0_ON;
  _delay_ms(125);
  LED1_ON;
  _delay_ms(125);
  LED2_ON;
  _delay_ms(125);
  LED3_ON;
  //_delay_ms(125);
  uint32_t timeTest = time_get();
  while(time_delta(time_get(), timeTest) < 2000)
  {

  }
  LED0_OFF;
  _delay_ms(125);
  LED1_OFF;
  _delay_ms(125);
  LED2_OFF;
  _delay_ms(125);
  LED3_OFF;
  _delay_ms(125);

  uint8_t count = 0;

  uint32_t timeStartShort = time_get();
  uint32_t timeStartLong = time_get();
  MovingAverage<50> druckAdcAvg(1023);
  while(1)
  {
    count++;

    if(time_delta(time_get(), timeStartShort) > 10)
    {
      timeStartShort = time_get();
      // 10ms timer
      
      druckAdcAvg.add(ReadChannel(ADRUCK));
    }

    if(time_delta(time_get(), timeStartLong) > 500)
    {
      timeStartLong = time_get();
      // 500ms timer
      
      if(T1_IS_ON)
      {
        LED1_ON;
        regulateFrequency(druckAdcAvg, ReadChannel(APOTI));
      }
      else
      {
        LED1_OFF;
        setFrequencyByPoti(ReadChannel(APOTI));

      }
      
      LED3_TOGGLE;
    }
  }

  return 0;
}
