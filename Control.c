#include "Control.h"
#include "stdio.h"
#include "Thlcd.h"
#include "Temp.h"
#include "Thjoy.h"
#include "rda5807m.h"
#include "pwm.h"
#include "vol.h"
#include "Time.h"
#include "com.h"

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
 
//hilo
osThreadId_t tid_ThCtrl;                        // thread id
void ThCtrl (void *argument);                   // thread function

//Control States
typedef enum {REPOSO, MANUAL, MEMORIA,PROG_HORA, MEMORIZACION_AUTO} control_state_t;
static control_state_t control_states;
//Prog_hora states
static int hora_state;

//conexiones
static osMessageQueueId_t JOY_MsgQueue;
static osMessageQueueId_t LCD_MsgQueue;
static osMessageQueueId_t TEMP_MsgQueue;
static osMessageQueueId_t RDA_MsgQueue_info;
static osMessageQueueId_t RDA_MsgQueue_comand;
static osMessageQueueId_t VOL_MsgQueue;
static osMessageQueueId_t COM_MsgQueue;

//mensajes
static MSGQUEUE_JOY_t msg_joystick;
static MSGQUEUE_TEMP_t msg_temperatura;
static MSGQUEUE_LCD_t msg_lcd;
static MSGQUEUE_RDA_INFO_t msg_info_rda;
static MSGQUEUE_RDA_COMAND_t msg_comand_rda;
static MSGQUEUE_VOL vol_rec;
static MSGQUEUE_COM com_send;

//variables
static float frec_array[16];
static int mem_position;
static bool bm;

//memorizacion automatica
static uint16_t rssi_menor;
static uint16_t posicion_rssi_menor;
static uint16_t rssi_registro[16];

//extern global
extern int lcd_refresh;
extern int segundos;
extern int segundos_co;//segundos cociente
extern int segundos_res;//segundos resto
extern int minutos;
extern int minutos_co;// minutos cociente
extern int minutos_res;// minutos resto
extern int horas;
extern int horas_co;// horas cociente
extern int horas_res; //horas resto 

//local functions
static void init_frec_mem(void);
static void init_ctrl(void);
static void lcd_clean(void);
static void memorizacion_automatica (void);
static void calcular_rssi_menor (void);
static void init_registros_rssi(void);

//extern id
extern osThreadId_t  tid_ThPwm;

int Init_MsgQueue_control(void) {
  LCD_MsgQueue = osMessageQueueNew(LCD_TEMP_COUNT, sizeof(MSGQUEUE_LCD_t), NULL);
  RDA_MsgQueue_comand = osMessageQueueNew(RDA_TEMP_COUNT, sizeof(MSGQUEUE_RDA_INFO_t), NULL);
  COM_MsgQueue = osMessageQueueNew(1, sizeof(MSGQUEUE_COM), NULL);
  if (LCD_MsgQueue == NULL | RDA_MsgQueue_comand == NULL | COM_MsgQueue == NULL) {return -1;}
  return(0);
}
 
