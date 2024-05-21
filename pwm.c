#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "pwm.h"
#include "stm32f4xx_hal.h"

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
 
 //thread
osThreadId_t tid_ThPwm;                        // thread id (no static, ya que es global)
static void ThPwm (void *argument);                   // thread function

//timer
static TIM_HandleTypeDef tim2;
static void Init_Timer(void);

int Init_ThPwm (void) {
  tid_ThPwm = osThreadNew(ThPwm, NULL, NULL);
  if (tid_ThPwm == NULL) {
    return(-1);
  }
  return(0);
}
 
void ThPwm (void *argument) {
  Init_Timer();
  while (1) {
    osThreadFlagsWait(FLAG_PWM, osFlagsWaitAny, osWaitForever);
    HAL_TIM_PWM_Start(&tim2, TIM_CHANNEL_4);
    osDelay(100);
    HAL_TIM_PWM_Stop(&tim2, TIM_CHANNEL_4);
  }
}

static void Init_Timer(void){
  static GPIO_InitTypeDef GPIO_InitStruct;
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitStruct.Pin= GPIO_PIN_3;
  GPIO_InitStruct.Mode= GPIO_MODE_AF_PP;
  GPIO_InitStruct.Alternate= GPIO_AF1_TIM2;
  GPIO_InitStruct.Pull= GPIO_NOPULL;
  GPIO_InitStruct.Speed= GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  __HAL_RCC_TIM2_CLK_ENABLE();
  tim2.Instance = TIM2;
  tim2.Init.Prescaler = 99;
  tim2.Init.Period = 299;
  tim2.Init.CounterMode= TIM_COUNTERMODE_UP;
  tim2.Init.ClockDivision= TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_PWM_Init(&tim2);
  
  static TIM_OC_InitTypeDef TIM_Channel_InitStruct;
  TIM_Channel_InitStruct.OCMode= TIM_OCMODE_PWM1;
  TIM_Channel_InitStruct.OCPolarity= TIM_OCPOLARITY_HIGH;
  TIM_Channel_InitStruct.OCFastMode= TIM_OCFAST_DISABLE;
  TIM_Channel_InitStruct.Pulse= 24;
  HAL_TIM_PWM_ConfigChannel(&tim2, &TIM_Channel_InitStruct, TIM_CHANNEL_4);
}

