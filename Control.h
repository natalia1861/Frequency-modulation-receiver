#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "Thjoy.h"
#include "Thlcd.h"
#include "stdbool.h"

#ifndef __THCTRL_H
#define __THCTRL_H

#define FLAG_PWM 0x0010
#define LCD_TEMP_COUNT 10

int Init_ThCtrl (void); 

#endif
