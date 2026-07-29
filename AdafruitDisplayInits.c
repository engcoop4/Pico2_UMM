#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/dma.h"
#include "AdafruitDisplayInits.h"
#include "hardware/pio.h"
#include "screen_spi.pio.h"

volatile int FLIP = 0;

extern const uint8_t console_font_12x16[];

// DMA display can be used when dealing with large areas (this is namely the "fill" functions such as LCD_Clear or Rectf)
// functions like LCD_DrawPixel should NOT be DMA, this will actually take longer than letting the CPU handle it
// functions like Rect (no fill) can go either way. opting to just use CPU so less code modifications required (subject to change)
static int display_dma_chan = -1;

extern PIO pio_global;
extern uint sm_global;
extern uint offset;

void LCD_selectLCD()
{
    gpio_put(PIN_CS, 0);
}

void LCD_deselectLCD()
{
    gpio_put(PIN_CS, 1);
}

// 2. The Delay
// FPROC/1000 logic on MSP430 is replaced by native millisecond sleep
void LCD_delay(unsigned int ms)
{
    sleep_ms(ms);
}

// 3. The Bus Write
// This replaces your UCB1TXBUF and while(UCBUSY) logic
void LCD_Write_Bus(unsigned char d)
{
    // 1. Push data to the PIO
    pio_sm_put_blocking(pio_global, sm_global, (uint32_t)d << 24);

    // 2. Wait for the FIFO to be empty (the "Inbox" is clear)
    while (!pio_sm_is_tx_fifo_empty(pio_global, sm_global))
        ;

    // 3. Wait for the Program Counter (PC) to return to the entry point.
    // This confirms the state machine has finished the 8-bit loop
    // and is now stalling/waiting for new data.
    while (pio_global->sm[sm_global].addr != (offset + screen_spi_get_entry_offset()))
        ;
}

// 4. Send Command
void LCD_writeCommand(unsigned char cmd)
{
#if defined(BOARD_TYPE_ADAFRUIT)
    Adafruit_writeCmd(cmd);
#elif defined(BOARD_TYPE_NEWHAVEN)
    NewHaven_writeCmd(cmd);
#endif
}

void Adafruit_writeCmd(unsigned char cmd)
{
    LCD_PIN_LOW_CMD;    // D/C Low
    LCD_selectLCD();    // CS Low
    LCD_Write_Bus(cmd); // Sends data
    sleep_us(1);
    LCD_deselectLCD(); // CS High
}

void NewHaven_writeCmd(unsigned char cmd)
{
    LCD_PIN_LOW_CMD;    // D/C Low
    LCD_selectLCD();    // CS Low
    LCD_Write_Bus(cmd); // Sends data
    sleep_us(2);        // Hold time before deasserting CS
    LCD_deselectLCD();  // CS High
    sleep_us(5);        // <--- CRITICAL: Give the display's SPI engine time to breathe!
}
// 5. Send Data
void LCD_writeData(unsigned char data)
{
#if defined(BOARD_TYPE_ADAFRUIT)
    Adafruit_writeData(data);
#elif defined(BOARD_TYPE_NEWHAVEN)
    NewHaven_writeData(data);
#endif
}

void Adafruit_writeData(unsigned char data)
{
    LCD_PIN_HI_DATA; // RS/DC = 1 (Macro from Hardware.h)
    LCD_selectLCD(); // CS = 0
    LCD_Write_Bus(data);
    sleep_us(1);
    LCD_deselectLCD(); // CS = 1
}

void NewHaven_writeData(unsigned char data)
{
    LCD_PIN_HI_DATA; // RS/DC = 1
    LCD_selectLCD(); // CS = 0
    LCD_Write_Bus(data);
    sleep_us(2);       // Hold time before deasserting CS
    LCD_deselectLCD(); // CS = 1
    sleep_us(5);       // <--- CRITICAL: Give the display's SPI engine time to breathe!
}

// ripped directly from CCS
void Screen_Init(void)
{
#if defined(BOARD_TYPE_ADAFRUIT)
    Adafruit_Init();
#elif defined(BOARD_TYPE_NEWHAVEN)
    NewHaven_Init();
#endif
}

