/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    tim.c
 * @brief   This file provides code for the configuration
 *          of the TIM instances.
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
/* Includes ------------------------------------------------------------------*/
#include "tim.h"

/* USER CODE BEGIN 0 */
volatile uint16_t buttonPressed = 0;
/* USER CODE END 0 */

TIM_HandleTypeDef htim5;

/* TIM5 init function */
void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 6;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 0xffff;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim5, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */
  HAL_TIM_IC_Start(&htim5, TIM_CHANNEL_2);
  /* USER CODE END TIM5_Init 2 */
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (tim_baseHandle->Instance == TIM5)
  {
    /* USER CODE BEGIN TIM5_MspInit 0 */

    /* USER CODE END TIM5_MspInit 0 */
    /* TIM5 clock enable */
    __HAL_RCC_TIM5_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM5 GPIO Configuration
    PA1     ------> TIM5_CH2
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* TIM5 interrupt Init */
    HAL_NVIC_SetPriority(TIM5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);
    /* USER CODE BEGIN TIM5_MspInit 1 */

    /* USER CODE END TIM5_MspInit 1 */
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *tim_baseHandle)
{

  if (tim_baseHandle->Instance == TIM5)
  {
    /* USER CODE BEGIN TIM5_MspDeInit 0 */

    /* USER CODE END TIM5_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM5_CLK_DISABLE();

    /**TIM5 GPIO Configuration
    PA1     ------> TIM5_CH2
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);

    /* TIM5 interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM5_IRQn);
    /* USER CODE BEGIN TIM5_MspDeInit 1 */

    /* USER CODE END TIM5_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
uint8_t tpad_init(void)
{
  uint16_t buf[10];
  uint16_t temp;
  uint8_t i, j;
  MX_TIM5_Init();

  for (int i = 0; i < 10; i++)
  {
    buf[i] = tpad_get_val();
    HAL_Delay(10);
  }

  for (int i = 0; i < 9; i++)
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (buf[i] > buf[j])
      {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  temp = 0;

  for (int i = 2; i < 8; i++)
  {
    temp += buf[i];
  }

  buttonPressed = temp / 6;
  printf("tpad default val: %d\n", buttonPressed);

  if (buttonPressed > TPAD_ARR_MAX_VAL / 2)
  {
    return 1;
  }
  return 0;
}

static void tpad_reset(void)
{
  GPIO_InitTypeDef gpio_init_struct;
  gpio_init_struct.Pin = GPIO_PIN_1;               /* 输入捕获的GPIO�? */
  gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;     /* 复用推挽输出 */
  gpio_init_struct.Pull = GPIO_PULLUP;             /* 上拉 */
  gpio_init_struct.Speed = GPIO_SPEED_FREQ_MEDIUM; /* 中�?? */
  HAL_GPIO_Init(GPIOA, &gpio_init_struct);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); /* TPAD引脚输出0, 放电 */

  HAL_Delay(5);

  htim5.Instance->SR = 0;  /* 清除标记 */
  htim5.Instance->CNT = 0; /* 归零 */

  gpio_init_struct.Pin = GPIO_PIN_1;               /* 输入捕获的GPIO口 */
  gpio_init_struct.Mode = GPIO_MODE_INPUT;         /* 复用推挽输入 */
  gpio_init_struct.Pull = GPIO_NOPULL;             /* 浮空 */
  gpio_init_struct.Speed = GPIO_SPEED_FREQ_MEDIUM; /* 中速 */
  HAL_GPIO_Init(GPIOA, &gpio_init_struct);         /* TPAD引脚浮空输入 */
}

static uint16_t tpad_get_val(void)
{
  uint32_t flag = TIM_FLAG_CC2;

  tpad_reset();

  while (__HAL_TIM_GET_FLAG(&htim5, flag) == RESET)
  {
    if (htim5.Instance->CNT > TPAD_ARR_MAX_VAL - 500)
    {
      return htim5.Instance->CNT;
    }
  }
  return TIM5->CCR2;
}

static uint16_t tpad_get_maxval(uint8_t n)
{
  uint16_t temp = 0;
  uint16_t maxval = 0;

  while (n--)
  {
    temp = tpad_get_val(); /* 得到�??次�?? */

    if (temp > maxval)
      maxval = temp;
  }

  return maxval;
}

uint8_t tpad_scan(uint8_t mode)
{
  static uint8_t keyen = 0; /* 0, 可以�??始检�??;  >0, 还不能开始检�??; */
  uint8_t res = 0;
  uint8_t sample = 3; /* 默认采样次数�??3�?? */
  uint16_t rval;

  if (mode)
  {
    sample = 6; /* 支持连按的时候，设置采样次数�??6�?? */
    keyen = 0;  /* 支持连按, 每次调用该函数都可以�??�?? */
  }

  rval = tpad_get_maxval(sample);

  if (rval > (buttonPressed + TPAD_GATE_VAL)) /* 大于tpad_default_val+TPAD_GATE_VAL,有效 */
  {
    if (keyen == 0)
    {
      res = 1; /* keyen==0, 有效 */
    }

    // printf("r:%d\r\n", rval);   /* 输出计数�??, 调试的时候才用到 */
    keyen = 3; /* 至少要再�??3次之后才能按键有�?? */
  }

  if (keyen)
    keyen--;

  return res;
}
/* USER CODE END 1 */
