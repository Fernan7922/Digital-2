/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Six Days - tarareo aproximado del MP3 adjunto
  *                   Fragmento vocal 20.04--28.34 s, PWM PA6 TIM3_CH1
  *                   Arranque automatico: suena en bucle continuo al encender
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Transcripcion aproximada: se simplifican portamentos y vibrato.
 * 1 = una octava arriba del MP3; 0 = registro aproximado de la voz.
 * Velocidad: 100 normal, 110 mas rapido, 90 mas lento.
 */
#define BUZZER_OCTAVE_UP  1U
#define TEMPO_PERCENT    100U
#define LOOP_PAUSE_MS    800U      /* Pausa entre repeticiones */
#define TIM3_TICK_HZ     1000000UL /* Configuracion actual: 84 MHz / 84 */
#define REST             0U
#if BUZZER_OCTAVE_UP > 2U
#error "BUZZER_OCTAVE_UP debe estar entre 0 y 2"
#endif
#if TEMPO_PERCENT < 50U || TEMPO_PERCENT > 200U
#error "TEMPO_PERCENT debe estar entre 50 y 200"
#endif
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DAC_HandleTypeDef hdac;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Frecuencias de la voz en Hz; Tocar_Tono aplica la octava elegida.
 * Duracion = tiempo total; gap = silencio al final de ese evento.
 * gap 0 permite unir las notas de un deslizamiento.
 */
const uint16_t han_meme_notes[] = {
    // Frase 1: inicio, 20.04--22.60 s
    233, 277, 311, 349, 349, 349, 349, 349, 0,
    // Frase 2: 22.60--25.61 s
    233, 277, 233, 277, 311, 349, 349, 349, 370, 349, 0,
    // Frase 3: 25.61--28.34 s
    277, 311, 415, 415, 415, 370, 349,
};

const uint16_t han_meme_durations[] = {
    // Frase 1: inicio, 20.04--22.60 s
    340, 350, 140, 230, 290, 360, 220, 350, 280,
    // Frase 2: 22.60--25.61 s
    180, 110, 210, 310, 140, 330, 330, 290, 180, 430, 500,
    // Frase 3: 25.61--28.34 s
    360, 340, 380, 360, 380, 310, 600,
};

const uint16_t han_meme_gaps[] = {
    // Frase 1: inicio, 20.04--22.60 s
    35, 100, 0, 18, 20, 25, 15, 70, 0,
    // Frase 2: 22.60--25.61 s
    15, 10, 15, 100, 0, 18, 20, 20, 0, 70, 0,
    // Frase 3: 25.61--28.34 s
    18, 18, 18, 18, 0, 0, 80,
};

const uint16_t MEME_LEN = sizeof(han_meme_notes) / sizeof(han_meme_notes[0]);

/* Tabla senoidal precalculada para el DAC (si se usa en tu laboratorio) */
const uint16_t sine_lut[64] = {
    2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346,
    3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
    4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630,
    3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
    2048, 1847, 1648, 1453, 1264, 1082,  910,  749,
     600,  465,  345,  242,  156,   88,   39,   10,
       0,   10,   39,   88,  156,  242,  345,  465,
     600,  749,  910, 1082, 1264, 1453, 1648, 1847
};

volatile uint32_t dac_phase_acc = 0;
volatile uint32_t dac_phase_step = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
void Tocar_Tono(uint32_t freq);
void Reproducir_Meme_Han(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Configura la frecuencia del PWM en TIM3 Canal 1 (Pin PA6)
void Tocar_Tono(uint32_t freq) {
    if (freq == REST) {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    } else {
        freq <<= BUZZER_OCTAVE_UP;
        uint32_t ticks = (TIM3_TICK_HZ + freq / 2U) / freq;
        if (ticks < 2U) ticks = 2U;
        if (ticks > 65536U) ticks = 65536U;
        __HAL_TIM_SET_AUTORELOAD(&htim3, ticks - 1U);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ticks / 2U);
    }
    /* Aplica ARR/CCR precargados ahora, incluido el silencio. */
    __HAL_TIM_SET_COUNTER(&htim3, 0U);
    htim3.Instance->EGR = TIM_EGR_UG;
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
}

// Reproduce el fragmento completo una vez
void Reproducir_Meme_Han(void) {
    for (uint16_t i = 0; i < MEME_LEN; ++i) {
        uint32_t total = ((uint32_t)han_meme_durations[i] * 100U
                          + TEMPO_PERCENT / 2U) / TEMPO_PERCENT;
        uint32_t gap = ((uint32_t)han_meme_gaps[i] * 100U
                        + TEMPO_PERCENT / 2U) / TEMPO_PERCENT;
        if (gap > total) gap = total;

        Tocar_Tono(han_meme_notes[i]);
        HAL_Delay(total - gap);

        if (gap > 0U) {
            Tocar_Tono(REST);
            HAL_Delay(gap);
        }
    }
    Tocar_Tono(REST);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_DAC_Init();
  MX_TIM6_Init();

  /* USER CODE BEGIN 2 */
  // Iniciar PWM en TIM3_CH1 (PA6)
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  Tocar_Tono(REST);
  HAL_Delay(500);   // pequeña pausa al encender
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    Reproducir_Meme_Han();
    HAL_Delay(LOOP_PAUSE_MS);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

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

/**
  * @brief DAC Initialization Function
  */
static void MX_DAC_Init(void)
{
  DAC_ChannelConfTypeDef sConfig = {0};

  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function (Pin PA6 -> TIM3_CH1)
  */
static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83; // 84 MHz / (83 + 1) = 1 MHz
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief TIM6 Initialization Function
  */
static void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 83;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 49;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function (115200 Baudios)
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        if (dac_phase_step > 0) {
            dac_phase_acc += dac_phase_step;
            uint8_t index = (dac_phase_acc >> 16) & 0x3F;
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, sine_lut[index]);
        }
    }
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
