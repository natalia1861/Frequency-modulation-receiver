#include "cmsis_os2.h"                          // CMSIS RTOS header file

#ifndef __THTIME_H
#define __THTIME_H

#define ONE_SEC 1000U

void Init_Thtime (void);
void timer_stop(void);
void timer_restart(void);

#endif
