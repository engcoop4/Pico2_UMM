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

int main()
{
    // 1. Core Hardware
    stdio_init_all();

    // GIVE USB TIME TO NEGOTIATE BEFORE STARTING INTERRUPTS
    // This allows the PC to "see" the Pico before the timer starts firing
    sleep_ms(1000);

    LEDs_Init();
    Buttons_Init();
    SPI_init();
    LCD_DMA_Init();
    LCDSetup();

    // ... timer setup ...

    static struct repeating_timer timer; // Static ensures it persists in memory

    // Start the 10ms background polling
    add_repeating_timer_ms(-10, timer_callback_reset_check, NULL, &timer);

    // controls how long a restart takes, but cannot be too short or any processes that take longer than the chosen amount of time will trigger a reset
    watchdog_enable(3000, false);

    // 3. Setup Logic (No blocking USB wait)
    rt.ParamPtr = NULL;
    ClearRxBuffer();
    setBit(rt.Host, CharEchoFlag);

    current_screen = Screen_ControlsDisplay;
    force_redraw = true;

    // --- The Main Engine ---
    while (true)
    {
        // Vital: Feed the dog every loop
        watchdog_update();

        // Serial/Command Pump
        ServiceSerialHardware();
        if (testBit(rt.Host, CharAvailableFlag))
            processChar();
        if (testBit(rt.Host, CmdAvailFlag))
        {
            ParseRCI();
            printf("> ");
            fflush(stdout);
        }

        // UI Logic
        if (current_screen != InitializationDone)
        {
            if (force_redraw)
            {
                if (Screen_Options[current_screen])
                    Screen_Options[current_screen]();
                force_redraw = false;
            }
            WaitForInput();
        }

        // The "USB Breather"
        // This ensures the loop doesn't run so fast it starves the USB stack
        sleep_ms(5);
    }
}