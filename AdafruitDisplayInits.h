#ifndef ADAFRUIT_DISPLAY_DIRECTORY_ADAFRUITDISPLAYINITS_H_
#define ADAFRUIT_DISPLAY_DIRECTORY_ADAFRUITDISPLAYINITS_H_

#include <stdint.h>
#include "Hardware.h"
#include "Global.h"

/*==============================================*/
/*                   COLORS                     */
/*==============================================*/
#define FPROC 16000000
#define WHITE 0xFFFFFF
#define BLACK 0x000000
#define BROWN 0xA52A2A
#define LIGHT_GREY 0xD3D3D3
#define GREY 0x808080 //
#define DARK_GREY 0x2F4F4F
#define MAROON 0x800000
#define RED 0xFF0000    //
#define ORANGE 0xFFA500 //
#define YELLOW 0xFFFF00
#define GREEN 0x00FF00 //
#define DARK_GREEN 0x228B22
#define CYAN 0x00FFFF //
#define BLUE 0x0000FF //
#define SKY_BLUE 0x87CEEB
#define NAVY_BLUE 0x8000080
#define MAGENTA 0xFF00FF //
#define PURPLE 0x4B0082

#define FONT_WIDTH 12    // 12 pixels wide per character
#define FONT_HEIGHT 16   // 16 pixels tall per character
#define COL_SPACING 8
#define ROW_SPACING 6

void LCD_selectLCD(void);
void LCD_deselectLCD();
void LCD_delay(unsigned int);
void Lcd_Write_Bus(unsigned char);
void LCD_writeCommand(unsigned char);
void LCD_writeData(unsigned char);
void Lcd_Init(void);
uint16_t rgb888_to_rgb565(uint8_t, uint8_t, uint8_t, uint8_t*, uint8_t*);
void setCursor(unsigned int, unsigned int, unsigned int, unsigned int);
void format_color(uint32_t);
void draw_pixel(unsigned int, unsigned int, uint32_t);
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
void LCD_DMA_Init();
void LCD_Clear(uint32_t);
void print(int16_t, int16_t, const char*, uint32_t, uint32_t, uint8_t, uint8_t, uint16_t);
void print_centered(int16_t, const char*, uint32_t, uint32_t, uint8_t, uint8_t, uint16_t);
int FindCenterX(int16_t, int16_t, const char*, uint8_t);
int FindCenterY(int16_t, int16_t, const char*, uint8_t);

extern volatile int FLIP;

#endif /* ADAFRUIT_DISPLAY_DIRECTORY_ADAFRUITDISPLAYINITS_H_ */