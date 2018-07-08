#include "modbus.h"
#include <inttypes.h>
#include <stdio.h>

void uart_setReadData(char* data);

int main()
{
  uint8_t data[12] = {0x01, 0x03, 0x21, 0x02, 0x00, 0x02,0,0} ;
  modbus_writeMessage(data, 6);

  data[0] = 0x01;
  data[1] = 0x03;
  data[2] = 0x04;
  data[3] = 0x01;
  data[4] = 0x00;
  data[5] = 0x01;
  modbus_writeMessage(data, 5);

  data[0] = 0x01;
  data[1] = 0x03;
  data[2] = 0x04;
  data[3] = 0x17;
  data[4] = 0x70;
  data[5] = 0x00;
  data[6] = 0x00;
  modbus_writeMessage(data, 7);

  data[0] = 0x01;
  data[1] = 0x10;
  data[2] = 0x00;
  data[3] = 0x11;
  data[4] = 0x00;
  data[5] = 0x02;
  data[6] = 0x04;
  data[7] = 0x13;
  data[8] = 0x88;
  data[9] = 0x0f;
  data[10] = 0xa0;
  modbus_writeMessage(data, 11);

  data[0] = 0x01;
  data[1] = 0x10;
  data[2] = 0x00;
  data[3] = 0x11;
  data[4] = 0x00;
  data[5] = 0x02;
  data[6] = 0x04;
  data[7] = 0x13;
  data[8] = 0x88;
  data[9] = 0x0f;
  data[10] = 0xa0;
  data[11] = 0x8e;
  modbus_writeMessage(data, 12);

  uart_setReadData(":0110001100020413880FA08E\r\n");
  uint16_t nRead = 12;
  int rc = modbus_readMessage(data, &nRead, 100);
  printf("read %u bytes, rc = %d\n", nRead, rc);
  if(rc==0)
  {
    modbus_writeMessage(data, nRead);
  }

  uint8_t dataReg[4] = {0x13, 0x88, 0x0f, 0xa0};
  uart_setReadData(":011000110002DC\r\n");
  rc = modbus_writeRegister(0x11, dataReg, 4);
  if(rc!= 0)
  { 
    printf("write failed: rc = %d\n", rc);
  }
  else{
    printf("writeOk\n");
  }
  return 0;
}
//:010321020002D6
