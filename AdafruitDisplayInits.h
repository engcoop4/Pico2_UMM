#ifndef ADAFRUIT_DISPLAY_DIRECTORY_ADAFRUITDISPLAYINITS_H_
#define ADAFRUIT_DISPLAY_DIRECTORY_ADAFRUITDISPLAYINITS_H_

#include <stdint.h>
#include "Hardware.h"
#include "Global.h"

/*===============================================*/
/*                GENERAL COLORS                 */
/*===============================================*/
// keep everything in 24-bit format and for Adafruit call function rgb888_to_rgb565 to convert to 16-bit, and then for NewHaven use 24-bit
#define FPROC 16000000
#define WHITE 0xFFFFFF
#define BLACK 0x000000
#define RED 0xFF0000
#define GREEN 0x00FF00

// "Innovative Tech Vibes" Color Palette
// too many blues that are too similar, maybe add more oranges?
// remove green, or add more green. the green and orange r ugly
// greys and blues are VERY similar and merge into the same color when viewed at an angle,
// def need contrasting colors like blue and orange/yellow
// maybe try the "Futuristic Tech Innovation Palette" ?
#define DARK_BLUE 0x0D47A1
#define LIGHT_BLUE 0xB3E5FC
#define MID_BLUE 0x1E88E5
#define OFFWHITE 0xE3F2FD
#define GREY 0x424242
#define ORANYEL 0xFF9800
#define GRASS 0x4CAF50
#define BLUEAGAIN 0x1976D2
#define LIGHT_GREY 0x9E9E9E

#define BROWN 0xA52A2A
// #define LIGHT_GREY 0xD3D3D3
// #define GREY 0x808080
#define DARK_GREY 0x2F4F4F
#define MAROON 0x800000
#define ORANGE 0xFFA500
#define YELLOW 0xFFFF00
#define DARK_GREEN 0x228B22
#define CYAN 0x00FFFF
#define BLUE 0x0000FF
#define SKY_BLUE 0x87CEEB
#define NAVY_BLUE 0x8000080
#define MAGENTA 0xFF00FF
#define PURPLE 0x4B0082

/*===============================================*/
/*                FONT CONSTANTS                 */
/*===============================================*/

#define FONT_WIDTH 12  // 12 pixels wide per character
#define FONT_HEIGHT 16 // 16 pixels tall per character
#define COL_SPACING 8
#define ROW_SPACING 6
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

/*===============================================*/
/*           FUNCTION NAMES (SHARED)             */
/*===============================================*/
void LCD_selectLCD(void);
void LCD_deselectLCD();
void LCD_delay(unsigned int);
void LCD_Write_Bus(unsigned char);
void LCD_writeCommand(unsigned char);
void LCD_writeData(unsigned char);
void Screen_Init(void);
void Screen_Setup(void);

uint16_t rgb888_to_rgb565(uint8_t, uint8_t, uint8_t, uint8_t *, uint8_t *);
void rgb888_to_bytes(uint32_t color, uint8_t *r_byte, uint8_t *g_byte, uint8_t *b_byte);
void setCursor(unsigned int, unsigned int, unsigned int, unsigned int);
void format_color(uint32_t);
void LCD_DrawPixel(unsigned int, unsigned int, uint32_t);
void H_line(unsigned int, unsigned int, unsigned int, uint32_t);
void V_line(unsigned int, unsigned int, unsigned int, uint32_t);
void Rect(unsigned int, unsigned int, unsigned int, unsigned int, uint32_t);
void Rectf(unsigned int, unsigned int, unsigned int, unsigned int, uint32_t);
void Circle(unsigned int, unsigned int, unsigned int, uint32_t);
void Circlef(unsigned int, unsigned int, unsigned int, uint32_t);
void swap(int16_t *, int16_t *);
void drawLine(int16_t, int16_t, int16_t, int16_t, uint32_t);
void Triangle(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, uint32_t);
void Trianglef(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, uint32_t);
void drawChar(int16_t, int16_t, unsigned char, uint32_t, uint32_t, uint8_t, uint8_t);

void LCD_Clear(uint32_t);
void print(int16_t, int16_t, const char *, uint32_t, uint32_t, uint8_t, uint8_t, uint16_t);
void print_centered(int16_t, const char *, uint32_t, uint32_t, uint8_t, uint8_t, uint16_t);
int FindCenterX(int16_t, int16_t, const char *, uint8_t);
int FindCenterY(int16_t, int16_t, const char *, uint8_t);

/*===============================================*/
/*          FUNCTION NAMES (ADAFRUIT)            */
/*===============================================*/
#if defined(BOARD_TYPE_ADAFRUIT)
    void Adafruit_Init(void);
    void Adafruit_Setup(void);
    void Adafruit_writeData(unsigned char data);
    void Adafruit_writeCmd(unsigned char cmd);

/*===============================================*/
/*          FUNCTION NAMES (NEWHAVEN)            */
/*===============================================*/
#elif defined(BOARD_TYPE_NEWHAVEN)
    void NewHaven_Init(void);
    void NewHaven_Setup(void);
    void NewHaven_writeData(unsigned char data);
    void NewHaven_writeCmd(unsigned char cmd);
#endif

extern volatile int FLIP;

#endif /* ADAFRUIT_DISPLAY_DIRECTORY_ADAFRUITDISPLAYINITS_H_ */