#include <avr/io.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define BAUD 9600UL      // Baudrate
 
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden                
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate               
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.
                                                                                
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))                                     
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch!       
#endif                                                                          
                                                                                
void uart_init(){                                                               
  UBRRH = UBRR_VAL >> 8;                                                        
  UBRRL = UBRR_VAL & 0xff;                                                      
  UCSRB |= (1<<TXEN) | (1<<RXEN);

  /* Set frame format: 7data, 2stop bit */
  UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ1);

  /* Set frame format: 7data, 2stop bit */
  //UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
}

void mputc(char c){
  while(!(UCSRA & (1<<UDRE))){
  }
  UDR = c;
}

unsigned char mgetc( void )
{
  /* Wait for data to be received */
  while ( !(UCSRA & (1<<RXC)) )
    ;
  /* Get and return received data from buffer */
  return UDR;
}

void mputs(char * s){
  while(*s!='\0'){
    mputc(*s);
    s++;
  }
}


void readRegister(uint8_t regAddr, uint8_t * data, uint8_t len)
{
  const size_t sizeBuf = 100;
  char buffer[sizeBuf];
  buffer[0]='\0';

  // send
  uint8_t chksum = 0x01+0x03 + regAddr + len;
  chksum = ~chksum;
  snprintf(buffer, sizeBuf, ":0103%04X%04X%02X\r\n", regAddr, len/2, chksum);
  mputs(buffer);

  // receive
  while(mgetc() != ':')
  {
    // wait for start symbol
    // TODO: add timeout
  }
  for(uint8_t i=0; i< 4; i++)
  {
    // skip 4 byte
    mgetc();
  }
  // TODO: continue parsing

}

void modbus_init()
{
  uart_init();

}
