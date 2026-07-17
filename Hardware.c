#include "Hardware.h"
#include "hardware/watchdog.h"
#include "LCDProcessing.h"
#include "cmdProcessing.h"
#include "Global.h"
#include "hardware/pio.h"
#include "screen_spi.pio.h"
#include "I2CExtension.h"
#include "TouchScreeninit.h"

#define RESET_THRESHOLD 300 // 3 seconds at 10ms intervals
volatile uint32_t reset_hold_counter = 0;
volatile bool timer_return_flag = false;

PIO pio_global = pio0;
uint sm_global;
uint offset;

void PIO_Init(void) {
    offset = pio_add_program(pio_global, &screen_spi_program);
    sm_global = pio_claim_unused_sm(pio_global, true);

    screen_spi_program_init(pio_global, sm_global, offset, 4, 5, 32000000.0f); // 30 MHz works, 32 MHz seems to be the most stable, highest value
}

void LEDs_Init(void)
{
    gpio_init(LED1);
    gpio_init(LED2);
    gpio_init(LED3);
    gpio_init(LED4);

    gpio_set_dir(LED1, GPIO_OUT);
    gpio_set_dir(LED2, GPIO_OUT);
    gpio_set_dir(LED3, GPIO_OUT);
    gpio_set_dir(LED4, GPIO_OUT);

    // LEDs are active low, so setting them high turns them off by default
    gpio_put(LED1, 1);
    gpio_put(LED2, 1);
    gpio_put(LED3, 1);
    gpio_put(LED4, 1);
}

void Buttons_Init(void)
{
    adc_init();
    adc_gpio_init(SWLADDER);
    adc_select_input(2);
}

bool timer_callback_reset_check(struct repeating_timer *t)
{
    uint8_t selected_input = adc_get_selected_input();

    adc_select_input(2);
    uint16_t adc_val = adc_read();

    // Is the Return button held? (Using your specific range)
    if (adc_val >= WFI_BUT_RETURN_THRESH_P_L && adc_val <= WFI_BUT_RETURN_THRESH_P_HI)
    {
        reset_hold_counter++;

        // MASTER RESET: Only if in the final display mode
        if (current_screen == Screen_Metering && reset_hold_counter >= RESET_THRESHOLD)
        {
            // Physical LCD Reset (Mimicking your MSP430 P3_7_low)
            // Replace LCD_RESET_PIN with your actual GP number
            gpio_put(PIN_RST, 0);
            sleep_ms(500);
            watchdog_reboot(0, 0, 0);
        }
    }
    else
    {
        // Button Released: Check if it was a valid short press
        if (reset_hold_counter > 5 && reset_hold_counter < RESET_THRESHOLD)
        {
            timer_return_flag = true;
        }
        reset_hold_counter = 0;
    }

    adc_select_input(selected_input);

    return true;
}

void Return_Timer_Setup(void)
{
    static struct repeating_timer timer; // Static ensures it persists in memory

    // background polls for RETURN inputs
    add_repeating_timer_ms(-10, timer_callback_reset_check, NULL, &timer);

    // controls how long a restart takes, but cannot be too short or any processes that take longer than the chosen amount of time will trigger a reset,
    // can prolly go shorter than 3 seconds tho (kinda long, 3 seconds hold + 3 seconds reset = 6 second cycle)
    // 1000 = 1 second, etc.
    // watchdog_enable(3000, false);
}

void Command_Processing_Setup(void)
{
    rt.ParamPtr = NULL;
    ClearRxBuffer();
    setBit(rt.Host, CharEchoFlag);

    current_screen = Screen_ControlsDisplay;
    force_redraw = true;
}

// The single, unified GPIO interrupt handler for Core 0
void master_gpio_irq_dispatcher(uint gpio, uint32_t events)
{
    if (gpio == PICO_I2C_INT) 
    {
        // 1. Handle Button Expander Interrupt
        button_event_pending = true;
        // The SDK automatically handles clearing the Pico's internal GPIO flags,
        // but remember: reading REG_INPUT_P0 later clears the TCA9539 hardware /INT line!
    }
    else if (gpio == Y_MINUS)
    {
        // 2. Handle Touch Screen Interrupt
        TouchInterrupt(gpio, events);
    }
}