/*****************************************************************************
 * @file    main.h
 * @brief   Main global definitions and variable declarations for STM32F1 project.
 *****************************************************************************/

#ifndef MAIN_H
#define MAIN_H

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "stm32f1xx.h"
#include <string.h>

/**
 * @brief Buffer sizes for UART and CAN communication.
 */
#define UART_RX_BUFFER_SIZE 50
#define CAN_RX_BUFFER_SIZE  20
#define CURRENT_FRAME_SIZE  20

/**
 * @brief Structure for tracking CAN message counters.
 */
typedef struct {
    uint32_t id;           /**< CAN message identifier */
    uint8_t  last_counter; /**< Last received counter value */
} CounterTracker;

/**
 * @brief UART receive buffer storing incoming bytes.
 */
extern volatile uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];

/**
 * @brief Current index in the UART receive buffer.
 */
extern volatile uint16_t uart_rx_index;

/**
 * @brief Flag indicating a complete UART frame has been received.
 */
extern volatile uint8_t uart_frame_ready;

/**
 * @brief CAN receive buffer storing incoming CAN message data.
 */
extern volatile uint8_t can_rx_buffer[CAN_RX_BUFFER_SIZE];

/**
 * @brief Flag indicating a complete CAN frame has been received.
 */
extern volatile uint8_t can_frame_ready;

/**
 * @brief Flag indicating if repeating frame transmission is active.
 */
extern volatile uint8_t repeat;

/**
 * @brief Buffer holding the current CAN frame to be repeated.
 */
extern volatile uint8_t current_frame[CURRENT_FRAME_SIZE];

/**
 * @brief Counter for transmitted CAN frames.
 */
extern volatile uint8_t tx_counter;

/**
 * @brief Flag to indicate detection of replay attack.
 */
extern volatile uint8_t attack_flag;

/**
 * @brief Structure instance to track received CAN frame counters.
 */
extern volatile CounterTracker rx_tracker;

#endif /* MAIN_H */

/*****************************************************************************
 * End of File
 *****************************************************************************/
