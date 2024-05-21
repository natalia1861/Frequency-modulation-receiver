#ifndef __RDA_H
#define __RDA_H
#include "cmsis_os2.h"

#define RDA_TEMP_COUNT 10

//ADDRESSES
#define RDA_WR_ADD 0x20 >> 1
#define RDA_RD_ADD 0x22 >> 1

//WRITE REGISTRERS
//REG2
#define RDA_DHIZ 							0x8000 				//reg2 bit 15 up
#define RDA_DMUTE 						0x6000				//reg2 bit 14 up
#define RDA_MONO 							0x4000				//reg2 bit 13 up
#define RDA_NEW_MET 					0x0004				//reg2 bit 2 up
#define RDA_PWR_ON 						0x0001				//reg2 bit 0 up
#define RDA_POWER 						0x0001
//REG4
#define RDA_AFC_DIS 					0x0100				//reg4 bit 8 up
//REG5
#define RDA_INT_MOD 					0x8000				//reg 5 bit 15 up
#define RDA_SEEK_MODE 				0x4000				//reg5 bit 14 up
#define RDA_SNR_TRESH 				0x0400				//reg 5 bit 10 up (thresshold 0100)
#define RDA_LNA_DUAL					0x00C0				//reg 5 bit 6, 7 up
#define RDA_LNA_2_5mA					0x0020				//reg 5 bit 5 up
#define RDA_LNA_2_1mA 				0x0010				//reg 5 bit 4 up
#define RDA_LNA_1_8mA 				0x0000				//reg 5 bit 4,5 down
#define RDA_INIT_VOL 					1//14					//reg 5 bit 0,3 (0 a 15)
//REG6
#define RDA_CONF_OPEN_MODE 		0xC000				//reg 6 bit 15, 14 up !! (si no, quitar 1 bit)

//RDA FUNCTIONS
#define RDA_TUNE_ON 					0x0010				//reg3 bit 4 up
#define RDA_SEEK_UP 					0x0300				//reg2 bit 8,9 up
#define RDA_SEEK_DOWN 				0x0100				//reg2 bit 8 up

//frec
#define MINFREC     			87500
#define MAXFREC						108000
#define StartingFreq    	87.0

//READ REGISTRERS
#define SEEK_COMPLETE 0x4000 								//regA bit 14 up
#define SEEK_FAILURE 0x2000									//regA bit 13 up


typedef enum{power_on, power_off, seek_up, seek_down, subida_freq, bajada_freq, set_vol, set_freq, obtener_info}comando_t;

typedef struct{
  comando_t comand;
  uint8_t vol_level;
	float freq;
}	MSGQUEUE_RDA_COMAND_t;

typedef struct {
	uint8_t	 vol;
	float freq_send;
	uint8_t rssi;
	uint16_t wr_reg2;
  uint16_t wr_reg3;
  uint16_t wr_reg4;
  uint16_t wr_reg5;
  uint16_t wr_reg6;
  uint16_t wr_reg7;
} MSGQUEUE_RDA_INFO_t;


//functions
osMessageQueueId_t Init_ThRDA(osMessageQueueId_t rda_queue_comand);



#endif
