/* USER CODE BEGIN Header */
/** Fernando Guzman
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h> // Se agrega esta librería para poder utilizar "sprintf" y dar formato de texto a los datos.
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
// Variables para medir el tiempo transcurrido entre flancos de subida de la señal en TIM2
uint32_t subida_anterior = 0;          // Almacena el valor del contador en el primer flanco detectado
uint32_t subida_actual = 0;            // Almacena el valor del contador en el segundo flanco detectado
uint32_t ticks_diferencia = 0;         // Guarda el periodo medido expresado en ticks de reloj
uint8_t primer_flanco_leido = 0;       // Bandera de control para diferenciar la primera lectura de la segunda

// Variable donde guardaremos el cálculo final de la frecuencia en Hertz
volatile uint32_t frecuencia_final = 0;

// Buffer temporal donde se escribe el texto que se enviará por el puerto serie
char bufer_texto[100];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  // Se inicializa el temporizador TIM2 en modo de captura de entrada usando interrupciones
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

  // Arrancamos el TIM3 en modo interrupción básica (generará interrupciones cada 250 ms)
  HAL_TIM_Base_Start_IT(&htim3);

  // Arrancamos el TIM4 en modo interrupción básica (generará interrupciones cada 1000 ms)
  HAL_TIM_Base_Start_IT(&htim4);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // Se genera la cadena de texto con la última lectura de frecuencia registrada
    int longitud_mensaje = sprintf(bufer_texto, "Frecuencia en pin PA0: %lu Hz\r\n", frecuencia_final);

    // Transmisión del mensaje a través de USART2 (directo a la terminal serie en la PC)
    HAL_UART_Transmit(&huart2, (uint8_t*)bufer_texto, longitud_mensaje, HAL_MAX_DELAY);

    // Pausa de 500 ms para evitar la sobrecarga de datos en la pantalla de Termite
    HAL_Delay(500);

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
// Callback de interrupción que se ejecuta automáticamente ante cada flanco de subida en el pin PA0
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    // Verificación para asegurar que la interrupción pertenece al temporizador TIM2
    if (htim->Instance == TIM2)
    {
        if (primer_flanco_leido == 0) // Primer punto de la onda (inicio de ciclo)
        {
            // Captura del primer tiempo registrado por el contador del timer
            subida_anterior = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            primer_flanco_leido = 1; // Indicación de que ya se registró la primera marca de tiempo
        }
        else // Segundo punto de la onda (fin del ciclo medido)
        {
            // Captura del segundo tiempo registrado
            subida_actual = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

            // Cálculo de la diferencia de tiempo. Se maneja el caso de desbordamiento por si el contador llega a cero entre medidas.
            if (subida_actual >= subida_anterior)
            {
                ticks_diferencia = subida_actual - subida_anterior;
            }
            else
            {
                ticks_diferencia = (0xFFFFFFFF - subida_anterior) + subida_actual + 1;
            }

            // Cálculo final de la frecuencia. Al tener una base de tiempo de 1 MHz, un tick representa 1 microsegundo.
            // Frecuencia = 1,000,000 Hz / ticks_transcurridos
            if (ticks_diferencia > 0)
            {
                frecuencia_final = 1000000 / ticks_diferencia;
            }

            // Se hace Restablecimiento de la bandera para iniciar la lectura del siguiente ciclo de onda
            primer_flanco_leido = 0;
        }
    }
} // AQUÍ SE CIERRA CORRECTAMENTE LA FUNCIÓN DE CAPTURA

// Esta función se ejecuta automáticamente cuando TIM3 o TIM4 completan su conteo de tiempo (desbordamiento)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // Si la interrupción proviene de TIM3 (cada 250 ms)
    if (htim->Instance == TIM3)
    {
        // Cambia el estado del pin PB3 (de encendido a apagado, o viceversa)
        HAL_GPIO_TogglePin(LED_500ms_GPIO_Port, LED_500ms_Pin);
    }

    // Si la interrupción proviene de TIM4 (cada 1000 ms o 1 segundo)
    if (htim->Instance == TIM4)
    {
        // Cambia el estado del pin PB4 (de encendido a apagado, o viceversa)
        HAL_GPIO_TogglePin(LED_2s_GPIO_Port, LED_2s_Pin);
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
#endif /* USE_FULL_ASSERT */
