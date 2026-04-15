/**
 * @file TouchScreeninit.c
 * @author engcoop#4 RW
 * @brief Handles the touch screen initialization for the Adafruit LCD. Imported from CCS.
 * @version 1.0.0
 * @date 2026-04-15
 * @copyright Copyright (c) 2026
 */

#include "TouchScreeninit.h"
#include "AdafruitDisplayInits.h"
#include <stdbool.h>
#include <stdint.h>

volatile int screenTouched;
extern int touch_init;
extern int cali;

volatile uint16_t touch_triggered;
extern uint32_t X_Cord;
extern uint32_t Y_Cord;

uint16_t touch_baseline = 0;

// DEFAULT VALUES USED TO REFINE FORMULA.
// USED FOR INITIAL CALIBRATION TO ENSURE USER IS PRESSING IN RELATIVE AREA.
// changed from defines to variables so they can be changed dependent on calibration settings
// most likely needs to be changed since ADC is switching from 10-bit (CCS) to 12-bit (Pico)
uint16_t TS_X_MIN = 150;
uint16_t TS_X_MAX = 850;
uint16_t TS_Y_MIN = 150;
uint16_t TS_Y_MAX = 850;

void TouchScreeninit(void)
{
    touch_init = 1;

    adc_init();

    // initialize GPIOs and ADC for touch screen
    gpio_init(X_PLUS);     // set X+
    adc_gpio_init(Y_PLUS); // set Y+
    gpio_init(X_MINUS);    // set X-
    gpio_init(Y_MINUS);    // set Y-

    // ground X-axis
    gpio_set_dir(X_PLUS, GPIO_OUT);  // X+ as output
    gpio_set_dir(X_MINUS, GPIO_OUT); // X- as output

    // set y-axis to inputs for detecting touch
    adc_select_input(0);            // Y+ as ADC input (ADC0 corresponds to GPIO 26)
    gpio_set_dir(Y_MINUS, GPIO_IN); // Y- as input

    // set initial states
    // Y- and Y+ are left as inputs, so no need to set them low
    gpio_put(X_PLUS, 0);  // X+ low, GND to detect touch
    gpio_put(X_MINUS, 0); // X- low, GND to detect touch

    CalculateTouch(); // take initial reading to set baseline

    // enable interrupts on Y- pin (replaces P1IE configuration)
    // the callback function TouchInterrupt will be called when a falling edge is detected on the Y_MINUS pin, which indicates a touch event
    // this replaces #pragma in msp430 architecture, and the interrupt logic now lives in this function rather than being separate
    gpio_set_irq_enabled_with_callback(Y_MINUS, GPIO_IRQ_EDGE_FALL, true, &TouchInterrupt);
}

void TouchScreen_deinit(void)
{
    // disable adc conversions
    adc_run(false);
    hw_clear_bits(&adc_hw->cs, ADC_CS_EN_BITS);

    // clear software flags
    touch_init = 0;
    screenTouched = 0;

    // flush coordinates
    X_Cord = 0;
    Y_Cord = 0;

    // disable interrupts for touch detection on Y- pin (GPIO 22)
    gpio_set_irq_enabled(Y_MINUS, GPIO_IRQ_EDGE_FALL, false);

    // GPIO reset
    uint pins[] = {X_PLUS, X_MINUS, Y_MINUS};
    for (int i = 0; i < NUMBER_OF_TOUCH_CHANNELS - 1; i++)
    {
        gpio_set_dir(pins[i], GPIO_OUT);
        gpio_put(pins[i], 0);
    }

    // returning GPIO 26 (ADC0) to a digital state and driving it low for power saving
    gpio_init(Y_PLUS);
    gpio_set_dir(Y_PLUS, GPIO_OUT);
    gpio_put(Y_PLUS, 0);
}

