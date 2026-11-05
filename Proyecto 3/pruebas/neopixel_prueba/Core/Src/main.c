/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Prueba de 4 NeoPixels WS2812B desde Termite
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

#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* USER CODE BEGIN PD */
#define NUM_PIXELES       4U
#define BRILLO            32U

/*
 * TIM3: reloj 16 MHz, PSC=0, ARR=19.
 * Periodo de bit: 1.25 us.
 */
#define PULSO_CERO        6U
#define PULSO_UNO         13U

/*
 * 320 periodos a nivel bajo = 400 us.
 * Se agrega una pausa antes y despues de los datos.
 */
#define RESET_SLOTS       320U
#define BUFFER_PWM_SIZE   (RESET_SLOTS + NUM_PIXELES * 24U + RESET_SLOTS)

#define DMA_TIMEOUT_MS    100U
#define LINEA_SIZE        16U
/* USER CODE END PD */

/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim3_ch1_trig;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* Componentes guardados en orden RGB. */
static uint8_t colores[NUM_PIXELES][3];

/*
 * El DMA debe usar Half Word tanto en memoria como
 * en periferico. La alineacion permite el cast de la API HAL.
 */
static uint16_t pwm_buffer[BUFFER_PWM_SIZE]
    __attribute__((aligned(4)));

static volatile uint8_t dma_terminado = 0U;
static volatile uint8_t dma_error = 0U;

static char mensaje[160];
static char linea[LINEA_SIZE];
static uint8_t longitud_linea = 0U;
static uint8_t linea_excedida = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
static void Serial_Enviar(const char *texto);
static void Mostrar_Ayuda(void);
static void Mostrar_Colores(void);
static void Procesar_Comando(char comando);

static void NeoPixel_PinReposo(void);
static void NeoPixel_PinPWM(void);
static void NeoPixel_Todos(uint8_t r, uint8_t g, uint8_t b);
static HAL_StatusTypeDef NeoPixel_Mostrar(void);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

static void Serial_Enviar(const char *texto)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)texto,
        (uint16_t)strlen(texto),
        500U
    );
}

/*
 * Mantener DIN en bajo entre transmisiones.
 * Es especialmente util si el adaptador tiene pull-ups.
 */
static void NeoPixel_PinReposo(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOB, &gpio);
}

static void NeoPixel_PinPWM(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF2_TIM3;

    HAL_GPIO_Init(GPIOB, &gpio);
}

static void NeoPixel_Todos(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint8_t i = 0U; i < NUM_PIXELES; i++)
    {
        colores[i][0] = r;
        colores[i][1] = g;
        colores[i][2] = b;
    }
}

