#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "uart.h"


void modbus_writeMessage(uint8_t * data, uint16_t len)
{
  // set driver enable pin to high
  uart_deHigh();

  // convert to chars and send:
  mputc(':');
  char buf[10];
  uint8_t checksum = 0;
  for(uint16_t i=0; i<len; i++)
  {
    snprintf(buf, 10, "%02X", data[i]);
    mputs(buf);
    // calculate checksum:
    checksum += data[i];
  }
  // now append checksum:
  snprintf(buf, 10, "%02X", ~checksum);
  // now append cr nl:
  mputs("\r\n");
  
  // set driver enable pin to low
  uart_deLow();
}

int modbus_readMessage(uint8_t * data, uint16_t * len, uint16_t timeoutMs)
{
  // convert to chars and send:
  char buf[3] = {'\0','\0','\0'};

  int rc = 0;

  // receive start:
  while((rc == 0) && (*buf != ':'))
  {
    rc = mgetc(buf, timeoutMs);
  }

  // receive bytes:
  uint16_t bytesRead = 0;
  uint8_t stop = 0;
  uint8_t checksumOverRead = 0;
  uint8_t checksum = 0;
  while( (!stop)  &&  (rc == 0))
  {
    rc |= mgetc(&buf[0], timeoutMs);
    rc |= mgetc(&buf[1], timeoutMs);

    if(rc != 0)
    {
      break;
    }

    if(buf[0] == '\r' && buf[1] == '\n')
    {
      stop = 1;
    }
    else
    {
      // have data byte:
      // yes we have room
      unsigned int byte;
      if(1 == sscanf(buf, "%02X", &byte) )
      {
        checksum += byte;
        // do we still have room?
        if(bytesRead < *len)
        {
          // yes we have -> add it to buffer
          *data = byte;
          data++;
          bytesRead++;
        }
        else
        {
          if(checksumOverRead == 0)
          {
            // still could read checksum
            // but next time we fail
            checksumOverRead = 1;
          }
          else
          {
            rc = -1;
            break;
          }
        }
      } // scanf
      else
      {
        // scanf failed
        rc = -1;
      }
    }
  }

  if(rc == 0 && checksum == 0xff)
  {
    // success
    *len = bytesRead;
  }
  else
  {
    *len = 0;
  }
  
  return rc;
}

//void readRegister(uint8_t regAddr, uint8_t * data, uint8_t len)
//{
//  const size_t sizeBuf = 100;
//  char buffer[sizeBuf];
//  buffer[0]='\0';
//
//  // send
//  uint8_t chksum = 0x01+0x03 + regAddr + len;
//  chksum = ~chksum;
//  snprintf(buffer, sizeBuf, ":0103%04X%04X%02X\r\n", regAddr, len/2, chksum);
//  mputs(buffer);
//
//  // receive
//  while(mgetc() != ':')
//  {
//    // wait for start symbol
//    // TODO: add timeout
//  }
//  for(uint8_t i=0; i< 4; i++)
//  {
//    // skip 4 byte
//    mgetc();
//  }
//  // TODO: continue parsing
//
//}

void modbus_init()
{
  uart_init();

}
