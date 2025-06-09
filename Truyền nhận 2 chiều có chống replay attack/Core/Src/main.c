/*****************************************************************************
 * @file    main.c
 * @brief   Main program for CAN-UART bridge and frame processing on STM32F103
 *****************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "main.h"
#include "gpio.h"
#include "uart.h"
#include "can.h"
#include "timer.h"

/******************************************************************************
 * Global variable definitions
 ******************************************************************************/
volatile uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];  // UART receive buffer
volatile uint16_t uart_rx_index = 0;                   // Current index in UART buffer
volatile uint8_t uart_frame_ready = 0;                 // Flag: UART frame received completely
volatile uint8_t can_rx_buffer[CAN_RX_BUFFER_SIZE];    // CAN receive buffer
volatile uint8_t can_frame_ready = 0;                  // Flag: CAN frame received
volatile uint8_t repeat = 0;                            // Flag: whether to repeatedly send CAN frame
volatile uint8_t current_frame[CURRENT_FRAME_SIZE];    // Buffer for repeated CAN frame
volatile uint8_t tx_counter = 0;                        // Transmit frame counter (increments every send)
volatile uint8_t attack_flag = 0;                       // Flag for special condition (e.g., attack detection)
volatile CounterTracker rx_tracker = {0, 0};            // Struct to track CAN receive counters (example)

/******************************************************************************
 * Function: main
 * Description:
 *   Main program entry point.
 *   Initializes all required modules:
 *     - Configures GPIO pins
 *     - Configures UART, CAN, and Timer2 peripherals
 *     - Initializes buffer indices and status flags
 *
 *   Main infinite loop:
 *     - Checks if a full UART frame has been received from PC, processes it, then resets buffer.
 *     - Checks if a new CAN frame is available (set in CAN interrupt), processes accordingly.
 ******************************************************************************/
int main(void) {
    // Initialize all hardware modules
    GPIO_Config();
    UART_Config();
    CAN_Config();
    Timer2_Config();

    // Initialize state variables and buffers
    uart_rx_index = 0;
    uart_frame_ready = 0;
    can_frame_ready = 0;
    repeat = 0;

    // Optional: send a startup message to UART
    // UART_SendString("CAN Bridge Ready\r\n");

    while (1) {
        // If a complete UART frame is ready, process it
        if (uart_frame_ready) {
            uart_frame_ready = 0;   // Clear flag
            Process_UART_Frame();   // Decode and handle UART frame
            uart_rx_index = 0;      // Reset UART buffer index for next frame
        }

        // If a CAN frame is received (flag set in CAN ISR)
        if (can_frame_ready) {
            can_frame_ready = 0;    // Clear flag
            // Frame is already processed inside CAN interrupt handler
            // Optional additional processing can be done here
        }
    }
}

/******************************************************************************
 * End of File
 ******************************************************************************/
