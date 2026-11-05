/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM2 - LDR (P5-P8) + NeoPixels + I2C + Display 7 Segmentos
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

#define PORCENTAJE_OCUPADO        60U
#define PORCENTAJE_LIBRE          80U

/* Mapeo de pines para los segmentos del display */
#define SEG_A_PORT   GPIOA
#define SEG_A_PIN    GPIO_PIN_10   /* D2 -> Patita 7 del display */

#define SEG_B_PORT   GPIOB
#define SEG_B_PIN    GPIO_PIN_3    /* D3 -> Patita 6 del display */

#define SEG_C_PORT   GPIOB
#define SEG_C_PIN    GPIO_PIN_5    /* D4 -> Patita 4 del display */

#define SEG_D_PORT   GPIOB
#define SEG_D_PIN    GPIO_PIN_10   /* D6 -> Patita 2 del display */

#define SEG_E_PORT   GPIOA
#define SEG_E_PIN    GPIO_PIN_8    /* D7 -> Patita 1 del display */

#define SEG_F_PORT   GPIOA
#define SEG_F_PIN    GPIO_PIN_9    /* D8 -> Patita 9 del display */

#define SEG_G_PORT   GPIOC
#define SEG_G_PIN    GPIO_PIN_7    /* D9 -> Patita 10 del display */

#define SEG_DP_PORT  GPIOB
#define SEG_DP_PIN   GPIO_PIN_6    /* D10 -> Patita 5 del display (opcional) */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim3_ch1_trig;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
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

static uint8_t estado_local = 0U;
static char mensaje[192];

/* NeoPixels WS2812B */
static uint16_t pwm_buffer[736] __attribute__((aligned(4)));
static volatile uint8_t dma_terminado = 0U;
static volatile uint8_t dma_error = 0U;
static uint8_t neo_habilitado = 0U;
static uint8_t lectura_valida[NUM_LDR] = {0};

/* I2C esclavo */
static volatile uint8_t i2c_local = 0xFFU;
static uint8_t i2c_tx = 0U;
static uint8_t i2c_rx = 0U;
static volatile uint8_t global_recibido = 0U;
static volatile uint8_t global_disponible = 0U;
static volatile uint32_t global_tick = 0U;
static volatile uint32_t i2c_lecturas = 0U;
static volatile uint32_t i2c_escrituras = 0U;
static volatile uint32_t i2c_errores = 0U;
static volatile uint32_t i2c_ultimo_error = 0U;
static volatile uint8_t i2c_recuperar = 0U;
static uint32_t i2c_ultimo_reintento = 0U;

static uint8_t espacios_libres_global = 0U;
static uint8_t global_vigente = 0U;

/* Matriz de verdad para catodo comun (1 = segmento encendido)
 * Columnas: { a, b, c, d, e, f, g }
 */
