/*****************************************************************************
 * @file    uart.c
 * @brief   UART1 configuration, interrupt handler and frame processing for CAN transmission
 *****************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "uart.h"
#include "can.h"
#include "timer.h"

/******************************************************************************
 * Function: UART_Config
 * Description:
 *   Configures UART1 at 9600 baudrate with 8MHz clock.
 *   Sets PA9 as TX (Alternate function push-pull) and PA10 as RX (Input floating).
 *   Enables UART RX interrupt for receiving data byte-by-byte.
 ******************************************************************************/
void UART_Config(void) {
	RCC->APB2ENR |= (1 << 2)|(1 << 14);  // Enable clocks for GPIOA and USART1

    // Configure PA9 TX (AF push-pull) and PA10 RX (Input floating)
	GPIOA->CRH &= ~((0b11 << 6) | (0b11 << 4))|~((0b11 << 10) | (0b11 << 8)); // Clear settings
	GPIOA->CRH |= (0b10 << 4) | (0b10 << 6); // TX: Alternate function push-pull, max speed 2 MHz
	GPIOA->CRH |= (0b01 << 10);  // RX: Input floating mode

    USART1->BRR = 0x45; // Set baud rate register for 9600 baud at 8 MHz clock
    USART1->CR1 |= (1 << 2) | (1 << 3) | (1 << 13) | (1 << 5); // Enable UART RX, TX, UART peripheral, and RX interrupt
    NVIC_EnableIRQ(USART1_IRQn);  // Enable USART1 interrupt in NVIC
}

/******************************************************************************
 * Function: UART_SendByte
 * Description:
 *   Sends one byte via UART1.
 *   Waits until transmit buffer is empty before sending.
 ******************************************************************************/
void UART_SendByte(uint8_t b) {
	while (!(USART1->SR & (1 << 7)));  // Wait until transmit data register empty
    USART1->DR = b;                        // Load byte into data register to send
}

/******************************************************************************
 * Function: UART_SendHex
 * Description:
 *   Sends a byte as a 2-digit hexadecimal string (e.g., 0x1F -> "1F").
 ******************************************************************************/
void UART_SendHex(uint8_t b) {
    char hex[3];
    const char* digits = "0123456789ABCDEF";
    hex[0] = digits[(b >> 4) & 0x0F];    // Extract high nibble and convert to hex character
    hex[1] = digits[b & 0x0F];           // Extract low nibble and convert to hex character
    hex[2] = '\0';                       // Null-terminate string
    UART_SendString(hex);                // Send the hex string via UART
}

/******************************************************************************
 * Function: UART_SendString
 * Description:
 *   Sends a null-terminated string via UART.
 ******************************************************************************/
void UART_SendString(const char* s) {
    while (*s) {
        UART_SendByte(*s++);             // Send characters one by one until null-terminator
    }
}

/******************************************************************************
 * Function: USART1_IRQHandler
 * Description:
 *   UART1 RX interrupt handler to receive data byte-by-byte.
 *   Stores received bytes in buffer.
 *   Checks for valid frame format:
 *     - mode byte (0 or 1)
 *     - data length (max 7)
 *     - total frame length depending on mode
 *   Sets frame_ready flag when a complete frame is received.
 *   Resets buffer on overflow or invalid data.
 ******************************************************************************/
