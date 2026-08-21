/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

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
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define LED_P1_0_Pin GPIO_PIN_0
#define LED_P1_0_GPIO_Port GPIOC
#define LED_P1_1_Pin GPIO_PIN_1
#define LED_P1_1_GPIO_Port GPIOC
#define LED_P1_2_Pin GPIO_PIN_2
#define LED_P1_2_GPIO_Port GPIOC
#define LED_P1_3_Pin GPIO_PIN_3
#define LED_P1_3_GPIO_Port GPIOC
#define SEG_A_Pin GPIO_PIN_0
#define SEG_A_GPIO_Port GPIOA
#define SEG_B_Pin GPIO_PIN_1
#define SEG_B_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define SEG_C_Pin GPIO_PIN_4
#define SEG_C_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define SEG_D_Pin GPIO_PIN_6
#define SEG_D_GPIO_Port GPIOA
#define SEG_E_Pin GPIO_PIN_7
#define SEG_E_GPIO_Port GPIOA
#define BTN_START_Pin GPIO_PIN_0
#define BTN_START_GPIO_Port GPIOB
#define BTN_START_EXTI_IRQn EXTI0_IRQn
#define BTN_P1_Pin GPIO_PIN_1
#define BTN_P1_GPIO_Port GPIOB
#define BTN_P1_EXTI_IRQn EXTI1_IRQn
#define BTN_P2_Pin GPIO_PIN_2
#define BTN_P2_GPIO_Port GPIOB
#define BTN_P2_EXTI_IRQn EXTI2_IRQn
#define LED_P2_0_Pin GPIO_PIN_12
#define LED_P2_0_GPIO_Port GPIOB
#define LED_P2_1_Pin GPIO_PIN_13
#define LED_P2_1_GPIO_Port GPIOB
#define LED_P2_2_Pin GPIO_PIN_14
#define LED_P2_2_GPIO_Port GPIOB
#define LED_P2_3_Pin GPIO_PIN_15
#define LED_P2_3_GPIO_Port GPIOB
#define SEG_F_Pin GPIO_PIN_9
#define SEG_F_GPIO_Port GPIOA
#define SEG_G_Pin GPIO_PIN_10
#define SEG_G_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
