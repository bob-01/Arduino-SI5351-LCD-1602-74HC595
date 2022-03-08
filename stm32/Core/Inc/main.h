/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ENC_A_Pin GPIO_PIN_6
#define ENC_A_GPIO_Port GPIOA
#define ENC_A_EXTI_IRQn EXTI4_15_IRQn
#define ENC_B_Pin GPIO_PIN_7
#define ENC_B_GPIO_Port GPIOA
#define ENC_B_EXTI_IRQn EXTI4_15_IRQn
#define ENC_BUTTON_Pin GPIO_PIN_15
#define ENC_BUTTON_GPIO_Port GPIOA
#define LCD_DATA_Pin GPIO_PIN_3
#define LCD_DATA_GPIO_Port GPIOB
#define LCD_CLK_Pin GPIO_PIN_4
#define LCD_CLK_GPIO_Port GPIOB
#define LCD_EN_Pin GPIO_PIN_5
#define LCD_EN_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define VERSION   "1.01a"

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))

#define _N(a) sizeof(a)/sizeof(a[0])
#define N_PARAMS 22  // number of (visible) parameters
#define N_ALL_PARAMS (N_PARAMS + 2)  // number of parameters

#define EEPROM_OFFSET 0x150
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
