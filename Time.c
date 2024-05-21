#include "Time.h"
 
/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
 
 //thread                        // thread id
static void Thtime (void *argument);                   // thread function

//timer
static osTimerId_t tim_id1;
static void timer_init(void);
static void timer_start(void);


//variables globales
int lcd_refresh;
int segundos;
int segundos_co;//segundos cociente
int segundos_res;//segundos resto
int minutos;
int minutos_co;// minutos cociente
int minutos_res;// minutos resto
int horas;
int horas_co;// horas cociente
int horas_res; //horas resto
 

void Init_Thtime (void) {
  osThreadNew(Thtime, NULL, NULL);
}
 
void Thtime (void *argument) {
  timer_init();
  timer_start();
  while (1) {
    //timer periodico funcionando
    osDelay(10000);
  }
}

static void Timer1_Callback (void const *arg) {
  lcd_refresh = 1;
  if(segundos ==59){
    segundos = 0;
    segundos_co = 0;
    segundos_res = 0;
    if(minutos ==59){
      minutos = 0;
      minutos_co = 0;
      minutos_res = 0;
      if(horas == 23){
        horas = 0;
        horas_co = 0;
        horas_res = 0;
      }else{
        horas++;
        horas_co=horas/10;
        horas_res = horas%10;
      }
    }else{
      minutos++;
      minutos_co = minutos/10;
      minutos_res = minutos%10;
    }
  }else{
    segundos++;
    segundos_res = segundos%10;
    segundos_co = segundos/10;
  }
}

static void timer_init(void){
  tim_id1 = osTimerNew((osTimerFunc_t)&Timer1_Callback, osTimerPeriodic, (void*)0, NULL);
}

static void timer_start(void) {
  osTimerStart(tim_id1, ONE_SEC);
}

void timer_stop(void) {
	osTimerStop(tim_id1);
}

void timer_restart(void) {
	osTimerStart(tim_id1, ONE_SEC);
	osTimerStart(tim_id1, ONE_SEC);
}
