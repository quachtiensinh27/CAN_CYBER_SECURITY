/*****************************************************************************
 * @file    can_handler.c
 * @author  [Your Name]
 * @brief   CAN configuration, transmission, reception and processing handler
 *****************************************************************************/

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "can_handler.h"
#include "uart_handler.h"
#include "main.h"

/*****************************************************************************
 * Global variables
 *****************************************************************************/

/**
 * @brief Buffer used for storing received CAN messages
 * @note  Volatile because it is accessed in interrupt context
 */
volatile uint8_t can_rx_buffer[CAN_BUFFER_SIZE];

/*****************************************************************************
 * Function Definitions
 *****************************************************************************/

/**
 * @brief  Initialize the CAN1 peripheral.
 *
 * Configures CAN1 with:
 * - Bit timing for 500 kbps at 8 MHz clock.
 * - Filter 0 to accept all standard and extended IDs.
 * - Enables interrupts for RX FIFO0, transmit complete, and error conditions.
 *
 * Bit timing parameters:
 * - Prescaler = 4
 * - BS1 = 1
 * - BS2 = 0
 * - SJW = 1
 *
 * @retval None
 */
void CAN_Config(void) {
    // Enable CAN1 clock
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;

    // Enter initialization mode
    CAN1->MCR |= CAN_MCR_INRQ;
    while (!(CAN1->MSR & CAN_MSR_INAK)); // Wait for acknowledgment

    // Disable sleep, TTCM, AWUM, NART; enable ABOM
    CAN1->MCR &= ~(CAN_MCR_SLEEP | CAN_MCR_TTCM | CAN_MCR_AWUM | CAN_MCR_NART);
    CAN1->MCR |= CAN_MCR_ABOM;

    // Set bit timing: SJW=1, BS1=1, BS2=0, Prescaler=4
    CAN1->BTR = (0 << 24) | (1 << 16) | (0 << 20) | (3 << 0);

    // Filter configuration
    CAN1->FMR |= CAN_FMR_FINIT;
    CAN1->FA1R &= ~(1 << 0);  // Deactivate filter 0
    CAN1->FS1R |= (1 << 0);   // 32-bit scale
    CAN1->FM1R &= ~(1 << 0);  // Mask mode
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FFA1R &= ~(1 << 0); // Assign to FIFO 0
    CAN1->FA1R |= (1 << 0);   // Activate filter 0
    CAN1->FMR &= ~CAN_FMR_FINIT;

    // Enable CAN interrupts
    CAN1->IER |= CAN_IER_FMPIE0 | CAN_IER_EWGIE | CAN_IER_EPVIE | CAN_IER_BOFIE;

    // Enable NVIC interrupts
    NVIC_EnableIRQ(CAN1_RX0_IRQn);
    NVIC_EnableIRQ(CAN1_TX_IRQn);

    // Exit initialization mode
    CAN1->MCR &= ~CAN_MCR_INRQ;
    while (CAN1->MSR & CAN_MSR_INAK);
}

/**
 * @brief  Transmit a CAN frame using mailbox 0.
 *
 * Supports both standard (11-bit) and extended (29-bit) identifiers.
 * Waits for mailbox availability and checks for bus-off state.
 *
 * @param[in] isExtended Set to 1 for extended ID, 0 for standard ID
 * @param[in] id         CAN ID (11-bit or 29-bit)
 * @param[in] data       Pointer to data buffer (max 8 bytes)
 * @param[in] len        Number of data bytes to send (0–8)
 * @retval None
 */
