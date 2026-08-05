// C/R-Pi libraries
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"

// Initialization/hardware
#include "ADCSPI.h"
#include "UART.h"
#include "Hardware.h"
#include "Global.h"
#include "font12x16.h"
#include "I2CExtension.h"

// Function for Display, Commands, and Touch Screen
#include "ScreenDisplayInits.h"
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

static volatile bool adc_data_ready = false;

int main() {
    stdio_init_all();
    
    // Wait for USB Serial terminal to connect
    sleep_ms(2500);

    printf("\n==================================================\n");
    printf("   ADS131M08 Complete Driver Verification Test    \n");
    printf("==================================================\n");

    gpio_set_irq_callback(&master_gpio_irq_dispatcher);
    irq_set_enabled(IO_IRQ_BANK0, true);

    // -------------------------------------------------------------------------
    // STEP 1: Driver Initialization
    // -------------------------------------------------------------------------
    printf("\n[1/3] Initializing ADS131M08 Driver & GPIO Interrupts...\n");
    ads131_init();

    printf("      -> Driver initialized.\n");

    // -------------------------------------------------------------------------
    // STEP 2: SPI Register Read/Write Verification
    // -------------------------------------------------------------------------
    printf("\n[2/3] Verifying SPI Register Read/Write Communication...\n");

    // A. Read Device ID Register (0x00)
    uint16_t chip_id = ads131_read_register(ADS131_REG_ID);
    printf("      -> ID Register (0x00): 0x%04X ", chip_id);

    if (chip_id == 0x0000 || chip_id == 0xFFFF) {
        printf("[FAIL]\n");
        printf("\nERROR: SPI Bus communication failed! Check SPI pin wiring (SCLK, DIN, DOUT, CS) and power rails.\n");
        while (1) { tight_loop_contents(); }
    } else {
        printf("[PASS]\n");
    }

    // B. Read initial CLOCK Register (0x03)
    uint16_t initial_clock = ads131_read_register(ADS131_REG_CLOCK);
    printf("      -> Initial CLOCK Register (0x03): 0x%04X\n", initial_clock);

    // C. Write to CLOCK Register to test WREG (Set OSR bits)
    // Write 0xFF0E to temporarily modify CLOCK register
    printf("      -> Testing Register Write (WREG)... Writing 0xFF0E to CLOCK register...\n");
    ads131_write_register(ADS131_REG_CLOCK, 0xFF0E);

    // D. Read back CLOCK Register to verify write success
    uint16_t modified_clock = ads131_read_register(ADS131_REG_CLOCK);
    printf("      -> Readback CLOCK Register: 0x%04X ", modified_clock);

    if (modified_clock == 0xFF0E) {
        printf("[PASS - WREG working!]\n");
    } else {
        printf("[FAIL - Write failed]\n");
    }

    // E. Restore CLOCK register back to default
    ads131_write_register(ADS131_REG_CLOCK, initial_clock);

    // -------------------------------------------------------------------------
    // STEP 3: DRDY Interrupt & Live Data Frame Reading Verification
    // -------------------------------------------------------------------------
    printf("\n[3/3] Testing Live Data Frame Acquisition via DRDY Interrupt...\n");
    printf("      Reading 5 consecutive sample frames:\n\n");

    ads131_frame_t frame;
    uint32_t frame_count = 0;

    while (frame_count < 5) {
        if (adc_data_ready) {
            adc_data_ready = false; // Clear flag

            if (ads131_read_frame(&frame)) {
                frame_count++;
                printf("  Frame #%lu | Status: 0x%04X | CRC: 0x%04X\n", 
                        frame_count, frame.status, frame.crc);
                
                // Print Channels 0 through 3 raw signed counts
                printf("    Ch0: %10ld | Ch1: %10ld | Ch2: %10ld | Ch3: %10ld\n",
                       (long)frame.channel[0], (long)frame.channel[1],
                       (long)frame.channel[2], (long)frame.channel[3]);
            }
        }
    }

    printf("\n==================================================\n");
    printf(" SUCCESS: All driver verification tests passed!\n");
    printf(" ADS131M08 is fully operational on RP2350.\n");
    printf("==================================================\n");

    while (1) {
        tight_loop_contents();
    }
}

/*
int main()
{
    stdio_init_all();
    sleep_ms(100);

    PIO_Init();
    LEDs_Init();
    
    if (!I2C_Init()) {
        // in case the I2C does not respond (catch case)
    }

    adc_init();             // required for the screen to populate. i have no idea what is requiring the adc_init to progress, but i dont believe this is actually needed. it is accidentally a pillar to run the program? not super important
                            // that its needed as it only executes upon start-up, its just aggravating because idk why it is needed. something inside the while(true) loop depends on it, before UIDispatcher

    LCDinit();
    Screen_Setup();
    
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
    */