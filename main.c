// C/R-Pi libraries
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"

// Initialization/hardware
#include "initSPI.h"
#include "UART.h"
#include "Hardware.h"
#include "Global.h"
#include "font12x16.h"

// Function for Display, Commands, and Touch Screen
#include "AdafruitDisplayInits.h"
#include "LCDProcessing.h"
#include "cmdProcessing.h"
#include "TouchScreeninit.h"

// LCD PARAMETERS FOR TESTING
int8_t numberdisplays = 1;
int8_t numberchannels = 1;
int8_t cursor_position = 0;
int8_t selected_display = 0;
int8_t unit_index = 0;

volatile State_of_Screen current_screen = Screen_ControlsDisplay;
volatile bool force_redraw = true;
volatile uint8_t uart_command_received = 0;
volatile uint8_t return_request_flag = 0;
volatile uint8_t entry_method = 0;

volatile uint32_t ADCbuffer[NUM_SD24_ADC_CHANNELS]; // required to be 32-bit since SD24 memory holds 32-bit (8 bits of sign extension + 24-bit)
volatile uint16_t pulse_width;
uint pwm_channel;
volatile bool force_full_redraw = false;
SYS_SPECIFIC_DATA SysData;

RealTimeVars rt;

const MainScreenFunction Screen_Options[9] = {
    MENU_ControlsDisplay,
    MENU_TouchScreenDecision,
    MENU_TouchCalibration,
    MENU_OperatingMode,
    MENU_NumberOfDisplays,
    MENU_ChannelSelection,
    PresetConfigs,
    DisplayChannels,
    NULL // safety precaution for 'InitializationDone'
};

char const *display_unit[6] = {
    "VAC",
    "VDC",
    "IAC",
    "IDC",
    "W",
    "Hz"};

#include "pico/stdlib.h"
#include <stdio.h>

#define LED1 6 // Ensure this matches your GP pin for LED1

int main() {
    stdio_init_all();

    // Initialize LED1 immediately so we can see status
    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);
    gpio_put(LED1, 1); // OFF (Active Low)

    // Wait for terminal connection
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }

    // Single burst to terminal
    printf("\r\n--- HARDWARE RAW RX TEST ---\r\n");
    fflush(stdout);

    while (true) {
        // HEARTBEAT (10ms blink every ~100ms)
        gpio_put(LED1, 0); 
        sleep_ms(10);
        gpio_put(LED1, 1);

        // RAW GETCHAR
        int c = getchar_timeout_us(0);

        if (c != PICO_ERROR_TIMEOUT) {
            // If the chip sees ANY byte, LED1 stays ON for 2 seconds
            // This bypasses any terminal display issues
            gpio_put(LED1, 0); 
            
            // Send back exactly what was received
            printf("RX: %c\n", (char)c);
            fflush(stdout);
            
            sleep_ms(2000); 
            gpio_put(LED1, 1);
        }

        sleep_ms(90);
    }
}