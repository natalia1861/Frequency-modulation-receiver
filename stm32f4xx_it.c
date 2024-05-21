#include "main.h"
#include "stm32f4xx_it.h"
#include "cmsis_os2.h"


#ifdef _RTE_
#include "RTE_Components.h"             /* Component selection */
#endif


/* IRQs Perso */

void NMI_Handler(void){}

void HardFault_Handler(void){
  while(1){}
}

void MemManage_Handler(void){
  while(1){}
}


void BusFault_Handler(void){
  while(1){}
}

void UsageFault_Handler(void){  
  while(1){}
}

#ifndef RTE_CMSIS_RTOS2_RTX5
void SVC_Handler(void){}
#endif

void DebugMon_Handler(void){}

#ifndef RTE_CMSIS_RTOS2_RTX5
void PendSV_Handler(void){}
#endif

#ifndef RTE_CMSIS_RTOS2_RTX5
void SysTick_Handler(void){
  HAL_IncTick();
}
#endif
