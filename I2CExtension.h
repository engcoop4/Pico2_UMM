/**
 * @file I2CExtension.h
 * @author engcoop#4 RW
 * @brief Header file for I2CExtension.c.
 * @version 1.0.0
 * @date 2026-07-01
 * @copyright Copyright (c) 2026
 */

#ifndef I2C_EXTENSIONS_H
#define I2C_EXTENSIONS_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Hardware Definitions
#define I2C_PORT i2c0
#define PICO_SDA_PIN 8
#define PICO_SCL_PIN 9

// TCA9539 Target Specifications
#define ADDR_1 0x74
#define ADDR_2 0x75
#define ADDR_3 0x76

// TCA9539 Internal Register Commands
#define REG_INPUT_P0 0x00 // Read physical pin states
#define REG_INPUT_P1 0X01

#define REG_OUTPUT_P0 0x02 // Write/Read target output states
#define REG_OUTPUT_P1 0x03

#define REG_POL_OUT_P0 0x04
#define REG_POL_OUT_P1 0x05

#define REG_CONFIG_P0 0x06 // Configuration Port 0 (Input/Output select)
#define REG_CONFIG_P1 0x07 // Configuration Port 1 (Input/Output select)

static bool configure_single_extender(uint8_t address);
bool I2C_Init(void);
bool I2C_LEDs(uint8_t led_index, bool turn_on);

#endif /* I2C_EXTENSIONS_H */