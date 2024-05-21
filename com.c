#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "com.h"
#include "Driver_USART.h"
#include <stdio.h>

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
//Thread id
static osThreadId_t tid_ThCom;                        // thread id

//Variables
extern ARM_DRIVER_USART Driver_USART3;
void ThCom (void *argument);                   // thread function

//colas
static osMessageQueueId_t COM_MsgQueue;
static MSGQUEUE_COM com_rec;

int Init_ThCom (osMessageQueueId_t com_queue) {
	COM_MsgQueue = com_queue;
  tid_ThCom = osThreadNew(ThCom, NULL, NULL);
  if (tid_ThCom == NULL) {
    return(-1);
  }
  return(0);
}
 
void ThCom (void *argument) {
  //Iniciamos el UART como inciabamos el I2C
    static ARM_DRIVER_USART * USARTdrv = &Driver_USART3;
  
      /*Initialize the USART driver */
    USARTdrv->Initialize(myUSART_callback);
    /*Power up the USART peripheral */
    USARTdrv->PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 4800 Bits/sec */
    USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                      ARM_USART_DATA_BITS_8 |
                      ARM_USART_PARITY_NONE |
                      ARM_USART_STOP_BITS_1 |
                      ARM_USART_FLOW_CONTROL_NONE, 19200); // bits up to 19200?
     
    /* Enable Receiver and Transmitter lines */
    USARTdrv->Control (ARM_USART_CONTROL_TX, 1);
    USARTdrv->Control (ARM_USART_CONTROL_RX, 1);
  
  while (1) {
		osMessageQueueGet(COM_MsgQueue, &com_rec, NULL, osWaitForever);
    USARTdrv->Send(com_rec.txt,sizeof(com_rec.txt));
  }
}


void myUSART_callback(uint32_t event){
  if( event & ARM_USART_EVENT_TX_COMPLETE )
       osThreadFlagsSet(tid_ThCom ,ARM_USART_EVENT_TX_COMPLETE);
    if( event & ARM_USART_EVENT_RECEIVE_COMPLETE)
      osThreadFlagsSet(tid_ThCom ,ARM_USART_EVENT_RECEIVE_COMPLETE);
}
