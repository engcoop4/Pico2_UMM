#include "initSPI.h"
#include "Hardware.h"

void LCDinit()
{
    // 3. Setup Chip Select
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1); // Deselected

    // 4. Setup Data/Command Pin
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    RSUP; // Default to Data mode

    // 5. HARDWARE RESET SEQUENCE (Critical for "Run" mode)
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);

    RESETDOWN;    // Pull Reset LOW
    sleep_ms(50); // Give it a solid 50ms pulse
    RESETUP;      // Pull Reset HIGH

    // CRITICAL: The ILI9341 takes ~120ms to restart its
    // internal oscillators after a reset pulse.
    sleep_ms(150);
}