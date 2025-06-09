/*****************************************************************************
 * @file    timer.h
 * @brief   Timer configuration and interrupt handling for STM32F1 series.
 *****************************************************************************/

#ifndef TIMER_H
#define TIMER_H

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "main.h"

/*****************************************************************************
 * Macro definitions
 *****************************************************************************/
// (Nếu có macro định nghĩa liên quan timer, đặt ở đây)

/*****************************************************************************
 * Global variables
 *****************************************************************************/
// (Nếu có biến toàn cục dùng cho timer, khai báo extern ở đây)

/*****************************************************************************
 * Function prototypes
 *****************************************************************************/

/**
 * @brief Configure Timer 2 peripheral with basic settings.
 */
void Timer2_Config(void);

/**
 * @brief Start Timer 2 with a specified interval in milliseconds.
 * @param interval_ms Timer interval period in milliseconds.
 */
void Timer2_Start(uint16_t interval_ms);

/**
 * @brief Stop Timer 2.
 */
void Timer2_Stop(void);

/**
 * @brief Timer 2 interrupt handler.
 *        Handles Timer 2 update interrupts.
 */
void TIM2_IRQHandler(void);

#endif /* TIMER_H */

/*****************************************************************************
 * End of File
 *****************************************************************************/