void USART1_IRQHandler(void) {
	if (USART1->SR & (1 << 5)) {                // Check if RX data register is not empty (data received)
        uint8_t received_byte = USART1->DR;          // Read received byte clears RXNE flag

        // Prevent buffer overflow
        if (uart_rx_index >= sizeof(uart_rx_buffer)) {
            uart_rx_index = 0;                         // Reset buffer index if overflow would occur
            return;
        }

        uart_rx_buffer[uart_rx_index++] = received_byte;  // Store received byte in buffer

        // Check minimal frame length and validity
        if (uart_rx_index >= 6 && !uart_frame_ready) {    // Only process if enough bytes received and frame not ready
            uint8_t mode = uart_rx_buffer[0];             // Get mode byte

            // Validate mode
            if (mode != 0 && mode != 1) {
                uart_rx_index = 0;                         // Invalid mode, reset buffer
                return;
            }

            uint8_t data_len = (mode == 0) ? uart_rx_buffer[3] : uart_rx_buffer[5];  // Get data length depending on mode
            if (data_len > 7) {
                uart_rx_index = 0;                         // Invalid data length, reset buffer
                return;
            }

            uint8_t expected_total_len = (mode == 0) ? (6 + data_len) : (8 + data_len);  // Calculate expected frame length

            if (uart_rx_index >= expected_total_len) {   // Check if full frame received
                uart_frame_ready = 1;                      // Set frame ready flag
            }
        }

        // Reset buffer if it grows too large (timeout simulation)
        if (uart_rx_index > 30) {
            uart_rx_index = 0;                             // Reset buffer to avoid stuck state
        }
    }
}

/******************************************************************************
 * Function: Process_UART_Frame
 * Description:
 *   Processes a complete UART frame.
 *   Decodes mode, ID (standard or extended), data length, payload, and interval.
 *   If interval == 0, sends CAN frame once.
 *   If interval > 0, saves the frame and starts timer for repeated sending.
 *   The last byte of payload is a counter byte that increments with each send.
 ******************************************************************************/
void Process_UART_Frame(void)
{
    uint8_t mode = uart_rx_buffer[0];               // Extract mode byte from UART buffer
    uint32_t id  = 0;                               // Initialize CAN ID
    uint8_t  data_len = (mode == 0) ? uart_rx_buffer[3]
                                    : uart_rx_buffer[5]; // Extract data length

    if (data_len > 7) return;                        // Safety check on data length

    uint16_t interval;
    uint8_t *data_ptr;

    if (mode == 0) {     // Standard ID frame format
        id       = (uart_rx_buffer[1] << 8) | uart_rx_buffer[2];     // Combine two bytes into 11-bit standard ID
        data_ptr = (uint8_t*)&uart_rx_buffer[4];                     // Data payload start index
        interval = (uart_rx_buffer[4 + data_len] << 8) |             // High byte of interval
                    uart_rx_buffer[5 + data_len];                     // Low byte of interval
    } else {             // Extended ID frame format
        id       = (uart_rx_buffer[1] << 24) | (uart_rx_buffer[2] << 16) | // Combine four bytes into 29-bit extended ID
                   (uart_rx_buffer[3] <<  8) |  uart_rx_buffer[4];
        data_ptr = (uint8_t*)&uart_rx_buffer[6];                     // Data payload start index
        interval = (uart_rx_buffer[6 + data_len] << 8) |             // High byte of interval
                    uart_rx_buffer[7 + data_len];                     // Low byte of interval
    }

    uint8_t can_len  = data_len + 1;                  // CAN frame length = data length + 1 counter byte
    uint8_t can_data[8] = {0};                         // Initialize CAN data buffer

    memcpy(can_data, data_ptr, data_len);              // Copy payload data from UART buffer to CAN data buffer
    can_data[data_len] = tx_counter++;                  // Append/increment counter byte at end of payload

    if (interval == 0) {                               // If no repeat interval
        repeat = 0;                                    // Disable repeated sending
        Timer2_Stop();                                 // Stop the timer to prevent repeated sending
        CAN_Send(mode, id, can_data, can_len);        // Send CAN frame once immediately
    } else {                                           // If interval > 0 (repeat enabled)
        repeat = 1;                                    // Enable repeat mode

        // Save the current frame in global buffer for repeated sending in Timer2 interrupt
        current_frame[0] = mode;
        current_frame[1] = (id >> 24) & 0xFF;
        current_frame[2] = (id >> 16) & 0xFF;
        current_frame[3] = (id >>  8) & 0xFF;
        current_frame[4] =  id        & 0xFF;
        current_frame[5] = can_len;
        memcpy((uint8_t*)&current_frame[6], can_data, can_len);  // Copy CAN payload including counter

        Timer2_Start(interval);                         // Start timer with specified interval for repeated sending
    }
}

/******************************************************************************
 * End of File
 ******************************************************************************/
