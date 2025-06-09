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
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;

    // Configure PA11 (CAN_RX) as input floating
    GPIOA->CRH &= ~(GPIO_CRH_CNF11 | GPIO_CRH_MODE11);
    GPIOA->CRH |= GPIO_CRH_CNF11_0;

    // Configure PA12 (CAN_TX) as alternate function push-pull output
    GPIOA->CRH &= ~(GPIO_CRH_CNF12 | GPIO_CRH_MODE12);
    GPIOA->CRH |= GPIO_CRH_MODE12_1 | GPIO_CRH_CNF12_1;
}

/*****************************************************************************
 * End of File
 *****************************************************************************/
