/*****************************************************************************
 * @file    gpio.c
 * @brief   GPIO configuration for CAN1 interface on STM32F103
 *****************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "gpio.h"

/******************************************************************************
 * Function: GPIO_Config
 * Description:
 *   Configure GPIO pins for CAN1 on STM32F103.
 *   - PA11 (CAN_RX) is configured as input floating (to receive CAN data).
 *   - PA12 (CAN_TX) is configured as alternate function push-pull (to transmit CAN data).
 *   - Enable clocks for GPIOA, GPIOC, and AFIO.
 ******************************************************************************/
void GPIO_Config(void) {
    // Enable clocks for GPIOA, GPIOC, and AFIO
	RCC->APB2ENR |= (1 << 2) | (1 << 4) | (1 << 0);

    // Configure PA11 as input floating (CAN_RX)
	GPIOA->CRH &= ~((0b11 << 12) | (0b11 << 14));
	GPIOA->CRH |= (0b01 << 14);

    // Configure PA12 as alternate function push-pull output (CAN_TX)
	GPIOA->CRH &= ~((0b11 << 16) | (0b11 << 18));
	GPIOA->CRH |= (0b10 << 16) | (0b10 << 18);
}

/******************************************************************************
 * End of File
 ******************************************************************************/
