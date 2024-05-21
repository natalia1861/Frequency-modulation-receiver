#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "stm32f4xx_hal.h"
#ifndef __ADC_H

typedef struct {
  uint8_t volume;		//REVISAR CUANTO DEBE OCUPAR
} MSGQUEUE_VOL;

void ADC1_pins_F429ZI_config(void);
int ADC_Init_Single_Conversion(ADC_HandleTypeDef *, ADC_TypeDef  *);
float ADC_getVoltage(ADC_HandleTypeDef * , uint32_t );
	
osMessageQueueId_t Init_ThVol (void);
#endif