// slightly changes from msp430 architecture. rather than #pragma dictating the interrupt, the built in
// function gpio_set_irq_enabled_with_callback allows us to specify the callback function directly in the init function,
// and it handles the rest of the interrupt setup behind the scenes. The callback will be triggered on the specified edge
// (falling edge in this case) for the specified GPIO pin (Y_MINUS).
// aka, the interrupt logic now lives in this function
void TouchInterrupt(uint gpio, uint32_t events)
{
    // only act if the interrupt came from the Y_MINUS pin
    if (gpio == Y_MINUS)
    {
        // set software flag
        touch_triggered = 1;

        // set to impossible values so it must read new coordinates each time (prevents false reads from "stale" coordinates)
        X_Cord = -1;
        Y_Cord = -1;

        // disable interrupt (equivalent to P1IE &= ~BIT0)
        gpio_set_irq_enabled(Y_MINUS, GPIO_IRQ_EDGE_FALL, false);

        // the Pico2 handles clearing clearing flags via the callback function (replaces P1IFG &~BIT0)
    }
}

void CalibrateTouch(void)
{
    uint32_t accumulator = 0;
    for (int i = 0; i < CALIBRATE_SAMPLES; i++)
    {
        accumulator += CalculateTouch();
    }
    // (accumulator >> 4) no longer required to save processing power by avoiding division, since the Pico can handle it easily and it's only done once during calibration
    // pico is smart enough to detect /16 as a bit shift and optimize it, but we can just do the division for clarity since it's only done once
    touch_baseline = (uint16_t)(accumulator / CALIBRATE_SAMPLES);
}

uint16_t CalculateTouch(void)
{
    // disable adc to prevent data tears (?)
    adc_run(false);

    // emulate setting all pins to GPIO and setting X to output and Y to input
    // P1SEL0 &= ~0x0F
    // P1SEL1 &= ~0x0F
    // P1OUT &= ~0x0F
    // P1DIR &= ~(BIT0 | BIT2)
    // P1DIR |= (BIT1 | BIT3)

    // set X to ground
    gpio_init(X_PLUS);
    gpio_init(X_MINUS);
    gpio_set_dir(X_PLUS, GPIO_OUT);
    gpio_set_dir(X_MINUS, GPIO_OUT);
    gpio_put(X_PLUS, 0);
    gpio_put(X_MINUS, 0);

    // set Y to input
    gpio_init(Y_MINUS);
    gpio_set_dir(Y_MINUS, GPIO_IN);

    adc_gpio_init(Y_PLUS);
    adc_select_input(0);

    sleep_us(150);

    // hw_set_bits(&adc_hw->cs, ADC_CS_EN_BITS); // enable ADC (replaces ADC10CTL0 |= ADC10ON)

    uint16_t result = adc_read();
    gpio_init(Y_PLUS);
    // hw_clear_bits(&adc_hw->cs, ADC_CS_EN_BITS); // disable ADC (replaces ADC10CTL0 &= ~ADC10ON)

    return result;
}

// will need to be changed since now values go up to 4096 rather than 1024
bool CaliBoundsCheckTouch(void)
{
    uint16_t rx = ReadTouchX_Raw();
    uint16_t ry = ReadTouchY_Raw();

    if (!cali)
    {
        if ((rx < CALI_BOUNDS_MIN_X) && (ry < CALI_BOUNDS_MIN_Y))
        {
            return true;
        }
    }
    else if (cali)
    {
        if ((rx > CALI_BOUNDS_MAX_X) && (ry > CALI_BOUNDS_MAX_Y))
        {
            return true;
        }
    }
    return false;
}

