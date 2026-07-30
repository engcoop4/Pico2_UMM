/**
 * @file TouchScreeninit.c
 * @author engcoop#4 RW
 * @brief Handles the touch screen initialization for the Adafruit LCD. Imported from CCS.
 * @version 1.0.0
 * @date 2026-04-15
 * @copyright Copyright (c) 2026
 */

#include "pico/time.h"
#include "TouchScreeninit.h"
#include "Hardware.h"
#include "ScreenDisplayInits.h"
#include "hardware/watchdog.h"
#include <stdbool.h>
#include <stdint.h>

volatile int screenTouched;
extern int touch_init;
extern int cali;

volatile uint16_t touch_triggered;
extern uint32_t X_Cord;
extern uint32_t Y_Cord;
volatile bool screen_updating = false;

uint16_t touch_baseline = 0;

// DEFAULT VALUES USED TO REFINE FORMULA.
// USED FOR INITIAL CALIBRATION TO ENSURE USER IS PRESSING IN RELATIVE AREA.
// changed from defines to variables so they can be changed dependent on calibration settings
// most likely needs to be changed since ADC is switching from 10-bit (CCS) to 12-bit (Pico)
#if defined(BOARD_TYPE_ADAFRUIT)
uint16_t TS_X_MIN = 600;
uint16_t TS_X_MAX = 3400;
uint16_t TS_Y_MIN = 600;
uint16_t TS_Y_MAX = 3400;

#elif defined(BOARD_TYPE_NEWHAVEN)
uint16_t TS_X_MIN = 600;
uint16_t TS_X_MAX = 3400;
uint16_t TS_Y_MIN = 600;
uint16_t TS_Y_MAX = 3400;
#endif

void TouchScreeninit(void)
{
    touch_init = 1;

    adc_init();

    // 1. Set up touch GPIO directions & pull-ups (leaves X+/X- as outputs LOW for GND path)
    SetTouchState();

    busy_wait_ms(10);

    adc_run(true); // Enable ADC
    CalibrateTouch();

    // 2. Start repeating hardware timer for touch polling (runs every 20ms)
    if (!timer_running)
    {
        add_repeating_timer_ms(-20, TouchTimer_Callback, NULL, &touch_timer);
        timer_running = true;
    }
}

void TouchScreen_deinit(void)
{
    // Clear software flags
    touch_init = 0;
    screenTouched = 0;

    // Flush coordinates
    X_Cord = 0;
    Y_Cord = 0;

    // Stop background hardware timer
    if (timer_running)
    {
        cancel_repeating_timer(&touch_timer);
        timer_running = false;
    }

    // Set all touch pins to high-impedance inputs
    gpio_set_dir(X_PLUS, GPIO_IN);
    gpio_set_dir(X_MINUS, GPIO_IN);
    gpio_set_function(Y_PLUS, GPIO_FUNC_SIO); // Convert Y+ back to GPIO
    gpio_set_dir(Y_PLUS, GPIO_IN);
    gpio_set_dir(Y_MINUS, GPIO_IN);

    // Disable pulls on all touch pins
    gpio_disable_pulls(X_PLUS);
    gpio_disable_pulls(X_MINUS);
    gpio_disable_pulls(Y_PLUS);
    gpio_disable_pulls(Y_MINUS);
}

bool TouchTimer_Callback(repeating_timer_t *rt)
{
    // Skip if screen is updating or an unhandled touch is still pending
    if (screen_updating || touch_triggered)
    {
        return true; // keep timer active
    }

    // 1. Set trap state pins (X+/X- LOW, Y- input)
    gpio_set_dir(X_PLUS, GPIO_OUT);   gpio_put(X_PLUS, 0);
    gpio_set_dir(X_MINUS, GPIO_OUT);  gpio_put(X_MINUS, 0);
    gpio_set_dir(Y_MINUS, GPIO_IN);

    // 2. Sample Y+ on ADC Channel 0
    adc_gpio_init(Y_PLUS);
    adc_select_input(0);
    busy_wait_us(50); // settling time

    uint16_t val = adc_read();

    // 3. Trigger touch if voltage drops below ~2.5V
    if (val < TOUCH_ADC_THRESHOLD)
    {
        touch_triggered = 1;
        X_Cord = -1;
        Y_Cord = -1;
    }

    return true; // keep timer repeating
}

