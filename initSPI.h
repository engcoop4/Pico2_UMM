#ifndef INITSPI_H_
#define INITSPI_H_

#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT spi1
#define PIN_MISO 12
#define PIN_CS 9
#define PIN_SCK 10
#define PIN_MOSI 11

void SPI_init(void);

#endif /* INITSPI_H_ */