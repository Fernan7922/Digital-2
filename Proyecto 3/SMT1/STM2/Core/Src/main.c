/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM2 - Cuatro LDR + cuatro NeoPixels independientes
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
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */
/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_LDR                  4U
#define PRIMER_PARQUEO           5U
#define MUESTRAS_ADC             16U
#define MUESTRAS_CALIBRACION      32U
#define ESPERA_CALIBRACION_MS     3000U
#define PERIODO_REPORTE_MS        300U
/*
 * Porcentajes respecto a la lectura con el LDR descubierto.
 * Valores iniciales que deben comprobarse en el montaje:
 *
 * Lectura <= 60 %: ocupado.
 * Lectura >= 80 %: libre.
 * Entre ambos limites: conservar estado.
 */
#define PORCENTAJE_OCUPADO        60U
#define PORCENTAJE_LIBRE          80U
/* USER CODE END PD */
/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */
/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim3_ch1_trig;
UART_HandleTypeDef huart2;
/* USER CODE BEGIN PV */
/*
 * Conexion de STM2:
 *
 * P5: A0 -> PA0 -> ADC1_IN0
 * P6: A1 -> PA1 -> ADC1_IN1
 * P7: A2 -> PA4 -> ADC1_IN4
 * P8: A3 -> PB0 -> ADC1_IN8
 *
 * Cada divisor:
 * - Una pata del LDR a 3V3.
 * - Otra pata al pin analogico y a la resistencia de 1 kohm.
 * - Otro extremo de la resistencia a GND.
 */
static const uint32_t canales[NUM_LDR] = {
    ADC_CHANNEL_0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_4,
    ADC_CHANNEL_8
};
static uint16_t lectura[NUM_LDR] = {0};
static uint16_t referencia[NUM_LDR] = {0};
static uint16_t umbral_ocupado[NUM_LDR] = {0};
static uint16_t umbral_libre[NUM_LDR] = {0};
static uint8_t ocupado[NUM_LDR] = {0};
static uint8_t calibrado = 0U;
/*
 * Byte local: 0000 P5 P6 P7 P8.
 * En esta prueba solo se muestra por serial.
 */
static uint8_t estado_local = 0U;
static char mensaje[192];

/* NeoPixels WS2812B: PB4/D5, TIM3_CH1, 16 MHz, PSC=0, ARR=19. */
static uint16_t pwm_buffer[736] __attribute__((aligned(4)));
static volatile uint8_t dma_terminado = 0U;
static volatile uint8_t dma_error = 0U;
static uint8_t neo_habilitado = 0U;
static uint8_t lectura_valida[NUM_LDR] = {0};

