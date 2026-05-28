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
#include "hardware/pio.h"
#include "screen_spi.pio.h"

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
    // core hardware
    stdio_init_all();

    // delay for USB
    sleep_ms(100);

    PIO_Init();
    LEDs_Init();
    Buttons_Init();
    LCDinit();
    LCDSetup();
    Return_Timer_Setup();
    Command_Processing_Setup();

    while (true)
    {
        //watchdog_update();

        ServiceSerialHardware(); // Pulls bytes from USB/UART into your buffer, for command processing

        // check if character was received, for command processing
        if (testBit(rt.Host, CharAvailableFlag))
        {
            processChar();
        }

        // check if full command (hitting ENTER key as trigger) is available, for command processing
        if (testBit(rt.Host, CmdAvailFlag))
        {
            ParseRCI();
            printf("> ");
            fflush(stdout);
        }

        // draws general menus, not responsible for updating the screens, for UI
        if (force_redraw)
        {
            if (Screen_Options[current_screen])
                Screen_Options[current_screen]();
            force_redraw = false;
        }

        // responsible for updating the screens, will ultimately call ActiveMetering (the final display screen), for UI
        UIDispatcher();

        // This polls buttons and touch
        WaitForInput();

        sleep_ms(1);
    }
}