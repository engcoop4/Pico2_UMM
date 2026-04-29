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

int main()
{
    // 1. Basic System Init
    sleep_ms(100);
    stdio_init_all();

    // 2. Hardware Peripheral Setup (Crucial: Do this BEFORE starting the timer)
    Buttons_Init();  // Initializes ADC and GPIOs
    LEDs_Init();
    SPI_init();
    LCD_DMA_Init();
    
    // 3. Prepare the Background Monitor
    static struct repeating_timer timer; // Static ensures it persists in memory
    
    // Start the 10ms background polling
    add_repeating_timer_ms(-10, timer_callback_reset_check, NULL, &timer);

    // Enable Watchdog for the Hard Reset functionality
    watchdog_enable(2000, 1);

    // 4. Initial Screen Draw
    LCDSetup();

    /* ----- MAIN LOOP ----- */
    while (1)
    {
        // --- PHASE 1: UI / SETUP ---
        // Runs until current_screen == InitializationDone
        while (current_screen != InitializationDone)
        {
            watchdog_update();
            ParseRCI();
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

        // --- PHASE 2: ACTIVE METERING ---
        // This is where your DMA-based voltage readings happen.
        // Because the timer is global, the 3-second Hard Reset is active here!
        
        while (current_screen == InitializationDone) 
        {
            // 1. Run your high-speed ADC/DMA Metering logic here
            // Run_Metering_Cycle(); 
            watchdog_update();
            // 2. Check for the Short-Press Return flag from the timer
            if (timer_return_flag) 
            {
                timer_return_flag = false; // Consume flag
                
                // Logic to "stop" metering and go back to menu
                // Stop_DMA_Transfers(); 
                
                current_screen = Screen_OperatingMode; // Or your preferred back-page
                force_redraw = true;
                break; // Break back into the Setup Loop
            }
            
            // Optional: Small sleep or watchdog update if your metering is slow
            // watchdog_update(); 
        }
    }
}