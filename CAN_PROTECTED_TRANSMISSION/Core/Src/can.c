/*****************************************************************************
 * @file    can.c
 * @brief   CAN peripheral configuration, sending/receiving CAN frames,
 *          interrupt handlers, and CAN frame processing for STM32F1.
 *****************************************************************************/

#include "can.h"
#include "uart.h"

/*****************************************************************************
 * Function prototypes
 *****************************************************************************/
void Process_CAN_Frame(uint32_t id, uint8_t isExtended, uint8_t *data, uint8_t len);

/*****************************************************************************
 * Functions
 *****************************************************************************/

/**
 * @brief Initialize CAN peripheral: clock, bit timing, filters, interrupts.
 */
void CAN_Config(void) {
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;                     // Enable CAN1 peripheral clock

    // Enter initialization mode
    CAN1->MCR |= CAN_MCR_INRQ;                              // Request initialization mode
    while (!(CAN1->MSR & CAN_MSR_INAK));                    // Wait until initialization acknowledged

    // Configure CAN control registers
    CAN1->MCR &= ~(CAN_MCR_SLEEP | CAN_MCR_TTCM | CAN_MCR_AWUM | CAN_MCR_NART);  // Disable sleep, time-triggered, auto wakeup, no auto retransmit
    CAN1->MCR |= CAN_MCR_ABOM;                              // Enable automatic bus-off management

    // Bit timing for 125kbps @ 8MHz: Prescaler=4, SJW=1, BS1=13, BS2=2
    CAN1->BTR = (0 << 24) | (1 << 16) | (0 << 20) | (3 << 0);  // SJW=1, BS2=2, BS1=13, Prescaler=4

    // Configure filter 0 to accept all messages
    CAN1->FMR |= CAN_FMR_FINIT;                             // Enter filter initialization mode
    CAN1->FA1R &= ~(1 << 0);                                // Deactivate filter 0
    CAN1->FS1R |= (1 << 0);                                 // 32-bit scale configuration for filter 0
    CAN1->FM1R &= ~(1 << 0);                                // Mask mode for filter 0 (not list mode)
    CAN1->sFilterRegister[0].FR1 = 0x00000000;              // Filter ID mask register 1 (all bits zero = accept all)
    CAN1->sFilterRegister[0].FR2 = 0x00000000;              // Filter ID mask register 2
    CAN1->FFA1R &= ~(1 << 0);                               // Assign filter 0 to FIFO 0
    CAN1->FA1R |= (1 << 0);                                 // Activate filter 0
    CAN1->FMR &= ~CAN_FMR_FINIT;                            // Leave filter initialization mode

    // Enable CAN interrupts for FIFO0 message pending, error warning/passive, bus-off
    CAN1->IER |= CAN_IER_FMPIE0 | CAN_IER_EWGIE | CAN_IER_EPVIE | CAN_IER_BOFIE;  // Enable interrupts

    NVIC_EnableIRQ(CAN1_RX0_IRQn);                          // Enable CAN RX0 interrupt in NVIC
    NVIC_EnableIRQ(CAN1_TX_IRQn);                           // Enable CAN TX interrupt in NVIC

    // Exit initialization mode
    CAN1->MCR &= ~CAN_MCR_INRQ;                             // Clear initialization request
    while (CAN1->MSR & CAN_MSR_INAK);                       // Wait until normal mode
}

/**
 * @brief Send a CAN frame.
 * @param isExtended 1 if extended ID (29-bit), 0 if standard ID (11-bit).
 * @param id CAN identifier.
 * @param data Pointer to data bytes.
 * @param len Number of data bytes (0-8).
 */
void CAN_Send(uint8_t isExtended, uint32_t id, uint8_t *data, uint8_t len) {
    // Check if CAN bus is off
    if (CAN1->ESR & CAN_ESR_BOFF) {                         // If bus-off state detected
        // UART_SendString("CAN Bus Off!\r\n");
        return;
    }

    // Wait for empty transmit mailbox with timeout
    uint32_t timeout = 10000;
    while (!(CAN1->TSR & CAN_TSR_TME0) && timeout--) ;      // Wait mailbox 0 empty or timeout
    if (timeout == 0) {
        // UART_SendString("CAN TX Timeout!\r\n");
        return;
    }

    // Set up identifier and IDE bit
    if (isExtended) {
        CAN1->sTxMailBox[0].TIR = (id << 3) | CAN_TI0R_IDE;  // Extended ID format, IDE=1
    } else {
        CAN1->sTxMailBox[0].TIR = (id << 21);               // Standard ID format, IDE=0 (bit 2)
    }

    // Set data length and clear data registers
    CAN1->sTxMailBox[0].TDTR = len & 0x0F;                  // DLC (Data Length Code)
    CAN1->sTxMailBox[0].TDLR = 0;                            // Clear low data register
    CAN1->sTxMailBox[0].TDHR = 0;                            // Clear high data register

    // Copy data bytes into mailbox registers
    for (uint8_t i = 0; i < len && i < 8; i++) {
        if (i < 4) {
            CAN1->sTxMailBox[0].TDLR |= ((uint32_t)data[i] << (8 * i));   // Pack first 4 bytes into TDLR
        } else {
            CAN1->sTxMailBox[0].TDHR |= ((uint32_t)data[i] << (8 * (i - 4)));  // Pack bytes 4-7 into TDHR
        }
    }

    // Request transmission
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;                // Set transmit request bit

    // Wait for transmission complete with timeout
    timeout = 10000;
    while (!(CAN1->TSR & (CAN_TSR_RQCP0 | CAN_TSR_TERR0 | CAN_TSR_ALST0)) && timeout--) ;

    // Clear status flags
    CAN1->TSR |= (CAN_TSR_RQCP0 | CAN_TSR_TERR0 | CAN_TSR_ALST0);

    if (timeout == 0) {
        // UART_SendString("CAN Send Failed!\r\n");
    }
}

