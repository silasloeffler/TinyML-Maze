#ifndef __MAIN_H
#define __MAIN_H

/* Main application hardware handles and base firmware prototypes. */
/* 1. Include the hardware library first. */
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 2. Declare the hardware handles. */
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart2;

#ifdef __cplusplus
}
#endif

#endif