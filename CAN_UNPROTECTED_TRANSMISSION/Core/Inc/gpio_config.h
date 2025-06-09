/**
 *****************************************************************************
 * @file    gpio_config.h
 * @brief   Header file for GPIO configuration on STM32F1 series.
 *          Contains function prototype for GPIO initialization.
 *****************************************************************************
 */

#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "stm32f1xx.h"

/*****************************************************************************
 * Function prototypes
 *****************************************************************************/

/**
 * @brief Configure GPIO pins for CAN and other peripherals.
 *
 * Initializes GPIO pins as needed for CAN communication and other functions.
 *
 * @param None
 * @retval None
 */
void GPIO_Config(void);

#endif /* GPIO_CONFIG_H */

/*****************************************************************************
 * End of File
 *****************************************************************************/