// slightly changes from msp430 architecture. rather than #pragma dictating the interrupt, the built in
// function gpio_set_irq_enabled_with_callback allows us to specify the callback function directly in the init function,
// and it handles the rest of the interrupt setup behind the scenes. The callback will be triggered on the specified edge
// (falling edge in this case) for the specified GPIO pin (Y_MINUS).
// aka, the interrupt logic now lives in this function
void TouchInterrupt(uint gpio, uint32_t events)
{
    if (screen_updating)
    {
        // Must acknowledge so the interrupt doesn't immediately re-fire
        gpio_acknowledge_irq(gpio, events);
        return;
    }

    // Process touch detection (the dispatcher already verified this is Y_MINUS!)
    SetTouchState();

    // Set your software flag for WaitForInput() to find
    touch_triggered = 1;

    // Set to impossible values so it must read fresh coordinates
    X_Cord = -1;
    Y_Cord = -1;

    // Temporarily turn off the touch pin interrupt so it doesn't bounce
    // while we process this press in the main loop
    gpio_set_irq_enabled(Y_MINUS, GPIO_IRQ_EDGE_FALL, false);
}

// used to re-initialize GPIOs to handle interrupt
void SetTouchState(void)
{
    gpio_init(X_PLUS);
    gpio_init(X_MINUS);
    adc_gpio_init(Y_PLUS);
    gpio_init(Y_MINUS);

    // X+ and X- output LOW (GND plane)
    gpio_set_dir(X_PLUS, GPIO_OUT);
    gpio_put(X_PLUS, 0);

    gpio_set_dir(X_MINUS, GPIO_OUT);
    gpio_put(X_MINUS, 0);

    // Set Y- as input
    gpio_set_dir(Y_MINUS, GPIO_IN);

    // *** CHANGE THIS LINE ***
    // Replace gpio_pull_up(Y_MINUS); with:
    gpio_disable_pulls(Y_MINUS);

    // Disable pulls on all other pins
    gpio_disable_pulls(X_PLUS);
    gpio_disable_pulls(X_MINUS);
    gpio_disable_pulls(Y_PLUS);
}

void TouchToButtons(void)
{
    // 1. Reset software flag
    touch_triggered = 0;

    // 2. Park touch pins as High-Z Inputs
    // We use set_dir instead of gpio_init to avoid wiping the IRQ config
    gpio_set_dir(X_PLUS, GPIO_IN);
    gpio_set_dir(X_MINUS, GPIO_IN);
    gpio_set_dir(Y_PLUS, GPIO_IN);

    // 3. Ensure Y- is an input with Pull-Up for the next touch
    gpio_set_dir(Y_MINUS, GPIO_IN);
    gpio_pull_up(Y_MINUS);

    // 4. Point ADC to Buttons
    adc_select_input(2);

    // 5. Clear and Re-enable Interrupt
    gpio_acknowledge_irq(Y_MINUS, GPIO_IRQ_EDGE_FALL);
    gpio_set_irq_enabled(Y_MINUS, GPIO_IRQ_EDGE_FALL, true);
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
    adc_run(false);
    // Set X pins to ground (drive X-axis)
    gpio_set_dir(X_PLUS, GPIO_OUT);
    gpio_set_dir(X_MINUS, GPIO_OUT);
    gpio_put(X_PLUS, 0);
    gpio_put(X_MINUS, 0);

    // Set Y+ as ADC input, Y- as input
    adc_gpio_init(Y_PLUS);
    gpio_set_dir(Y_MINUS, GPIO_IN);

    // Select Y_PLUS (ADC0) as the input
    adc_select_input(0);

    // Allow voltages to settle
    busy_wait_us(250);

    // Read the ADC value
    uint16_t result = adc_read();

    return result;
}

