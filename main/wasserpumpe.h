#pragma once
#include "driver/adc.h"

extern struct wasserpumpe_t wp;

typedef enum {IDLE, LOW, RUN, HIGH} wp_state;
typedef enum {PRESSURE, LEVEL, POTI} wp_mode;

typedef struct wasserpumpe_t
{
    const char name[32];
    wp_state state;
    wp_mode mode;

    uint32_t fu_level;
    float druck;
    float druck_max;
    float druck_min;
    float druck_soll;

    bool st1;
    bool st2;
    bool st3;
    bool st4;
} wasserpumpe_t;

void wp_ReadPressure();


void wp_FuSetLevel(int level); //fu speed level 0-7
