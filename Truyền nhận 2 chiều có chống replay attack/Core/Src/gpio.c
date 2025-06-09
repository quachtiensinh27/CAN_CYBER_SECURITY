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
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;

    // Configure PA11 as input floating (CAN_RX)
    GPIOA->CRH &= ~(GPIO_CRH_CNF11 | GPIO_CRH_MODE11);
    GPIOA->CRH |= GPIO_CRH_CNF11_0;

    // Configure PA12 as alternate function push-pull output (CAN_TX)
    GPIOA->CRH &= ~(GPIO_CRH_CNF12 | GPIO_CRH_MODE12);
    GPIOA->CRH |= GPIO_CRH_MODE12_1 | GPIO_CRH_CNF12_1;
}

/******************************************************************************
 * End of File
 ******************************************************************************/
