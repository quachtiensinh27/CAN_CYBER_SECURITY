/*****************************************************************************
 * @file    timer_handler.c
 * @brief   Timer 2 configuration and repeat-frame logic for CAN transmission
 *****************************************************************************/

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "timer_handler.h"
#include "can_handler.h"
#include "main.h"

/*****************************************************************************
 * Global variables
 *****************************************************************************/
/**
 * @brief Buffer that holds the CAN frame to be sent repeatedly.
 * @note  Declared volatile because it is accessed from an ISR.
 */
volatile uint8_t current_frame[REPEAT_FRAME_SIZE];

/*****************************************************************************
 * @brief Configure Timer 2.
 *
 * Sets up TIM2 to generate a 1 ms tick (APB1 = 8 MHz, PSC = 999) and a default
 * period of 1 s (ARR = 1000). The update interrupt is enabled so that
 * @ref TIM2_IRQHandler is invoked on overflow.
 *
 * @retval None
 *****************************************************************************/
void Timer2_Config(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;   /* Enable TIM2 clock                */
    TIM2->PSC  = 1000 - 1;                /* 1 ms tick @ 8 MHz                */
    TIM2->ARR  = 1000;                    /* Default period 1000 ms           */
    TIM2->DIER |= TIM_DIER_UIE;           /* Enable update interrupt          */
    NVIC_EnableIRQ(TIM2_IRQn);            /* Enable TIM2 IRQ in NVIC          */
}

/*****************************************************************************
 * @brief Stop Timer 2 and clear its update flag.
 *
 * Disables the counter, the update interrupt, and clears any pending
 * update-event flag.
 *
 * @retval None
 *****************************************************************************/
void Timer_Stop(void)
{
    TIM2->CR1 &= ~TIM_CR1_CEN;   /* Stop counter            */
    TIM2->DIER &= ~TIM_DIER_UIE; /* Disable update interrupt */
    TIM2->SR  &= ~TIM_SR_UIF;    /* Clear update flag        */
}

/*****************************************************************************
 * @brief Store a CAN frame and start periodic retransmission.
 *
 * Copies the specified CAN frame into @ref current_frame and configures TIM2
 * so that it overflows every **interval** ms.  On each overflow, the ISR
 * resends the stored frame.
 *
 * @param mode      CAN mode: 0 = standard (11-bit ID), 1 = extended (29-bit ID)
 * @param id        CAN identifier
 * @param data      Pointer to data bytes
 * @param len       Number of data bytes (0 – 8)
 * @param interval  Repeat interval in milliseconds
 *
 * @retval None
 *****************************************************************************/
void Timer_Setup_Repeat(uint8_t mode,
                        uint32_t id,
                        uint8_t *data,
                        uint8_t len,
                        uint16_t interval)
{
    /* Copy frame header */
    current_frame[0] = mode;
    current_frame[1] = (id >> 24) & 0xFF;
    current_frame[2] = (id >> 16) & 0xFF;
    current_frame[3] = (id >>  8) & 0xFF;
    current_frame[4] =  id        & 0xFF;
    current_frame[5] = len;

    /* Copy payload */
    for (uint8_t i = 0; i < len; ++i) {
        current_frame[6 + i] = data[i];
    }

    /* Configure timer period (tick = 1 ms ⇒ ARR = interval) */
    TIM2->ARR = interval;
    TIM2->CNT = 0;            /* Reset counter                      */
    TIM2->SR  &= ~TIM_SR_UIF; /* Clear pending flag                 */
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1  |= TIM_CR1_CEN;/* Start timer                        */
}

/*****************************************************************************
 * @brief TIM2 update-event interrupt handler.
 *
 * When @c repeat mode is active (global flag in @ref main.c), this ISR sends
 * the stored CAN frame on every timer overflow.
 *
 * @retval None
 *****************************************************************************/
void TIM2_IRQHandler(void)
{
    if (!(TIM2->SR & TIM_SR_UIF))
        return;                       /* Spurious interrupt               */

    TIM2->SR &= ~TIM_SR_UIF;          /* Clear update flag                */
    if (!repeat)
        return;                       /* Repeat mode not enabled          */

    /* Reconstruct CAN parameters from buffer */
    uint8_t  mode = current_frame[0];
    uint32_t id   = (mode == 0)
                  ? ((current_frame[3] << 8) | current_frame[4])          /* 11-bit */
                  : ((current_frame[1] << 24) | (current_frame[2] << 16)  /* 29-bit */
                   | (current_frame[3] <<  8) |  current_frame[4]);

    uint8_t len  = current_frame[5];
    uint8_t data[8];
    for (uint8_t i = 0; i < len && i < 8; ++i) {
        data[i] = current_frame[6 + i];
    }

    CAN_Send(mode, id, data, len);    /* Re-transmit frame                */
}

/*****************************************************************************
 * End of File
 *****************************************************************************/