static const uint8_t digitos_7seg[10][7] = {
    {1, 1, 1, 1, 1, 1, 0}, /* 0 */
    {0, 1, 1, 0, 0, 0, 0}, /* 1 */
    {1, 1, 0, 1, 1, 0, 1}, /* 2 */
    {1, 1, 1, 1, 0, 0, 1}, /* 3 */
    {0, 1, 1, 0, 0, 1, 1}, /* 4 */
    {1, 0, 1, 1, 0, 1, 1}, /* 5 */
    {1, 0, 1, 1, 1, 1, 1}, /* 6 */
    {1, 1, 1, 0, 0, 0, 0}, /* 7 */
    {1, 1, 1, 1, 1, 1, 1}, /* 8 */
    {1, 1, 1, 1, 0, 1, 1}  /* 9 */
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
static void Serial_Enviar(const char *texto);
static HAL_StatusTypeDef LDR_Leer(uint32_t canal, uint16_t *valor);
static HAL_StatusTypeDef LDR_Calibrar(void);
static void LDR_ActualizarEstados(void);
static void Display_Mostrar(uint8_t valor);
static void Display_TestInicial(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Serial_Enviar(const char *texto)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)texto, (uint16_t)strlen(texto), 200U);
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

    Serial_Enviar("\r\nCALIBRACION STM2\r\n"
                  "Deja descubiertos los LDR de P5, P6, P7 y P8.\r\n"
                  "La medicion comienza en 3 segundos.\r\n");
    HAL_Delay(ESPERA_CALIBRACION_MS);
    Serial_Enviar("Midiendo... manten descubiertos los LDR.\r\n");

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
        referencia[i] = (uint16_t)(suma[i] / MUESTRAS_CALIBRACION);
        umbral_ocupado[i] = (uint16_t)(((uint32_t)referencia[i] * PORCENTAJE_OCUPADO) / 100U);
        umbral_libre[i] = (uint16_t)(((uint32_t)referencia[i] * PORCENTAJE_LIBRE) / 100U);
        ocupado[i] = 0U;

        snprintf(mensaje, sizeof(mensaje),
                 "P%u | Referencia=%u | Ocupado<=%u | Libre>=%u\r\n",
                 (unsigned int)(PRIMER_PARQUEO + i),
                 (unsigned int)referencia[i],
                 (unsigned int)umbral_ocupado[i],
                 (unsigned int)umbral_libre[i]);
        Serial_Enviar(mensaje);

        if ((umbral_ocupado[i] == 0U) || (umbral_libre[i] <= umbral_ocupado[i]))
        {
            referencia_invalida = 1U;
        }
    }

    if (referencia_invalida)
    {
        Serial_Enviar("\r\nReferencia insuficiente. Revisa luz y reinicia.\r\n");
        return HAL_ERROR;
    }

    estado_local = 0U;
    Serial_Enviar("\r\nCalibracion terminada correctamente.\r\n\r\n");
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
        estado_local |= (uint8_t)(ocupado[i] << (3U - i));
    }
}

static void NeoPixel_Preparar(uint8_t mode)
{
    memset(pwm_buffer, 0, sizeof(pwm_buffer));
    uint32_t posicion = 320U;
    for (uint8_t i = 0U; i < NUM_LDR; i++)
    {
        uint32_t grb = 0U;
        if ((mode == 2U) || ((mode == 1U) && !lectura_valida[i]))
        {
            grb = 32U;
        }
        else if (mode == 1U)
        {
            grb = ocupado[i] ? (32UL << 8) : (32UL << 16);
        }

        for (int32_t bit = 23; bit >= 0; bit--)
        {
            pwm_buffer[posicion++] = (grb & (1UL << bit)) ? 13U : 6U;
        }
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
        Serial_Enviar("ERROR NeoPixel/DMA: envio suspendido hasta RESET.\r\n");
    }
    return resultado;
}

static void I2C_Prioridades(void)
{
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0U, 0U);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0U, 0U);
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 1U, 0U);
    HAL_NVIC_SetPriority(SysTick_IRQn, 15U, 0U);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

static void I2C_Escuchar(void)
{
    if (HAL_I2C_EnableListen_IT(&hi2c1) != HAL_OK)
    {
        i2c_recuperar = 1U;
    }
}

static void I2C_Servicio(void)
{
    if (!i2c_recuperar) return;
    uint32_t ahora = HAL_GetTick();
    if ((ahora - i2c_ultimo_reintento) < 250U) return;
    i2c_ultimo_reintento = ahora;
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
    i2c_recuperar = 0U;
    HAL_StatusTypeDef resultado = HAL_I2C_DeInit(&hi2c1);
    if (resultado == HAL_OK) resultado = HAL_I2C_Init(&hi2c1);
    HAL_NVIC_ClearPendingIRQ(I2C1_EV_IRQn);
    HAL_NVIC_ClearPendingIRQ(I2C1_ER_IRQn);
    I2C_Prioridades();
    if (resultado == HAL_OK) I2C_Escuchar();
    else i2c_recuperar = 1U;
}

static uint8_t ContarLibres(uint8_t dato)
{
    uint8_t libres = 8U;
    for (uint8_t i = 0U; i < 8U; i++)
    {
        libres -= (dato >> i) & 1U;
    }
    return libres;
}

