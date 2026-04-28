/**
 * @file USB.c
 * @author engcoop#4 RW
 * @brief New form of command sending, over USB-C rather than UART (save pins for RS-485).
 * @version 1.0.0
 * @date 2026-04-28
 * @copyright Copyright (c) 2026
 */

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "USB.h"
#include "Global.h"
#include "cmdProcessing.h"

void initUSB() {
    stdio_usb_init();
}

RealTimeVars rt;
void serviceUSB() {
    int ch;

    // Pull every available character from the USB RX FIFO
    while ((ch = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        uint8_t c = (uint8_t)ch;

        // 1. Handle Echo (if your MSP430 logic used it)
        if (rt.Host & CharEchoFlag) {
            putchar(c);
        }

        // 2. Check for "End of Command" (Enter key / Newline)
        if (c == '\r' || c == '\n') {
            rt.HostRxBuff[rt.HostRxBuffPtr] = '\0'; // Null terminate the string
            rt.Host |= CmdAvailFlag;               // Set the flag for ParseRCI
            // Note: We don't reset the pointer here; ParseRCI does that after processing.
        } 
        // 3. Add character to buffer if there's room
        else if (rt.HostRxBuffPtr < HOST_RX_BUFF_SIZE - 1) {
            rt.HostRxBuff[rt.HostRxBuffPtr++] = c;
            rt.Host |= CharAvailableFlag;          // Set the "byte received" flag
        }
    }
}