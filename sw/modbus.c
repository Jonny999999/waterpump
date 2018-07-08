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
    checksum = (checksum + data[i]) & 0xff;
  }
  unsigned int checksumBig = ((~checksum)+1) & 0xff;
  // now append checksum:
  snprintf(buf, 10, "%02" PRIX8, checksumBig);
  mputs(buf);

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
            // too much data
            rc = -1;
            break;
          }
        }
      } // scanf
      else
      {
        // scanf failed
        // no hex number seen
        rc = -2;
      }
    }
  }

  
  if(checksum != 0)
  {
    //printf("Chksum = 0x%02x, bytesRead = %u\n", checksum, bytesRead);
    rc = -3;
  }

  if(rc == 0)
  {
    // success
    if(checksumOverRead == 1)
    {
      *len = bytesRead;
    }
    else
    {
      *len = bytesRead - 1;
    }
  }
  else
  {
    *len = 0;
  }
  return rc;
}

int modbus_writeRegister(uint16_t regAddr, const uint8_t * data, uint8_t len)
{
  int rc = 0;
  if(len > 12)
  {
    return -1;
  }
  uint8_t buffer[7+12];
  buffer[0] = 0x01;  // device addr 1
  buffer[1] = 0x10;  // cmd write
  // register addr:
  buffer[2] = regAddr>>8;
  buffer[3] = regAddr & 0xff;
  // data length in words:
  buffer[4] = 0;
  buffer[5] = len/2;
  // data length in bytes:
  buffer[6] = len;
  memcpy(&buffer[7], data, len);
  
  uint8_t totalLen = 7+len;
  modbus_writeMessage(buffer, totalLen);

  uint8_t rxbuf[6];
  uint16_t rxlen = 6;
  rc = modbus_readMessage(rxbuf, &rxlen, 500);

  if(rc != 0)
  {
    return rc;
  }

  if(rxlen != 6)
  {
    return -2;
  }

  if(memcmp(rxbuf, buffer, rxlen) != 0)
  {
    return  -3;
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
