#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "vol.h"
#include "stm32f4xx_hal.h"
#define RESOLUTION_12B 4096U
#define VOL_REF 15U
#define VREF 3.3f
/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
 
 
//hilo
static osThreadId_t tid_ThVol;                        // thread id
void ThVol (void *argument);

//colas
static osMessageQueueId_t VOL_MsgQueue;
static MSGQUEUE_VOL vol_send;

int Init_MsgQueue_VOL (void) {
	VOL_MsgQueue = osMessageQueueNew(1, sizeof(MSGQUEUE_VOL), NULL);
	if (VOL_MsgQueue == NULL) {
	}
	return(0);
}
 
osMessageQueueId_t Init_ThVol (void) {
	Init_MsgQueue_VOL();
	tid_ThVol = osThreadNew(ThVol, NULL, NULL);
	if (tid_ThVol == NULL) {
		return(NULL);
	}
	return(VOL_MsgQueue);
}

void ADC1_pins_F429ZI_config(){
		GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_GPIOC_CLK_ENABLE();
	/*PC0     ------> ADC1_IN10*/
		GPIO_InitStruct.Pin = GPIO_PIN_0;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

int ADC_Init_Single_Conversion(ADC_HandleTypeDef *hadc, ADC_TypeDef  *ADC_Instance){
	__HAL_RCC_ADC1_CLK_ENABLE();
	 /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	*/
	hadc->Instance = ADC1;
	hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc->Init.Resolution = ADC_RESOLUTION_12B;
	hadc->Init.ScanConvMode = DISABLE;
	hadc->Init.ContinuousConvMode = DISABLE;
	hadc->Init.DiscontinuousConvMode = DISABLE;
	hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc->Init.NbrOfConversion = 1;
	hadc->Init.DMAContinuousRequests = DISABLE;
	HAL_ADC_Init(hadc);
	
	hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(hadc) != HAL_OK){
		return -1;
	}
	return 0;
}

float ADC_getVoltage(ADC_HandleTypeDef *hadc, uint32_t Channel){
		ADC_ChannelConfTypeDef sConfig;
		HAL_StatusTypeDef status;

		uint32_t raw = 0;
		float voltage = 0;
	
		 /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.	*/
	sConfig.Channel = Channel;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	
	if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
		return -1;
	} else {
		HAL_ADC_Start(hadc);
		do {
			status = HAL_ADC_PollForConversion(hadc, 0); //This funtions uses the HAL_GetTick(), then it only can be executed wehn the OS is running
		}while(status != HAL_OK);
		raw = HAL_ADC_GetValue(hadc);
		voltage = raw*VREF/RESOLUTION_12B; 
		return voltage;
	}
} 

void ThVol (void *argument) {
	float value;
	int volumen;
	ADC_HandleTypeDef adchandle; //handler definition
	ADC1_pins_F429ZI_config(); //specific PINS configuration

	ADC_Init_Single_Conversion(&adchandle , ADC1); //ADC1 configuration
	value=ADC_getVoltage(&adchandle , 10 ); //get values from channel 10->ADC123_IN10
	volumen = 14.2*value/3.3;
  vol_send.volume = volumen;
  osMessageQueuePut(VOL_MsgQueue, &vol_send, 0U, 0U);
	while (1) {
		value=ADC_getVoltage(&adchandle , 10 ); //get values from channel 10->ADC123_IN10
		volumen = 14.2*value/3.3;
		if (volumen != vol_send.volume) {
			vol_send.volume = volumen;
			osMessageQueuePut(VOL_MsgQueue, &vol_send, 0U, 0U);
		}
	}
}
