#pragma once
#include <inttypes.h>

void uart_init();                                                             
void mputc(char c);
int mgetc( char * result, uint16_t timeout);
void mputs(const char * s);

void uart_deHigh();
void uart_deLow();
