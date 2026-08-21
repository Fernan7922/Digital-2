/* USER CODE BEGIN Header */
//Fernando JOsé Guzman González
//24734
//Lab_Juego de Carreras 2.0 LEDS_DISPLAY_BOTON
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ********************************----------------------------------------------
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// Definición de los estados del juego
typedef enum {
    STATE_IDLE,
    STATE_COUNTDOWN,
    STATE_PLAYING,
    STATE_FINISHED
} GameState;

volatile GameState game_state = STATE_IDLE; // Estado inicial del juego

// Contadores de posición secuencial (0 a 4) para cada jugador
volatile uint8_t p1_counter = 0;
volatile uint8_t p2_counter = 0;

// Variable de control del ganador (0: sin ganador, 1: Jugador 1, 2: Jugador 2)
volatile uint8_t winner = 0;

// Banderas de control para saber si se presionó el botón Start
volatile uint8_t start_pressed = 0;

// Tiempos para el anti-rebote (Debounce) por software de cada botón
volatile uint32_t last_btn_p1_time = 0;
volatile uint32_t last_btn_p2_time = 0;
volatile uint32_t last_btn_start_time = 0; // Guarda el tiempo del botón Start

const uint8_t WINNING_SCORE = 4; // Meta: completar las 4 posiciones secuenciales
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Display_WriteDigit(char digit);
void Update_P1_LEDs_Sequential(uint8_t value);
void Update_P2_LEDs_Sequential(uint8_t value);
void Set_P1_LEDs_All_ON(void);
void Set_P1_LEDs_All_OFF(void);
void Set_P2_LEDs_All_ON(void);
void Set_P2_LEDs_All_OFF(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  // Apagar LEDs y display de 7 segmentos al iniciar o resetear
  Set_P1_LEDs_All_OFF();
  Set_P2_LEDs_All_OFF();
  Display_WriteDigit(' ');
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // Mientras no haya un ganador, actualizamos los LEDs de forma secuencial (uno a la vez)
      if (winner == 0) {
          Update_P1_LEDs_Sequential(p1_counter);
          Update_P2_LEDs_Sequential(p2_counter);

          // Rutina para verificar si algún jugador llegó a la meta (LED 4 encendido)
          if (p1_counter >= WINNING_SCORE) {
              winner = 1;
          } else if (p2_counter >= WINNING_SCORE) {
              winner = 2;
          }
      }
      else {
          // Rutina para mostrar resultados al haber un ganador
          if (winner == 1) {
              Set_P1_LEDs_All_ON();    // Todos los LEDs del ganador encendidos
              Set_P2_LEDs_All_OFF();   // Todos los LEDs del perdedor apagados
              Display_WriteDigit('1'); // Muestra '1' en el display
          }
          else if (winner == 2) {
              Set_P1_LEDs_All_OFF();   // Todos los LEDs del perdedor apagados
              Set_P2_LEDs_All_ON();    // Todos los LEDs del ganador encendidos
              Display_WriteDigit('2'); // Muestra '2' en el display
          }
      }
      HAL_Delay(10); // Retardo de estabilidad
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
//Prueba de Cambios desde Cube ide 2
// Callback para procesar las interrupciones físicas de los botones
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t current_time = HAL_GetTick();

    // Si ya existe un ganador, ignoramos pulsaciones adicionales
    if (winner != 0) {
        return;
    }

    // Interrupción del botón de Jugador 1
    if (GPIO_Pin == BTN_P1_Pin) {
        // Anti-rebote por software: descarta señales en un rango de 150ms
        if (current_time - last_btn_p1_time > 150) {
            last_btn_p1_time = current_time;
            if (p1_counter < WINNING_SCORE) {
                p1_counter++;
            }
        }
    }
    // Interrupción del botón de Jugador 2
    else if (GPIO_Pin == BTN_P2_Pin) {
        // Anti-rebote por software: descarta señales en un rango de 150ms
        if (current_time - last_btn_p2_time > 150) {
            last_btn_p2_time = current_time;
            if (p2_counter < WINNING_SCORE) {
                p2_counter++;
            }
        }
    }
}

// Actualización secuencial del Jugador 1: enciende únicamente el LED activo
void Update_P1_LEDs_Sequential(uint8_t value) {
    HAL_GPIO_WritePin(GPIOC, LED_P1_0_Pin, (value == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, LED_P1_1_Pin, (value == 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, LED_P1_2_Pin, (value == 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, LED_P1_3_Pin, (value == 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// Actualización secuencial del Jugador 2: enciende únicamente el LED activo
void Update_P2_LEDs_Sequential(uint8_t value) {
    HAL_GPIO_WritePin(GPIOB, LED_P2_0_Pin, (value == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_P2_1_Pin, (value == 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_P2_2_Pin, (value == 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_P2_3_Pin, (value == 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// Funciones auxiliares para encender/apagar todos los LEDs de cada puerto
void Set_P1_LEDs_All_ON(void) {
    HAL_GPIO_WritePin(GPIOC, LED_P1_0_Pin|LED_P1_1_Pin|LED_P1_2_Pin|LED_P1_3_Pin, GPIO_PIN_SET);
}

void Set_P1_LEDs_All_OFF(void) {
    HAL_GPIO_WritePin(GPIOC, LED_P1_0_Pin|LED_P1_1_Pin|LED_P1_2_Pin|LED_P1_3_Pin, GPIO_PIN_RESET);
}

void Set_P2_LEDs_All_ON(void) {
    HAL_GPIO_WritePin(GPIOB, LED_P2_0_Pin|LED_P2_1_Pin|LED_P2_2_Pin|LED_P2_3_Pin, GPIO_PIN_SET);
}

void Set_P2_LEDs_All_OFF(void) {
    HAL_GPIO_WritePin(GPIOB, LED_P2_0_Pin|LED_P2_1_Pin|LED_P2_2_Pin|LED_P2_3_Pin, GPIO_PIN_RESET);
}

// Control del Display de 7 Segmentos (Cátodo Común)
void Display_WriteDigit(char digit) {
    // Apaga todos los segmentos
    HAL_GPIO_WritePin(GPIOA, SEG_A_Pin|SEG_B_Pin|SEG_C_Pin|SEG_D_Pin|SEG_E_Pin|SEG_F_Pin|SEG_G_Pin, GPIO_PIN_RESET);

    switch(digit) {
        case '1':
            HAL_GPIO_WritePin(GPIOA, SEG_B_Pin|SEG_C_Pin, GPIO_PIN_SET);
            break;
        case '2':
            HAL_GPIO_WritePin(GPIOA, SEG_A_Pin|SEG_B_Pin|SEG_G_Pin|SEG_E_Pin|SEG_D_Pin, GPIO_PIN_SET);
            break;
        default:
            break;
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USER CODE END 6 */
