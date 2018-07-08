#include "uart.h"
#include "stdio.h"
#include <string.h>

char readBuf[100];
uint8_t readBufPos = 0;

void uart_setReadData(const char* data)
{

  strcpy(readBuf, data);
  readBufPos = 0;
}
void uart_init()
{
}

void mputc(char c)
{
  printf("%c", c);
}
int mgetc( char * result, uint16_t timeout)
{
  
  *result = readBuf[readBufPos];
  readBufPos++;
  return 0;

}
void mputs(const char * s)
{
  printf("%s", s);

}

void uart_deHigh()
{
  printf("TX EN\n");

}

void uart_deLow()
{
  printf("TX DIS\n");

}
