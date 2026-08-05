#ifndef INITADCSPI_H_
#define INITADCSPI_H_

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define ADS131_SPI_PORT spi1
#define ADS131_CS 3
#define ADS131_SCLK 10
#define ADS131_DIN 11
#define ADS131_DOUT 12
#define ADS131_DRDY 6

#define ADS131_BAUDRATE (4 * 1000 * 1000)

#define ADS131_CPOL SPI_CPOL_0
#define ADS131_CPHA SPI_CPHA_1
#define ADS131_SPI_MODE 1

#define ADS131_CMD_NULL 0x0000
#define ADS131_CMD_RESET 0x0011
#define ADS131_CMD_STANDBY 0x0022
#define ADS131_CMD_WAKEUP 0x0033
#define ADS131_CMD_LOCK 0x0555
#define ADS131_CMD_UNLOCK 0x0666

#define ADS131_REG_ID 0x00
#define ADS131_REG_STATUS 0x01
#define ADS131_REG_MODE 0x02
#define ADS131_REG_CLOCK 0x03

#define ADS131_NUM_CHANNELS 8
#define ADS131_FRAME_WORDS 10

typedef struct {
    uint16_t status;
    int32_t channel[ADS131_NUM_CHANNELS];
    uint16_t crc;
} ads131_frame_t;

void ads131_init(void);
uint16_t ads131_read_register(uint8_t reg_addr);
void ads131_write_register(uint8_t reg_addr, uint16_t reg_value);
bool ads131_read_frame(ads131_frame_t *frame);

#endif /* INITADCSPI_H_ */