#pragma once
#include <avr/io.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "time.h"

void uart_init();                                                             
void mputc(char c);
int mgetc( char * result, uint16_t timeout);
void mputs(const char * s);

void uart_deHigh();
void uart_deLow();