/* Control del display de 7 segmentos */
static void Display_Mostrar(uint8_t valor)
{
    if (valor <= 8U)
    {
        HAL_GPIO_WritePin(SEG_A_PORT, SEG_A_PIN, digitos_7seg[valor][0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_B_PORT, SEG_B_PIN, digitos_7seg[valor][1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_C_PORT, SEG_C_PIN, digitos_7seg[valor][2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_D_PORT, SEG_D_PIN, digitos_7seg[valor][3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_E_PORT, SEG_E_PIN, digitos_7seg[valor][4] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_F_PORT, SEG_F_PIN, digitos_7seg[valor][5] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_G_PORT, SEG_G_PIN, digitos_7seg[valor][6] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_DP_PORT, SEG_DP_PIN, GPIO_PIN_RESET);
    }
    else
    {
        /* Muestra un guion central (-) cuando no hay dato valido o vigente */
        HAL_GPIO_WritePin(SEG_A_PORT, SEG_A_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_B_PORT, SEG_B_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_C_PORT, SEG_C_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_D_PORT, SEG_D_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_E_PORT, SEG_E_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_F_PORT, SEG_F_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SEG_G_PORT, SEG_G_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(SEG_DP_PORT, SEG_DP_PIN, GPIO_PIN_RESET);
    }
}

/* Secuencia inicial de prueba: enciende todos los segmentos brevemente */
static void Display_TestInicial(void)
{
    Display_Mostrar(8U);
    HAL_Delay(700U);
    Display_Mostrar(0xFFU);
    HAL_Delay(300U);
}

static void I2C_Reporte(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint8_t dato = global_recibido;
    uint8_t disponible = global_disponible;
    uint32_t fecha = global_tick;
    uint32_t lecturas = i2c_lecturas;
    uint32_t escrituras = i2c_escrituras;
    uint32_t errores = i2c_errores;
    uint32_t codigo = i2c_ultimo_error;
    __set_PRIMASK(primask);

    uint32_t edad = HAL_GetTick() - fecha;
    global_vigente = disponible && (edad <= 2000U);

    if (global_vigente)
    {
        espacios_libres_global = ContarLibres(dato);
        char bits[9];
        for (uint8_t i = 0U; i < 8U; i++)
        {
            bits[i] = ((dato >> (7U - i)) & 1U) ? '1' : '0';
        }
        bits[8] = '\0';
        snprintf(mensaje, sizeof(mensaje),
                 "ESP32 -> GLOBAL P1-P8: %s | Libres: %u | Edad: %lu ms\r\n",
                 bits, (unsigned)espacios_libres_global, (unsigned long)edad);
        Serial_Enviar(mensaje);
    }
    else
    {
        Serial_Enviar(disponible ?
          "ESP32: dato global VENCIDO. Libres no disponibles; control local activo.\r\n" :
          "ESP32: sin estado global recibido. Control local activo.\r\n");
    }

    snprintf(mensaje, sizeof(mensaje),
             "I2C: lecturas ESP=%lu | globales RX=%lu | errores=%lu | ultimo=0x%08lX\r\n",
             (unsigned long)lecturas, (unsigned long)escrituras,
             (unsigned long)errores, (unsigned long)codigo);
    Serial_Enviar(mensaje);
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
    I2C_Prioridades();
    I2C_Escuchar();

    /* Prueba fisica rapida del display al encender */
    Display_TestInicial();

    Serial_Enviar("\r\nSTM2 - VERSION I2C + LDR + NEOPIXEL + DISPLAY - direccion 0x11\r\n");
    NeoPixel_Pin(0U);
    HAL_Delay(10U);

    neo_habilitado = NeoPixel_VerificarDMA();
    if (!neo_habilitado)
    {
        Serial_Enviar("ERROR configuracion DMA: Stream4/Channel5, Normal, Half Word.\r\n");
    }
    (void)NeoPixel_Enviar(0U);

    if (LDR_Calibrar() == HAL_OK)
    {
        calibrado = 1U;
    }
    else
    {
        Serial_Enviar("Calibracion fallida: revisar y reiniciar con LDR descubiertos.\r\n");
    }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      I2C_Servicio();
      uint8_t todas_validas = 1U;

      for (uint8_t i = 0U; i < NUM_LDR; i++)
      {
          lectura_valida[i] = (LDR_Leer(canales[i], &lectura[i]) == HAL_OK);
          if (!lectura_valida[i]) todas_validas = 0U;
      }

      if (calibrado)
      {
          LDR_ActualizarEstados();
          i2c_local = todas_validas ? estado_local : 0xFFU;
          (void)NeoPixel_Enviar(1U);
      }
      else
      {
          i2c_local = 0xFFU;
          (void)NeoPixel_Enviar(2U);
      }

      for (uint8_t i = 0U; i < NUM_LDR; i++)
      {
          if (!lectura_valida[i])
          {
              snprintf(mensaje, sizeof(mensaje), "P%u | ERROR ADC | AZUL\r\n",
                       (unsigned)(PRIMER_PARQUEO + i));
          }
          else
          {
              snprintf(mensaje, sizeof(mensaje), "P%u | ADC=%4u | %s\r\n",
                       (unsigned)(PRIMER_PARQUEO + i), (unsigned)lectura[i],
                       !calibrado ? "SIN CALIBRAR - AZUL" :
                       ocupado[i] ? "OCUPADO - ROJO" : "LIBRE - VERDE");
          }
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
      {
          Serial_Enviar("Estado local NO VALIDO para comunicar ocupacion.\r\n");
      }

      I2C_Reporte();

      /* Actualiza el display segun la cantidad de espacios libres global */
      if (global_vigente)
      {
          Display_Mostrar(espacios_libres_global);
      }
      else
      {
          Display_Mostrar(0xFFU);
      }

      uint32_t pausa = HAL_GetTick();
      while ((HAL_GetTick() - pausa) < PERIODO_REPORTE_MS)
      {
          I2C_Servicio();
          HAL_Delay(1U);
      }
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
  __HAL_RCC_ADC1_CLK_ENABLE();
  ADC_ChannelConfTypeDef sConfig = {0};

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

  GPIO_InitTypeDef gpio_ldr = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  gpio_ldr.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
  gpio_ldr.Mode = GPIO_MODE_ANALOG;
  gpio_ldr.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &gpio_ldr);
  gpio_ldr.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOB, &gpio_ldr);
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 34;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  hi2c1.Init.OwnAddress1 = (0x11U << 1);
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

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

  __HAL_TIM_ENABLE_OCxPRELOAD(&htim3, TIM_CHANNEL_1);
  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, LD2_Pin|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /* Pines configurados para los segmentos del display */
  GPIO_InitStruct.Pin = LD2_Pin|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t direccion, uint16_t direccion_detectada)
{
    (void)direccion_detectada;
    if (hi2c->Instance != I2C1) return;
    HAL_StatusTypeDef resultado;
    if (direccion == I2C_DIRECTION_TRANSMIT)
    {
        resultado = HAL_I2C_Slave_Seq_Receive_IT(hi2c, &i2c_rx, 1U, I2C_FIRST_AND_LAST_FRAME);
    }
    else
    {
        i2c_tx = i2c_local;
        resultado = HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &i2c_tx, 1U, I2C_FIRST_AND_LAST_FRAME);
    }
    if (resultado != HAL_OK) i2c_recuperar = 1U;
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1) i2c_lecturas++;
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C1) return;
    global_recibido = i2c_rx;
    global_tick = HAL_GetTick();
    global_disponible = 1U;
    i2c_escrituras++;
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C1) return;
    if (HAL_I2C_EnableListen_IT(hi2c) != HAL_OK)
    {
        i2c_recuperar = 1U;
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C1) return;
    uint32_t error = HAL_I2C_GetError(hi2c);
    if (error != HAL_I2C_ERROR_AF)
    {
        i2c_errores++;
        i2c_ultimo_error = error;
    }
    if (HAL_I2C_GetState(hi2c) == HAL_I2C_STATE_READY)
    {
        if (HAL_I2C_EnableListen_IT(hi2c) != HAL_OK)
            i2c_recuperar = 1U;
    }
    else if ((error != HAL_I2C_ERROR_AF) || (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_LISTEN))
    {
        i2c_recuperar = 1U;
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if ((htim->Instance == TIM3) && (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1))
    {
        dma_terminado = 1U;
    }
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    (void)file;
    (void)line;
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