void ThCtrl (void *argument) {
  //status, se activa cuando se recibe algun mensaje/flag de algun hilo
  static osStatus_t status_joystick;
  static osStatus_t status_temp;
  static osStatus_t status_vol;
  static osStatus_t status_rda_info;

  //init
  init_ctrl();
  
  while (1) {
    status_joystick = osMessageQueueGet(JOY_MsgQueue, &msg_joystick, NULL, 0U);
    status_temp = osMessageQueueGet(TEMP_MsgQueue, &msg_temperatura, NULL, 0U);
    status_vol = osMessageQueueGet(VOL_MsgQueue, &vol_rec, NULL, 0U);
    status_rda_info = osMessageQueueGet(RDA_MsgQueue_info, &msg_info_rda, NULL, 0U);
    
    switch(control_states){
      case REPOSO:
        if (status_temp == osOK | lcd_refresh == 1) {
          lcd_refresh = 0;
          sprintf(msg_lcd.mensaje1,"  SBM2022  T:%.1lf$C",msg_temperatura.temp);
          sprintf(msg_lcd.mensaje2,"      %d%d:%d%d:%d%d   ",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res);
          osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
        }
        if (status_joystick == osOK) {
          if (msg_joystick.buzz) {
            osThreadFlagsSet(tid_ThPwm, FLAG_PWM);
          } else if(msg_joystick.corto_larga && msg_joystick.pulsacion == center){
            //before next state
            msg_comand_rda.comand = power_on;			//se enciende la radio para el siguiente estado
            osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            lcd_clean();
            sprintf(msg_lcd.mensaje1,"%d%d:%d%d:%d%d  T:%.1lf$C",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res,msg_temperatura.temp);
            sprintf(msg_lcd.mensaje2,"Freq:%.1lfMHz Vol:%d", (msg_info_rda.freq_send/1000), msg_info_rda.vol); //mandar frec sintonizada y nivel de volumen
            osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
            control_states = MANUAL;
            }
          }
      break;
      case MANUAL:
        if(status_rda_info == osOK){
          sprintf(com_send.txt, "\r%d%d:%d%d:%d%d -> %.4X %.4X %.4X %.4X %.4X %.4X" , horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, msg_info_rda.wr_reg2, msg_info_rda.wr_reg3, msg_info_rda.wr_reg4, msg_info_rda.wr_reg5, msg_info_rda.wr_reg6, msg_info_rda.wr_reg7);
          osMessageQueuePut(COM_MsgQueue, &com_send, 0U, 0U);
        }
        if (status_temp == osOK | status_rda_info == osOK | lcd_refresh == 1) {
          lcd_refresh = 0;
          sprintf(msg_lcd.mensaje1,"%d%d:%d%d:%d%d  T:%.1lf$C",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res,msg_temperatura.temp);
          sprintf(msg_lcd.mensaje2,"Freq:%.1lfMHz Vol:%d", (msg_info_rda.freq_send/1000), msg_info_rda.vol); //mandar frec sintonizada y nivel de volumen
          osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
        }
        if (status_vol == osOK) {
          msg_comand_rda.comand = set_vol;
          msg_comand_rda.vol_level = vol_rec.volume;
          osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
        }
        if (status_joystick == osOK) {
          if (msg_joystick.buzz) {
            osThreadFlagsSet(tid_ThPwm, FLAG_PWM);
          } else if(msg_joystick.corto_larga && msg_joystick.pulsacion == center){
            //before next state
            lcd_clean();
            msg_comand_rda.freq = frec_array[mem_position];
            msg_comand_rda.comand = set_freq;
            osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            sprintf(msg_lcd.mensaje1,"%d%d:%d%d:%d%d T:%.1lf$C",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res,msg_temperatura.temp);
            sprintf(msg_lcd.mensaje2,"M:%d F:%.1lfMHz V:%d", (mem_position+1), (msg_comand_rda.freq/1000), msg_info_rda.vol);
            osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);	
            control_states = MEMORIA;
          } else {
            if (msg_joystick.pulsacion == right) {
              msg_comand_rda.comand = subida_freq;
              osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            } else if (msg_joystick.pulsacion == left) {
              msg_comand_rda.comand = bajada_freq;
              osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            } else if (msg_joystick.pulsacion == up) {
              msg_comand_rda.comand = seek_up;
              osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            } else if (msg_joystick.pulsacion == down) {
              msg_comand_rda.comand = seek_down;
              osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            }
          }
        }
      break;
      case MEMORIA:
        if(status_rda_info == osOK){
          sprintf(com_send.txt, "\r%d%d:%d%d:%d%d -> %.4X %.4X %.4X %.4X %.4X %.4X" , horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, msg_info_rda.wr_reg2, msg_info_rda.wr_reg3, msg_info_rda.wr_reg4, msg_info_rda.wr_reg5, msg_info_rda.wr_reg6, msg_info_rda.wr_reg7);
          osMessageQueuePut(COM_MsgQueue, &com_send, 0U, 0U);
        }
        if (status_temp == osOK | status_rda_info == osOK | lcd_refresh == 1) {
          lcd_refresh = 0;
          sprintf(msg_lcd.mensaje1,"%d%d:%d%d:%d%d T:%.1lf$C",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res,msg_temperatura.temp);
          sprintf(msg_lcd.mensaje2,"M:%d F:%.1lfMHz V:%d", (mem_position+1), (msg_info_rda.freq_send/1000), msg_info_rda.vol);
          osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
        }
        if (status_vol == osOK) {
          msg_comand_rda.comand = set_vol;
          msg_comand_rda.vol_level = vol_rec.volume;
          osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
        }
        if (status_joystick == osOK) {
          if (msg_joystick.buzz) {
            osThreadFlagsSet(tid_ThPwm, FLAG_PWM);
          } else if(msg_joystick.corto_larga && msg_joystick.pulsacion == center){
            //before next state
            timer_stop(); //se para el timer para antes del siguiente modo
            lcd_clean();
            sprintf(msg_lcd.mensaje1,"PROG_HORA");
            sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d BM:%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, bm);
            osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
            control_states = PROG_HORA;
					} else if (msg_joystick.corto_larga && msg_joystick.pulsacion == left) {
						lcd_clean();
						sprintf(msg_lcd.mensaje1,"memorizacion auto");
						sprintf(msg_lcd.mensaje2,"calculando...");
						osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
						control_states = MEMORIZACION_AUTO;
          } else {
            if (msg_joystick.pulsacion == right) {
              mem_position = (15 == mem_position) ? 0 : mem_position+1;
              msg_comand_rda.comand = set_freq;
              msg_comand_rda.freq = frec_array[mem_position];
              osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            } else if (msg_joystick.pulsacion == left) {
              mem_position = (0 == mem_position) ? 15 : mem_position-1;
              msg_comand_rda.freq = frec_array[mem_position];
              msg_comand_rda.comand = set_freq;
              osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            } else if (msg_joystick.pulsacion == up) {
              msg_comand_rda.comand = seek_up;
              osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            } else if (msg_joystick.pulsacion == down) {
              frec_array[mem_position] = msg_info_rda.freq_send;
            }
          }
        }
      break;
      case PROG_HORA:
        if (status_joystick == osOK) {
          if (msg_joystick.buzz) {
            osThreadFlagsSet(tid_ThPwm, FLAG_PWM);
          } else	if(msg_joystick.corto_larga && msg_joystick.pulsacion == center){
            //before next state
            if (bm == 1) {
              init_frec_mem();
            }
            lcd_clean();
            sprintf(msg_lcd.mensaje1,"SBM2022 T:%.1lf$C",msg_temperatura.temp);
            sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res);
            osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
            timer_restart();
            msg_comand_rda.comand = power_off;
            osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
            control_states = REPOSO;
          } else {
            if (msg_joystick.pulsacion == right) {
              hora_state = (3==hora_state) ? 0 : hora_state+1;
            } else if (msg_joystick.pulsacion == left) {
              hora_state = (0==hora_state) ? 3 : hora_state-1;
            } else if (msg_joystick.pulsacion == up) {
              switch(hora_state) {
                case 0:
                  horas = (22 < horas) ? 0 : horas+1;
                  horas_co = horas/10;
                  horas_res = horas%10;
                  sprintf(msg_lcd.mensaje1,"PROG_HORA");
                  sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d BM:%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, bm);
                  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
                break;
                case 1:
                  minutos = (58 < minutos) ? 0 : minutos+1;
                  minutos_co = minutos/10;
                  minutos_res = minutos%10;
                  sprintf(msg_lcd.mensaje1,"PROG_HORA");
                  sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d BM:%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, bm);
                  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
                break;
                case 2:
                  segundos = (58 < segundos) ? 0 : segundos+1;
                  segundos_co = segundos/10;
                  segundos_res = segundos%10;
                  sprintf(msg_lcd.mensaje1,"PROG_HORA");
                  sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d BM:%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, bm);
                  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
                break;
                case 3:
                  bm = !bm;
                  sprintf(msg_lcd.mensaje1,"PROG_HORA");
                  sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d BM:%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, bm);
                  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
                break;
              }
            } else if (msg_joystick.pulsacion == down) {
              switch(hora_state) {
                case 0:
                  horas = (1 > horas) ? 23 : horas-1;
                  horas_co = horas/10;
                  horas_res = horas%10;
                  sprintf(msg_lcd.mensaje1,"PROG_HORA");
                  sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d BM:%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, bm);
                  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
                break;
                case 1:
                  minutos = (1 > minutos) ? 59 : minutos-1;
                  minutos_co = minutos/10;
                  minutos_res = minutos%10;
                  sprintf(msg_lcd.mensaje1,"PROG_HORA");
                  sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d BM:%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, bm);
                  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
                break;
                case 2:
                  segundos = (1 > segundos) ? 59 : segundos-1;
                  segundos_co = segundos/10;
                  segundos_res = segundos%10;
                  sprintf(msg_lcd.mensaje1,"PROG_HORA");
                  sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d BM:%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, bm);
                  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
                break;
                case 3:
                  bm = !bm;
                  sprintf(msg_lcd.mensaje1,"PROG_HORA");
                  sprintf(msg_lcd.mensaje2,"%d%d:%d%d:%d%d BM:%d",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res, bm);
                  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
                break;
              }
            }
          }
        }
      break;
			case MEMORIZACION_AUTO:
				memorizacion_automatica();
				mem_position = 0;
				msg_comand_rda.comand = set_freq;
        msg_comand_rda.freq = frec_array[mem_position];
        osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, 0U);
				lcd_clean();
				sprintf(msg_lcd.mensaje1,"%d%d:%d%d:%d%d T:%.1lf$C",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res,msg_temperatura.temp);
        sprintf(msg_lcd.mensaje2,"M:%d F:%.1lfMHz V:%d", (mem_position+1), (frec_array[mem_position]/1000), msg_info_rda.vol);
        osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);	
				control_states = MEMORIA;	
			break;
    }
  }
}

