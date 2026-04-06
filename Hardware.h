#ifndef HARDWARE_H_
#define HARDWARE_H_

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

// --- SPI Configuration ---
// Replaces SMCLK_init and UCB1 configurations
#define SPI_PORT spi1               // determines spi0 or spi1 (must be spi1 for Ethernet)
#define SPI_BAUD_RATE (40 * 1000 * 1000)      //  for testing (pretty fast)

// --- Buttons ---
// make sure to use corresponding GP number, not just pin number (e.g., GPIO 2 is pin 4 on the board)
#define SW1 2
#define SW2 3
#define SW3 4
#define SW4 5

// --- LCD Pin Mapping ---
// Replaces P4_56_mode and P4_7_MODE logic
#define PIN_SCK 10 // #define [name] [associated GPIO]
#define PIN_MOSI 11
#define PIN_MISO 12 // Not used by LCD but reserved for SPI1
#define PIN_CS 9
#define PIN_DC 14  // Replaces P4_7 (UMM board), Logic: 0 for cmd, 1 for data
#define PIN_RST 13 // Replaces P3_7 (UMM board), Logic: 0 for RESETDOWN, and 1 for RESETUP

// --- LCD Logic Macros ---
// Replaces LCD_PIN_HI_DATA, RSUP, etc.
#define LCD_PIN_HI_DATA gpio_put(PIN_DC, 1) // GPIO 10 high (data)
#define LCD_PIN_LOW_CMD gpio_put(PIN_DC, 0) // GPIO 10 low (cmd)
#define RSUP LCD_PIN_HI_DATA
#define RSDOWN LCD_PIN_LOW_CMD

#define RESETUP gpio_put(PIN_RST, 1)   // GPIO 13 high
#define RESETDOWN gpio_put(PIN_RST, 0) // GPIO 13 low

// --- UART Configuration ---
#define UART_ID uart0
#define UART_BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// --- Helper Macros ---
// Replaces pgm_read_byte for compatibility
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

#endif /* HARDWARE_H_ */