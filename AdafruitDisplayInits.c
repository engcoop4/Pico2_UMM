#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/dma.h"
#include "AdafruitDisplayInits.h"

volatile int FLIP = 0;

extern const uint8_t console_font_12x16[];

// DMA display can be used when dealing with large areas (this is namely the "fill" functions such as LCD_Clear or Rectf)
// functions like draw_pixel should NOT be DMA, this will actually take longer than letting the CPU handle it
// functions like Rect (no fill) can go either way. opting to just use CPU so less code modifications required (subject to change)
static int display_dma_chan = -1;

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
void Lcd_Write_Bus(unsigned char d)
{
    // spi_write_blocking handles the "while busy" check internally
    spi_write_blocking(SPI_PORT, &d, 1);
}

// 4. Send Command
void LCD_writeCommand(unsigned char cmd)
{
    LCD_PIN_LOW_CMD; // RS/DC = 0 (Macro from Hardware.h)
    LCD_selectLCD(); // CS = 0
    Lcd_Write_Bus(cmd);
    LCD_deselectLCD(); // CS = 1
}

// 5. Send Data
void LCD_writeData(unsigned char data)
{
    LCD_PIN_HI_DATA; // RS/DC = 1 (Macro from Hardware.h)
    LCD_selectLCD(); // CS = 0
    Lcd_Write_Bus(data);
    LCD_deselectLCD(); // CS = 1
}

// ripped directly from CCS
void Lcd_Init(void)
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

void setCursor(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
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
}

void format_color(uint32_t color)
{
    // Extract the RGB components from the 24-bit color integer
    unsigned char r = (color >> 16) & 0xFF; // Extract Red (bits 16-23)
    unsigned char g = (color >> 8) & 0xFF;  // Extract Green (bits 8-15)
    unsigned char b = color & 0xFF;         // Extract Blue (bits 0-7)

    // Convert from RGB888 to RGB565 and store in high_byte, low_byte
    uint8_t high_byte, low_byte;
    rgb888_to_rgb565(r, g, b, &high_byte, &low_byte);

    // Write the converted color data to the LCD
    LCD_writeData(high_byte);
    LCD_writeData(low_byte);
}

void draw_pixel(unsigned int x, unsigned int y, uint32_t color)
{
    // 1. Set the area first (a 1x1 box at x,y)
    setCursor(x, y, x, y);

    // 2. Tell the LCD we are about to send color data
    LCD_writeCommand(0x2C);

    // 3. Send the color
    format_color(color);
}

// Determines horizontal placement of pixel
void H_line(unsigned int x, unsigned int y, unsigned int l, uint32_t color)
{

    unsigned int i;
    LCD_writeCommand(0x02c);   // Write memory start
    setCursor(x, y, x + l, y); // Set cursor to the horizontal line's starting position

    for (i = 1; i <= l; i++)
    {
        format_color(color); // color each pixel along the horizontal line
    }
}

// Determines vertical placement of pixel - Needs to be flipped
void V_line(unsigned int x, unsigned int y, unsigned int l, uint32_t color)
{

    unsigned int i;
    LCD_writeCommand(0x02c);   // Write memory start
    setCursor(x, y, x, y + l); // Set cursor to the line's starting position

    for (i = 1; i <= l; i++)
    {
        format_color(color); // Draw each pixel along the vertical line
    }
}

// Draw the four sides of the rectangle - Needs to be flipped
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t color)
{
    H_line(x, y, w, color);     // Top horizontal line
    H_line(x, y + h, w, color); // Bottom horizontal line
    V_line(x, y, h, color);     // Left vertical line
    V_line(x + w, y, h, color); // Right vertical line
}

// Fill a rectangle color (same color as border unless followed by Rect function) - Needs to be flipped
void Rectf(unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t color)
{

    unsigned int i;
    for (i = 0; i < h; i++)
    {
        H_line(x, y + i, w, color); // Draw a horizontal line for each row in the rectangle
    }
}

