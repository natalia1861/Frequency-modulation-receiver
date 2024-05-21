#include "rda5807m.h"
#include "Driver_I2C.h" 
#include "stm32f4xx_hal.h"


//threads
static osThreadId_t tid_ThRDA;
static void ThRDA(void *argument);

//local functions
static int WriteAll(void);
static int ReadRegistrers(void);
static void Set_Vol(uint8_t volume);
static void Power_On(void);
static void Power_Off(void);
static void Seek_Up(void);
static void Seek_Down(void);
static void Set_Freq(float frecuencia);
static void Get_Info(void);
static void subir_100khz(void);
static void bajar_100khz(void);

//driver i2c
extern ARM_DRIVER_I2C Driver_I2C2;
static ARM_DRIVER_I2C *I2Cdrv = &Driver_I2C2;

static void callback_i2c(uint32_t event){
  osThreadFlagsSet(tid_ThRDA, event);
}

//Write registrers
static uint16_t wr_registrers[6];

//Read registrers
static uint16_t rd_regA;
static uint16_t rd_regB;
//static uint16_t rd_regC;
//static uint16_t rd_regD;
//static uint16_t rd_regE;
//static uint16_t rd_regF;

//queue and msg queues
static osMessageQueueId_t RDA_MsgQueue_comand;
static osMessageQueueId_t RDA_MsgQueue_info;

static MSGQUEUE_RDA_COMAND_t RDA_rec;
static MSGQUEUE_RDA_INFO_t RDA_TOC;

static uint8_t volumen;

int Init_MsgQueue_RDA_TOC (void) {
  RDA_MsgQueue_info = osMessageQueueNew(1, sizeof(MSGQUEUE_RDA_INFO_t), NULL);
  if (RDA_MsgQueue_info == NULL) {
		return -1;
  }
  return(0);
}

osMessageQueueId_t Init_ThRDA(osMessageQueueId_t rda_queue_comand){
	Init_MsgQueue_RDA_TOC();
	RDA_MsgQueue_comand = rda_queue_comand;
  tid_ThRDA = osThreadNew(ThRDA, NULL, NULL);
  if(tid_ThRDA == NULL)
    return(NULL);
  return RDA_MsgQueue_info;
}