/* USER CODE END PV */
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
static void Serial_Enviar(const char *texto);
static HAL_StatusTypeDef LDR_Leer(uint32_t canal, uint16_t *valor);
static HAL_StatusTypeDef LDR_Calibrar(void);
static void LDR_ActualizarEstados(void);
/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Serial_Enviar(const char *texto)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)texto,
        (uint16_t)strlen(texto),
        200U
    );
}
static HAL_StatusTypeDef LDR_Leer(uint32_t canal, uint16_t *valor)
{
    ADC_ChannelConfTypeDef config = {0};
    uint32_t suma = 0U;
    config.Channel = canal;
    config.Rank = 1;
    config.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &config) != HAL_OK)
    {
        return HAL_ERROR;
    }
    /*
     * Descartar la primera conversion despues del cambio
     * de canal y promediar las siguientes.
     */
    for (uint32_t n = 0U; n <= MUESTRAS_ADC; n++)
    {
        if (HAL_ADC_Start(&hadc1) != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);
            return HAL_ERROR;
        }
        if (HAL_ADC_PollForConversion(&hadc1, 20U) != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);
            return HAL_ERROR;
        }
        uint32_t muestra = HAL_ADC_GetValue(&hadc1);
        if (HAL_ADC_Stop(&hadc1) != HAL_OK)
        {
            return HAL_ERROR;
        }
        if (n > 0U)
        {
            suma += muestra;
        }
    }
    *valor = (uint16_t)(suma / MUESTRAS_ADC);
    return HAL_OK;
}
static HAL_StatusTypeDef LDR_Calibrar(void)
{
    uint32_t suma[NUM_LDR] = {0};
    uint16_t muestra = 0U;
    uint8_t referencia_invalida = 0U;
    Serial_Enviar(
        "\r\nCALIBRACION STM2\r\n"
        "Deja descubiertos los LDR de P5, P6, P7 y P8.\r\n"
        "La medicion comienza en 3 segundos.\r\n"
    );
    HAL_Delay(ESPERA_CALIBRACION_MS);
    Serial_Enviar(
        "Midiendo... manten los cuatro LDR descubiertos.\r\n"
    );
    for (uint32_t n = 0U; n < MUESTRAS_CALIBRACION; n++)
    {
        for (uint8_t i = 0U; i < NUM_LDR; i++)
        {
            if (LDR_Leer(canales[i], &muestra) != HAL_OK)
            {
                Serial_Enviar(
                    "ERROR: fallo del ADC durante la calibracion.\r\n"
                );
                return HAL_ERROR;
            }
            suma[i] += muestra;
        }
        HAL_Delay(10U);
    }
    for (uint8_t i = 0U; i < NUM_LDR; i++)
    {
        referencia[i] =
            (uint16_t)(suma[i] / MUESTRAS_CALIBRACION);
        umbral_ocupado[i] = (uint16_t)(
            ((uint32_t)referencia[i] * PORCENTAJE_OCUPADO) / 100U
        );
        umbral_libre[i] = (uint16_t)(
            ((uint32_t)referencia[i] * PORCENTAJE_LIBRE) / 100U
        );
        ocupado[i] = 0U;
        snprintf(
            mensaje,
            sizeof(mensaje),
            "P%u | Referencia=%u | Ocupado<=%u | Libre>=%u\r\n",
            (unsigned int)(PRIMER_PARQUEO + i),
            (unsigned int)referencia[i],
            (unsigned int)umbral_ocupado[i],
            (unsigned int)umbral_libre[i]
        );
        Serial_Enviar(mensaje);
        /*
         * Rechazar umbrales nulos o iguales.
         * Esta comprobacion no garantiza que exista
         * suficiente diferencia entre luz y sombra.
         */
        if ((umbral_ocupado[i] == 0U) ||
            (umbral_libre[i] <= umbral_ocupado[i]))
        {
            referencia_invalida = 1U;
        }
    }
    if (referencia_invalida)
    {
        Serial_Enviar(
            "\r\nReferencia insuficiente.\r\n"
            "Revisa iluminacion, conexiones y GND.\r\n"
            "Despues deja los LDR descubiertos y presiona RESET.\r\n"
        );
        return HAL_ERROR;
    }
    estado_local = 0U;
    Serial_Enviar(
        "\r\nCalibracion terminada.\r\n"
        "Ahora puedes tapar cada LDR para probarlo.\r\n"
        "Orden de bits: P5 P6 P7 P8\r\n"
        "1 = ocupado | 0 = libre\r\n\r\n"
    );
    return HAL_OK;
}
static void LDR_ActualizarEstados(void)
{
    estado_local = 0U;
    for (uint8_t i = 0U; i < NUM_LDR; i++)
    {
        if (lectura_valida[i] && lectura[i] <= umbral_ocupado[i])
        {
            ocupado[i] = 1U;
        }
        else if (lectura_valida[i] && lectura[i] >= umbral_libre[i])
        {
            ocupado[i] = 0U;
        }
        /*
         * Entre ambos umbrales se conserva el estado.
         *
         * P5 -> bit 3
         * P6 -> bit 2
         * P7 -> bit 1
         * P8 -> bit 0
         */
        estado_local |= (uint8_t)(ocupado[i] << (3U - i));
    }
}