void Adafruit_Init(void)
{
    RESETUP;
    LCD_delay(15);
    RESETDOWN;
    LCD_delay(15);
    RESETUP;
    LCD_delay(15); // if white screen check if state of reset lcd pin is being set correctly

    //    CSDOWN;  //CS

    LCD_writeCommand(0xCB); // PWR CNTRL A sets to 1.7V core
    LCD_writeData(0x39);
    LCD_writeData(0x2C);
    LCD_writeData(0x00);
    LCD_writeData(0x34);
    LCD_writeData(0x02);

    LCD_writeCommand(0xCF); // PWR CONTRL B - unsure if its 011 in ds
    LCD_writeData(0x00);
    LCD_writeData(0XC1); // original 0xC1
    LCD_writeData(0X30);

    LCD_writeCommand(0xE8); // Driver timing control A
    LCD_writeData(0x85);    // Should be 84, 11, 7A?
    LCD_writeData(0x00);    // original 85, 00, 78
    LCD_writeData(0x78);

    LCD_writeCommand(0xEA); // driver timing control B
    LCD_writeData(0x00);    // try 66 and 00?
    LCD_writeData(0x00);    // original 00, 00

    LCD_writeCommand(0xED); // power on sequence control
    LCD_writeData(0x64);    // try 55 01 23 1
    LCD_writeData(0x03);    // originally 64, 3, 12, 81
    LCD_writeData(0X12);
    LCD_writeData(0X81);

    LCD_writeCommand(0xF7);
    LCD_writeData(0x20); // try 10 original 20

    LCD_writeCommand(0xC0); // Power control
    LCD_writeData(0x23);    // 23 VRH[5:0]       /try 21     0010 0011

    LCD_writeCommand(0xC1); // Power control
    LCD_writeData(0x10);    // SAP[2:0];BT[3:0]

    LCD_writeCommand(0xC5); // VCM control
    LCD_writeData(0x31);    // Contrast           try 31 & 3C
    LCD_writeData(0x3C);    // original 3e and 28

    LCD_writeCommand(0xC7); // VCM control2
    LCD_writeData(0xC0);    //--   try C0 original 86

    LCD_writeCommand(0x36); // Memory Access Control         48 = 0100 1000 try 00
    LCD_writeData(0x88);    // 48    (28 -> mode paysage, 48 -> mode portrait)
                            // 88 portrait mode in OTHER direction

    LCD_writeCommand(0x3A); // Pixel format set
    LCD_writeData(0x55);    // 55

    LCD_writeCommand(0xB1); // frame rate control
    LCD_writeData(0x00);
    LCD_writeData(0x1B); // 18  try 1B

    LCD_writeCommand(0xB6); // Display Function Control
    LCD_writeData(0x08);    // try 0A original 08
    LCD_writeData(0x82);    // 82
    LCD_writeData(0x27);

    LCD_writeCommand(0x11); // Exit Sleep
    // 20260324RW: added delay seemingly ELIMINATES pixel offset. slightly slows updates, but required.
    // this delay is REQUIRED for LCDs. Do not remove/alter.
    LCD_delay(120); // 120 ms delay

    LCD_writeCommand(0x29); // Display on
    LCD_delay(20);
    LCD_writeCommand(0x2c);

    //    CSUP;
}

