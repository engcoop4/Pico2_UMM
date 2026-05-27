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
#include "Global.h"
#include "UART.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

// DEFINITION: Allocate the actual memory here
uint32_t UART_BAUD = MODE_SMCLK_9600;

void initUART()
{
    // 1. uart_init RETURNS the actual baud rate achieved.
    // This removes the need for a separate "get" function call.
    UART_BAUD = uart_init(UART_ID, UART_BAUD);

    // 2. Setup GPIO Pins
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // 3. Data Format (8N1)
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // 4. Enable FIFOs (32-byte deep buffers)
    uart_set_fifo_enabled(UART_ID, true);

    // 5. Final check (Optional)
    // You can remove the line: UART_BAUD = uart_get_baudrate(UART_ID);
    // Because step 1 already handled it.
}