/* mode: 0=apagados, 1=estados LDR, 2=error general azul.
 * Reset bajo de 400 us al principio y al final; datos en orden GRB.
 */
static void NeoPixel_Preparar(uint8_t mode)
{
    memset(pwm_buffer, 0, sizeof(pwm_buffer));
    uint32_t posicion = 320U;
    for (uint8_t i = 0U; i < NUM_LDR; i++)
    {
        uint32_t grb = 0U;
        if ((mode == 2U) || ((mode == 1U) && !lectura_valida[i]))
            grb = 32U; /* Azul: no interpretar como libre/ocupado. */
        else if (mode == 1U)
            grb = ocupado[i] ? (32UL << 8) : (32UL << 16);
        for (int32_t bit = 23; bit >= 0; bit--)
            pwm_buffer[posicion++] = (grb & (1UL << bit)) ? 13U : 6U;
    }
}

static void NeoPixel_Pin(uint8_t pwm)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = pwm ? GPIO_MODE_AF_PP : GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static uint8_t NeoPixel_VerificarDMA(void)
{
    DMA_HandleTypeDef *dma = htim3.hdma[TIM_DMA_ID_CC1];
    if ((dma == NULL) || (dma->Instance != DMA1_Stream4)) return 0U;
    return (dma->Init.Channel == DMA_CHANNEL_5) &&
           (dma->Init.Direction == DMA_MEMORY_TO_PERIPH) &&
           (dma->Init.PeriphInc == DMA_PINC_DISABLE) &&
           (dma->Init.MemInc == DMA_MINC_ENABLE) &&
           (dma->Init.PeriphDataAlignment == DMA_PDATAALIGN_HALFWORD) &&
           (dma->Init.MemDataAlignment == DMA_MDATAALIGN_HALFWORD) &&
           (dma->Init.Mode == DMA_NORMAL);
}

