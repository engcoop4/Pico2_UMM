// C/R-Pi libraries
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h" // find version of this and SPI lines for standard GPIO
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
                switch (current_screen)
                {
                case Screen_ControlsDisplay:
                    ControlsDisplay();
                    break;
                case Screen_TouchDecision:
                    TouchScreenDecision();
                    break;
                case Screen_TouchCalibration:
                    TouchCalibration();
                    break;
                case Screen_OperatingMode:
                    OperatingMode();
                    break;
                case Screen_NumberDisplays:
                    NumberOfDisplays();
                    break;
                case Screen_ChannelSelection:
                    ChannelSelection();
                    break;
                case Index_PresetConfigs:
                    PresetConfigs();
                    break;
                case Screen_DisplayChannels:
                    DisplayChannels();
                    break;
                }
                force_redraw = false;
            }
            WaitForInput(); // Now detects the Soft Return flag
        }
            
    }
    
}