// just outline
void Circle(unsigned int x, unsigned int y, unsigned int r, uint32_t color)
{
    int x1 = 0;
    int y1 = r;
    int d = 3 - 2 * r;

    while (y1 >= x1)
    {
        draw_pixel(x - x1, y + y1, color);
        draw_pixel(x + x1, y + y1, color);

        draw_pixel(x - x1, y - y1, color);
        draw_pixel(x + x1, y - y1, color);

        draw_pixel(x - y1, y + x1, color);
        draw_pixel(x + y1, y + x1, color);

        draw_pixel(x - y1, y - x1, color);
        draw_pixel(x + y1, y - x1, color);

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
        return; // Safety check
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
            draw_pixel(y0, x0, color);
        }
        else
        {
            draw_pixel(x0, y0, color);
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

// Draws individual characters
void drawChar(int16_t x, int16_t y, unsigned char c,
              uint32_t color, uint32_t bg, uint8_t size_x,
              uint8_t size_y)
{
    //   CSDOWN;
    int8_t i, j;

    // Access the bitmap for the given character (sequentially in 1D array)
    for (j = 0; j < 16; j++)
    { // 16 rows per character
        // Access the two bytes for this row (12 columns = 12 bits per row)
        uint8_t byte1 = pgm_read_byte(&console_font_12x16[c * 32 + j * 2]);     // First byte (8 bits)
        uint8_t byte2 = pgm_read_byte(&console_font_12x16[c * 32 + j * 2 + 1]); // Second byte (8 bits)

        // Combine the two bytes to form a 16-bit value (use the first 12 bits)
        uint16_t row = (byte1 << 8) | byte2; // Combine two 8-bit values into one 16-bit value

        // Iterate through the 12 columns (12 bits per row)
        for (i = 0; i < 12; i++)
        {
            if (row & (0x8000 >> i))
            { // Check if the corresponding bit is set in the row
                if (size_x == 1 && size_y == 1)
                {
                    setCursor(x + i, y + j, x + i, y + j);
                    format_color(color); // Draw the foreground color (text color)
                }
                else
                {
                    Rectf(x + i * size_x, y + j * size_y, size_x, size_y, color); // Draw character with scaling
                }
            }
            else if (bg != color)
            { // Draw background only if it's different from the text color
                if (size_x == 1 && size_y == 1)
                {
                    setCursor(x + i, y + j, x + i, y + j);
                    format_color(bg); // Draw the background color
                }
                else
                {
                    Rectf(x + i * size_x, y + j * size_y, size_x, size_y, bg); // Draw background color with scaling
                }
            }
        }
    }

    // Draw the last column for the background if necessary
    if (bg != color)
    {
        if (size_x == 1 && size_y == 1)
        {
            V_line(x + 12, y, 16, bg); // Draw background color for the last column
        }
        else
        {
            Rectf(x + 12 * size_x, y, size_x, 16 * size_y, bg); // Background for last column
        }
    }

    //  CSUP;
}


// claim unused dma channel
void LCD_DMA_Init()
{
    // Only claim a channel if we haven't already
    if (display_dma_chan == -1)
    {
        display_dma_chan = dma_claim_unused_channel(true);
        // Now display_dma_chan might be 0, 1, or 2... no longer -1!
    }
}

void LCD_Clear(uint32_t color)
{
    // 1. Prepare the window
    setCursor(0, 0, 239, 319);

    // Wait for setCursor's 0x2C command to actually leave the SPI wires
    while (spi_is_busy(SPI_PORT))
    {
        tight_loop_contents();
    }

    // 2. Pre-calculate the 16-bit color using your logic
    uint8_t hi, lo;
    rgb888_to_rgb565(
        (color >> 16) & 0xFF,
        (color >> 8) & 0xFF,
        color & 0xFF,
        &hi, &lo);

    // Create a 2-byte array for the DMA to "loop" over
    // ILI9341 expects High Byte then Low Byte
    uint8_t color_bytes[2] = {hi, lo};

    // 3. Set D/C to Data mode
    LCD_PIN_HI_DATA;
    LCD_selectLCD();

    // 4. Configure DMA
    dma_channel_config c = dma_channel_get_default_config(display_dma_chan);

    // Change to 8-bit to ensure the SPI FIFO accepts it correctly
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_index(SPI_PORT) ? DREQ_SPI1_TX : DREQ_SPI0_TX);

    // We want the DMA to read hi, then lo, then hi, then lo...
    // So we enable "Ring" wrapping on the read side (size of 2 bytes)
    channel_config_set_read_increment(&c, true);
    channel_config_set_ring(&c, false, 1); // 2^1 = 2 bytes wrap

    dma_channel_configure(
        display_dma_chan,
        &c,
        &spi_get_hw(SPI_PORT)->dr, // Destination
        color_bytes,               // Source: Our 2-byte array
        240 * 320 * 2,             // Count: 2 bytes per pixel, so * 2 is REQUIRED
        true);

    dma_channel_wait_for_finish_blocking(display_dma_chan);
    LCD_deselectLCD();

    /* //original code implemented from CCS
    unsigned int ii, mm;
    setCursor(0, 0, 239, 319);
    for (ii = 0; ii < 240; ii++)
    {
        for (mm = 0; mm < 320; mm++)
        {
            format_color(color);
        }
    }
    */
}

void print(int16_t x, int16_t y, const char* str, uint32_t color, uint32_t bg, uint8_t size_x, uint8_t size_y, uint16_t screen_width) {
    int16_t cursorX = x;
    int16_t cursorY = y;
    uint8_t i = 0;

    while (str[i]) {
        // if(str[i] != '\0') removed for being redundant to while str[i]
        // search code for space, once space identified scan until the next space is encountered
        if (str[i] == ' ') {
            int next_word_end = i + 1;
            // find the end of the next word
            while (str[next_word_end] != ' ' && str[next_word_end] != '\0') {
                next_word_end++;                // increment through string until next space is encountered (or string is terminated)
            }

            int next_word_len = next_word_end - (i + 1);            // compensate offset
            int next_word_width = next_word_len * (size_x * FONT_WIDTH);

            //
            if (cursorX + (size_x * FONT_WIDTH) + next_word_width > screen_width) {
                cursorX = x; // Reset to start of line
                cursorY += size_y * FONT_HEIGHT;
                i++; // skip the space itself so the new line doesn't start with a ' '
                continue; // Jump to start of loop to draw the first char of the word
            }
        }
        //
        drawChar(cursorX, cursorY, str[i], color, bg, size_x, size_y);
        cursorX += size_x * FONT_WIDTH;

        // previous code implementation still included in case one single word extends the entire screen and needs to be split into two
        if (cursorX + size_x * FONT_WIDTH > screen_width) {
            cursorX = x;
            cursorY += size_y * FONT_HEIGHT;
        }
        i++;
    }
}

// modified code for centered text, only used in few positions and only used for text in very center of screen
// may be eventually implemented in normal print function, but the "starting x" value messes with the centering of the text
void print_centered(int16_t y, const char* str, uint32_t color, uint32_t bg, uint8_t size_x, uint8_t size_y, uint16_t screen_width) {
    int16_t cursorY = y;
    int i = 0;
    uint16_t char_width = size_x * FONT_WIDTH;

    while (str[i] != '\0') {
        int line_start = i;
        int line_end = i;
        int current_line_width = 0;

        // --- PASS 1: Look ahead to see what fits on this line ---
        while (str[i] != '\0') {
            int word_start = i;
            while (str[i] != ' ' && str[i] != '\0') i++; // Find end of word
            int word_end = i;

            int word_len = word_end - word_start;
            int word_pixel_width = word_len * char_width;

            // Check if word fits (including a space if not the first word)
            int space_extra = (current_line_width > 0) ? char_width : 0;

            if (current_line_width + space_extra + word_pixel_width <= screen_width) {
                current_line_width += space_extra + word_pixel_width;
                line_end = i;
                if (str[i] == ' ') i++; // Move past space for next word check
            } else {
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
        for (j = line_start; j < line_end; j++) {
            drawChar(cursorX, cursorY, str[j], color, bg, size_x, size_y);
            cursorX += char_width;
        }

        // Move cursor to next line
        cursorY += size_y * FONT_HEIGHT;

        // Skip the space at the end of the line if there is one
        if (str[i] == ' ') i++;
    }
}

// x1 indicates offset from left side, x2 indicates rectangle width, str is string being used
// FindCenterX and FindCenterY implemented as separate functions so that print can still have the coordinates manually chosen if needed
// Font consideration implemented
int FindCenterX(int16_t x1, int16_t x2, const char* str, uint8_t font_size) {
        int last_space = 0;
        int calc_x;
        int i;

        // calculate total width produced by character string (10 for each character and 2 for buffer space between characters)
        int total_width = (font_size * FONT_WIDTH * strlen(str)); //(10 * strlen(str)) + (2 * (strlen(str) - 1));

        // if this width is less than rectangle width (most often the case), perform equation calculation
        if (total_width <= x2) {
            calc_x = (x2 + (2 * x1) - total_width) * 0.5 - (1 * font_size);
        }
        // if total width is greater than rectangle width, find last location of space closest to edge
        // error is coming from print wrap-around function placing character in top row
        else {
            for (i = 0; i < strlen(str); i++) {
                if (str[i] == ' ') {
                    int current_width = (10 * i) + (2 * (i - 1));
                    if (current_width < x2) {
                        last_space = i;
                    } else {
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
int FindCenterY(int16_t y1, int16_t y2, const char* str, uint8_t font) {
    int calc_y;
    int scale = strlen(str) * 0.0526315789473684;

    calc_y = (strlen(str) > 19) ? y1 + (y2 * 0.5) - (7 * font) - (7 * scale) : y1 + (y2 * 0.5) - (7 * font);

    return calc_y;
}
