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

int main() {
    // 1. Hardware Init
    stdio_init_all();
    
    LEDs_Init();

    // 2. Wait for PuTTY
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }

    // 3. Clear state
    ClearRxBuffer();
    setBit(rt.Host, CharEchoFlag); // Enable the software echo we want to test

    printf("\r\n--- Phase 1: Echo & Buffer Test ---\r\n");
    printf("Testing: ServiceSerialHardware & processChar\r\n> ");
    fflush(stdout);

    while (true) {
        // HEARTBEAT
        static uint32_t last_heartbeat = 0;
        if (to_ms_since_boot(get_absolute_time()) - last_heartbeat > 500) {
            gpio_xor_mask(1 << LED1); 
            last_heartbeat = to_ms_since_boot(get_absolute_time());
        }

        // STEP A: Pull hardware bytes into rt.HostRxBuff
        ServiceSerialHardware();

        // STEP B: Process the buffer (Echo, Backspace logic)
        // We call this manually here to see if characters echo back to PuTTY
        if (testBit(rt.Host, CharAvailableFlag)) {
            processChar(); 
            // processChar clears CharAvailableFlag when done
        }

        // STEP C: Monitor Command Trigger
        if (testBit(rt.Host, CmdAvailFlag)) {
            printf("\r\n[SYSTEM]: Command detected in buffer! Clearing for next test.\r\n> ");
            ClearRxBuffer(); 
            // We clear it here so you can keep testing echos 
            // without the parser interfering yet.
        }

        tight_loop_contents(); // Optimization for RP2350
    }
}