// will need to be changed since now values go up to 4096 rather than 1024
bool CaliBoundsCheckTouch(void)
{
    uint16_t rx = ReadTouchX_Raw();
    uint16_t ry = ReadTouchY_Raw();

    // re-initialize the interrupt state after every reading
    SetTouchState();

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
        busy_wait_us(150);
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
        busy_wait_us(150);
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

void WaitForTouchRelease(void)
{
    uint32_t avg_val = 0;
    uint8_t count = 0;

    // 1. Establish Trap State for release checking
    gpio_set_dir(X_PLUS, GPIO_OUT);  gpio_put(X_PLUS, 0);
    gpio_set_dir(X_MINUS, GPIO_OUT); gpio_put(X_MINUS, 0);
    gpio_set_dir(Y_MINUS, GPIO_IN);

    adc_gpio_init(Y_PLUS);
    adc_select_input(0); // ADC0 (GPIO 26)

    // 2. Loop until voltage stays above threshold (released) for CHECK_RELEASE consecutive reads
    while (count < CHECK_RELEASE)
    {
        avg_val = 0;

        for (int i = 0; i < AVERAGE_SAMPLES_RELEASE; i++)
        {
            avg_val += adc_read();
            busy_wait_us(10);
        }
        avg_val = avg_val / AVERAGE_SAMPLES_RELEASE;

        // Screen is released when voltage returns above the threshold (+ hysteresis buffer)
        if (avg_val > (TOUCH_ADC_THRESHOLD + 400))
        {
            count++;
        }
        else
        {
            count = 0; // Finger still on screen, reset counter
        }

        busy_wait_us(100);
    }

    // 3. Return Y+ back to regular GPIO state
    gpio_set_function(Y_PLUS, GPIO_FUNC_SIO);
    gpio_set_dir(Y_PLUS, GPIO_IN);

    busy_wait_us(10);

    // 4. Reset software flag so background timer can sample again
    touch_triggered = 0;

    // REMOVED: gpio_acknowledge_irq(Y_MINUS, ...)
    // REMOVED: gpio_set_irq_enabled(Y_MINUS, ...)
}

bool IsFingerPhysicallyTouching(void)
{
    // If using direct GPIO digital readings for the touch pressure line:
    return (gpio_get(Y_MINUS) == 0); // Assuming active-low falling edge
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
        busy_wait_us(150);
    }
    uint32_t avg_raw = (raw_sum - min_raw - max_raw) / (AVERAGE_READ_SAMPLES - 2); // Remove outliers and keep fast division by 16

    const uint16_t TARGET_WIDTH = DISP_WIDTH - (CALI_OFFSET_X * 2); // 210 pixel span

    uint32_t adj_min_x = (TS_X_MIN > X_COMPENSATION) ? (TS_X_MIN - X_COMPENSATION) : 0;

    // clamping
    if (avg_raw < adj_min_x)
        avg_raw = adj_min_x;
    if (avg_raw > TS_X_MAX)
        avg_raw = TS_X_MAX;

    float scale = (float)TARGET_WIDTH / (float)(TS_X_MAX - adj_min_x);
    uint16_t x_pixel = (uint16_t)((avg_raw - adj_min_x) * scale + 0.5f) + (CALI_OFFSET_X / 2);

    return x_pixel;
}