/**
 * @brief CAN FIFO 0 RX interrupt handler.
 *        Checks for errors, reads received frame, releases FIFO,
 *        and calls Process_CAN_Frame().
 */
void CAN1_RX0_IRQHandler(void) {
    // Clear CAN error flags if any
    if (CAN1->ESR & (CAN_ESR_EWGF | CAN_ESR_EPVF | CAN_ESR_BOFF)) {
        CAN1->ESR &= ~(CAN_ESR_EWGF | CAN_ESR_EPVF | CAN_ESR_BOFF);   // Clear error flags
        // UART_SendString("CAN Error!\r\n");
        return;
    }

    // Return if no message pending in FIFO 0
    if (!(CAN1->RF0R & CAN_RF0R_FMP0)) return;              // No pending message

    // Read ID type, length, and data bytes
    uint32_t id = 0;
    uint8_t isExtended = (CAN1->sFIFOMailBox[0].RIR & CAN_RI0R_IDE) ? 1 : 0;  // IDE bit: 1=extended, 0=standard
    uint8_t len = CAN1->sFIFOMailBox[0].RDTR & 0x0F;         // Data length code (DLC)
    uint8_t data[8];

    if (isExtended)
        id = (CAN1->sFIFOMailBox[0].RIR >> 3);               // Extended ID is bits 3..31
    else
        id = (CAN1->sFIFOMailBox[0].RIR >> 21);              // Standard ID is bits 21..31

    uint32_t rdlr = CAN1->sFIFOMailBox[0].RDLR;             // Low data register
    uint32_t rdhr = CAN1->sFIFOMailBox[0].RDHR;             // High data register

    for (uint8_t i = 0; i < len && i < 8; i++) {
        data[i] = (i < 4) ? (rdlr >> (8 * i)) & 0xFF : (rdhr >> (8 * (i - 4))) & 0xFF;  // Extract data bytes
    }

    // Release FIFO
    CAN1->RF0R |= CAN_RF0R_RFOM0;                            // Release FIFO 0 output mailbox

    // Process received CAN frame
    Process_CAN_Frame(id, isExtended, data, len);
}

/**
 * @brief Process a received CAN frame.
 *        Checks replay attacks via counter byte and sends frame info to UART.
 * @param id CAN identifier.
 * @param isExtended 1 if extended ID, 0 if standard ID.
 * @param data Pointer to data bytes.
 * @param len Length of data bytes.
 */
void Process_CAN_Frame(uint32_t id, uint8_t isExtended, uint8_t *data, uint8_t len)
{
    if (len == 0) return;                      // No counter byte present

    uint8_t counter  = data[len-1];            // Last byte is counter
    uint8_t payload_len = len - 1;             // Payload length (0…7)

    /* ---- Replay attack detection ---- */
    if (id == rx_tracker.id) {
        // If counter difference modulo 256 == 0, frame replayed or reversed
        if ((uint8_t)(counter - rx_tracker.last_counter) == 0) {
            attack_flag = 1;                   // Suspected replay attack
        }
    } else {
        rx_tracker.id = id;                    // New frame, reset tracker
    }
    rx_tracker.last_counter = counter;

    /* ---- Send to PC via UART ---- */
    UART_SendByte(isExtended);                  // Send IDE flag
    if (isExtended) {
        UART_SendByte((id >> 24) & 0xFF);      // Extended ID byte 3 (MSB)
        UART_SendByte((id >> 16) & 0xFF);      // Extended ID byte 2
        UART_SendByte((id >>  8) & 0xFF);      // Extended ID byte 1
        UART_SendByte( id        & 0xFF);      // Extended ID byte 0 (LSB)
    } else {
        UART_SendByte((id >> 8) & 0xFF);       // Standard ID high byte
        UART_SendByte( id       & 0xFF);       // Standard ID low byte
    }

    UART_SendByte(payload_len);                  // Actual payload length (excluding counter)
    for (uint8_t i = 0; i < payload_len; i++) {
        UART_SendByte(data[i]);                  // Send payload bytes
    }

    UART_SendByte(attack_flag);                  // Send attack flag byte

    attack_flag = 0;                             // Reset flag after sending
    can_frame_ready = 1;                         // Indicate new CAN frame ready
}

/**
 * @brief CAN TX interrupt handler.
 *        Clears transmit status flags for all mailboxes.
 */
void CAN1_TX_IRQHandler(void) {
    if (CAN1->TSR & (CAN_TSR_TERR0 | CAN_TSR_ALST0 | CAN_TSR_TXOK0)) {
        CAN1->TSR |= (CAN_TSR_TERR0 | CAN_TSR_ALST0 | CAN_TSR_TXOK0);  // Clear mailbox 0 status flags
    }
    if (CAN1->TSR & (CAN_TSR_TERR1 | CAN_TSR_ALST1 | CAN_TSR_TXOK1)) {
        CAN1->TSR |= (CAN_TSR_TERR1 | CAN_TSR_ALST1 | CAN_TSR_TXOK1);  // Clear mailbox 1 status flags
    }
    if (CAN1->TSR & (CAN_TSR_TERR2 | CAN_TSR_ALST2 | CAN_TSR_TXOK2)) {
        CAN1->TSR |= (CAN_TSR_TERR2 | CAN_TSR_ALST2 | CAN_TSR_TXOK2);  // Clear mailbox 2 status flags
    }
}

/*****************************************************************************
 * End of File
 *****************************************************************************/