void NewHaven_Init(void)
{
    // Wake up driver core
    LCD_writeCommand(0x11); // Sleep Out (SLPOUT)
    sleep_ms(120);

    // Memory Data Access Control (MADCTL)
    LCD_writeCommand(0x36);
    LCD_writeData(0xC0);

    // Interface Pixel Format (COLMOD) -> 18-bit serial SPI formatting
    LCD_writeCommand(0x3A);
    LCD_writeData(0x06);

    // Display Inversion On (INVON)
    LCD_writeCommand(0x21);

    // --- CLEANLY FRAMED CONFIGURATION BLOCKS ---
    // Porch Setting (0xB2)
    LCD_writeCommand(0xB2);
    LCD_writeData(0x0C);
    LCD_writeData(0x0C);
    LCD_writeData(0x00);
    LCD_writeData(0x33);
    LCD_writeData(0x33);

    // Gate Control (0xB7)
    LCD_writeCommand(0xB7);
    LCD_writeData(0x35);

    // VCOM Setting (0xBB)
    LCD_writeCommand(0xBB);
    LCD_writeData(0x2B);

    // LCM Control (0xC0)
    LCD_writeCommand(0xC0);
    LCD_writeData(0x2C);

    // VDV and VRH Command Enable (0xC2)
    LCD_writeCommand(0xC2);
    LCD_writeData(0x01);
    LCD_writeData(0xFF);

    // VRH Set (0xC3)
    LCD_writeCommand(0xC3);
    LCD_writeData(0x11);

    // VDV Set (0xC4)
    LCD_writeCommand(0xC4);
    LCD_writeData(0x20);

    // Frame Rate Control in Normal Mode (0xC6)
    LCD_writeCommand(0xC6);
    LCD_writeData(0x0F);

    // Power Control 1 (0xD0)
    LCD_writeCommand(0xD0);
    LCD_writeData(0xA4);
    LCD_writeData(0xA1);

    // Positive Voltage Gamma Control (0xE0)
    LCD_writeCommand(0xE0);
    LCD_writeData(0xD0);
    LCD_writeData(0x00);
    LCD_writeData(0x05);
    LCD_writeData(0x0E);
    LCD_writeData(0x15);
    LCD_writeData(0x0D);
    LCD_writeData(0x37);
    LCD_writeData(0x43);
    LCD_writeData(0x47);
    LCD_writeData(0x09);
    LCD_writeData(0x15);
    LCD_writeData(0x01);
    LCD_writeData(0x16);
    LCD_writeData(0x19);

    // Negative Voltage Gamma Control (0xE1)
    LCD_writeCommand(0xE1);
    LCD_writeData(0xD0);
    LCD_writeData(0x00);
    LCD_writeData(0x05);
    LCD_writeData(0x0D);
    LCD_writeData(0x0C);
    LCD_writeData(0x06);
    LCD_writeData(0x2D);
    LCD_writeData(0x44);
    LCD_writeData(0x40);
    LCD_writeData(0x0E);
    LCD_writeData(0x1C);
    LCD_writeData(0x18);
    LCD_writeData(0x16);
    LCD_writeData(0x19);

    // Set Default Address Boundaries to Full Screen
    setCursor(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

    // Final Core Wake Activation Commands
    LCD_writeCommand(0x13); // Normal Display Mode On (NORON)
    sleep_ms(10);

    LCD_writeCommand(0x29); // Main Display Engine On (DISPON)
    sleep_ms(50);
}

void Screen_Setup(void)
{
#if defined(BOARD_TYPE_ADAFRUIT)
    Adafruit_Setup();
#elif defined(BOARD_TYPE_NEWHAVEN)
    NewHaven_Setup();
#endif
}

void Adafruit_Setup(void)
{
    RSUP;
    RESETUP;

    Screen_Init();
    LCD_Clear(BLACK);
}

void NewHaven_Setup(void)
{
    RSUP;
    RESETUP;

    NewHaven_Init();
    LCD_Clear(BLACK);
}

uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b, uint8_t *high_byte, uint8_t *low_byte)
{
    uint16_t color = 0;

    // Red (5 bits) - Shift red to the most significant 5 bits
    color |= ((r >> 3) << 11); // Red: Shift 8-bit down to 5-bit

    // Green (6 bits) - Shift green to the middle 6 bits
    color |= ((g >> 2) << 5); // Green: Shift 8-bit down to 6-bit

    // Blue (5 bits) - Shift blue to the least significant 5 bits
    color |= (b >> 3); // Blue: Shift 8-bit down to 5-bit

    *high_byte = (color >> 8) & 0xFF;
    *low_byte = color & 0xFF;

    return color;
}

void rgb888_to_bytes(uint32_t color, uint8_t *r_byte, uint8_t *g_byte, uint8_t *b_byte)
{
    // Extract full 8-bit Red channel and mask it
    *r_byte = (color >> 16) & 0xFF;

    // Extract full 8-bit Green channel and mask it
    *g_byte = (color >> 8) & 0xFF;

    // Extract full 8-bit Blue channel and mask it
    *b_byte = color & 0xFF;
}

void setCursor(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
#if defined(BOARD_TYPE_ADAFRUIT)
    unsigned int nx1 = (x1 * (1 - FLIP)) + ((239 - x1) * FLIP);
    unsigned int nx2 = (x2 * (1 - FLIP)) + ((239 - x2) * FLIP);
    unsigned int ny1 = (y1 * (1 - FLIP)) + ((319 - y1) * FLIP);
    unsigned int ny2 = (y2 * (1 - FLIP)) + ((319 - y2) * FLIP);

    unsigned int final_x_start = (nx1 < nx2) ? nx1 : nx2;
    unsigned int final_x_end = (nx1 > nx2) ? nx1 : nx2;
    unsigned int final_y_start = (ny1 < ny2) ? ny1 : ny2;
    unsigned int final_y_end = (ny1 > ny2) ? ny1 : ny2;

    LCD_writeCommand(0x2a); // Column Address Set
    LCD_writeData(final_x_start >> 8);
    LCD_writeData(final_x_start & 0xFF);
    LCD_writeData(final_x_end >> 8);
    LCD_writeData(final_x_end & 0xFF);

    LCD_writeCommand(0x2b); // Row Address Set
    LCD_writeData(final_y_start >> 8);
    LCD_writeData(final_y_start & 0xFF);
    LCD_writeData(final_y_end >> 8);
    LCD_writeData(final_y_end & 0xFF);

    LCD_writeCommand(0x2c); // Memory Write
#elif defined(BOARD_TYPE_NEWHAVEN)
    unsigned int nx1 = (x1 * (1 - FLIP)) + ((239 - x1) * FLIP);
    unsigned int nx2 = (x2 * (1 - FLIP)) + ((239 - x2) * FLIP);
    unsigned int ny1 = (y1 * (1 - FLIP)) + ((319 - y1) * FLIP);
    unsigned int ny2 = (y2 * (1 - FLIP)) + ((319 - y2) * FLIP);

    unsigned int final_x_start = (nx1 < nx2) ? nx1 : nx2;
    unsigned int final_x_end = (nx1 > nx2) ? nx1 : nx2;
    unsigned int final_y_start = (ny1 < ny2) ? ny1 : ny2;
    unsigned int final_y_end = (ny1 > ny2) ? ny1 : ny2;

    // 1. Column Address Set (Keep CS Low for all 5 bytes)
    LCD_selectLCD();
    LCD_PIN_LOW_CMD;
    LCD_Write_Bus(0x2A);
    LCD_PIN_HI_DATA;
    LCD_Write_Bus(final_x_start >> 8);
    LCD_Write_Bus(final_x_start & 0xFF);
    LCD_Write_Bus(final_x_end >> 8);
    LCD_Write_Bus(final_x_end & 0xFF);
    LCD_deselectLCD();

    // 2. Row Address Set (Keep CS Low for all 5 bytes)
    LCD_selectLCD();
    LCD_PIN_LOW_CMD;
    LCD_Write_Bus(0x2B);
    LCD_PIN_HI_DATA;
    LCD_Write_Bus(final_y_start >> 8);
    LCD_Write_Bus(final_y_start & 0xFF);
    LCD_Write_Bus(final_y_end >> 8);
    LCD_Write_Bus(final_y_end & 0xFF);
    LCD_deselectLCD();

    // 3. Open RAM Write Gates (0x2C)
    // Send the command, but DO NOT drop CS here because LCD_Clear needs to stream pixels immediately next!
    LCD_selectLCD();
    LCD_PIN_LOW_CMD;
    LCD_Write_Bus(0x2C);
    // Leave CS Low and D/C state handled by the start of your loop
#endif
}