void CaptureCaliCoordsTouch(void)
{
    uint32_t x_acc = 0;
    uint32_t y_acc = 0;

    for (int i = 0; i < CAPTURE_CALI_COORDS_SAMPLES; i++)
    {
        x_acc += ReadTouchX_Raw();
        y_acc += ReadTouchY_Raw();
        sleep_us(150);
    }
    if (!cali)
    {
        TS_X_MIN = (uint16_t)(x_acc / CAPTURE_CALI_COORDS_SAMPLES);
        TS_Y_MIN = (uint16_t)(y_acc / CAPTURE_CALI_COORDS_SAMPLES);
    }
    else if (cali)
    {
        TS_X_MAX = (uint16_t)(x_acc / CAPTURE_CALI_COORDS_SAMPLES);
        TS_Y_MAX = (uint16_t)(y_acc / CAPTURE_CALI_COORDS_SAMPLES);
    }
}

uint16_t CalculateTouch_Stable(void)
{
    uint16_t last_val = 0;
    uint16_t current_val = CalculateTouch();
    uint8_t stable_count = 0;

    for (int i = 0; i < STABILIZATION_TIMEOUT; i++)
    {
        sleep_us(150);
        last_val = current_val;
        current_val = CalculateTouch();

        int16_t diff = (int16_t)current_val - (int16_t)last_val;
        if (diff < 0)
            diff = -diff;

        if (diff < DIFF_STABLE_THRESHOLD)
            stable_count++;
        if (stable_count > STABLE_COUNT_MIN)
            break;
    }
    return current_val;
}

void WaitForReleaseTouch(void)
{
    uint32_t avg_val = 0;
    uint8_t count = 0;

    // 1. Pin Setup (Trap State)
    gpio_set_dir(X_PLUS, GPIO_OUT);
    gpio_set_dir(X_MINUS, GPIO_OUT);
    gpio_put(X_PLUS, 0);
    gpio_put(X_MINUS, 0);

    // Prepare Y+ for ADC reading
    adc_gpio_init(Y_PLUS);
    adc_select_input(0); // ADC0 (GPIO 26)

    // Scaling the MARGIN: Since 12-bit is 4x more sensitive,
    // ensure your MARGIN is scaled up (e.g., if it was 50, use 200).

    while (count < CHECK_RELEASE)
    {
        avg_val = 0;

        // Take 4 samples to average
        for (int i = 0; i < AVERAGE_SAMPLES_RELEASE; i++)
        {
            avg_val += adc_read();
            sleep_us(10); // replaces __delay_cycles(100)
        }
        avg_val = avg_val / AVERAGE_SAMPLES_RELEASE;

        // Check if the voltage returned to baseline (screen released)
        if (avg_val > (touch_baseline - MARGIN))
        {
            count++;
        }
        else
        {
            count = 0;
        }

        sleep_us(100); // replaces __delay_cycles(1000)
    }

    // 2. Re-establish physical Trap (Digital state)
    gpio_init(Y_PLUS);
    gpio_set_dir(Y_PLUS, GPIO_IN); // High-Z, waiting for pull-up

    // 3. Settling window
    sleep_us(500);

    // 4. Clear interrupt flags (RP2350 SDK handles this, but we ensure state is ready)
    // The next time the interrupt is enabled, it won't see "stale" noise.
}

void TouchScreenReset(void)
{

    uint pins[] = {Y_PLUS, X_PLUS, X_MINUS, Y_MINUS};
    for (int i = 0; i < NUMBER_OF_TOUCH_CHANNELS; i++)
    {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN); // Sets to High-Impedance Input
        gpio_disable_pulls(pins[i]);    // Replaces P1REN &= ~0x0F
    }
}

