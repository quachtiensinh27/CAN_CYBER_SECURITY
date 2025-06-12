/*****************************************************************************
 * @file    uart_handler.c
 * @brief   UART handler for receiving CAN control frames from PC and
 *          triggering CAN transmission.
 *****************************************************************************/

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "uart_handler.h"   // Header file for UART handling declarations
#include "can_handler.h"    // Header file for CAN communication functions
#include "timer_handler.h"  // Header file for timer functions used for repeated sending
#include "main.h"           // Main project header (e.g. global defines, variables)

/*****************************************************************************
 * Global variables
 *****************************************************************************/

/**
 * @brief UART receive buffer for commands from PC.
 */
volatile uint8_t uart_rx_buffer[UART_BUFFER_SIZE];  // Buffer to store incoming UART bytes

/**
 * @brief Current index in UART receive buffer.
 */
volatile uint16_t uart_rx_index = 0;  // Index to track current position in receive buffer

/**
 * @brief Flag indicating a complete UART frame has been received.
 */
volatile uint8_t uart_frame_ready = 0;  // Flag set to 1 when a full frame is ready to process

/*****************************************************************************
 * Function: UART_Config
 *****************************************************************************/

/**
 * @brief Configure UART1 for 9600 baud rate at 8 MHz clock.
 *        - PA9: TX (AF Push-Pull, 2 MHz)
 *        - PA10: RX (Input floating)
 *        - Enable USART1 and RX interrupt
 */
void UART_Config(void) {
	RCC->APB2ENR |= (1 << 2)|(1 << 14);
    // Enable clock for GPIOA port and USART1 peripheral

    // Configure PA9 as TX (Alternate Function Push-Pull, max speed 2 MHz)
	GPIOA->CRH &= ~((0xF) << 4); // Clear previous config bits for PA9
	GPIOA->CRH |= ((0x2 << 2) | (0x2)); // MODE9=10 (2 MHz output), CNF9=10 (AF Push-Pull)

    // Configure PA10 as RX (Input floating)
	GPIOA->CRH &= ~((0xF) << 8); // Clear previous config bits for PA10
	GPIOA->CRH |= (0x1 << 10);                    // CNF10=01 (Floating input), MODE10=00 (Input mode)

    USART1->BRR = 0x45;  // Set baud rate register for 9600 baud at 8 MHz clock (calculated value)
    USART1->CR1 |= (1 << 13)|(1 << 2)|(1 << 3)|(1 << 5);
    // Enable Receiver, Transmitter, USART, and RX interrupt enable

    NVIC_EnableIRQ(USART1_IRQn);  // Enable USART1 interrupt in the NVIC (Nested Vector Interrupt Controller)
}

/*****************************************************************************
 * Function: UART_Init_Buffers
 *****************************************************************************/

/**
 * @brief Reset UART receive buffer and frame flag.
 *
 * This function clears the receive buffer index and frame ready flag to
 * prepare for receiving a new UART frame.
 */
void UART_Init_Buffers(void) {
    uart_rx_index = 0;     // Reset index to start filling buffer from beginning
    uart_frame_ready = 0;  // Clear flag indicating a frame is ready for processing
}

/*****************************************************************************
 * Function: UART_SendByte
 *****************************************************************************/

/**
 * @brief Send a single byte over UART1.
 * @param b Byte to transmit.
 *
 * Waits until the transmit data register is empty before sending the byte.
 */
void UART_SendByte(uint8_t b) {
	while (!(USART1->SR & (1 << 7)));  // Wait until TX register is empty (ready to transmit)
    USART1->DR = b;                        // Write byte to data register to send
}

/*****************************************************************************
 * Function: UART_SendHex
 *****************************************************************************/

/**
 * @brief Send a byte as a hexadecimal string (2 characters).
 * @param b Byte to send.
 *
 * Converts the byte to a two-character hexadecimal string and sends it
 * over UART.
 */
void UART_SendHex(uint8_t b) {
    char hex[3];                    // Buffer to hold 2 hex chars + null terminator
    const char* digits = "0123456789ABCDEF";  // Lookup table for hex digits

    hex[0] = digits[(b >> 4) & 0x0F];  // Extract high nibble and convert to hex char
    hex[1] = digits[b & 0x0F];         // Extract low nibble and convert to hex char
    hex[2] = '\0';                     // Null-terminate string

    UART_SendString(hex);              // Send hex string over UART
}

/*****************************************************************************
 * Function: UART_SendString
 *****************************************************************************/

/**
 * @brief Send a null-terminated string via UART.
 * @param s Pointer to the string.
 *
 * Transmits each character in the string until the null terminator is reached.
 */
void UART_SendString(const char* s) {
    while (*s) {                      // Loop until null terminator
        UART_SendByte(*s++);          // Send current character, then increment pointer
    }
}

/*****************************************************************************
 * Function: USART1_IRQHandler
 *****************************************************************************/

