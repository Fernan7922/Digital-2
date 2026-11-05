
/* Se procesa la mitad que DMA acaba de completar; DMA escribe la otra. */
static void Promediar_Mitad(uint32_t inicio)
{
  for (uint32_t canal = 0; canal < LDR_COUNT; canal++)
  {
    uint32_t suma = 0;
    for (uint32_t k = 0; k < SAMPLES_PER_HALF; k++)
      suma += adc_dma[inicio + k * LDR_COUNT + canal];
    adc_promedio[canal] = (uint16_t)(suma / SAMPLES_PER_HALF);
  }
  adc_bloques++;
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1) Promediar_Mitad(0U);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1) Promediar_Mitad(DMA_SAMPLES / 2U);
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1) adc_error = 1U;
}

static void Actualizar_Estados(uint32_t ahora)
{
  for (uint32_t i = 0; i < LDR_COUNT; i++)
  {
    uint8_t propuesto = estado[i];
    if (lectura[i] < umbral_ocupado[i]) propuesto = OCUPADO;
    else if (lectura[i] > umbral_libre[i]) propuesto = LIBRE;

    if (propuesto == estado[i])
    {
      candidato[i] = estado[i];
      candidato_desde[i] = ahora;
    }
    else if (propuesto != candidato[i])
    {
      candidato[i] = propuesto;
      candidato_desde[i] = ahora;
    }
    else if ((uint32_t)(ahora - candidato_desde[i]) >= CONFIRM_MS)
    {
      estado[i] = propuesto;
    }
  }
}

static void Reportar(void)
{
  char texto[220];
  const char *nombres[] = {"LIBRE", "OCUPADO", "SIN DEFINIR"};
  if (!datos_validos)
  {
    Serial_Texto("SIN DATOS ADC: revisar DMA circular, MSP e IRQ; "
                 "reiniciar tras corregir.\r\n");
    return;
  }
  for (uint32_t i = 0; i < LDR_COUNT; i++)
  {
    (void)snprintf(texto, sizeof(texto),
                   "P%lu | ADC=%u | %s\r\n",
                   (unsigned long)(i + 5U), (unsigned int)lectura[i],
                   nombres[estado[i]]);
    Serial_Texto(texto);
  }
  uint32_t libres = 0U, ocupados = 0U;
  for (uint32_t i = 0; i < LDR_COUNT; i++)
  {
    libres += (estado[i] == LIBRE);
    ocupados += (estado[i] == OCUPADO);
  }
  (void)snprintf(texto, sizeof(texto),
                 "STM1: libres=%lu ocupados=%lu sin_definir=%lu (de 4)\r\n"
                 "----------------------------------------\r\n",
                 (unsigned long)libres, (unsigned long)ocupados,
                 (unsigned long)(LDR_COUNT - libres - ocupados));
  Serial_Texto(texto);
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
