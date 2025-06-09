/*****************************************************************************
 * @file    timer_handler.h
 * @brief   Header file for Timer2 configuration and handling on STM32F1 series.
 *          Provides function prototypes and variables for repeating CAN frames.
 *****************************************************************************/

#ifndef TIMER_HANDLER_H
#define TIMER_HANDLER_H

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "stm32f1xx.h"
#include <stdint.h>

/*****************************************************************************
 * Macro definitions
 *****************************************************************************/

/**
 * @brief Size of buffer to store repeating CAN frame data.
 */
#define REPEAT_FRAME_SIZE 20

/*****************************************************************************
 * Global variables
 *****************************************************************************/

/**
 * @brief Buffer to store current CAN frame for repeated transmission.
 */
extern volatile uint8_t current_frame[REPEAT_FRAME_SIZE];

/*****************************************************************************
 * Function prototypes
 *****************************************************************************/

/**
 * @brief Initialize Timer 2 with interrupt for periodic CAN frame sending.
 *
 * Configures Timer 2 to generate interrupts at specified intervals for
 * sending repeated CAN frames.
 *
 * @param None
 * @retval None
 */
void Timer2_Config(void);

/**
 * @brief Stop Timer 2 and disable its interrupt.
 *
 * @param None
 * @retval None
 */
void Timer_Stop(void);

/**
 * @brief Setup Timer 2 to periodically send a CAN frame with given parameters.
 *
 * @param mode      CAN mode: 0 = Standard ID, 1 = Extended ID
 * @param id        CAN identifier
 * @param data      Pointer to data bytes
 * @param len       Length of data in bytes (0-8)
 * @param interval  Interval in milliseconds for repeat sending
 *
 * @retval None
 */
void Timer_Setup_Repeat(uint8_t mode, uint32_t id, uint8_t *data, uint8_t len, uint16_t interval);

/**
 * @brief Timer 2 interrupt handler to send repeated CAN frames.
 *
 * This handler is called on Timer 2 interrupt to transmit the CAN frame
 * stored in the current_frame buffer.
 *
 * @param None
 * @retval None
 */
void TIM2_IRQHandler(void);

#endif /* TIMER_HANDLER_H */

/*****************************************************************************
 * End Of File
 *****************************************************************************/
