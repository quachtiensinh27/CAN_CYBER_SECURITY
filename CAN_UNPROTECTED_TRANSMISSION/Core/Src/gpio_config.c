/*****************************************************************************
 * @file    gpio_config.c
 * @brief   GPIO configuration for CAN communication (STM32F103 series)
 *****************************************************************************/

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "gpio_config.h"

/*****************************************************************************
 * @brief  Configures GPIO pins for CAN communication.
 *
 * This function sets up the required GPIO pins to interface with the CAN
 * peripheral:
 * - Enables clocks for GPIOA, GPIOC (if needed), and AFIO.
 * - Configures PA11 as CAN_RX in input floating mode.
 * - Configures PA12 as CAN_TX in alternate function push-pull mode.
 *
 * @note  This configuration is based on STM32F103 series using remap default.
 *        Make sure CAN remapping is disabled unless explicitly required.
 *
 * @param None
 * @retval None
 *****************************************************************************/
void GPIO_Config(void) {
    // Enable clocks for GPIOA, GPIOC (optional), and AFIO
	RCC->APB2ENR |= (1 << 2) | (1 << 4) | (1 << 0);

    // Configure PA11 (CAN_RX) as input floating
    // Clear CNF11[1:0] and MODE11[1:0] bits
	GPIOA->CRH &= ~((0b1111) << 12);
    // Set CNF11 to input floating (01), MODE11 to input (00)
	GPIOA->CRH |= (0b01 << 14);

    // Configure PA12 (CAN_TX) as alternate function push-pull output
    // Clear CNF12[1:0] and MODE12[1:0] bits
	GPIOA->CRH &= ~((0b1111) << 16);
    // Set MODE12 to output mode, max speed 2 MHz (10)
    // Set CNF12 to alternate function push-pull (10)
	GPIOA->CRH |= (0b10 << 16) | (0b10 << 18);
}

/*****************************************************************************
 * End of File
 *****************************************************************************/
