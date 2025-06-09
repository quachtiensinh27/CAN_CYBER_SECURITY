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
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    // Configure PA9 TX (AF push-pull) and PA10 RX (Input floating)
    GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9 | GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
    GPIOA->CRH |= GPIO_CRH_MODE9_1 | GPIO_CRH_CNF9_1; // TX: AF push-pull, 2 MHz
    GPIOA->CRH |= GPIO_CRH_CNF10_0;                    // RX: Input floating

    USART1->BRR = 0x45; // 9600 baud @ 8MHz
    USART1->CR1 |= USART_CR1_RE | USART_CR1_TE | USART_CR1_UE | USART_CR1_RXNEIE; // Enable UART and RX interrupt
    NVIC_EnableIRQ(USART1_IRQn);
}

/******************************************************************************
 * Function: UART_SendByte
 * Description:
 *   Sends one byte via UART1.
 *   Waits until transmit buffer is empty before sending.
 ******************************************************************************/
void UART_SendByte(uint8_t b) {
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = b;
}

/******************************************************************************
 * Function: UART_SendHex
 * Description:
 *   Sends a byte as a 2-digit hexadecimal string (e.g., 0x1F -> "1F").
 ******************************************************************************/
void UART_SendHex(uint8_t b) {
    char hex[3];
    const char* digits = "0123456789ABCDEF";
    hex[0] = digits[(b >> 4) & 0x0F];
    hex[1] = digits[b & 0x0F];
    hex[2] = '\0';
    UART_SendString(hex);
}

/******************************************************************************
 * Function: UART_SendString
 * Description:
 *   Sends a null-terminated string via UART.
 ******************************************************************************/
void UART_SendString(const char* s) {
    while (*s) {
        UART_SendByte(*s++);
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
    if (USART1->SR & USART_SR_RXNE) {
        uint8_t received_byte = USART1->DR;

        // Prevent buffer overflow
        if (uart_rx_index >= sizeof(uart_rx_buffer)) {
            uart_rx_index = 0;
            return;
        }

        uart_rx_buffer[uart_rx_index++] = received_byte;

        // Check minimal frame length and validity
        if (uart_rx_index >= 6 && !uart_frame_ready) {
            uint8_t mode = uart_rx_buffer[0];

            // Validate mode
            if (mode != 0 && mode != 1) {
                uart_rx_index = 0;
                return;
            }

            uint8_t data_len = (mode == 0) ? uart_rx_buffer[3] : uart_rx_buffer[5];
            if (data_len > 7) {
                uart_rx_index = 0;
                return;
            }

            uint8_t expected_total_len = (mode == 0) ? (6 + data_len) : (8 + data_len);

            if (uart_rx_index >= expected_total_len) {
                uart_frame_ready = 1;
            }
        }

        // Reset buffer if it grows too large (timeout simulation)
        if (uart_rx_index > 30) {
            uart_rx_index = 0;
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
    uint8_t mode = uart_rx_buffer[0];
    uint32_t id  = 0;
    uint8_t  data_len = (mode == 0) ? uart_rx_buffer[3]
                                    : uart_rx_buffer[5];

    if (data_len > 7) return;

    uint16_t interval;
    uint8_t *data_ptr;

    if (mode == 0) {     // Standard ID
        id       = (uart_rx_buffer[1] << 8) | uart_rx_buffer[2];
        data_ptr = (uint8_t*)&uart_rx_buffer[4];
        interval = (uart_rx_buffer[4 + data_len] << 8) |
                    uart_rx_buffer[5 + data_len];
    } else {             // Extended ID
        id       = (uart_rx_buffer[1] << 24) | (uart_rx_buffer[2] << 16) |
                   (uart_rx_buffer[3] <<  8) |  uart_rx_buffer[4];
        data_ptr = (uint8_t*)&uart_rx_buffer[6];
        interval = (uart_rx_buffer[6 + data_len] << 8) |
                    uart_rx_buffer[7 + data_len];
    }

    uint8_t can_len  = data_len + 1;      // Add 1 byte for counter
    uint8_t can_data[8] = {0};

    memcpy(can_data, data_ptr, data_len);
    can_data[data_len] = tx_counter++;    // Update counter byte

    if (interval == 0) {
        repeat = 0;
        Timer2_Stop();
        CAN_Send(mode, id, can_data, can_len);
    } else {
        repeat = 1;

        // Save frame for repeated sending in timer interrupt
        current_frame[0] = mode;
        current_frame[1] = (id >> 24) & 0xFF;
        current_frame[2] = (id >> 16) & 0xFF;
        current_frame[3] = (id >>  8) & 0xFF;
        current_frame[4] =  id        & 0xFF;
        current_frame[5] = can_len;
        memcpy((uint8_t*)&current_frame[6], can_data, can_len);

        Timer2_Start(interval);
    }
}

/******************************************************************************
 * End of File
 ******************************************************************************/
