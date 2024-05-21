#include "Temp.h"

#define TEMP_I2C_ADDR       0x48
#define REG_TEMP            0x00

/*----------------------------------------------------------------------------
 *      Thlcd 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
 
 //
osMessageQueueId_t temp_queue;
static MSGQUEUE_TEMP_t temperature;
void Thread_Temperatura (void *argument);

int Init_MsgQueue_TEMP (void) {
  temp_queue = osMessageQueueNew(MSG_TEMP_COUNT, sizeof(MSGQUEUE_TEMP_t), NULL);
  if (temp_queue == NULL) {
  }
  return(0);
}

//I2C DRIVER

extern ARM_DRIVER_I2C            Driver_I2C1;
static ARM_DRIVER_I2C *I2Cdrv = &Driver_I2C1;
 
static volatile uint32_t I2C_Event;
float calc;

static osThreadId_t tid_Temperatura;                        // thread id
static osTimerId_t tim_id_temp;

//TIMER

static void Timer_Temp_Callback(void const *arg){
  osThreadFlagsSet(tid_Temperatura, TEMP_FLAG);
}
 
static int Init_Tim_Temp (void) {
  osStatus_t status;
  tim_id_temp = osTimerNew((osTimerFunc_t)&Timer_Temp_Callback, osTimerPeriodic, NULL ,NULL);
  if (tim_id_temp != NULL) {
    if(status != osOK)
      return NULL;
  }
  return(0);
}

//Thread

osMessageQueueId_t Init_ThTemp (void) {
	Init_MsgQueue_TEMP();
  tid_Temperatura = osThreadNew (Thread_Temperatura,NULL, NULL);
  if (tid_Temperatura == NULL) {
      return(NULL);
    }
    return temp_queue;
}

static void Thread_Temperatura (void *argument){
  float temp_aux;
  uint8_t cmd;
  uint8_t buffer_temp[5];
  
  I2Cdrv->Initialize (NULL);
  I2Cdrv->PowerControl (ARM_POWER_FULL);
  I2Cdrv->Control      (ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
  I2Cdrv->Control      (ARM_I2C_BUS_CLEAR, 0);
  Init_Tim_Temp();
  cmd=REG_TEMP;
  
  osTimerStart(tim_id_temp,1000U);//timer init
  //first measure
    I2Cdrv->MasterTransmit (TEMP_I2C_ADDR, &cmd, 2, true);
    while (I2Cdrv->GetStatus().busy){//Bus busy, espera hasta ser liberado
    }
    I2Cdrv->MasterReceive(TEMP_I2C_ADDR,buffer_temp ,2,false);//Lee temp
    while (I2Cdrv->GetStatus().busy){//espera liberacion del bus
    }
    temp_aux=((buffer_temp[0]<<8)|(buffer_temp[1]))>>5;
    calc=temp_aux*(0.125);
    temperature.temp = calc;
    osMessageQueuePut(temp_queue, &temperature, 0U, osWaitForever);
  
  while (1) {
    
    osThreadFlagsWait(TEMP_FLAG,osFlagsWaitAny,osWaitForever);//wait flag 1s
      
    I2Cdrv->MasterTransmit (TEMP_I2C_ADDR, &cmd, 2, true);
    while (I2Cdrv->GetStatus().busy){//Bus busy, espera hasta ser liberado
    }
    I2Cdrv->MasterReceive(TEMP_I2C_ADDR,buffer_temp ,2,false);//Lee temp
    while (I2Cdrv->GetStatus().busy){//espera liberacion del bus
    }
    temp_aux=((buffer_temp[0]<<8)|(buffer_temp[1]))>>5;
    calc=temp_aux*(0.125);
    temperature.temp = calc;
    osMessageQueuePut(temp_queue, &temperature, 0U, osWaitForever);
    
  }
}
