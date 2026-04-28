/**
 * @file UART.c
 * @author engcoop#3 and engcoop#4 RW
 * @brief Handles UART setup. Imported from CCS, majority of CCS code unneccessary (register set-ups)
 * @version 1.0.0
 * @date 2025-02-12 (MODIFIED: 2026-04-28)
 * @copyright Copyright (c) 2026
 */

#include <stdbool.h>
#include <stdint.h>
#include "UART.h"
#include "Global.h"

#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define MODE_SMCLK_115200  115200
#define MODE_SMCLK_9600    9600
#define MODE_SMCLK_230400  230400

// current mode
#define UART_MODE MODE_SMCLK_9600 

// Map your MSP430 P3.0/P3.1 pins to RP2350 GPIOs
// (Adjust these numbers to your actual PCB layout)
#define UART_TX_PIN 0 
#define UART_RX_PIN 1
#define UART_ID     uart0

void initUART() {
    // 1. Initialize UART at the requested baud rate
    // This replaces all the UCA0BRW and UCA0MCTLW math
    uart_init(UART_ID, UART_MODE);

    // 2. Setup GPIO Pins (Replaces your initGPIO)
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // 3. Data Format (8N1 is standard, matching your MSP430 setup)
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // 4. Enable FIFOs (The RP2350 has a 32-deep buffer, which is a huge upgrade)
    uart_set_fifo_enabled(UART_ID, true);

    // 5. Interrupts (Replaces UCA0IE |= UCRXIE)
    // You'll need to define a handler function to actually catch the data
    // uart_set_irq_enables(UART_ID, true, false); 
}