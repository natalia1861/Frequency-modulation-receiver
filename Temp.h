#ifndef __THTEMP_H
#define __THTEMP_H

#include "cmsis_os2.h"
#include "Driver_I2C.h"
#include "RTE_Device.h"
#include "stm32f4xx_hal.h"

#define TEMP_FLAG 0x01
#define MSG_TEMP_COUNT 10

typedef struct{
  float temp;
} MSGQUEUE_TEMP_t;

osMessageQueueId_t Init_ThTemp(void);

#endif