uint16_t ReadTouchX(void)
{
    uint32_t raw_sum = 0;
    uint16_t min_raw = MIN_RAW;
    uint16_t max_raw = 0;
    uint16_t current;

    for (int i = 0; i < AVERAGE_READ_SAMPLES; i++)
    {
        current = ReadTouchX_Raw();
        if (current < min_raw)
            min_raw = current;
        if (current > max_raw)
            max_raw = current;

        raw_sum += current;
        sleep_us(150);
    }
    uint32_t avg_raw = (raw_sum - min_raw - max_raw) / (AVERAGE_READ_SAMPLES - 2); // Remove outliers and keep fast division by 16

    const uint16_t TARGET_WIDTH = DISP_WIDTH - (CALI_OFFSET_X * 2); // 210 pixel span

    uint32_t adj_min_x = TS_X_MIN - X_COMPENSATION;

    // clamping
    if (avg_raw < adj_min_x)
        avg_raw = adj_min_x;
    if (avg_raw > TS_X_MAX)
        avg_raw = TS_X_MAX;

    float scale = (float)TARGET_WIDTH / (float)(TS_X_MAX - adj_min_x);
    uint16_t x_pixel = (uint16_t)((avg_raw - adj_min_x) * scale + 0.5f) + CALI_OFFSET_X;

    return x_pixel;
}

uint16_t ReadTouchX_Raw(void)
{
    uint16_t result;

    gpio_set_dir(X_PLUS, GPIO_OUT);
    gpio_set_dir(X_MINUS, GPIO_OUT);

    gpio_put(X_PLUS, 1);
    gpio_put(X_MINUS, 0);

    adc_gpio_init(Y_PLUS);
    adc_select_input(0); // ADC0 (GPIO 26)
    sleep_us(250);

    result = adc_read();

    gpio_set_dir(X_PLUS, GPIO_OUT);
    gpio_set_dir(X_MINUS, GPIO_OUT);
    gpio_put(X_PLUS, 0);
    gpio_put(X_MINUS, 0);

    gpio_init(Y_PLUS);
    gpio_set_dir(Y_PLUS, GPIO_IN);

    return result;
}

uint16_t ReadTouchY(void)
{
    uint32_t raw_sum = 0;
    uint16_t min_raw = MIN_RAW;
    uint16_t max_raw = 0;
    uint16_t current;

    for (int i = 0; i < AVERAGE_READ_SAMPLES; i++)
    {
        current = ReadTouchY_Raw();
        if (current < min_raw)
            min_raw = current;
        if (current > max_raw)
            max_raw = current;

        raw_sum += current;
        sleep_us(150);
    }
    uint32_t avg_raw = (raw_sum - min_raw - max_raw) / (AVERAGE_READ_SAMPLES - 2); // Remove outliers and keep fast division by 16

    const uint16_t TARGET_HEIGHT = DISP_HEIGHT - (CALI_OFFSET_Y * 2); // 290 pixel span

    uint32_t adj_min_y = TS_Y_MIN - Y_COMPENSATION;

    // clamping
    if (avg_raw < adj_min_y)
        avg_raw = adj_min_y;
    if (avg_raw > TS_Y_MAX)
        avg_raw = TS_Y_MAX;

    float scale = (float)TARGET_HEIGHT / (float)(TS_Y_MAX - adj_min_y);
    uint16_t y_pixel = (uint16_t)((avg_raw - adj_min_y) * scale + 0.5f) + CALI_OFFSET_Y;

    return y_pixel;
}

uint16_t ReadTouchY_Raw(void)
{
    uint16_t result;

    gpio_set_dir(Y_PLUS, GPIO_OUT);
    gpio_set_dir(Y_MINUS, GPIO_OUT);
    gpio_put(Y_PLUS, 1);
    gpio_put(Y_MINUS, 0);

    adc_gpio_init(X_PLUS);
    adc_select_input(1); // ADC1 (GPIO 27)
    sleep_us(250);

    result = adc_read();

    gpio_init(X_PLUS);
    gpio_set_dir(X_PLUS, GPIO_OUT);
    gpio_put(X_PLUS, 0);

    gpio_init(Y_PLUS);
    gpio_set_dir(Y_PLUS, GPIO_IN);

    gpio_init(Y_MINUS);
    gpio_set_dir(Y_MINUS, GPIO_IN);

    return result;
}

uint8_t Display_Bounds_Check(uint16_t tx, uint16_t ty, uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height)
{
    return (tx >= x_start && tx <= (x_start + width) && ty >= y_start && ty <= (y_start + height));
}