/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.h
  * @brief   This file contains all the function prototypes for
  *          the tim.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern TIM_HandleTypeDef htim5;

/* USER CODE BEGIN Private defines */
#define TPAD_GATE_VAL 100
#define TPAD_ARR_MAX_VAL 0xFFFF

extern volatile uint16_t buttonPressed;/* 空载的时�?(没有手按�?),计数器需要的时间 */

/* 静�?�函�??, 仅限 tapd.c调用 */
static void tpad_reset(void);               /* 复位 */
static uint16_t tpad_get_val(void);         /* 得到定时器捕�? �? */
static uint16_t tpad_get_maxval(uint8_t n); /* 读取n�?, 获取�?大�?? */


/* 接口函数, 可以在其�?.c调用 */
uint8_t tpad_init(void);    /* TPAD 初始�? 函数 */
uint8_t tpad_scan(uint8_t mode);    /* TPAD 扫描 函数 */
/* USER CODE END Private defines */

void MX_TIM5_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

