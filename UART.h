/**
 * @file UART.h
 * @author engcoop#3 and engcoop#4 RW
 * @brief Header file for UART.c. Imported from CCS.
 * @version 1.0.0
 * @date 2025-02-12 (MODIFIED: 2026-04-28)
 * @copyright Copyright (c) 2026
 */

#ifndef UART_H
#define UART_H

#include "pico/stdlib.h"
#include "hardware/uart.h"

// Baud rate definitions
#define MODE_SMCLK_9600 9600
#define MODE_SMCLK_115200 115200
#define MODE_SMCLK_230400 230400

// Hardware Mapping
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_ID uart0

// Global variable declaration
extern uint32_t UART_BAUD;

// Function Prototypes
void initUART(void);

#endif