/**
 * @brief UART1 RX interrupt handler.
 *        - Stores incoming bytes into uart_rx_buffer
 *        - Detects valid frames by checking mode, length and frame size
 *        - Sets uart_frame_ready flag when complete frame received
 *
 * This ISR reads received bytes, accumulates them in a buffer, validates
 * frame format based on mode and length, and signals when a full frame
 * has been received for processing.
 */
void USART1_IRQHandler(void) {
	if (USART1->SR & (1 << 5)) {          // Check if RX data register not empty (byte received)
        uint8_t received_byte = USART1->DR;   // Read received byte (also clears RXNE flag)

        if (uart_rx_index >= sizeof(uart_rx_buffer)) {  // Prevent buffer overflow
            uart_rx_index = 0;               // Reset buffer index if overflow happens
            return;                         // Exit ISR early to avoid writing out of bounds
        }

        uart_rx_buffer[uart_rx_index++] = received_byte;  // Store received byte in buffer and increment index

        if (uart_rx_index >= 6 && !uart_frame_ready) {   // Only check frame validity if minimum length reached
            uint8_t mode = uart_rx_buffer[0];            // First byte is mode: 0=standard CAN, 1=extended CAN

            if (mode != 0 && mode != 1) {                 // Validate mode is either 0 or 1
                uart_rx_index = 0;                         // Invalid mode: reset buffer to discard frame
                return;
            }

            uint8_t data_len = (mode == 0) ? uart_rx_buffer[3] : uart_rx_buffer[5];  // Data length at different offsets based on mode
            if (data_len > 8) {                            // Validate data length max 8 bytes (CAN limit)
                uart_rx_index = 0;                         // Invalid length: reset buffer
                return;
            }

            // Calculate expected total frame length including header, data, interval bytes
            uint8_t expected_total_len = (mode == 0) ? (6 + data_len) : (8 + data_len);

            if (uart_rx_index >= expected_total_len) {    // Check if entire frame has been received
                uart_frame_ready = 1;                      // Set flag to signal frame ready for processing
            }
        }

        if (uart_rx_index > 30) {   // If buffer length exceeds protocol max size, reset to avoid errors
            uart_rx_index = 0;
        }
    }
}

/*****************************************************************************
 * Function: Process_UART_Frame
 *****************************************************************************/

/**
 * @brief Parse a complete UART frame and send it as a CAN frame.
 *        - Extract mode, ID, data, length, interval
 *        - If interval = 0, send once
 *        - If interval > 0, send repeatedly using Timer
 *
 * Parses the received UART frame buffer to extract CAN frame parameters
 * and either sends the CAN frame once or sets up periodic retransmission
 * via the timer handler.
 */
void Process_UART_Frame(void) {
    uint8_t mode = uart_rx_buffer[0];              // Read mode byte (0=standard, 1=extended)
    uint32_t id = 0;                               // Variable to hold CAN ID
    uint8_t data_len = (mode == 0) ? uart_rx_buffer[3] : uart_rx_buffer[5];  // Extract data length
    uint16_t interval = 0;                          // Interval between repeated sends (ms)
    uint8_t *data_ptr;                              // Pointer to data bytes in buffer

    if (data_len > 8) data_len = 8;                 // Limit data length to max 8 bytes

    if (mode == 0) {
        // For standard CAN frame:
        id = (uart_rx_buffer[1] << 8) | uart_rx_buffer[2];   // Combine two bytes to form 11-bit ID (stored as 16-bit)
        data_ptr = (uint8_t*)&uart_rx_buffer[4];             // Data bytes start after 4th byte
        interval = (uart_rx_buffer[4 + data_len] << 8) | uart_rx_buffer[5 + data_len];  // Interval is two bytes after data
    } else {
        // For extended CAN frame:
        id = (uart_rx_buffer[1] << 24) | (uart_rx_buffer[2] << 16) |
             (uart_rx_buffer[3] << 8) | uart_rx_buffer[4];    // Combine 4 bytes to form 29-bit ID (stored as 32-bit)
        data_ptr = (uint8_t*)&uart_rx_buffer[6];             // Data bytes start after 6th byte
        interval = (uart_rx_buffer[6 + data_len] << 8) | uart_rx_buffer[7 + data_len];  // Interval two bytes after data
    }

    if (interval == 0) {          // If interval is zero, send CAN frame only once
        repeat = 0;               // Clear repeat flag (global variable from context)
        Timer_Stop();             // Stop timer for repeated sending
        CAN_Send(mode, id, data_ptr, data_len);   // Send CAN frame once
    } else {
        repeat = 1;               // Set repeat flag to enable repeated sending
        Timer_Setup_Repeat(mode, id, data_ptr, data_len, interval);  // Setup timer to send CAN frame repeatedly
    }

    uart_rx_index = 0;            // Reset buffer index to receive next frame
    uart_frame_ready = 0;         // Clear frame ready flag
}

/*****************************************************************************
 * End Of File
 *****************************************************************************/
