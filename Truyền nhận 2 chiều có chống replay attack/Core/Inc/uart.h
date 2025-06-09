/*****************************************************************************
 * @file    uart.h
 * @brief   UART configuration, communication functions, and interrupt handler
 *          for STM32F1 series.
 *****************************************************************************/

#ifndef UART_H
#define UART_H

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "main.h"

/*****************************************************************************
 * Macro definitions
 *****************************************************************************/
// (Nếu có macro liên quan UART, định nghĩa tại đây)

/*****************************************************************************
 * Global variables
 *****************************************************************************/
// (Khai báo biến extern nếu có dùng trong UART)

/*****************************************************************************
 * Function prototypes
 *****************************************************************************/

/**
 * @brief Initialize UART peripheral with configured baud rate, GPIO, and interrupts.
 */
void UART_Config(void);

/**
 * @brief Send a single byte via UART.
 * @param b Byte to be sent.
 */
void UART_SendByte(uint8_t b);

/**
 * @brief Send a null-terminated string via UART.
 * @param s Pointer to the string to send.
 */
void UART_SendString(const char* s);

/**
 * @brief Send a byte as two hexadecimal characters via UART.
 * @param b Byte to send as hexadecimal.
 */
void UART_SendHex(uint8_t b);

/**
 * @brief Process a received UART frame stored in the receive buffer.
 *        Typically called when a full frame is received.
 */
void Process_UART_Frame(void);

/**
 * @brief UART1 interrupt handler.
 *        Handles UART RX interrupts and stores incoming bytes.
 */
void USART1_IRQHandler(void);

#endif /* UART_H */

/*****************************************************************************
 * End of File
 *****************************************************************************/
