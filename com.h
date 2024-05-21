#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "stm32f4xx_hal.h"
#ifndef __COM_H
#define __COM_H

int Init_ThCom (osMessageQueueId_t com_queue);
void myUSART_callback(uint32_t event);

typedef struct{
  char test;
} MSGQUEUE_COM_t;

typedef struct{
  char txt[100];
}MSGQUEUE_COM;

#endif