int Init_ThCtrl(void){
  Init_MsgQueue_control();
  Init_Thtime();
  JOY_MsgQueue = Init_Thjoy();
  TEMP_MsgQueue = Init_ThTemp();
  RDA_MsgQueue_info = Init_ThRDA(RDA_MsgQueue_comand);
  Init_Thlcd(LCD_MsgQueue);
  VOL_MsgQueue = Init_ThVol();
  Init_ThPwm();
  Init_ThCom(COM_MsgQueue);

  tid_ThCtrl = osThreadNew(ThCtrl,NULL,NULL);
    if(tid_ThCtrl==NULL){
      return -1;
    }
    return 0;
}

static void init_frec_mem(void) {
  for(int i=0; i<16;i++) {
    frec_array[i] = 87500;
  }
}

static void init_ctrl(void) {
  mem_position = 0;
  hora_state = 0;
  bm = 0;
  init_frec_mem();
  control_states = REPOSO;
  
  //init rda off
  msg_comand_rda.comand = power_off;
  osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, osWaitForever);
  
  //init state
  sprintf(msg_lcd.mensaje1,"  SBM2022  T:%.1lf$C",msg_temperatura.temp);
  sprintf(msg_lcd.mensaje2,"      %d%d:%d%d:%d%d   ",horas_co,horas_res,minutos_co,minutos_res,segundos_co,segundos_res);
  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
}

