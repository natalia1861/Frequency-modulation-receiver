#ifndef __THLCD_H
#define __THLCD_H

#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "Driver_SPI.h"
#include "stm32f4xx_hal.h"
#include "string.h"
#include "stdio.h"

typedef struct{
  char mensaje1[20];
  char mensaje2[20];
  bool clean;
} MSGQUEUE_LCD_t;

int Init_Thlcd(osMessageQueueId_t LCD_Queue);

#endif
