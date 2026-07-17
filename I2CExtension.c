/**
 * @file I2CExtension.c
 * @author engcoop#4 RW
 * @brief Contains initialization and functions in regards to the GPIO I2C Extenders used for the LEDs, buttons, and blade board identification.
 * @version 1.0.0
 * @date 2026-07-01
 * @copyright Copyright (c) 2026
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "I2CExtension.h"

volatile bool button_event_pending = false;

// single chip I2C initialization
static bool configure_single_extender(uint8_t address)
{
    uint8_t buffer[2];

    // 1. Force outputs high first to prevent active-low LED startup flash
    buffer[0] = REG_OUTPUT_P0;
    buffer[1] = 0xFF;
    if (i2c_write_blocking(I2C_PORT, address, buffer, 2, false) == PICO_ERROR_GENERIC)
    {
        return false;
    }

    // 2. Set Port 0 Direction (P00-P03 Outputs, P04-P07 Inputs)
    buffer[0] = REG_CONFIG_P0;
    buffer[1] = 0xF0;
    if (i2c_write_blocking(I2C_PORT, address, buffer, 2, false) == PICO_ERROR_GENERIC)
    {
        return false;
    }

    // 3. Set Port 1 Direction (All Inputs for Blade IDs)
    buffer[0] = REG_CONFIG_P1;
    buffer[1] = 0xFF;
    if (i2c_write_blocking(I2C_PORT, address, buffer, 2, false) == PICO_ERROR_GENERIC)
    {
        return false;
    }

    return true;
}

// initialize all three of the I2C chips in order
bool I2C_Init(void)
{
    // 1. Setup the physical Pico hardware block once
    i2c_init(I2C_PORT, 400 * 1000); // 400 kHz Fast Mode
    gpio_set_function(PICO_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_SDA_PIN);
    gpio_pull_up(PICO_SCL_PIN);

    // 2. Step through each physical target chip sequentially
    uint8_t targets[3] = {ADDR_1, ADDR_2, ADDR_3};

    // change to i < 3 when other I2Cs are added
    for (int i = 0; i < 1; i++)
    {
        if (!configure_single_extender(targets[i]))
        {
            // Early exit if any chip fails to answer on the bus
            return false;
        }
    }

    // 3. ADDED: Set up the physical Pico interrupt pin (GPIO 6)
    gpio_init(PICO_I2C_INT);
    gpio_set_dir(PICO_I2C_INT, GPIO_IN);
    gpio_pull_up(PICO_I2C_INT); // Keep line sitting high at 3.3V

    // Register our callback function to trigger when the line is pulled Low (Falling Edge)
    gpio_set_irq_enabled_with_callback(
        PICO_I2C_INT, 
        GPIO_IRQ_EDGE_FALL, 
        true, 
        &master_gpio_irq_dispatcher
    );

    return true;
}

bool I2C_LEDs(uint8_t led_index, bool turn_on)
{
    // ensure value passed is in range
    if (led_index > 3)
        return false;

    // create bitmask for targeted pin
    uint8_t led_mask = (1 << led_index);

    uint8_t reg_addr = REG_OUTPUT_P0;
    uint8_t current_output_val = 0;

    // ADDR_1 because LEDs are only present on 0x74 address I2C chip
    // checks for presence of 0x74 address/chip
    if (i2c_write_blocking(I2C_PORT, ADDR_1, &reg_addr, 1, true) == PICO_ERROR_GENERIC)
    {
        return false;
    }
    // if chip is present and acknowledges previous line, R-Pi pico2 reads what the current I2C chip has stored, data sanity check
    if (i2c_read_blocking(I2C_PORT, ADDR_1, &current_output_val, 1, false) == PICO_ERROR_GENERIC)
    {
        return false;
    }

    // modify the affected LED bit (which LED is being turned on or off)
    if (turn_on)
    {
        // Active-Low: Force the bit to 0 (pulls pin to GND, lighting the LED)
        current_output_val &= ~led_mask;
    }
    else
    {
        // Active-Low: Force the bit to 1 (pulls pin to 3.3V, turning off the LED)
        current_output_val |= led_mask;
    }

    // actually sends data to the chip to control the LED output
    uint8_t write_buffer[2] = {REG_OUTPUT_P0, current_output_val};
    if (i2c_write_blocking(I2C_PORT, ADDR_1, write_buffer, 2, false) == PICO_ERROR_GENERIC)
    {
        return false;
    }

    return true;
}

// need to implement interrupt so that the master is not constantly checking the slave for a button data change (and potentially missing it)
uint8_t I2C_ReadAllButtons(void)
{
    uint8_t reg_addr = REG_INPUT_P0;
    uint8_t raw_port_val = 0xFF; // Default to all High (unpressed)

    // 1. Point to the Port 0 Input Register (resets physical /INT)
    if (i2c_write_blocking(I2C_PORT, ADDR_1, &reg_addr, 1, true) == PICO_ERROR_GENERIC)
    {
        return 0; // Return 0 (no buttons pressed) on bus error
    }

    // 2. Read the 8-bit state
    if (i2c_read_blocking(I2C_PORT, ADDR_1, &raw_port_val, 1, false) == PICO_ERROR_GENERIC)
    {
        return 0;
    }

    // 3. Unpack bits 4-7, shift them down to bits 0-3, and invert 
    // because the physical buttons are active-low (Pressed = 0).
    uint8_t button_bits = (raw_port_val >> 4) & 0x0F;
    return (~button_bits) & 0x0F; 
}