static HAL_StatusTypeDef NeoPixel_Mostrar(void)
{
    DMA_HandleTypeDef *dma = htim3.hdma[TIM_DMA_ID_CC1];

    /*
     * Verificar la configuracion generada en el archivo MSP.
     * Esta rutina usa DMA1 Stream4 Channel5, Half Word.
     */
    if (dma == NULL)
    {
        Serial_Enviar("ERROR: TIM3_CH1 no tiene DMA enlazado.\r\n");
        return HAL_ERROR;
    }

    if ((dma->Instance != DMA1_Stream4) ||
        (dma->Init.Channel != DMA_CHANNEL_5) ||
        (dma->Init.Direction != DMA_MEMORY_TO_PERIPH) ||
        (dma->Init.PeriphInc != DMA_PINC_DISABLE) ||
        (dma->Init.MemInc != DMA_MINC_ENABLE) ||
        (dma->Init.PeriphDataAlignment != DMA_PDATAALIGN_HALFWORD) ||
        (dma->Init.MemDataAlignment != DMA_MDATAALIGN_HALFWORD) ||
        (dma->Init.Mode != DMA_NORMAL))
    {
        Serial_Enviar(
            "ERROR: revisa DMA de TIM3_CH1 en CubeMX.\r\n"
            "DMA1 Stream4 Channel5, Normal, Half Word en ambos lados.\r\n"
            "Memory increment: enabled. Peripheral increment: disabled.\r\n"
        );

        return HAL_ERROR;
    }

    memset(pwm_buffer, 0, sizeof(pwm_buffer));

    uint32_t posicion = RESET_SLOTS;

    for (uint8_t i = 0U; i < NUM_PIXELES; i++)
    {
        /*
         * WS2812B: transmitir GRB, bit mas significativo primero.
         */
        uint32_t grb =
            ((uint32_t)colores[i][1] << 16) |
            ((uint32_t)colores[i][0] << 8)  |
            ((uint32_t)colores[i][2]);

        for (int32_t bit = 23; bit >= 0; bit--)
        {
            pwm_buffer[posicion++] =
                (grb & (1UL << bit)) ? PULSO_UNO : PULSO_CERO;
        }
    }

    dma_terminado = 0U;
    dma_error = 0U;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
    __HAL_TIM_SET_COUNTER(&htim3, 0U);

    /* Cargar el valor cero en el registro activo del canal. */
    htim3.Instance->EGR = TIM_EGR_UG;
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE | TIM_FLAG_CC1);

    NeoPixel_PinPWM();

    HAL_StatusTypeDef resultado = HAL_TIM_PWM_Start_DMA(
        &htim3,
        TIM_CHANNEL_1,
        (uint32_t *)pwm_buffer,
        (uint16_t)BUFFER_PWM_SIZE
    );

    if (resultado != HAL_OK)
    {
        HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
        NeoPixel_PinReposo();

        Serial_Enviar("ERROR: no se pudo iniciar PWM con DMA.\r\n");
        return resultado;
    }

    uint32_t inicio = HAL_GetTick();

    while ((dma_terminado == 0U) && (dma_error == 0U))
    {
        if ((HAL_GetTick() - inicio) >= DMA_TIMEOUT_MS)
        {
            HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
            NeoPixel_PinReposo();

            Serial_Enviar(
                "ERROR: timeout DMA.\r\n"
                "Revisa DMA1_Stream4_IRQHandler y su llamada a HAL_DMA_IRQHandler.\r\n"
            );

            return HAL_TIMEOUT;
        }
    }

    HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
    NeoPixel_PinReposo();

    /* Mantener nivel bajo despues de la transmision. */
    HAL_Delay(1U);

    if (dma_error)
    {
        Serial_Enviar("ERROR: fallo durante la transferencia DMA.\r\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

static void Mostrar_Ayuda(void)
{
    Serial_Enviar(
        "\r\n========================================\r\n"
        "PRUEBA DE 4 NEOPIXELS - NUCLEO F446RE\r\n"
        "Datos: D5 / PB4 / TIM3_CH1\r\n"
        "Escribe UN comando y presiona ENTER.\r\n"
        "----------------------------------------\r\n"
        "0 : todos libres, color verde\r\n"
        "1 : LED 1 rojo; los demas verdes\r\n"
        "2 : LED 2 rojo; los demas verdes\r\n"
        "3 : LED 3 rojo; los demas verdes\r\n"
        "4 : LED 4 rojo; los demas verdes\r\n"
        "r : todos rojos\r\n"
        "g : todos verdes\r\n"
        "b : todos azules, prueba de color\r\n"
        "x : todos apagados\r\n"
        "? : mostrar esta ayuda\r\n"
        "Cada comando reemplaza el estado anterior.\r\n"
        "========================================\r\n"
    );
}

static void Mostrar_Colores(void)
{
    Serial_Enviar("\r\nColores enviados:\r\n");

    for (uint8_t i = 0U; i < NUM_PIXELES; i++)
    {
        const char *descripcion;

        if (colores[i][0] != 0U)
        {
            descripcion = "ROJO - OCUPADO";
        }
        else if (colores[i][1] != 0U)
        {
            descripcion = "VERDE - LIBRE";
        }
        else if (colores[i][2] != 0U)
        {
            descripcion = "AZUL - PRUEBA";
        }
        else
        {
            descripcion = "APAGADO - PRUEBA";
        }

        snprintf(
            mensaje,
            sizeof(mensaje),
            "LED %u: %s\r\n",
            (unsigned int)(i + 1U),
            descripcion
        );

        Serial_Enviar(mensaje);
    }

    Serial_Enviar(
        "Transferencia DMA completada; comprueba los colores fisicamente.\r\n"
    );
}

static void Procesar_Comando(char comando)
{
    if ((comando >= 'A') && (comando <= 'Z'))
    {
        comando = (char)(comando + ('a' - 'A'));
    }

    if (comando == '?')
    {
        Mostrar_Ayuda();
        return;
    }

    if ((comando >= '1') && (comando <= '4'))
    {
        uint8_t seleccionado = (uint8_t)(comando - '1');

        NeoPixel_Todos(0U, BRILLO, 0U);

        colores[seleccionado][0] = BRILLO;
        colores[seleccionado][1] = 0U;
        colores[seleccionado][2] = 0U;
    }
    else
    {
        switch (comando)
        {
            case '0':
            case 'g':
                NeoPixel_Todos(0U, BRILLO, 0U);
                break;

            case 'r':
                NeoPixel_Todos(BRILLO, 0U, 0U);
                break;

            case 'b':
                NeoPixel_Todos(0U, 0U, BRILLO);
                break;

            case 'x':
                NeoPixel_Todos(0U, 0U, 0U);
                break;

            default:
                Serial_Enviar(
                    "\r\nComando invalido. Usa 0-4, r, g, b, x o ?.\r\n"
                );
                return;
        }
    }

    snprintf(
        mensaje,
        sizeof(mensaje),
        "\r\nComando recibido: %c\r\n",
        comando
    );

    Serial_Enviar(mensaje);

    if (NeoPixel_Mostrar() == HAL_OK)
    {
        Mostrar_Colores();
    }
    else
    {
        Serial_Enviar(
            "No se completo el envio. Revisa el error indicado.\r\n"
        );
    }
}

/* USER CODE END 0 */

int main(void)
{
    /* USER CODE BEGIN 1 */
    uint8_t recibido = 0U;
    /* USER CODE END 1 */

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_TIM3_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    NeoPixel_PinReposo();
    HAL_Delay(10U);

    Mostrar_Ayuda();

    Serial_Enviar("\r\nEstado inicial: todos verdes.\r\n");

    NeoPixel_Todos(0U, BRILLO, 0U);

    if (NeoPixel_Mostrar() == HAL_OK)
    {
        Mostrar_Colores();
    }

    Serial_Enviar("\r\nComando> ");
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    while (1)
    {
        HAL_StatusTypeDef estado_rx =
            HAL_UART_Receive(&huart2, &recibido, 1U, 100U);

        if (estado_rx == HAL_OK)
        {
            if ((recibido == '\r') || (recibido == '\n'))
            {
                if (linea_excedida || (longitud_linea > 1U))
                {
                    Serial_Enviar(
                        "\r\nEnvia solo UN comando por linea. Ejemplo: 2\r\n"
                    );
                    Serial_Enviar("\r\nComando> ");
                }
                else if (longitud_linea == 1U)
                {
                    Procesar_Comando(linea[0]);
                    Serial_Enviar("\r\nComando> ");
                }

                longitud_linea = 0U;
                linea_excedida = 0U;
            }
            else if ((recibido == '\b') || (recibido == 127U))
            {
                if (longitud_linea > 0U)
                {
                    longitud_linea--;
                }
            }
            else if ((recibido == ' ') || (recibido == '\t'))
            {
                /* Ignorar espacios alrededor del comando. */
            }
            else if ((recibido >= 32U) && (recibido <= 126U))
            {
                if (longitud_linea < (LINEA_SIZE - 1U))
                {
                    linea[longitud_linea++] = (char)recibido;
                }
                else
                {
                    linea_excedida = 1U;
                }
            }
        }
        else if (estado_rx == HAL_ERROR)
        {
            /*
             * Recuperar una posible sobrecarga de recepcion.
             * Esta prueba recibe un comando a la vez.
             */
            __HAL_UART_CLEAR_OREFLAG(&huart2);
            longitud_linea = 0U;
            linea_excedida = 0U;
        }

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;
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
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

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

    if (HAL_TIM_ConfigClockSource(
            &htim3,
            &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim3,
            &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(
            &htim3,
            &sConfigOC,
            TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN TIM3_Init 2 */
    __HAL_TIM_ENABLE_OCxPRELOAD(&htim3, TIM_CHANNEL_1);
    /* USER CODE END TIM3_Init 2 */

    HAL_TIM_MspPostInit(&htim3);
}

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

static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
}

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

/*
 * La HAL llama a esta funcion cuando termina el DMA del PWM.
 * No transmitir mensajes seriales desde esta interrupcion.
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if ((htim->Instance == TIM3) &&
        (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1))
    {
        dma_terminado = 1U;
    }
}

void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        dma_error = 1U;
    }
}

/* USER CODE END 4 */

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
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    (void)file;
    (void)line;
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