static void Init_RDA(){
  I2Cdrv-> Initialize   (callback_i2c);
  I2Cdrv-> PowerControl (ARM_POWER_FULL);
  I2Cdrv-> Control      (ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
  I2Cdrv-> Control      (ARM_I2C_BUS_CLEAR, 0);
  
  wr_registrers[0] = (0x0000 |RDA_DHIZ|RDA_DMUTE|RDA_MONO|RDA_NEW_MET);																									//reg2
  wr_registrers[1] = (0x0000);																																													//reg3
  wr_registrers[2] = (RDA_AFC_DIS);																																											//reg4
  wr_registrers[3] = (RDA_INT_MOD | RDA_SEEK_MODE | RDA_SNR_TRESH | RDA_LNA_DUAL | RDA_LNA_2_1mA | RDA_INIT_VOL);				//reg5
  wr_registrers[4] = (RDA_CONF_OPEN_MODE);																																							//reg6
  wr_registrers[5] = (0x0000);																																													//reg7
  WriteAll();
	
	volumen = RDA_INIT_VOL;
	RDA_TOC.vol = volumen;
  RDA_TOC.freq_send = MINFREC;
  RDA_TOC.wr_reg2 = wr_registrers[0];
  RDA_TOC.wr_reg3 = wr_registrers[1];
  RDA_TOC.wr_reg4 = wr_registrers[2];
  RDA_TOC.wr_reg5 = wr_registrers[3];
  RDA_TOC.wr_reg6 = wr_registrers[4];
  RDA_TOC.wr_reg7 = wr_registrers[5];
  osMessageQueuePut(RDA_MsgQueue_info, &RDA_TOC, 0U, 0U);
}

static void ThRDA(void *argument){
  Init_RDA();
  while(1){
    osMessageQueueGet(RDA_MsgQueue_comand, &RDA_rec, 0U, osWaitForever);
      switch(RDA_rec.comand){
        case power_on:
          Power_On();
          Set_Freq(MINFREC);
        break;
        case power_off:
          Power_Off();
        break;
        case seek_up:
          Seek_Up();
        break;
        case seek_down:
          Seek_Down();
        break;
        case subida_freq:
					subir_100khz();
        break;
        case bajada_freq:
					bajar_100khz();
        break;
        case set_vol:
					Set_Vol(RDA_rec.vol_level);
        break;
        case set_freq:
					Set_Freq(RDA_rec.freq);
        break;
        case obtener_info:
					Get_Info();
        break;
			}
  }
}

static int WriteAll(){
    static uint32_t flags;
    static uint8_t buffer[30];
//Buffer even values
    buffer[0]  = wr_registrers[0] >> 8;
    buffer[2]  = wr_registrers[1] >> 8;
    buffer[4]  = wr_registrers[2] >> 8;
    buffer[6]  = wr_registrers[3] >> 8;
    buffer[8]  = wr_registrers[4] >> 8;
    buffer[10] = wr_registrers[5] >> 8;
//Buffer odd values
    buffer[3]  = wr_registrers[1] & 0x00FF;
    buffer[1]  = wr_registrers[0] & 0x00FF;
    buffer[5]  = wr_registrers[2] & 0x00FF;
    buffer[7]  = wr_registrers[3] & 0x00FF;
    buffer[9]  = wr_registrers[4] & 0x00FF;
    buffer[11] = wr_registrers[5] & 0x00FF;
    I2Cdrv->MasterTransmit(RDA_WR_ADD, buffer, 30, false);
    flags = osThreadFlagsWait(0xFFFF, osFlagsWaitAny, osWaitForever);
    if((flags & ARM_I2C_EVENT_TRANSFER_INCOMPLETE) != 0U && (flags & ARM_I2C_EVENT_TRANSFER_DONE) != true)
      return -1;
    return 0;
}

static int ReadRegistrers() {
	static uint32_t flags;
  static uint8_t data[30];
	
	I2Cdrv->MasterReceive(RDA_RD_ADD, data, 12, false);
	flags = osThreadFlagsWait(0xFFFF, osFlagsWaitAny, osWaitForever);
	if((flags & ARM_I2C_EVENT_TRANSFER_INCOMPLETE) != 0U && (flags & ARM_I2C_EVENT_TRANSFER_DONE) != true) {
		return -1;
	} else {
		rd_regA = ((rd_regA & (data[0]) << 8) | data[1]);
		rd_regB = ((data[2] << 8) | data[3]);
		//rd_regC = ((data[4] << 8) | data[5]);
		//rd_regD = ((data[6] << 8) | data[7]);
		//rd_regE = ((data[8] << 8) | data[9]);
		//rd_regF = ((data[10] << 8) | data[11]);
    return 0;
	}
}


static void Power_On(){
  wr_registrers[0] = wr_registrers[0] | RDA_PWR_ON;
  wr_registrers[1] = wr_registrers[1] | RDA_TUNE_ON;
  RDA_TOC.wr_reg2 = wr_registrers[0];
  RDA_TOC.wr_reg3 = wr_registrers[1];
  WriteAll();
  wr_registrers[1] = wr_registrers[1] & ~RDA_TUNE_ON ;
	osDelay(300);
	Get_Info();
}

static void Power_Off() {
	wr_registrers[0] = wr_registrers[0] ^ RDA_POWER;
  RDA_TOC.wr_reg2 = wr_registrers[0];
	WriteAll();
}

static void Seek_Up(){
  wr_registrers[0] = wr_registrers[0] | RDA_SEEK_UP;
  RDA_TOC.wr_reg2 = wr_registrers[0];
  WriteAll();
	wr_registrers[0] = wr_registrers[0] & ~RDA_SEEK_UP;
	osDelay(300);
	Get_Info();
}

static void Seek_Down(){
  wr_registrers[0] = wr_registrers[0] | RDA_SEEK_DOWN;
  RDA_TOC.wr_reg2 = wr_registrers[0];
  WriteAll();
	wr_registrers[0] = wr_registrers[0] & ~RDA_SEEK_DOWN;
	osDelay(300);
	Get_Info();
}

static void Set_Vol(uint8_t vol){
	if (vol > 15) {
		volumen = 15;
	} else {
		volumen = vol;
	}
  wr_registrers[3]= (wr_registrers[3] & 0xFFF0) | volumen;
  RDA_TOC.wr_reg5 = wr_registrers[3];
  WriteAll();
	RDA_TOC.vol = volumen;
  osMessageQueuePut(RDA_MsgQueue_info, &RDA_TOC, 0U, 0U);
}

static void Set_Freq(float frecuencia){
	if (frecuencia < MINFREC) {
		frecuencia = MINFREC;
	} else if (frecuencia > MAXFREC) {
		frecuencia = MAXFREC;
	}
	int Channel;
  Channel = (((frecuencia/1000)-StartingFreq)/0.1)+0.05;
  Channel = Channel & 0x03FF;
  wr_registrers[1] = Channel*64 + 0x10;  // Channel + TUNE-Bit + Band=00(87-108) + Space=00(100kHz)
  RDA_TOC.wr_reg3 = wr_registrers[1];
  WriteAll();
  wr_registrers[1] = wr_registrers[1] & ~RDA_TUNE_ON;
	RDA_TOC.freq_send = frecuencia;
  osMessageQueuePut(RDA_MsgQueue_info, &RDA_TOC, 0U, 0U);
}

static void subir_100khz() {
	ReadRegistrers();
	float frec = RDA_TOC.freq_send;
	frec = frec + 100;
	Set_Freq(frec);
}

static void bajar_100khz() {
	ReadRegistrers();
	float frec = RDA_TOC.freq_send;
	frec = frec - 100;
	Set_Freq(frec);
}

static void Get_Info(){
	ReadRegistrers();
  RDA_TOC.freq_send = (((rd_regA & 0x03FF) * 100) + 87000);
	RDA_TOC.vol = volumen;
	RDA_TOC.rssi = (rd_regB >> 9);
  osMessageQueuePut(RDA_MsgQueue_info, &RDA_TOC, 0U, 0U);
}