void CAN_Send(uint8_t isExtended, uint32_t id, uint8_t *data, uint8_t len) {
    if (CAN1->ESR & CAN_ESR_BOFF) {
        return;
    }

    uint32_t timeout = 10000;
    while (!(CAN1->TSR & CAN_TSR_TME0) && timeout--);
    if (timeout == 0) {
        return;
    }

    if (isExtended) {
        CAN1->sTxMailBox[0].TIR = ((id << 3) | CAN_TI0R_IDE);
    } else {
        CAN1->sTxMailBox[0].TIR = (id << 21);
    }

    CAN1->sTxMailBox[0].TDTR = len & 0x0F;
    CAN1->sTxMailBox[0].TDLR = 0;
    CAN1->sTxMailBox[0].TDHR = 0;

    for (uint8_t i = 0; i < len && i < 8; i++) {
        if (i < 4) {
            CAN1->sTxMailBox[0].TDLR |= ((uint32_t)data[i] << (8 * i));
        } else {
            CAN1->sTxMailBox[0].TDHR |= ((uint32_t)data[i] << (8 * (i - 4)));
        }
    }

    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;

    timeout = 10000;
    while (!(CAN1->TSR & (CAN_TSR_RQCP0 | CAN_TSR_TERR0 | CAN_TSR_ALST0)) && timeout--);

    CAN1->TSR |= (CAN_TSR_RQCP0 | CAN_TSR_TERR0 | CAN_TSR_ALST0);
}

/**
 * @brief  Interrupt handler for CAN RX FIFO 0 (CAN1).
 *
 * Handles reception of CAN messages:
 * - Clears error flags
 * - Extracts ID and data
 * - Forwards message to processing function
 *
 * @retval None
 */
void USB_LP_CAN1_RX0_IRQHandler(void) {
    if (CAN1->ESR & (CAN_ESR_EWGF | CAN_ESR_EPVF | CAN_ESR_BOFF)) {
        CAN1->ESR &= ~(CAN_ESR_EWGF | CAN_ESR_EPVF | CAN_ESR_BOFF);
        return;
    }

    if (!(CAN1->RF0R & CAN_RF0R_FMP0)) return;

    uint32_t id = 0;
    uint8_t isExtended = (CAN1->sFIFOMailBox[0].RIR & CAN_RI0R_IDE) ? 1 : 0;
    uint8_t len = CAN1->sFIFOMailBox[0].RDTR & 0x0F;
    uint8_t data[8];

    if (isExtended)
        id = (CAN1->sFIFOMailBox[0].RIR >> 3);
    else
        id = (CAN1->sFIFOMailBox[0].RIR >> 21);

    uint32_t rdlr = CAN1->sFIFOMailBox[0].RDLR;
    uint32_t rdhr = CAN1->sFIFOMailBox[0].RDHR;

    for (uint8_t i = 0; i < len && i < 8; i++) {
        data[i] = (i < 4) ? (rdlr >> (8 * i)) & 0xFF : (rdhr >> (8 * (i - 4))) & 0xFF;
    }

    CAN1->RF0R |= CAN_RF0R_RFOM0;

    Process_CAN_Frame(id, isExtended, data, len);
}

/**
 * @brief  Process received CAN frame and forward via UART.
 *
 * Sends frame as raw bytes over UART in the following format:
 * [isExtended][ID bytes][length][data bytes]
 *
 * Also sets a global flag to indicate a CAN frame has been processed.
 *
 * @param[in] id         CAN ID
 * @param[in] isExtended 1 if extended ID, 0 if standard
 * @param[in] data       Pointer to received data bytes
 * @param[in] len        Number of received data bytes
 * @retval None
 */
void Process_CAN_Frame(uint32_t id, uint8_t isExtended, uint8_t *data, uint8_t len) {
    UART_SendByte(isExtended);

    if (isExtended) {
        UART_SendByte((id >> 24) & 0xFF);
        UART_SendByte((id >> 16) & 0xFF);
        UART_SendByte((id >> 8) & 0xFF);
        UART_SendByte(id & 0xFF);
    } else {
        UART_SendByte((id >> 8) & 0xFF);
        UART_SendByte(id & 0xFF);
    }

    UART_SendByte(len);

    for (uint8_t i = 0; i < len; i++) {
        UART_SendByte(data[i]);
    }

    can_frame_ready = 1;
}

/*****************************************************************************
 * End of File
 *****************************************************************************/
