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


  while(1)
  {
    uint16_t potiVal = ReadChannel(APOTI);
    uint16_t druckVal = ReadChannel(ADRUCK);

    if(druckVal > potiVal )
    {
      LED0_ON;
      LEDP_ON;

    }
    else
    {
      LED0_OFF;
      LEDP_OFF;

    }
    //while(potiVal--)
    //{
    //  _delay_ms(2);
    //}
    //LEDP_TOGGLE;
    //LED0_TOGGLE;

  }

  return 0;
}
