#include "ADCSPI.h"
#include <stdio.h>

static inline void ads131_cs_select(void) {
    gpio_put(ADS131_CS, 0);
}

static inline void ads131_cs_deselect(void) {
    gpio_put(ADS131_CS, 1);
}

void ads131_init(void) {
    // 1. Initialize Chip Select Pin (Drive HIGH initially)
    gpio_init(ADS131_CS);
    gpio_set_dir(ADS131_CS, GPIO_OUT);
    ads131_cs_deselect();

    // 2. Initialize SPI1 Peripheral
    spi_init(ADS131_SPI_PORT, ADS131_BAUDRATE);
    spi_set_format(ADS131_SPI_PORT, 8, ADS131_CPOL, ADS131_CPHA, SPI_MSB_FIRST);

    gpio_set_function(ADS131_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(ADS131_DIN,  GPIO_FUNC_SPI);
    gpio_set_function(ADS131_DOUT, GPIO_FUNC_SPI);

    // 3. Initialize DRDY Pin (GP6 as Input)
    gpio_init(ADS131_DRDY);
    gpio_set_dir(ADS131_DRDY, GPIO_IN);

    // 4. Perform ADS131 Soft Reset Frame
    sleep_ms(10);
    ads131_cs_select();
    uint8_t reset_cmd[3] = {0x00, 0x11, 0x00};
    spi_write_blocking(ADS131_SPI_PORT, reset_cmd, 3);
    
    uint8_t dummy[3] = {0x00, 0x00, 0x00};
    for (int i = 1; i < ADS131_FRAME_WORDS; i++) {
        spi_write_blocking(ADS131_SPI_PORT, dummy, 3);
    }
    ads131_cs_deselect();
    sleep_ms(5);

    // 5. Enable DRDY Falling Edge Interrupt (Shared ISR Model)
    // Note: gpio_set_irq_enabled() DOES NOT overwrite master_gpio_irq_dispatcher!
    gpio_set_irq_enabled(ADS131_DRDY, GPIO_IRQ_EDGE_FALL, true);
}

// Read a 16-bit register value from the ADS131M08
uint16_t ads131_read_register(uint8_t reg_addr) {
    // 1. Construct RREG Opcode: 0xA000 | (reg_addr << 7)
    uint16_t opcode = 0xA000 | ((uint16_t)reg_addr << 7);
    
    uint8_t tx_buf[30] = {0};
    uint8_t rx_buf[30] = {0};

    // Word 0 holds the 16-bit command (MSB first)
    tx_buf[0] = (opcode >> 8) & 0xFF;
    tx_buf[1] = opcode & 0xFF;

    // --- Frame 1: Send RREG Command Request ---
    ads131_cs_select();
    spi_write_read_blocking(ADS131_SPI_PORT, tx_buf, rx_buf, 30);
    ads131_cs_deselect();

    // Small delay between SPI frames
    sleep_us(1);

    // --- Frame 2: Clock out NULL Command to collect the Response ---
    // Clear tx_buf to send NULL command (0x0000)
    tx_buf[0] = 0x00;
    tx_buf[1] = 0x00;

    ads131_cs_select();
    spi_write_read_blocking(ADS131_SPI_PORT, tx_buf, rx_buf, 30);
    ads131_cs_deselect();

    // The register response value is returned in Word 0 (bytes 0 and 1)
    return ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
}

// Write a 16-bit value to a single register
void ads131_write_register(uint8_t reg_addr, uint16_t reg_value) {
    // Construct WREG opcode: 0x6000 | (reg_addr << 7)
    uint16_t opcode = 0x6000 | ((uint16_t)reg_addr << 7);

    // Create a buffer for 10 words (30 bytes total, 24 bits per word)
    uint8_t tx_buf[30] = {0};

    // Word 0: Write command opcode
    tx_buf[0] = (opcode >> 8) & 0xFF;
    tx_buf[1] = opcode & 0xFF;

    // Word 1: 16-bit payload/register value
    tx_buf[3] = (reg_value >> 8) & 0xFF;
    tx_buf[4] = reg_value & 0xFF;

    // Send the complete command frame
    ads131_cs_select();
    spi_write_blocking(ADS131_SPI_PORT, tx_buf, 30);
    ads131_cs_deselect();
}

// Helper function: converts 3 raw SPI bytes (24-bit 2's complement) into a signed int32_t
static inline int32_t raw_24_to_int32(const uint8_t *bytes) {
    uint32_t val = ((uint32_t)bytes[0] << 16) | 
                   ((uint32_t)bytes[1] << 8)  | 
                    (uint32_t)bytes[2];

    // If Bit 23 is 1 (negative number), sign-extend bits 31:24 with 1s
    if (val & 0x00800000) {
        val |= 0xFF000000;
    }

    return (int32_t)val;
}

// Read a full conversion frame (Status + 8 Channels + CRC)
bool ads131_read_frame(ads131_frame_t *frame) {
    if (frame == NULL) return false;

    uint8_t tx_buf[30] = {0}; // Clocks out NULL commands
    uint8_t rx_buf[30] = {0}; // Receives status, 8 channels, and CRC

    // Clock out 30 bytes to pull in the frame
    ads131_cs_select();
    spi_write_read_blocking(ADS131_SPI_PORT, tx_buf, rx_buf, 30);
    ads131_cs_deselect();

    // Word 0: Status word (bytes 0 & 1)
    frame->status = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];

    // Words 1 through 8: Channels 0 to 7 (3 bytes per channel)
    for (int ch = 0; ch < ADS131_NUM_CHANNELS; ch++) {
        int byte_idx = 3 + (ch * 3); // Channel 0 starts at byte offset 3
        frame->channel[ch] = raw_24_to_int32(&rx_buf[byte_idx]);
    }

    // Word 9: CRC Checksum (bytes 27 & 28)
    frame->crc = ((uint16_t)rx_buf[27] << 8) | rx_buf[28];

    return true;
}
