/*****************************************************************************
 * @file    main.c
 * @brief   Application entry point – sets up peripherals and event loop.
 *****************************************************************************/

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "main.h"
#include "gpio_config.h"
#include "uart_handler.h"
#include "can_handler.h"
#include "timer_handler.h"

/*****************************************************************************
 * Global variables
 *****************************************************************************/


/** Flag set when a CAN frame has been received via interrupt. */
volatile uint8_t can_frame_ready  = 0;

/** Flag indicating whether to repeat sending a CAN frame. */
volatile uint8_t repeat           = 0;

/*****************************************************************************
 * Function definitions
 *****************************************************************************/

/**
 * @brief  Main program entry point.
 *
 * The function performs the following steps:
 * 1. Initializes GPIO, UART, CAN and Timer 2.
 * 2. Clears UART buffers and disables repeat mode.
 * 3. Enters an infinite loop that
 *    - Processes a UART frame when @ref uart_frame_ready is set
 *    - Clears @ref can_frame_ready when a CAN frame was handled in the ISR
 *
 * @note  Additional CAN-frame post-processing can be placed where indicated
 *        in the loop if required.
 *
 * @retval int This function never returns; the value is for ISO-C compliance.
 */
int main(void)
{
    /* ---- Peripheral initialization -------------------------------------- */
    GPIO_Config();          /**< Configure GPIO pins                         */
    UART_Config();          /**< Initialize UART1                            */
    CAN_Config();           /**< Initialize CAN1                             */
    Timer2_Config();        /**< Initialize Timer 2 for repeated CAN frames  */

    UART_Init_Buffers();    /**< Clear UART receive buffers                  */
    repeat = 0;             /**< Disable repeat mode initially               */

    /* ---- Main loop ------------------------------------------------------- */
    while (1)
    {
        /* Handle a complete UART frame */
        if (uart_frame_ready)
        {
            uart_frame_ready = 0;
            Process_UART_Frame();
        }

        /* Optional main-loop processing of a received CAN frame             */
        if (can_frame_ready)
        {
            can_frame_ready = 0;
            /* CAN frame already handled in USB_LP_CAN1_RX0_IRQHandler()    */
        }
    }
}

/*****************************************************************************
 * End of File
 *****************************************************************************/
