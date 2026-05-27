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

char const *display_unit[6] = {
    "VAC",
    "VDC",
    "IAC",
    "IDC",
    "W",
    "Hz"};

// PIO Setup
PIO pio_global = pio0;
uint sm_global;
uint offset;

int main()
{
    // core hardware
    stdio_init_all();

    // delay for USB
    sleep_ms(100);

    // setup PIO
    offset = pio_add_program(pio_global, &screen_spi_program);
    sm_global = pio_claim_unused_sm(pio_global, true);

    screen_spi_program_init(pio_global, sm_global, offset, 4, 5, 32000000.0f); // 30 MHz works, 32 MHz seems to be the most stable, highest value

    LEDs_Init();
    Buttons_Init();
    LCDinit();
    LCDSetup();
    Return_Timer_Setup();
    Command_Processing_Setup();

    while (true)
    {
        watchdog_update();

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