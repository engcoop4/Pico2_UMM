#ifndef INITSPI_H_
#define INITSPI_H_

#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT spi1

void SPI_init(void);

#endif /* INITSPI_H_ */