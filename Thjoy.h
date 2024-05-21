#ifndef __THJOY_H
#define __THJOY_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include "stdbool.h"

#define REBOTES_TIME 100U
#define LONG_PULSE_TIME 1000U

#define FLAGS_LONG_PULSE 0x0004U
#define FLAGS_REBOTES 0x0001U
#define FLAGS_PULSACION 0x0002U
#define FLAG_BUZZER 0x0008U
#define MSG_JOY_COUNT 10

typedef enum {right, left, up, down, center, blue} pulsacion_t;

typedef struct {                                // object data type
  pulsacion_t pulsacion;
  bool corto_larga;
  bool buzz;
} MSGQUEUE_JOY_t;

typedef struct pin_puerto {
  uint16_t pin;
  GPIO_TypeDef * puerto;
} pin_puerto;

osMessageQueueId_t Init_Thjoy(void);

#endif
