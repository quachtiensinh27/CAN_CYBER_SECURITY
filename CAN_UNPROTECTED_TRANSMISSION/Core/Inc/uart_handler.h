/*****************************************************************************
 * @file    uart_handler.h
 * @brief   Header file for UART handling on STM32F1 series.
 *          Contains UART buffer declarations, function prototypes, and IRQ handler.
 *****************************************************************************/

#ifndef UART_HANDLER_H
#define UART_HANDLER_H

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "stm32f1xx.h"
#include <stdint.h>

/*****************************************************************************
 * Macro definitions
 *****************************************************************************/

/**
 * @brief Size of UART receive buffer.
 */
#define UART_BUFFER_SIZE 50

/*****************************************************************************
 * Global variables
 *****************************************************************************/

/**
 * @brief UART receive buffer for commands from PC.
 */
extern volatile uint8_t uart_rx_buffer[UART_BUFFER_SIZE];

/**
 * @brief Index of the next byte to write in uart_rx_buffer.
 */
extern volatile uint16_t uart_rx_index;

/*****************************************************************************
 * Function prototypes
 *****************************************************************************/

/**
 * @brief Initialize UART peripheral, configure GPIOs, baud rate, and interrupts.
 */
void UART_Config(void);

/**
 * @brief Initialize UART buffers and index variables.
 */
void UART_Init_Buffers(void);

/**
 * @brief Send one byte via UART.
 *
 * @param b Byte to send.
 */
void UART_SendByte(uint8_t b);

/**
 * @brief Send a null-terminated string via UART.
 *
 * @param s Pointer to the string to send.
 */
void UART_SendString(const char* s);

/**
 * @brief Send one byte as two hexadecimal characters via UART.
 *
 * @param b Byte to send as hexadecimal.
 */
void UART_SendHex(uint8_t b);

/**
 * @brief Process a received UART frame stored in uart_rx_buffer.
 *
 * Parses and handles the command/data received from UART.
 */
void Process_UART_Frame(void);

/*****************************************************************************
 * Interrupt handler
 *****************************************************************************/

/**
 * @brief UART1 interrupt handler.
 *
 * Handles UART RX interrupts and stores incoming bytes into buffer.
 */
void USART1_IRQHandler(void);

#endif /* UART_HANDLER_H */

/*****************************************************************************
 * End Of File
 *****************************************************************************/
