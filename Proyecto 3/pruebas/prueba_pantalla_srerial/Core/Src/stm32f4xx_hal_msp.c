/* MSP for the independent NUCLEO-F446RE TFT test, based on supplied project. */
#include "main.h"
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0);
}
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio={0};
    if(hspi->Instance==SPI2){
        __HAL_RCC_SPI2_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        gpio.Pin=GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
        gpio.Mode=GPIO_MODE_AF_PP;gpio.Pull=GPIO_NOPULL;
        gpio.Speed=GPIO_SPEED_FREQ_VERY_HIGH;gpio.Alternate=GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB,&gpio);
    }
}
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if(hspi->Instance==SPI2){__HAL_RCC_SPI2_CLK_DISABLE();HAL_GPIO_DeInit(GPIOB,GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);}
}
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio={0};
    if(huart->Instance==USART2){
        __HAL_RCC_USART2_CLK_ENABLE();__HAL_RCC_GPIOA_CLK_ENABLE();
        gpio.Pin=USART_TX_Pin|USART_RX_Pin;gpio.Mode=GPIO_MODE_AF_PP;
        gpio.Pull=GPIO_NOPULL;gpio.Speed=GPIO_SPEED_FREQ_VERY_HIGH;gpio.Alternate=GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA,&gpio);
    }
}
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if(huart->Instance==USART2){HAL_NVIC_DisableIRQ(USART2_IRQn);__HAL_RCC_USART2_CLK_DISABLE();HAL_GPIO_DeInit(GPIOA,USART_TX_Pin|USART_RX_Pin);}
}
