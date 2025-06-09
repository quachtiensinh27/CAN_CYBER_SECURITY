/*****************************************************************************
 * @file    timer.c
 * @brief   Timer2 configuration and interrupt handler for repeated CAN frame sending
 *****************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "timer.h"
#include "can.h"

/******************************************************************************
 * Function: Timer2_Config
 * Description:
 *   Configure Timer2 to generate an update event every 1ms based on an 8MHz clock.
 *   Enable update interrupt and enable NVIC interrupt for Timer2.
 ******************************************************************************/
void Timer2_Config(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;   // Enable clock for TIM2
    TIM2->PSC = 1000 - 1;                  // Prescaler: 8 MHz / 1000 = 8 kHz (1 tick = 125 us)
    TIM2->ARR = 8000;                      // Auto-reload register for 1s period (8000 ticks * 125 us)
    TIM2->DIER |= TIM_DIER_UIE;            // Enable update interrupt (UIE)
    NVIC_EnableIRQ(TIM2_IRQn);             // Enable TIM2 interrupt in NVIC
}

/******************************************************************************
 * Function: Timer2_Start
 * Description:
 *   Start Timer2 with interrupt period set by interval_ms parameter.
 *   Timer counts with 1ms ticks configured in Timer2_Config.
 * Params:
 *   interval_ms - Timer duration in milliseconds before update interrupt occurs.
 ******************************************************************************/
void Timer2_Start(uint16_t interval_ms) {
    // Adjust ARR according to 1 tick = 125 us (8 kHz)
    TIM2->ARR = (uint32_t)interval_ms * 8;  // 1 ms = 8 ticks (125 us * 8 = 1 ms)
    TIM2->CNT = 0;                          // Reset counter
    TIM2->SR &= ~TIM_SR_UIF;                // Clear update interrupt flag
    TIM2->DIER |= TIM_DIER_UIE;             // Enable update interrupt
    TIM2->CR1 |= TIM_CR1_CEN;               // Enable timer counter
}

/******************************************************************************
 * Function: Timer2_Stop
 * Description:
 *   Stop Timer2, disable update interrupt, and clear interrupt flag.
 ******************************************************************************/
void Timer2_Stop(void) {
    TIM2->CR1 &= ~TIM_CR1_CEN;              // Disable timer counter
    TIM2->DIER &= ~TIM_DIER_UIE;            // Disable update interrupt
    TIM2->SR &= ~TIM_SR_UIF;                 // Clear update interrupt flag
}

/******************************************************************************
 * Function: TIM2_IRQHandler
 * Description:
 *   Interrupt handler for Timer2 update event.
 *   If global flag 'repeat' is enabled, send the stored CAN frame repeatedly.
 *   Update the last byte in payload as a counter to avoid duplicate frames.
 ******************************************************************************/
void TIM2_IRQHandler(void) {
    if (!(TIM2->SR & TIM_SR_UIF)) return;  // Check update interrupt flag
    TIM2->SR &= ~TIM_SR_UIF;                // Clear interrupt flag

    if (!repeat) return;                    // Exit if repeat mode is off

    uint8_t mode = current_frame[0];
    uint32_t id = (mode == 0) ?
        ((current_frame[3] << 8) | current_frame[4]) :
        ((current_frame[1] << 24) | (current_frame[2] << 16) |
         (current_frame[3] << 8) | current_frame[4]);

    uint8_t len = current_frame[5];
    uint8_t data[8];

    __disable_irq();                        // Disable interrupts for atomic copy
    memcpy(data, &current_frame[6], len);  // Copy payload
    __enable_irq();

    data[len - 1] = tx_counter++;          // Increment last byte as counter
    current_frame[6 + len - 1] = data[len - 1]; // Update frame buffer

    CAN_Send(mode, id, data, len);         // Send CAN frame
}

/******************************************************************************
 * End of File
 ******************************************************************************/