static void lcd_clean() {
  msg_lcd.clean = true;
  osMessageQueuePut(LCD_MsgQueue, &msg_lcd, 0U, 0U);
  msg_lcd.clean = false;
}

static void memorizacion_automatica () {
	float freq_bf = MINFREC;
	rssi_menor = 0xFFFF;
	msg_comand_rda.comand = set_freq;
	msg_comand_rda.freq = MINFREC;
	osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, osWaitForever);
	osMessageQueueGet(RDA_MsgQueue_info, &msg_info_rda, 0U, osWaitForever);
	msg_comand_rda.comand = seek_up;
	init_registros_rssi();
 do {
		freq_bf = msg_info_rda.freq_send;
		osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, osWaitForever);
		osMessageQueueGet(RDA_MsgQueue_info, &msg_info_rda, 0U, osWaitForever);
		if (msg_info_rda.rssi > rssi_menor) {
			frec_array[posicion_rssi_menor] = msg_info_rda.freq_send;
			rssi_registro[posicion_rssi_menor] = msg_info_rda.rssi;
			rssi_menor = msg_info_rda.rssi;
			calcular_rssi_menor();
		}
	} while (msg_info_rda.freq_send >= freq_bf);
} 

static void calcular_rssi_menor () {
	for (int p=0; p<16; p++) {
		if (rssi_registro[p] < rssi_menor) {
			rssi_menor = rssi_registro[p];
			posicion_rssi_menor = p;
		}
	}
}

static void init_registros_rssi() {
	for (int i=0; i< 16; i++) {
		osMessageQueuePut(RDA_MsgQueue_comand, &msg_comand_rda, 0U, osWaitForever);
		osMessageQueueGet(RDA_MsgQueue_info, &msg_info_rda, 0U, osWaitForever);
		frec_array[i] = msg_info_rda.freq_send;
		rssi_registro[i] = msg_info_rda.rssi;
		if (msg_info_rda.rssi < rssi_menor) {
			rssi_menor = msg_info_rda.rssi;
			posicion_rssi_menor = i;
		}
	}
}
