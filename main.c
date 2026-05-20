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

    // 1. Setup PIO
    offset = pio_add_program(pio_global, &screen_spi_program);
    sm_global = pio_claim_unused_sm(pio_global, true);

    screen_spi_program_init(pio_global, sm_global, offset, 4, 5, 32000000.0f); // 30 MHz works, 32 MHz seems to be the most stable, highest value

    // all hardware initializations (convert to its own function ? -> avoid "losing" any of them like losing LCD_DMA_Init and bricking unit)    
    LEDs_Init();
    Buttons_Init();
    LCDinit();
    LCDSetup();

    // convert to its own function ?

    static struct repeating_timer timer; // Static ensures it persists in memory

    // background polls for RETURN inputs
    add_repeating_timer_ms(-10, timer_callback_reset_check, NULL, &timer);

    // controls how long a restart takes, but cannot be too short or any processes that take longer than the chosen amount of time will trigger a reset,
    // can prolly go shorter than 3 seconds tho (kinda long, 3 seconds hold + 3 seconds reset = 6 second cycle)
    // 1000 = 1 second, etc.
    watchdog_enable(3000, false);

    // command logic (gets its own function ?)
    
    rt.ParamPtr = NULL;
    ClearRxBuffer();
    setBit(rt.Host, CharEchoFlag);

    current_screen = Screen_ControlsDisplay;
    force_redraw = true;
    

    while (true)
    {
        
        watchdog_update();

        // 1. HARDWARE SERVICE
        ServiceSerialHardware(); // Pulls bytes from USB/UART into your buffer

        // 2. COMMAND PROCESSING (The missing piece)
        // Check if a character was received
        if (testBit(rt.Host, CharAvailableFlag))
        {
            processChar();
        }

        // Check if a full command (like hitting 'Enter' in PuTTY) is ready
        if (testBit(rt.Host, CmdAvailFlag))
        {
            printf("\r\n[PARSER]: Analyzing buffer...\r\n");
            ParseRCI();
            printf("> ");
            fflush(stdout);
        }

        // 3. UI STATIC DRAW
        if (force_redraw)
        {
            if (Screen_Options[current_screen])
                Screen_Options[current_screen]();
            force_redraw = false;
        }

        // 4. UI DYNAMIC LOGIC & INPUT
        // This calls ActiveMetering or Menu logic
        UIDispatcher();

        // This polls buttons and touch
        WaitForInput();

        sleep_ms(1);
    }
}