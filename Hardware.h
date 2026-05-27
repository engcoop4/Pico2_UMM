#ifndef HARDWARE_H_
#define HARDWARE_H_

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

// --- Buttons ---
// make sure to use corresponding GP number, not just pin number (e.g., GPIO 2 is pin 4 on the board)
// need ONE switch set to GPIO 28 (ADC2)
#define SWLADDER 28 // SW1, GPIO 28, ADC2

// --- LCD Pin Mapping ---
#define PIN_CS 13
#define PIN_DC 14  // Replaces P4_7 (UMM board), Logic: 0 for cmd, 1 for data
#define PIN_RST 15 // Replaces P3_7 (UMM board), Logic: 0 for RESETDOWN, and 1 for RESETUP

// --- LCD Logic Macros ---
// Replaces LCD_PIN_HI_DATA, RSUP, etc.
#define LCD_PIN_HI_DATA gpio_put(PIN_DC, 1) // GPIO 10 high (data)
#define LCD_PIN_LOW_CMD gpio_put(PIN_DC, 0) // GPIO 10 low (cmd)
#define RSUP LCD_PIN_HI_DATA
#define RSDOWN LCD_PIN_LOW_CMD

#define LED1 6
#define LED2 7
#define LED3 8
#define LED4 9

#define RESETUP gpio_put(PIN_RST, 1)   // GPIO 13 high
#define RESETDOWN gpio_put(PIN_RST, 0) // GPIO 13 low

/*
// --- UART Configuration ---
#define UART_ID uart0
#define UART_BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1
*/

// --- Helper Macros ---
// Replaces pgm_read_byte for compatibility
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

void Buttons_Init(void);
void LEDs_Init(void);
bool timer_callback_reset_check(struct repeating_timer *t);
void Return_Timer_Setup(void);
void Command_Processing_Setup(void);

#endif /* HARDWARE_H_ */