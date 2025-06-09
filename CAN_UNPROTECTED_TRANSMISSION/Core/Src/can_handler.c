/*****************************************************************************
 * @file    can_handler.c
 * @brief   CAN configuration, transmission, reception and processing handler
 *****************************************************************************/

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "can_handler.h"    // Include header defining CAN functions and constants
#include "uart_handler.h"   // Include UART header to use UART sending functions
#include "main.h"           // Include main header with common definitions and global variables

/*****************************************************************************
 * Global variables
 *****************************************************************************/

/**
 * @brief Buffer to store CAN data received from interrupt.
 * @note  Marked volatile because accessed inside ISR.
 */
volatile uint8_t can_rx_buffer[CAN_BUFFER_SIZE]; // Buffer array for storing received CAN data

/*****************************************************************************
 * Function Definitions
 *****************************************************************************/

/**
 * @brief  Initialize CAN1 configuration.
 *
 * Setup:
 * - Bit timing for 500 kbps speed with 8 MHz clock.
 * - Filter 0 accepts all standard and extended IDs.
 * - Enable interrupts for FIFO0, successful transmission, and CAN errors.
 *
 * Bit timing configuration:
 * - Prescaler = 4
 * - BS1 = 1
 * - BS2 = 0
 * - SJW = 1
 *
 * @retval None
 */
void CAN_Config(void) {
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;   // Enable clock for CAN1

    CAN1->MCR |= CAN_MCR_INRQ;             // Request initialization mode
    while (!(CAN1->MSR & CAN_MSR_INAK));  // Wait until CAN enters init mode (INAK flag set)

    CAN1->MCR &= ~(CAN_MCR_SLEEP | CAN_MCR_TTCM | CAN_MCR_AWUM | CAN_MCR_NART);
    // Disable sleep mode, time-triggered communication, auto wake-up, and no automatic retransmission

    CAN1->MCR |= CAN_MCR_ABOM;             // Enable automatic bus-off management

    CAN1->BTR = (0 << 24)                  // SJW = 1 (bits 24-25 = 00)
              | (1 << 16)                  // BS1 = 1 (bits 16-19)
              | (0 << 20)                  // BS2 = 0 (bits 20-22)
              | (3 << 0);                  // Prescaler = 4 (0-based: 3 means divide by 4)

    CAN1->FMR |= CAN_FMR_FINIT;            // Enter filter init mode
    CAN1->FA1R &= ~(1 << 0);               // Deactivate filter 0
    CAN1->FS1R |= (1 << 0);                // Set filter 0 to 32-bit scale
    CAN1->FM1R &= ~(1 << 0);               // Set filter 0 to mask mode
    CAN1->sFilterRegister[0].FR1 = 0x00000000; // Set 32-bit mask = 0 (accept all)
    CAN1->sFilterRegister[0].FR2 = 0x00000000; // Set 32-bit mask = 0 (accept all)
    CAN1->FFA1R &= ~(1 << 0);              // Assign filter 0 to FIFO 0
    CAN1->FA1R |= (1 << 0);                // Activate filter 0
    CAN1->FMR &= ~CAN_FMR_FINIT;           // Exit filter init mode

    CAN1->IER |= CAN_IER_FMPIE0            // Enable FIFO0 message pending interrupt
                 | CAN_IER_EWGIE            // Enable error warning interrupt
                 | CAN_IER_EPVIE            // Enable error passive interrupt
                 | CAN_IER_BOFIE;           // Enable bus-off interrupt

    NVIC_EnableIRQ(CAN1_RX0_IRQn);         // Enable CAN1 RX FIFO0 interrupt in NVIC
    NVIC_EnableIRQ(CAN1_TX_IRQn);          // Enable CAN1 TX interrupt in NVIC

    CAN1->MCR &= ~CAN_MCR_INRQ;            // Exit initialization mode
    while (CAN1->MSR & CAN_MSR_INAK);      // Wait until initialization mode cleared
}

/**
 * @brief  Send one CAN frame using mailbox 0.
 *
 * Supports standard (11-bit) and extended (29-bit) IDs.
 * Waits for mailbox availability or timeout, checks bus-off status.
 *
 * @param[in] isExtended  1 if extended ID, 0 if standard ID
 * @param[in] id          CAN ID (11 or 29 bits)
 * @param[in] data        Pointer to data bytes (up to 8)
 * @param[in] len         Number of data bytes (0-8)
 * @retval None
 */
