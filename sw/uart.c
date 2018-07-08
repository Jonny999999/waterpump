#include "uart.h"
#include "bsp_io.h"
#define BAUD 9600UL      // Baudrate
 
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden                
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate               
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.
                                                                                
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))                                     
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch!       
#endif                                                                          
void uart_deHigh()
{
  TXDRVEN_ON;
  LED0_ON;
}
void uart_deLow()
{
  TXDRVEN_OFF;
  LED0_OFF;
}
                                                                                
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

  // wait for transmit buffer to be empty
  while(!(UCSRA & (1<<UDRE))){
  }

  // clear transmit complete flag
  UCSRA |= (1<<TXC);

  // write out data to buffer
  UDR = c;

  // wait for transmit complete flag
  while( !(UCSRA & (1<<TXC) ) )
  {
    // wait
  }
}

int mgetc( char * result, uint16_t timeout)
{
  /* Wait for data to be received */
  uint32_t startTime = time_get();
  while ( !(UCSRA & (1<<RXC)) )
  {
    if(time_delta(time_get(), startTime) > timeout)
    {
      return -1;
    }
  }
  /* Get and return received data from buffer */
  *result =  UDR;

  return 0;
}

void mputs(const char * s){
  while(*s!='\0'){
    mputc(*s);
    s++;
  }
}
