/*****************************************************************************
 * @file    can.h
 * @brief   CAN peripheral interface for STM32F1 series.
 *          Contains function prototypes for CAN initialization, sending,
 *          receiving, and interrupt handlers.
 *****************************************************************************/

#ifndef CAN_H
#define CAN_H

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "main.h"

/*****************************************************************************
 * Macro definitions
 *****************************************************************************/
// (Nếu có các macro định nghĩa liên quan CAN, bạn đặt ở đây)

/*****************************************************************************
 * Global variables
 *****************************************************************************/
// (Nếu có biến toàn cục, buffer CAN, hoặc biến volatile thì khai báo extern ở đây)

/*****************************************************************************
 * Function prototypes
 *****************************************************************************/

/**
 * @brief Initialize CAN peripheral, GPIOs, filters, and interrupts.
 *
 * Configures CAN hardware, sets up filters, and enables relevant interrupts.
 */
void CAN_Config(void);

/**
 * @brief Send a CAN message.
 *
 * @param[in] isExtended  Set to 0 for standard 11-bit ID, 1 for extended 29-bit ID.
 * @param[in] id          CAN identifier.
 * @param[in] data        Pointer to data bytes array (max 8 bytes).
 * @param[in] len         Number of data bytes (0 to 8).
 */
void CAN_Send(uint8_t isExtended, uint32_t id, uint8_t *data, uint8_t len);

/**
 * @brief Process a received CAN frame.
 *
 * Application-specific handler called when a CAN message is received.
 *
 * @param[in] id          CAN identifier.
 * @param[in] isExtended  Set to 0 for standard frame, 1 for extended frame.
 * @param[in] data        Pointer to data bytes received.
 * @param[in] len         Number of data bytes received.
 */
void Process_CAN_Frame(uint32_t id, uint8_t isExtended, uint8_t *data, uint8_t len);

/*****************************************************************************
 * Interrupt handlers
 *****************************************************************************/

/**
 * @brief CAN RX0 interrupt handler.
 *
 * Handles reception of CAN messages in FIFO0.
 */
void USB_LP_CAN1_RX0_IRQHandler(void);

/**
 * @brief CAN TX interrupt handler.
 *
 * Handles CAN transmit complete events.
 */
void USB_HP_CAN1_TX_IRQHandler(void);

#endif /* CAN_H */

/*****************************************************************************
 * End of File
 *****************************************************************************/