void CAN_Send(uint8_t isExtended, uint32_t id, uint8_t *data, uint8_t len) {
    if (CAN1->ESR & CAN_ESR_BOFF) {       // If bus is off, cannot send
        return;                           // Exit function
    }

    uint32_t timeout = 10000;             // Timeout counter waiting for free mailbox
    while (!(CAN1->TSR & CAN_TSR_TME0)   // Check if mailbox 0 is free
           && timeout--) ;                // Decrement timeout
    if (timeout == 0) {                   // If timeout expired and mailbox not free
        return;                          // Exit without sending
    }

    if (isExtended) {                     // Extended frame
        CAN1->sTxMailBox[0].TIR = ((id << 3) | CAN_TI0R_IDE); // Set extended ID and IDE bit
    } else {                             // Standard frame
        CAN1->sTxMailBox[0].TIR = (id << 21);                 // Set standard 11-bit ID
    }

    CAN1->sTxMailBox[0].TDTR = len & 0x0F;  // Set data length (DLC) (0-8)

    CAN1->sTxMailBox[0].TDLR = 0;         // Clear low data register (first 4 bytes)
    CAN1->sTxMailBox[0].TDHR = 0;         // Clear high data register (next 4 bytes)

    for (uint8_t i = 0; i < len && i < 8; i++) {  // Copy data bytes into mailbox registers
        if (i < 4) {
            CAN1->sTxMailBox[0].TDLR |= ((uint32_t)data[i] << (8 * i));  // first 4 bytes
        } else {
            CAN1->sTxMailBox[0].TDHR |= ((uint32_t)data[i] << (8 * (i - 4))); // next 4 bytes
        }
    }

    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;    // Request transmission

    timeout = 10000;                         // Timeout waiting for transmit complete or error
    while (!(CAN1->TSR & (CAN_TSR_RQCP0     // Wait for transmit complete flag mailbox 0
                         | CAN_TSR_TERR0    // or transmit error flag
                         | CAN_TSR_ALST0))  // or arbitration lost flag
           && timeout--);                    // Decrement timeout

    CAN1->TSR |= (CAN_TSR_RQCP0 | CAN_TSR_TERR0 | CAN_TSR_ALST0); // Clear status flags
}

/**
 * @brief  Interrupt handler for CAN FIFO 0 receive.
 *
 * - Clears error flags if any.
 * - Reads ID and data from FIFO.
 * - Calls processing function for received frame.
 *
 * @retval None
 */
void USB_LP_CAN1_RX0_IRQHandler(void) {
    if (CAN1->ESR & (CAN_ESR_EWGF | CAN_ESR_EPVF | CAN_ESR_BOFF)) { // Check CAN errors
        CAN1->ESR &= ~(CAN_ESR_EWGF | CAN_ESR_EPVF | CAN_ESR_BOFF);  // Clear error flags
        return;                                                     // Exit if error present
    }

    if (!(CAN1->RF0R & CAN_RF0R_FMP0)) return;   // Exit if FIFO0 empty

    uint32_t id = 0;                              // Variable for CAN ID
    uint8_t isExtended = (CAN1->sFIFOMailBox[0].RIR & CAN_RI0R_IDE) ? 1 : 0; // Check extended ID
    uint8_t len = CAN1->sFIFOMailBox[0].RDTR & 0x0F;  // Read data length (DLC)
    uint8_t data[8];                              // Data array

    if (isExtended)                              // Extended frame
        id = (CAN1->sFIFOMailBox[0].RIR >> 3); // Read 29-bit ID (shift right by 3)
    else
        id = (CAN1->sFIFOMailBox[0].RIR >> 21); // Read 11-bit ID (shift right by 21)

    uint32_t rdlr = CAN1->sFIFOMailBox[0].RDLR; // Read data low register (4 bytes)
    uint32_t rdhr = CAN1->sFIFOMailBox[0].RDHR; // Read data high register (4 bytes)

    for (uint8_t i = 0; i < len && i < 8; i++) { // Extract each data byte
        data[i] = (i < 4) ? (rdlr >> (8 * i)) & 0xFF : (rdhr >> (8 * (i - 4))) & 0xFF;
    }

    CAN1->RF0R |= CAN_RF0R_RFOM0;                // Release FIFO0 (remove read message)

    Process_CAN_Frame(id, isExtended, data, len); // Call function to process received CAN frame
}

/**
 * @brief  Process received CAN frame and send raw data via UART.
 *
 * Data sent over UART format:
 * [isExtended][ID bytes][length][data bytes]
 *
 * Also sets flag to indicate frame is ready.
 *
 * @param[in] id          CAN ID
 * @param[in] isExtended  1 if extended ID, 0 if standard
 * @param[in] data        Pointer to received data
 * @param[in] len         Number of data bytes
 * @retval None
 */
void Process_CAN_Frame(uint32_t id, uint8_t isExtended, uint8_t *data, uint8_t len) {
    UART_SendByte(isExtended);                    // Send byte indicating extended or standard ID

    if (isExtended) {                             // If extended ID (4 bytes)
        UART_SendByte((id >> 24) & 0xFF);        // Send highest byte
        UART_SendByte((id >> 16) & 0xFF);        // Send second byte
        UART_SendByte((id >> 8) & 0xFF);         // Send third byte
        UART_SendByte(id & 0xFF);                 // Send lowest byte
    } else {                                      // If standard ID (2 bytes)
        UART_SendByte((id >> 8) & 0xFF);         // Send high byte
        UART_SendByte(id & 0xFF);                 // Send low byte
    }

    UART_SendByte(len);                           // Send data length (DLC)

    for (uint8_t i = 0; i < len; i++) {          // Send each data byte
        UART_SendByte(data[i]);
    }

    can_frame_ready = 1;                          // Set flag indicating CAN frame ready for processing
}

/*****************************************************************************
 * End of File
 *****************************************************************************/
