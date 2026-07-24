// C/R-Pi libraries
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"

// Initialization/hardware
#include "initSPI.h"
#include "UART.h"
#include "Hardware.h"
#include "Global.h"
#include "font12x16.h"
#include "I2CExtension.h"

// Function for Display, Commands, and Touch Screen
#include "AdafruitDisplayInits.h"
#include "LCDProcessing.h"
#include "cmdProcessing.h"
#include "TouchScreeninit.h"

volatile State_of_Screen current_screen = Screen_ControlsDisplay;
volatile bool force_redraw = true;

volatile uint32_t ADCbuffer[NUM_SD24_ADC_CHANNELS]; // required to be 32-bit since SD24 memory holds 32-bit (8 bits of sign extension + 24-bit)

const MainScreenFunction Screen_Options[9] = {
    [Screen_ControlsDisplay] = MENU_ControlsDisplay,
    [Screen_TouchDecision] = MENU_TouchScreenDecision,
    [Screen_TouchCalibration] = MENU_TouchCalibration,
    [Screen_OperatingMode] = MENU_OperatingMode,
    [Screen_NumberDisplays] = MENU_NumberOfDisplays,
    [Screen_ChannelSelection] = MENU_ChannelSelection,
    [Index_PresetConfigs] = PresetConfigs,
    [Screen_DisplayChannels] = DisplayChannels,
    [Screen_Metering] = DisplayChannels // safety precaution for 'InitializationDone'
};

int main()
{
    stdio_init_all();
    sleep_ms(100);

    PIO_Init();
    LEDs_Init();
    
    if (!I2C_Init()) {
        // Handle I2C failure if necessary
    }

    TouchScreeninit();

    LCDinit();
    LCDSetup();
    
    Return_Timer_Setup();
    
    Command_Processing_Setup();

    while (true)
    {
        // 4. Feed the Watchdog continuously
        watchdog_update();

        // Pulls bytes from USB/UART into buffer for command processing
        ServiceSerialHardware();

        // Check if character was received for command processing
        if (testBit(rt.Host, CharAvailableFlag))
        {
            processChar();
        }

        // Check if full command (hitting ENTER key as trigger) is available
        if (testBit(rt.Host, CmdAvailFlag))
        {
            ParseRCI();
            printf("> ");
            fflush(stdout);
        }

        // Draws general menus, not responsible for updating the screens, for UI
        if (force_redraw)
        {
            if (Screen_Options[current_screen])
            {
                Screen_Options[current_screen]();
            }
            force_redraw = false;
        }

        // Responsible for updating the screens (UI)
        UIDispatcher();

        // Polls I2C buttons and touchscreen
        WaitForInput();

        sleep_ms(1);
    }
}