/*
 * dfu_jump.c
 *
 *  Created on: Nov 13, 2025
 *      Author: Qinh
 */
#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "usb_device.h"
#include "flash.h"
/* DFU Bootloader Jump Variables and Functions */
#define DFU_BOOTLOADER_ADDRESS    0x1FFFC800  /* System Memory for STM32F072 */

void (*SysMemBootJump)(void);
extern PCD_HandleTypeDef hpcd_USB_FS;
extern USBD_HandleTypeDef hUsbDevice;
extern TIM_HandleTypeDef htim17;
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi1;


/**
  * @brief  Jump to DFU bootloader for STM32F072
  * @note   Implementation based on stm32samples official reference
  *         https://github.com/eddyem/stm32samples
  * @retval None (function does not return)
  */
void Jump_To_DFU_Bootloader(void)
{

  volatile uint32_t addr = DFU_BOOTLOADER_ADDRESS;
  HAL_TIM_Base_MspDeInit(&htim17);
  USBD_DeInit(&hUsbDevice);
  HAL_PCD_MspDeInit(&hpcd_USB_FS);
  HAL_UART_DMAStop(&huart1);
  HAL_UART_DeInit(&huart1);
  __HAL_RCC_USART1_CLK_DISABLE();
  HAL_NVIC_DisableIRQ(SPI1_IRQn);
  HAL_SPI_DeInit(&hspi1);
  __HAL_RCC_SPI1_CLK_DISABLE();
  HAL_NVIC_DisableIRQ(EXTI0_1_IRQn);
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 |
		  	  	  GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14);
  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
  // Disable RCC, set it to default (after reset) settings
  //       Internal clock, no PLL, etc.
  HAL_RCC_DeInit();

  // Disable systick timer and reset it to default values
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;

  // Disable all interrupts
//  __disable_irq();

  /* Remap system memory to 0x00000000 (STM32F0 specific feature) */
    SYSCFG->CFGR1 = 0x01;
  /* Get bootloader reset vector (PC) from vector table */
  SysMemBootJump = (void (*)(void)) (*((uint32_t *)(addr + 4)));\
  /* Set main stack pointer from bootloader's vector table */
  __set_MSP(*((uint32_t *)addr));

  /* Jump to bootloader */
  SysMemBootJump();

  /* Should never reach here */
  while (1);
}