static inline void format_color(uint32_t color)
{
#if defined(BOARD_TYPE_ADAFRUIT)
    uint8_t hi, lo;
    rgb888_to_rgb565((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, &hi, &lo);
    pio_sm_put_blocking(pio_global, sm_global, (uint32_t)hi << 24);
    pio_sm_put_blocking(pio_global, sm_global, (uint32_t)lo << 24);

#elif defined(BOARD_TYPE_NEWHAVEN)
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    pio_sm_put_blocking(pio_global, sm_global, (uint32_t)r << 24);
    pio_sm_put_blocking(pio_global, sm_global, (uint32_t)g << 24);
    pio_sm_put_blocking(pio_global, sm_global, (uint32_t)b << 24);
#endif
}

void LCD_DrawPixel(unsigned int x, unsigned int y, uint32_t color)
{
    // Bounds safety check (both panels are 240 x 320)
    if (x >= 240 || y >= 320)
        return;

    // 1. Set the 1x1 address window
    setCursor(x, y, x, y);

    // 2. Begin RAM Write command (0x2C is standard for both controllers)
    LCD_writeCommand(0x2C);

    // 3. Stream converted color bytes (Handles 16-bit vs 24-bit under the hood)
    format_color(color);
}

void H_line(unsigned int x, unsigned int y, unsigned int l, uint32_t color)
{
    if (l == 0)
        return;

    // 1. Set address window (1 pixel tall, 'l' pixels wide)
    setCursor(x, y, x + l - 1, y);

#if defined(BOARD_TYPE_ADAFRUIT)

    // Pre-convert 16-bit color bytes once outside the loop
    uint8_t hi, lo;
    rgb888_to_rgb565((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, &hi, &lo);

    // Stream 2 bytes per pixel
    for (unsigned int i = 0; i < l; i++)
    {
        LCD_writeData(hi);
        LCD_writeData(lo);
    }

#elif defined(BOARD_TYPE_NEWHAVEN)

    // Pre-extract 24-bit color bytes once outside the loop
    uint8_t r, g, b;
    rgb888_to_bytes(color, &r, &g, &b);

    // Stream 3 bytes per pixel
    for (unsigned int i = 0; i < l; i++)
    {
        LCD_writeData(r);
        LCD_writeData(g);
        LCD_writeData(b);
    }

#endif
}

void V_line(unsigned int x, unsigned int y, unsigned int l, uint32_t color)
{
    if (l == 0)
        return;

    // 1. Set address window (1 pixel wide, 'l' pixels tall)
    setCursor(x, y, x, y + l - 1);

#if defined(BOARD_TYPE_ADAFRUIT)

    // Pre-convert 16-bit color bytes once outside the loop
    uint8_t hi, lo;
    rgb888_to_rgb565((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, &hi, &lo);

    // Stream 2 bytes per pixel
    for (unsigned int i = 0; i < l; i++)
    {
        LCD_writeData(hi);
        LCD_writeData(lo);
    }

#elif defined(BOARD_TYPE_NEWHAVEN)

    // Pre-extract 24-bit color bytes once outside the loop
    uint8_t r, g, b;
    rgb888_to_bytes(color, &r, &g, &b);

    // Stream 3 bytes per pixel
    for (unsigned int i = 0; i < l; i++)
    {
        LCD_writeData(r);
        LCD_writeData(g);
        LCD_writeData(b);
    }

#endif
}

// Draw the four sides of the rectangle - Needs to be flipped
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t color)
{
    H_line(x, y, w, color);     // Top horizontal line
    H_line(x, y + h, w, color); // Bottom horizontal line
    V_line(x, y, h, color);     // Left vertical line
    V_line(x + w, y, h, color); // Right vertical line
}

void Rectf(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
    // 1. Set bounding window ONCE for the entire box area
    setCursor(x, y, x + w - 1, y + h - 1);

    LCD_PIN_HI_DATA;
    LCD_selectLCD();

    uint32_t total_pixels = (uint32_t)w * h;

#if defined(BOARD_TYPE_ADAFRUIT)

    uint8_t hi, lo;
    rgb888_to_rgb565((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, &hi, &lo);

    uint32_t phi = (uint32_t)hi << 24;
    uint32_t plo = (uint32_t)lo << 24;

    // Stream 2 bytes per pixel for RGB565
    for (uint32_t i = 0; i < total_pixels; i++) {
        pio_sm_put_blocking(pio_global, sm_global, phi);
        pio_sm_put_blocking(pio_global, sm_global, plo);
    }

#elif defined(BOARD_TYPE_NEWHAVEN)

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    uint32_t pr = (uint32_t)r << 24;
    uint32_t pg = (uint32_t)g << 24;
    uint32_t pb = (uint32_t)b << 24;

    // Stream 3 bytes per pixel for RGB888
    for (uint32_t i = 0; i < total_pixels; i++) {
        pio_sm_put_blocking(pio_global, sm_global, pr);
        pio_sm_put_blocking(pio_global, sm_global, pg);
        pio_sm_put_blocking(pio_global, sm_global, pb);
    }

#endif

    // Completion check using the target-specific entry offset
    while (!pio_sm_is_tx_fifo_empty(pio_global, sm_global)) ;
    while (pio_global->sm[sm_global].addr != (offset + screen_spi_get_entry_offset())) ;
    sleep_us(10);

    LCD_deselectLCD();
}

// just outline
void Circle(unsigned int x, unsigned int y, unsigned int r, uint32_t color)
{
    int x1 = 0;
    int y1 = r;
    int d = 3 - 2 * r;

    while (y1 >= x1)
    {
        LCD_DrawPixel(x - x1, y + y1, color);
        LCD_DrawPixel(x + x1, y + y1, color);

        LCD_DrawPixel(x - x1, y - y1, color);
        LCD_DrawPixel(x + x1, y - y1, color);

        LCD_DrawPixel(x - y1, y + x1, color);
        LCD_DrawPixel(x + y1, y + x1, color);

        LCD_DrawPixel(x - y1, y - x1, color);
        LCD_DrawPixel(x + y1, y - x1, color);

        if (d < 0)
        {
            d = d + 4 * x1 + 6;
        }
        else
        {
            d = d + 4 * (x1 - y1) + 10;
            y1--;
        }
        x1++;
    }
}

/*ARDUINO CODE FOR REFERENCE*/
/*
int16_t f = 1 - r;
int16_t ddF_x = 1;
int16_t ddF_y = -2 * r;
int16_t x1 = 0;
int16_t y1 = r;

H_line(x  , y + r, 1, color);
H_line(x  , y - r, 1, color);
H_line(x +r, y  , 1, color);
H_line(x -r, y  , 1, color);

while (x < y) {
  if (f >= 0) {
    y--;
    ddF_y += 2;
    f += ddF_y;
  }
  x++;
  ddF_x += 2;
  f += ddF_x;

  H_line(x + x1, y + y1, 1, color);
  H_line(x - x1, y + y1, 1, color);
  H_line(x + x1, y - y1, 1, color);
  H_line(x - x1, y - y1, 1, color);
  H_line(x + y1, y + x1, 1, color);
  H_line(x - y1, y + x1, 1, color);
  H_line(x + y1, y - x1, 1, color);
  H_line(x - y1, y - x1, 1, color);
}
*/

// circle fill
void Circlef(unsigned int x, unsigned int y, unsigned int r, uint32_t color)
{
    int x1 = 0;
    int y1 = r;
    int d = 3 - 2 * r;

    while (y1 >= x1)
    {
        H_line(x - x1, y + y1, 2 * x1, color);
        H_line(x - x1, y - y1, 2 * x1, color);
        H_line(x - y1, y + x1, 2 * y1, color);
        H_line(x - y1, y - x1, 2 * y1, color);

        if (d < 0)
        {
            d = d + 4 * x1 + 6;
        }
        else
        {
            d = d + 4 * (x1 - y1) + 10;
            y1--;
        }
        x1++;
    }
}

void swap(int16_t *a, int16_t *b)
{
    if (a == NULL || b == NULL)
        return;
    int temp = *a;
    *a = *b;
    *b = temp;
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color)
{
    int steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep)
    {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }

    if (x0 > x1)
    {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }
    int dx, dy, err, ystep;
    dx = x1 - x0;
    dy = abs(y1 - y0);
    err = dx / 2;

    if (y0 < y1)
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }
    for (; x0 <= x1; x0++)
    {
        if (steep)
        {
            LCD_DrawPixel(y0, x0, color);
        }
        else
        {
            LCD_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}

// no fill, just a triangle outline
void Triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color)
{
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void Trianglef(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color)
{
    /* ARDUINO USED FOR REFERENCE */

    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1)
    {
        swap(&y0, &y1);
        swap(&x0, &x1);
    }
    if (y1 > y2)
    {
        swap(&y2, &y1);
        swap(&x2, &x1);
    }
    if (y0 > y1)
    {
        swap(&y0, &y1);
        swap(&x0, &x1);
    }

    if (y0 == y2)
    { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if (x1 < a)
            a = x1;
        else if (x1 > b)
            b = x1;
        if (x2 < a)
            a = x2;
        else if (x2 > b)
            b = x2;
        H_line(a, y0, b - a + 1, color);
        return;
    }

    int16_t
        dx01 = x1 - x0,
        dy01 = y1 - y0,
        dx02 = x2 - x0,
        dy02 = y2 - y0,
        dx12 = x2 - x1,
        dy12 = y2 - y1;
    int32_t
        sa = 0,
        sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2)
        last = y1; // Include y1 scanline
    else
        last = y1 - 1; // Skip it

    for (y = y0; y <= last; y++)
    {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        // longhand:
        // a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        // b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);

        if (a > b)
            swap(&a, &b);
        H_line(a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++)
    {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        // longhand:
        // a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        // b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);

        if (a > b)
            swap(&a, &b);
        H_line(a, y, b - a + 1, color);
    }
}

void drawChar(int16_t x, int16_t y, unsigned char c,
              uint32_t color, uint32_t bg, uint8_t size_x,
              uint8_t size_y)
{
    // 1. Calculate full block dimensions including scaling
    uint16_t total_width = 13 * size_x;
    uint16_t total_height = 16 * size_y;

    // 2. Set ONE bounding box window for the entire character block
    setCursor(x, y, x + total_width - 1, y + total_height - 1);

    // 3. Prepare display CS and DC lines for raw data phase
    LCD_PIN_HI_DATA;
    LCD_selectLCD();

    // 4. Pre-shift 32-bit words for direct PIO FIFO pushes
#if defined(BOARD_TYPE_ADAFRUIT)

    uint8_t fg_hi, fg_lo;
    uint8_t bg_hi, bg_lo;

    rgb888_to_rgb565((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, &fg_hi, &fg_lo);
    rgb888_to_rgb565((bg >> 16) & 0xFF, (bg >> 8) & 0xFF, bg & 0xFF, &bg_hi, &bg_lo);

    uint32_t p_fg_hi = (uint32_t)fg_hi << 24;
    uint32_t p_fg_lo = (uint32_t)fg_lo << 24;
    uint32_t p_bg_hi = (uint32_t)bg_hi << 24;
    uint32_t p_bg_lo = (uint32_t)bg_lo << 24;

#elif defined(BOARD_TYPE_NEWHAVEN)

    uint8_t fg_r, fg_g, fg_b;
    uint8_t bg_r, bg_g, bg_b;

    rgb888_to_bytes(color, &fg_r, &fg_g, &fg_b);
    rgb888_to_bytes(bg, &bg_r, &bg_g, &bg_b);

    uint32_t p_fg_r = (uint32_t)fg_r << 24;
    uint32_t p_fg_g = (uint32_t)fg_g << 24;
    uint32_t p_fg_b = (uint32_t)fg_b << 24;

    uint32_t p_bg_r = (uint32_t)bg_r << 24;
    uint32_t p_bg_g = (uint32_t)bg_g << 24;
    uint32_t p_bg_b = (uint32_t)bg_b << 24;

#endif

    int8_t i, j, sx, sy;

    // 5. Loop through font rows
    for (j = 0; j < 16; j++)
    {
        uint8_t byte1 = pgm_read_byte(&console_font_12x16[c * 32 + j * 2]);
        uint8_t byte2 = pgm_read_byte(&console_font_12x16[c * 32 + j * 2 + 1]);
        uint16_t row = (byte1 << 8) | byte2;

        for (sy = 0; sy < size_y; sy++)
        {
            // Stream the 12 font columns
            for (i = 0; i < 12; i++)
            {
                bool is_fg = (row & (0x8000 >> i)) != 0;

                for (sx = 0; sx < size_x; sx++)
                {
#if defined(BOARD_TYPE_ADAFRUIT)
                    pio_sm_put_blocking(pio_global, sm_global, is_fg ? p_fg_hi : p_bg_hi);
                    pio_sm_put_blocking(pio_global, sm_global, is_fg ? p_fg_lo : p_bg_lo);
#elif defined(BOARD_TYPE_NEWHAVEN)
                    pio_sm_put_blocking(pio_global, sm_global, is_fg ? p_fg_r : p_bg_r);
                    pio_sm_put_blocking(pio_global, sm_global, is_fg ? p_fg_g : p_bg_g);
                    pio_sm_put_blocking(pio_global, sm_global, is_fg ? p_fg_b : p_bg_b);
#endif
                }
            }

            // 13th column spacer (always background color)
            for (sx = 0; sx < size_x; sx++)
            {
#if defined(BOARD_TYPE_ADAFRUIT)
                pio_sm_put_blocking(pio_global, sm_global, p_bg_hi);
                pio_sm_put_blocking(pio_global, sm_global, p_bg_lo);
#elif defined(BOARD_TYPE_NEWHAVEN)
                pio_sm_put_blocking(pio_global, sm_global, p_bg_r);
                pio_sm_put_blocking(pio_global, sm_global, p_bg_g);
                pio_sm_put_blocking(pio_global, sm_global, p_bg_b);
#endif
            }
        }
    }

    // 6. Finish transaction and release CS
    while (!pio_sm_is_tx_fifo_empty(pio_global, sm_global)) ;
    while (pio_global->sm[sm_global].addr != (offset + screen_spi_get_entry_offset())) ;
    sleep_us(10);

    LCD_deselectLCD();
}

void LCD_Clear(uint32_t color)
{
    // 1. Set full screen bounds (handles its own CS internally)
    setCursor(0, 0, 239, 319);

    // 2. Prepare hardware lines for raw data streaming phase
    LCD_PIN_HI_DATA;
    LCD_selectLCD();
    sleep_us(5); // Setup safety margin

    // 3. Prepare display-specific PIO words and byte counts
#if defined(BOARD_TYPE_ADAFRUIT)

    uint8_t hi, lo;
    rgb888_to_rgb565((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, &hi, &lo);

    uint32_t pio_words[2] = {
        (uint32_t)hi << 24,
        (uint32_t)lo << 24};
    const uint8_t bytes_per_pixel = 2;

#elif defined(BOARD_TYPE_NEWHAVEN)

    uint8_t r, g, b;
    rgb888_to_bytes(color, &r, &g, &b);

    uint32_t pio_words[3] = {
        (uint32_t)r << 24,
        (uint32_t)g << 24,
        (uint32_t)b << 24};
    const uint8_t bytes_per_pixel = 3;

#endif

    // 4. Unified PIO Flood Loop (240 x 320 = 76,800 pixels)
    for (uint32_t i = 0; i < (240 * 320); i++)
    {
        for (uint8_t b_idx = 0; b_idx < bytes_per_pixel; b_idx++)
        {
            pio_sm_put_blocking(pio_global, sm_global, pio_words[b_idx]);
        }
    }

    // 5. Unified Completion Check (Drains FIFO + OSR completely)

    // Step A: Wait until the TX FIFO queue is completely empty
    // Wait for FIFO to drain
    while (!pio_sm_is_tx_fifo_empty(pio_global, sm_global))
        ;

    // Wait for state machine to wrap back to the active entry instruction
    while (pio_global->sm[sm_global].addr != (offset + screen_spi_get_entry_offset()))
        ;

    sleep_us(10); // Allow final byte to shift out over SCLK pin
    LCD_deselectLCD();

    // Step E: Pulse CS briefly to reset the controller's internal byte-framing logic
    sleep_us(1);
    LCD_selectLCD();
    sleep_us(1);
    LCD_deselectLCD();
}

void print(int16_t x, int16_t y, const char *str, uint32_t color, uint32_t bg, uint8_t size_x, uint8_t size_y, uint16_t screen_width)
{
    int16_t cursorX = x;
    int16_t cursorY = y;
    uint8_t i = 0;

    while (str[i])
    {
        // if(str[i] != '\0') removed for being redundant to while str[i]
        // search code for space, once space identified scan until the next space is encountered
        if (str[i] == ' ')
        {
            int next_word_end = i + 1;
            // find the end of the next word
            while (str[next_word_end] != ' ' && str[next_word_end] != '\0')
            {
                next_word_end++; // increment through string until next space is encountered (or string is terminated)
            }

            int next_word_len = next_word_end - (i + 1); // compensate offset
            int next_word_width = next_word_len * (size_x * FONT_WIDTH);

            //
            if (cursorX + (size_x * FONT_WIDTH) + next_word_width > screen_width)
            {
                cursorX = x; // Reset to start of line
                cursorY += size_y * FONT_HEIGHT;
                i++;      // skip the space itself so the new line doesn't start with a ' '
                continue; // Jump to start of loop to draw the first char of the word
            }
        }
        //
        drawChar(cursorX, cursorY, str[i], color, bg, size_x, size_y);
        cursorX += size_x * FONT_WIDTH;

        // previous code implementation still included in case one single word extends the entire screen and needs to be split into two
        if (cursorX + size_x * FONT_WIDTH > screen_width)
        {
            cursorX = x;
            cursorY += size_y * FONT_HEIGHT;
        }
        i++;
    }
}

// modified code for centered text, only used in few positions and only used for text in very center of screen
// may be eventually implemented in normal print function, but the "starting x" value messes with the centering of the text
void print_centered(int16_t y, const char *str, uint32_t color, uint32_t bg, uint8_t size_x, uint8_t size_y, uint16_t screen_width)
{
    int16_t cursorY = y;
    int i = 0;
    uint16_t char_width = size_x * FONT_WIDTH;

    while (str[i] != '\0')
    {
        int line_start = i;
        int line_end = i;
        int current_line_width = 0;

        // --- PASS 1: Look ahead to see what fits on this line ---
        while (str[i] != '\0')
        {
            int word_start = i;
            while (str[i] != ' ' && str[i] != '\0')
                i++; // Find end of word
            int word_end = i;

            int word_len = word_end - word_start;
            int word_pixel_width = word_len * char_width;

            // Check if word fits (including a space if not the first word)
            int space_extra = (current_line_width > 0) ? char_width : 0;

            if (current_line_width + space_extra + word_pixel_width <= screen_width)
            {
                current_line_width += space_extra + word_pixel_width;
                line_end = i;
                if (str[i] == ' ')
                    i++; // Move past space for next word check
            }
            else
            {
                // Word doesn't fit, this line is done.
                // Don't increment i; start next line with this word.
                i = word_start;
                break;
            }
        }

        // --- PASS 2: Calculate Offset and Draw the Line ---
        int16_t startX = (screen_width - current_line_width) / 2;
        int16_t cursorX = startX;
        int j;
        for (j = line_start; j < line_end; j++)
        {
            drawChar(cursorX, cursorY, str[j], color, bg, size_x, size_y);
            cursorX += char_width;
        }

        // Move cursor to next line
        cursorY += size_y * FONT_HEIGHT;

        // Skip the space at the end of the line if there is one
        if (str[i] == ' ')
            i++;
    }
}

// x1 indicates offset from left side, x2 indicates rectangle width, str is string being used
// FindCenterX and FindCenterY implemented as separate functions so that print can still have the coordinates manually chosen if needed
// Font consideration implemented
int FindCenterX(int16_t x1, int16_t x2, const char *str, uint8_t font_size)
{
    int last_space = 0;
    int calc_x;
    int i;

    // calculate total width produced by character string (10 for each character and 2 for buffer space between characters)
    int total_width = (font_size * FONT_WIDTH * strlen(str)); //(10 * strlen(str)) + (2 * (strlen(str) - 1));

    // if this width is less than rectangle width (most often the case), perform equation calculation
    if (total_width <= x2)
    {
        calc_x = (x2 + (2 * x1) - total_width) * 0.5 - (1 * font_size);
    }
    // if total width is greater than rectangle width, find last location of space closest to edge
    // error is coming from print wrap-around function placing character in top row
    else
    {
        for (i = 0; i < strlen(str); i++)
        {
            if (str[i] == ' ')
            {
                int current_width = font_size * FONT_WIDTH * i;
                if (current_width < x2)
                {
                    last_space = i;
                }
                else
                {
                    break;
                }
            }
        }
        int line1_width = (font_size * FONT_WIDTH * last_space); //(10 * last_space) + (2 * (last_space - 1));
        calc_x = (x2 + (2 * x1) - line1_width) * 0.5;
    }
    return calc_x;
}

// y1 indicates offset from top, y2 indicates rectangle height
// implemented to consider font size
// if string goes off screen, calculates how many lines are required and adjusts offset accordingly
int FindCenterY(int16_t y1, int16_t y2, const char *str, uint8_t font)
{
    int calc_y;
    int scale = strlen(str) * 0.0526315789473684;

    calc_y = (strlen(str) > 19) ? y1 + (y2 * 0.5) - (7 * font) - (7 * scale) : y1 + (y2 * 0.5) - (7 * font);

    return calc_y;
}
