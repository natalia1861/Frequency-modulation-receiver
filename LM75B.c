#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "LM75B.h"
/*----------------------------------------------------------------------------
 *      ThTemp 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
 
osThreadId_t tid_ThTemp;                        // thread id
 
void ThTemp (void *argument);                   // thread function
 
int Init_ThTemp (void) {
 
  tid_ThTemp = osThreadNew(ThTemp, NULL, NULL);
  if (tid_ThTemp == NULL) {
    return(-1);
  }
 
  return(0);
}
 
void ThTemp (void *argument) {
 
  while (1) {
  }
}
