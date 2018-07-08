#include "time.h"
void time_init(){
  time_counter = 0;
  TCNT2 = 0;
  // set compare value to 1ms
  OCR2 = F_CPU/1000/64;
  TIMSK |= (1<<OCIE2);
  //prescaler 64, reset timer at ocr0
  TCCR2 = (1<<CS22) | (1<<WGM21);
}


// more recent time first
uint32_t time_delta(uint32_t b, uint32_t a){
  if(a>b){
    return 0xffffffff - a + b;
  }else{
    return b-a;
  }
}

ISR(TIMER2_COMP_vect){
  time_counter++;
}

uint32_t time_get(void){
  cli();
  uint32_t result = time_counter;
  sei();
  return result;
}