uint16_t ReadTouchX_Raw(void)
{
    uint16_t result;

    // 1. RE-ARM the ADC Mux (The "Soft Kick")
    // This tells the ADC exactly which pin to look at and clears any stalls
    adc_select_input(0);

    // 2. Power the X-axis for measurement
    gpio_set_dir(X_PLUS, GPIO_OUT);
    gpio_put(X_PLUS, 1);
    gpio_set_dir(X_MINUS, GPIO_OUT);
    gpio_put(X_MINUS, 0);

    gpio_set_dir(Y_MINUS, GPIO_IN);
    // 3. Ensure the Sense pin (Y+) is handed to the ADC
    adc_gpio_init(Y_PLUS);

    // 4. Settle time - Use busy_wait to avoid the time.c hang
    busy_wait_us(250);

    // 5. Trigger a SINGLE conversion
    // This is the "Nuclear" alternative to a full adc_init
    result = adc_read();

    gpio_set_dir(X_PLUS, GPIO_IN);
    gpio_set_dir(X_MINUS, GPIO_IN);
    gpio_set_function(Y_PLUS, GPIO_FUNC_SIO);

    gpio_set_dir(Y_PLUS, GPIO_IN);
    gpio_set_dir(Y_MINUS, GPIO_IN);

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
        busy_wait_us(150);
    }
    uint32_t avg_raw = (raw_sum - min_raw - max_raw) / (AVERAGE_READ_SAMPLES - 2); // Remove outliers and keep fast division by 16

    const uint16_t TARGET_HEIGHT = DISP_HEIGHT - (CALI_OFFSET_Y * 2); // 290 pixel span

    uint32_t adj_min_y = (TS_Y_MIN > Y_COMPENSATION) ? (TS_Y_MIN - Y_COMPENSATION) : 0;

    // clamping
    if (avg_raw < adj_min_y)
        avg_raw = adj_min_y;
    if (avg_raw > TS_Y_MAX)
        avg_raw = TS_Y_MAX;

    float scale = (float)TARGET_HEIGHT / (float)(TS_Y_MAX - adj_min_y);
    uint16_t y_pixel = (uint16_t)((avg_raw - adj_min_y) * scale + 0.5f) + (CALI_OFFSET_Y / 2);

    return y_pixel;
}

uint16_t ReadTouchY_Raw(void)
{
    uint16_t result;

    // 1. "Soft Kick" the ADC
    // Select ADC1 (GPIO 27) and ensure conversion isn't stalled
    adc_select_input(1);

    // 2. Power the Y-axis (Vertical Gradient)
    // Drive Y+ High (3.3V) and Y- Low (GND)
    gpio_set_dir(Y_PLUS, GPIO_OUT);
    gpio_put(Y_PLUS, 1);
    gpio_set_dir(Y_MINUS, GPIO_OUT);
    gpio_put(Y_MINUS, 0);

    gpio_set_dir(X_MINUS, GPIO_IN);

    // 3. Prepare the Sense Pin (X+)
    // Hand X+ over to ADC and ensure no pulls are fighting the screen
    adc_gpio_init(X_PLUS);
    gpio_disable_pulls(X_PLUS);

    // 5. Settle time - Use busy_wait to prevent time.c deadlock
    busy_wait_us(250);

    // 6. Trigger a SINGLE conversion
    result = adc_read();

    // 7. RESTORE THE TRAP (Return to detection state)
    // We drive X pins low and set Y-up for interrupt as per your working init

    gpio_set_dir(Y_PLUS, GPIO_IN);
    gpio_set_dir(Y_MINUS, GPIO_IN);
    gpio_set_function(X_PLUS, GPIO_FUNC_SIO);
    gpio_set_dir(X_PLUS, GPIO_OUT);
    gpio_set_dir(X_MINUS, GPIO_OUT);

    return result;
}

bool Display_Bounds_Check_Total(uint16_t tx, uint16_t ty, uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height)
{
    return (tx >= x_start && tx <= (x_start + width) && ty >= y_start && ty <= (y_start + height));
}

bool Display_Bounds_Check_X(uint16_t tx, uint16_t x_start, uint16_t width)
{
    return (tx >= x_start && tx <= (x_start + width));
}

bool Display_Bounds_Check_Y(uint16_t ty, uint16_t y_start, uint16_t height)
{
    return (ty >= y_start && ty <= (y_start + height));
}