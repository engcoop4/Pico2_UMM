// C/R-Pi libraries
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h" 
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

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

int main()
{
    sleep_ms(100);
    stdio_init_all();

    Buttons_Init();
    LEDs_Init();

    // 2. Initialize ONLY the SPI1 and LCD pins
    SPI_init();
    LCD_DMA_Init();

    LCDSetup();

    /* ----- MAIN LOOP ----- */
    while (1)
    {
        // runs until current_screen == InitializationDone (changes in DisplayChannels)
        while (current_screen != InitializationDone)
        {
            /*
            uart_command_received = 0;
            ParseRCI();
            */
            if (force_redraw)
            {
                if (current_screen < NUM_MAIN_SCREENS && Screen_Options[current_screen] != NULL)
                {
                    Screen_Options[current_screen]();
                }
                force_redraw = false;
            }
            WaitForInput();
        }
    }
}