static HAL_StatusTypeDef NeoPixel_Enviar(uint8_t mode)
{
    if (!neo_habilitado) return HAL_ERROR;
    NeoPixel_Preparar(mode);
    dma_terminado = 0U;
    dma_error = 0U;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
    __HAL_TIM_SET_COUNTER(&htim3, 0U);
    htim3.Instance->EGR = TIM_EGR_UG;
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE | TIM_FLAG_CC1);
    NeoPixel_Pin(1U);
    HAL_StatusTypeDef resultado = HAL_TIM_PWM_Start_DMA(
        &htim3, TIM_CHANNEL_1, (uint32_t *)pwm_buffer, 736U);
    if (resultado == HAL_OK)
    {
        uint32_t inicio = HAL_GetTick();
        while (!dma_terminado && !dma_error)
        {
            if ((HAL_GetTick() - inicio) >= 100U)
            {
                resultado = HAL_TIMEOUT;
                break;
            }
        }
        if (dma_error) resultado = HAL_ERROR;
    }
    HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
    NeoPixel_Pin(0U);
    HAL_Delay(1U);
    if (resultado != HAL_OK)
    {
        neo_habilitado = 0U;
        Serial_Enviar("ERROR NeoPixel/DMA: envio suspendido hasta RESET.\r\n"
                      "Revisa MSP y DMA1_Stream4_IRQHandler. Los LDR siguen activos.\r\n");
    }
    return resultado;
}

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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
    NeoPixel_Pin(0U);
    HAL_Delay(10U);
    Serial_Enviar("\r\nSTM2: LDR + NEOPIXEL | P5-P8\r\n"
                  "Datos NeoPixel: D5/PB4. Brillo: 32/255.\r\n"
                  "Rojo=ocupado; verde=libre; azul=error.\r\n");
    neo_habilitado = NeoPixel_VerificarDMA();
    if (!neo_habilitado)
        Serial_Enviar("ERROR configuracion DMA: Stream4/Channel5, Normal, Half Word.\r\n");
    (void)NeoPixel_Enviar(0U);
    if (LDR_Calibrar() == HAL_OK) calibrado = 1U;
    else Serial_Enviar("Calibracion fallida: revisar y reiniciar con LDR descubiertos.\r\n");
    /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint8_t todas_validas = 1U;
      for (uint8_t i = 0U; i < NUM_LDR; i++)
      {
          lectura_valida[i] = (LDR_Leer(canales[i], &lectura[i]) == HAL_OK);
          if (!lectura_valida[i]) todas_validas = 0U;
      }
      if (calibrado)
      {
          LDR_ActualizarEstados();
          (void)NeoPixel_Enviar(1U);
      }
      else
          (void)NeoPixel_Enviar(2U);

      for (uint8_t i = 0U; i < NUM_LDR; i++)
      {
          if (!lectura_valida[i])
              snprintf(mensaje, sizeof(mensaje), "P%u | ERROR ADC | AZUL\r\n",
                       (unsigned)(PRIMER_PARQUEO + i));
          else
              snprintf(mensaje, sizeof(mensaje), "P%u | ADC=%4u | %s\r\n",
                       (unsigned)(PRIMER_PARQUEO + i), (unsigned)lectura[i],
                       !calibrado ? "SIN CALIBRAR - AZUL" :
                       ocupado[i] ? "OCUPADO - ROJO" : "LIBRE - VERDE");
          Serial_Enviar(mensaje);
      }
      if (calibrado && todas_validas)
      {
          snprintf(mensaje, sizeof(mensaje),
                   "Estado local: %u%u%u%u | Byte: 0x%02X\r\n",
                   (unsigned)ocupado[0], (unsigned)ocupado[1],
                   (unsigned)ocupado[2], (unsigned)ocupado[3],
                   (unsigned)estado_local);
          Serial_Enviar(mensaje);
      }
      else
          Serial_Enviar("Estado local NO VALIDO para comunicar ocupacion.\r\n");
      Serial_Enviar(neo_habilitado ? "NeoPixel: DMA operativo (sin confirmacion del LED).\r\n\r\n" :
                                    "NeoPixel: envio deshabilitado por error.\r\n\r\n");
      HAL_Delay(PERIODO_REPORTE_MS);
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}
/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{
  /* USER CODE BEGIN ADC1_Init 0 */
    /*
     * Habilitar el reloj antes de inicializar el ADC.
     * CubeMX normalmente tambien lo habilita en el MSP.
     */
    __HAL_RCC_ADC1_CLK_ENABLE();
  /* USER CODE END ADC1_Init 0 */
  ADC_ChannelConfTypeDef sConfig = {0};
  /* USER CODE BEGIN ADC1_Init 1 */
  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */
    /*
     * Configuracion explicita de las cuatro entradas analogicas.
     */
    GPIO_InitTypeDef gpio_ldr = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio_ldr.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
    gpio_ldr.Mode = GPIO_MODE_ANALOG;
    gpio_ldr.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_ldr);
    gpio_ldr.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOB, &gpio_ldr);
  /* USER CODE END ADC1_Init 2 */
}
/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  /* USER CODE BEGIN TIM3_Init 0 */
  /* USER CODE END TIM3_Init 0 */
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  /* USER CODE BEGIN TIM3_Init 1 */
  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 19;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  /* USER CODE BEGIN TIM3_Init 2 */
  __HAL_TIM_ENABLE_OCxPRELOAD(&htim3, TIM_CHANNEL_1);
  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);
}
/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  /* USER CODE BEGIN USART2_Init 0 */
  /* USER CODE END USART2_Init 0 */
  /* USER CODE BEGIN USART2_Init 1 */
  /* USER CODE END USART2_Init 1 */
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
  /* USER CODE BEGIN USART2_Init 2 */
  /* USER CODE END USART2_Init 2 */
}
/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  /* DMA interrupt init */
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
}
/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);
  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}
/* USER CODE BEGIN 4 */

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if ((htim->Instance == TIM3) &&
        (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)) dma_terminado = 1U;
}
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3) dma_error = 1U;
}

/* USER CODE END 4 */
/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
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
    (void)file;
    (void)line;
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
