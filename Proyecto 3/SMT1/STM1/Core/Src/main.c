/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM1 - Lectura de cuatro LDR y prueba por USART2
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
#define MUESTRAS_ADC             16U
#define MUESTRAS_CALIBRACION      32U

#define ESPERA_CALIBRACION_MS     3000U
#define PERIODO_REPORTE_MS        300U

/*
 * Porcentajes respecto a la lectura con el LDR descubierto.
 * Son valores iniciales para comprobar el montaje.
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
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/*
 * P1: A0 -> PA0 -> ADC1_IN0
 * P2: A1 -> PA1 -> ADC1_IN1
 * P3: A2 -> PA4 -> ADC1_IN4
 * P4: A3 -> PB0 -> ADC1_IN8
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
 * Bits: 0000 P1 P2 P3 P4.
 * Todavia no se transmite por I2C.
 */
static uint8_t estado_local = 0U;

static char mensaje[192];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
static void Serial_Enviar(const char *texto);
static HAL_StatusTypeDef LDR_Leer(uint32_t canal, uint16_t *valor);
static HAL_StatusTypeDef LDR_Calibrar(void);
static void LDR_ActualizarEstados(void);
static void LDR_MostrarEstados(void);
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
     * Se descarta la primera conversion despues de cambiar
     * de canal y se promedian las siguientes.
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
        "\r\nCALIBRACION: deja los cuatro LDR descubiertos.\r\n"
        "La medicion comienza en 3 segundos.\r\n"
    );

    HAL_Delay(ESPERA_CALIBRACION_MS);

    Serial_Enviar("Midiendo... manten los LDR descubiertos.\r\n");

    for (uint32_t n = 0U; n < MUESTRAS_CALIBRACION; n++)
    {
        for (uint8_t i = 0U; i < NUM_LDR; i++)
        {
            if (LDR_Leer(canales[i], &muestra) != HAL_OK)
            {
                Serial_Enviar("ERROR: fallo del ADC al calibrar.\r\n");
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
            (unsigned int)(i + 1U),
            (unsigned int)referencia[i],
            (unsigned int)umbral_ocupado[i],
            (unsigned int)umbral_libre[i]
        );

        Serial_Enviar(mensaje);

        /*
         * Evita usar umbrales nulos o iguales por falta
         * de senal. No sustituye la comprobacion del contraste.
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
            "Revisa luz, conexiones y GND; luego presiona RESET.\r\n"
        );

        return HAL_ERROR;
    }

    estado_local = 0U;

    Serial_Enviar(
        "\r\nCalibracion terminada.\r\n"
        "Ahora puedes tapar cada LDR para probarlo.\r\n"
        "1 = ocupado | 0 = libre\r\n\r\n"
    );

    return HAL_OK;
}

static void LDR_ActualizarEstados(void)
{
    estado_local = 0U;

    for (uint8_t i = 0U; i < NUM_LDR; i++)
    {
        if (lectura[i] <= umbral_ocupado[i])
        {
            ocupado[i] = 1U;
        }
        else if (lectura[i] >= umbral_libre[i])
        {
            ocupado[i] = 0U;
        }

        /*
         * Entre ambos umbrales se conserva el estado.
         *
         * P1 -> bit 3
         * P2 -> bit 2
         * P3 -> bit 1
         * P4 -> bit 0
         */
        estado_local |= (uint8_t)(ocupado[i] << (3U - i));
    }
}

static void LDR_MostrarEstados(void)
{
    uint8_t total_ocupados = 0U;

    for (uint8_t i = 0U; i < NUM_LDR; i++)
    {
        total_ocupados += ocupado[i];

        snprintf(
            mensaje,
            sizeof(mensaje),
            "P%u | ADC=%4u | %s\r\n",
            (unsigned int)(i + 1U),
            (unsigned int)lectura[i],
            ocupado[i]
                ? "OCUPADO - hay carro"
                : "LIBRE - no hay carro"
        );

        Serial_Enviar(mensaje);
    }

    snprintf(
        mensaje,
        sizeof(mensaje),
        "Estado: %u%u%u%u | Byte: 0x%02X | Libres STM1: %u\r\n"
        "----------------------------------------\r\n",
        (unsigned int)ocupado[0],
        (unsigned int)ocupado[1],
        (unsigned int)ocupado[2],
        (unsigned int)ocupado[3],
        (unsigned int)estado_local,
        (unsigned int)(NUM_LDR - total_ocupados)
    );

    Serial_Enviar(mensaje);
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

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */

    Serial_Enviar(
        "\r\n========================================\r\n"
        "STM1 - PRUEBA DE CUATRO LDR\r\n"
        "Divisores: LDR a 3V3 y resistencia 1k a GND\r\n"
        "P1=A0 | P2=A1 | P3=A2 | P4=A3\r\n"
        "USART2: 115200 baudios, 8N1\r\n"
        "========================================\r\n"
    );

    if (LDR_Calibrar() == HAL_OK)
    {
        calibrado = 1U;
    }

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        uint8_t lectura_correcta = 1U;

        for (uint8_t i = 0U; i < NUM_LDR; i++)
        {
            if (LDR_Leer(canales[i], &lectura[i]) != HAL_OK)
            {
                lectura_correcta = 0U;

                snprintf(
                    mensaje,
                    sizeof(mensaje),
                    "ERROR ADC en P%u\r\n",
                    (unsigned int)(i + 1U)
                );

                Serial_Enviar(mensaje);
            }
        }

        if (lectura_correcta)
        {
            if (calibrado)
            {
                LDR_ActualizarEstados();
                LDR_MostrarEstados();
            }
            else
            {
                /*
                 * Si fallo la calibracion, mostrar solo datos.
                 * No presentar estados de ocupacion no validos.
                 */
                snprintf(
                    mensaje,
                    sizeof(mensaje),
                    "SIN CALIBRAR | P1=%u P2=%u P3=%u P4=%u\r\n",
                    (unsigned int)lectura[0],
                    (unsigned int)lectura[1],
                    (unsigned int)lectura[2],
                    (unsigned int)lectura[3]
                );

                Serial_Enviar(mensaje);
            }
        }

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

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK
                               | RCC_CLOCKTYPE_SYSCLK
                               | RCC_CLOCKTYPE_PCLK1
                               | RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief ADC1 Initialization Function
  * @retval None
  */
static void MX_ADC1_Init(void)
{
    /* USER CODE BEGIN ADC1_Init 0 */
    /* USER CODE END ADC1_Init 0 */

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */
    /* USER CODE END ADC1_Init 1 */

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

    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN ADC1_Init 2 */

    /*
     * Configuracion explicita de las cuatro entradas.
     * Complementa la inicializacion MSP generada por CubeMX.
     */
    GPIO_InitTypeDef gpio_ldr = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

    gpio_ldr.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
    gpio_ldr.Mode = GPIO_MODE_ANALOG;
    gpio_ldr.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_ldr);

    gpio_ldr.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOB, &gpio_ldr);

    /* USER CODE END ADC1_Init 2 */
}

/**
  * @brief USART2 Initialization Function
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
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

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

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief This function is executed in case of error occurrence.
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
  * @brief Reports the source file and line of an assert error.
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    (void)file;
    (void)line;
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
