#include "initSPI.h"
#include "Hardware.h"

void SPI_init()
{
    // initialize SPI hardware
    // changing baud rate directly changes speed (16 MHz to match speed of msp430)
    spi_init(SPI_PORT, SPI_BAUD_RATE);

    // setup SPI pins - pick pin function
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);  // declares GPIO10 as SPI (like multiplexing the pin to use its secondary capability rather than just GPIO)
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI); // declares GPIO11 as SPI
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI); // declares GPIO12 as SPI

    // setup chip select (will need to be modified when connected to metering chip) - which device is being spoken to
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT); // set PIN_CS as output (like P1DIR in CCS)
    gpio_put(PIN_CS, 1);            // deselected (CS is an active LOW signal, so 1 disables it)

    // setup LCD control pins (replaces LCD_DCinit and P11_5_MODE) - data/command
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT); // set PIN_DC as output
    RSUP;                           // gpio_put(PIN_DC, 1)

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    RESETUP; // gpio_put(PIN_RST, 1)
}