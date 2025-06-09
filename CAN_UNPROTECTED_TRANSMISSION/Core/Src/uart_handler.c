/*****************************************************************************
 * @file    uart_handler.c
 * @brief   UART handler for receiving CAN control frames from PC and
 *          triggering CAN transmission.
 *****************************************************************************/

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "uart_handler.h"
#include "can_handler.h"
#include "timer_handler.h"
#include "main.h"

/*****************************************************************************
 * Global variables
 *****************************************************************************/

/**
 * @brief UART receive buffer for commands from PC.
 */
volatile uint8_t uart_rx_buffer[UART_BUFFER_SIZE];

/**
 * @brief Current index in UART receive buffer.
 */
volatile uint16_t uart_rx_index = 0;

/**
 * @brief Flag indicating a complete UART frame has been received.
 */
volatile uint8_t uart_frame_ready = 0;

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
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    // Configure PA9 as TX, PA10 as RX
    GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9 | GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
    GPIOA->CRH |= GPIO_CRH_MODE9_1 | GPIO_CRH_CNF9_1;   // TX: AF push-pull, 2 MHz
    GPIOA->CRH |= GPIO_CRH_CNF10_0;                     // RX: Input floating

    USART1->BRR = 0x45;  // Baud rate 9600 @ 8 MHz
    USART1->CR1 |= USART_CR1_RE | USART_CR1_TE | USART_CR1_UE | USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART1_IRQn);
}

/*****************************************************************************
 * Function: UART_Init_Buffers
 *****************************************************************************/

/**
 * @brief Reset UART receive buffer and frame flag.
 */
void UART_Init_Buffers(void) {
    uart_rx_index = 0;
    uart_frame_ready = 0;
}

/*****************************************************************************
 * Function: UART_SendByte
 *****************************************************************************/

/**
 * @brief Send a single byte over UART1.
 * @param b Byte to transmit.
 */
void UART_SendByte(uint8_t b) {
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = b;
}

/*****************************************************************************
 * Function: UART_SendHex
 *****************************************************************************/

/**
 * @brief Send a byte as a hexadecimal string (2 characters).
 * @param b Byte to send.
 */
void UART_SendHex(uint8_t b) {
    char hex[3];
    const char* digits = "0123456789ABCDEF";
    hex[0] = digits[(b >> 4) & 0x0F];
    hex[1] = digits[b & 0x0F];
    hex[2] = '\0';
    UART_SendString(hex);
}

/*****************************************************************************
 * Function: UART_SendString
 *****************************************************************************/

/**
 * @brief Send a null-terminated string via UART.
 * @param s Pointer to the string.
 */
void UART_SendString(const char* s) {
    while (*s) {
        UART_SendByte(*s++);
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
 */
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {
        uint8_t received_byte = USART1->DR;

        if (uart_rx_index >= sizeof(uart_rx_buffer)) {
            uart_rx_index = 0;
            return;
        }

        uart_rx_buffer[uart_rx_index++] = received_byte;

        if (uart_rx_index >= 6 && !uart_frame_ready) {
            uint8_t mode = uart_rx_buffer[0];

            if (mode != 0 && mode != 1) {
                uart_rx_index = 0;
                return;
            }

            uint8_t data_len = (mode == 0) ? uart_rx_buffer[3] : uart_rx_buffer[5];
            if (data_len > 8) {
                uart_rx_index = 0;
                return;
            }

            uint8_t expected_total_len = (mode == 0) ? (6 + data_len) : (8 + data_len);
            if (uart_rx_index >= expected_total_len) {
                uart_frame_ready = 1;
            }
        }

        if (uart_rx_index > 30) {
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
 */
void Process_UART_Frame(void) {
    uint8_t mode = uart_rx_buffer[0];
    uint32_t id = 0;
    uint8_t data_len = (mode == 0) ? uart_rx_buffer[3] : uart_rx_buffer[5];
    uint16_t interval = 0;
    uint8_t *data_ptr;

    if (data_len > 8) data_len = 8;

    if (mode == 0) {
        id = (uart_rx_buffer[1] << 8) | uart_rx_buffer[2];
        data_ptr = (uint8_t*)&uart_rx_buffer[4];
        interval = (uart_rx_buffer[4 + data_len] << 8) | uart_rx_buffer[5 + data_len];
    } else {
        id = (uart_rx_buffer[1] << 24) | (uart_rx_buffer[2] << 16) |
             (uart_rx_buffer[3] << 8) | uart_rx_buffer[4];
        data_ptr = (uint8_t*)&uart_rx_buffer[6];
        interval = (uart_rx_buffer[6 + data_len] << 8) | uart_rx_buffer[7 + data_len];
    }

    if (interval == 0) {
        repeat = 0;
        Timer_Stop();
        CAN_Send(mode, id, data_ptr, data_len);
    } else {
        repeat = 1;
        Timer_Setup_Repeat(mode, id, data_ptr, data_len, interval);
    }

    uart_rx_index = 0;
    uart_frame_ready = 0;
}

/*****************************************************************************
 * End Of File
 *****************************************************************************/
