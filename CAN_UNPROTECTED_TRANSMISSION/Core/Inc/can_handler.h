/*****************************************************************************
 * @file    can_handler.h
 * @brief   Header file for CAN peripheral handling on STM32F1 series.
 *          Contains CAN buffer declarations, function prototypes, and IRQ handlers.
 *****************************************************************************/

#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "stm32f1xx.h"
#include <stdint.h>

/*****************************************************************************
 * Macro definitions
 *****************************************************************************/

/**
 * @brief Size of CAN RX buffer.
 */
#define CAN_BUFFER_SIZE 20

/*****************************************************************************
 * Global variables
 *****************************************************************************/

/**
 * @brief CAN receive buffer for storing incoming CAN messages.
 */
extern volatile uint8_t can_rx_buffer[CAN_BUFFER_SIZE];

/*****************************************************************************
 * Function prototypes
 *****************************************************************************/

/**
 * @brief Configure CAN peripheral, GPIOs, filters, and interrupts.
 *
 * Initializes CAN1 peripheral with proper settings for baud rate, filters,
 * interrupt enabling, and GPIO pin configurations.
 */
void CAN_Config(void);

/**
 * @brief Send a CAN message.
 *
 * @param isExtended  CAN ID type: 0 for standard 11-bit ID, 1 for extended 29-bit ID.
 * @param id          CAN identifier.
 * @param data        Pointer to data bytes (up to 8 bytes).
 * @param len         Length of data in bytes (0 to 8).
 */
void CAN_Send(uint8_t isExtended, uint32_t id, uint8_t *data, uint8_t len);

/**
 * @brief Process a received CAN frame.
 *
 * Handles the received CAN frame, e.g., parsing or storing data.
 *
 * @param id          CAN identifier.
 * @param isExtended  Frame format: 0 = standard, 1 = extended.
 * @param data        Pointer to received data bytes.
 * @param len         Length of data in bytes.
 */
void Process_CAN_Frame(uint32_t id, uint8_t isExtended, uint8_t *data, uint8_t len);

/*****************************************************************************
 * Interrupt handlers
 *****************************************************************************/

/**
 * @brief CAN RX0 interrupt handler.
 *
 * This ISR is called when a new CAN message arrives in FIFO0.
 * Processes incoming CAN messages.
 */
void USB_LP_CAN1_RX0_IRQHandler(void);

/**
 * @brief CAN TX interrupt handler.
 *
 * Handles CAN transmit events such as successful transmission or errors.
 */
void USB_HP_CAN1_TX_IRQHandler(void);

#endif /* CAN_HANDLER_H */

/*****************************************************************************
 * End Of File
 *****************************************************************************/
