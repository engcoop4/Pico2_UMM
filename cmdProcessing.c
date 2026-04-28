#include <stdio.h>
#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "cmdProcessing.h"
#include "AdafruitDisplayInits.h"
#include "UART.h"
#include "Global.h"

//------------------------------------------------------------------------------------------VARIABLES-----------------------------------------------------------------------------------------------------------
/*+++++++++++++++ Updating LCD by ADC value +++++++++++++++*/
/* External variables defined in LCDProcessing.c */
extern int diff_display[8];
extern int16_t saved_Ypos[7];
extern uint8_t LCD_ch_source[7];

// (replaces fullMessageA-G)
static char shared_lcd_buffer[16];

static char buffer[64];

// pointer used by the printing logic to point to the current message
static char *msg;

// indexing and tracking
uint16_t LCD_CH_index;
int SD24_index;

// keeps track of loop iterations for timing updates
static uint16_t loop_ticks = 0;

// sets default last_displayed values to lowest "possible" value to force first update
// phase out this implementation
static float last_displayed_values[NUM_SD24_ADC_CHANNELS] = {0};
extern volatile bool force_full_redraw;

// Externally declared 24-bit ADC result array (from SD24_B ISR in main.c)
extern volatile int32_t ADCbuffer[NUM_SD24_ADC_CHANNELS];
/************************************************************************************************************/
/*+++++++++++++++Commands+++++++++++++++*/
Uchar *CommStr;
uint32 ErrorStatus = sizeof(SysData);

// unit command
#define UNIT_24V 24
#define UNIT_48V 48
#define UNIT_125V 125
#define UNIT_250V 250

// pwmo command
extern uint pwm_channel;

// Serial command processing
char *strOut; // not found in project
char FW_Date[] = "16-Jun-2025";
char FW_PartNumber[] = "866-501-A";
uint8 wrk_str[HOST_XMT_BUFF_LEN]; // building & sending msg

uint16 CopyConstString(char *str_f_ptr, char *dest);
char CMD_index;

// calibration commands
char *CalNames[8] = {
    // IK20250130 be careful with the length, I reserved only 40 bytes for a temporary string in stack in function SetGetCalParam(void) - char Cal_Name[40];
    "BatteryVolts",    // Y1 X1 Y2 X2 Battery Voltage calibration
    "FaultVolts",      // Y1 X1 Y2 X2 Fault Voltage calibration
    "MinusGndVolts",   // Y1 X1 Y2 X2 Minus Grnd voltage correction factor info
    "RippleVolts1ph",  // Y1 X1 Y2 X2 single phase ripple voltage calibration
    "RippleVolts3ph",  // Y1 X1 Y2 X2 three phase ripple voltage calibration
    "RippleCurr1ph",   // Y1 X1 Y2 X2 single phase ripple current calibration
    "RippleCurr3ph",   // Y1 X1 Y2 X2 three phase ripple current calibration
    "CurrentOut_I420", // NU X1 NU X2 current loop calibration, value in PWM register to get 4 mA or 20 mA
};

static char RCI_message[128]; // for output via UART from RCI
// float ghost = FW_ver_float;  // confirmed 3.001

int lcd_change = 0;

#define ostr buffer
char *zerostr = "0.0";

#ifndef PC
#else
SYS_SPECIFIC_DATA SysData;
SYS_SPECIFIC_DATA EEPROM_SysData;
SYS_SPECIFIC_DATA DefaultSysdata;
#endif
//------------------------------------------------------------------------------------------Update LCD by ADC-----------------------------------------------------------------------------------------------------------
bool UpdateLCD_byADCvalue()
{
    int ch;
    // only increment the tick counter once per full cycle of channels
    loop_ticks++;

    for (ch = 0; ch < numberdisplays; ch++)
    {

        Output_string_on_LCD_by_index(ch);
    }
    return true;
}

bool Output_string_on_LCD_by_index(int16_t LCD_screen_position)
{
    // 1. Timing and Bounds Check
    // We allow the update if enough ticks have passed OR if a redraw is forced
    if (loop_ticks < LCD_UPDATE_INTERVAL && !force_full_redraw)
        return true;

    // Reset loop ticks only after the final display index is handled
    if (LCD_screen_position == (numberdisplays - 1))
        loop_ticks = 0;

    // 2. Fetch and Scale ADC Data
    uint8_t assigned_ch = LCD_ch_source[LCD_screen_position];
    // We subtract 1 because ADCbuffer is 0-indexed, but user channels are 1-7
    float val = (0.0000001788139343261719f) * ((float)ADCbuffer[assigned_ch - 1]);

    // 3. Deadband/Hysteresis Check
    float diff = (val > last_displayed_values[LCD_screen_position]) ? (val - last_displayed_values[LCD_screen_position]) : (last_displayed_values[LCD_screen_position] - val);

    // --- INTEGRATED FORCE REDRAW LOGIC ---
    // We only skip the update if the difference is small AND no redraw is forced.
    // If force_full_redraw is true, this 'if' is bypassed, forcing a print.
    if (diff < 0.05f && !force_full_redraw)
    {
        return true;
    }

    // Update the history now that we are committed to printing
    last_displayed_values[LCD_screen_position] = val;

    // 4. Single Buffer Reuse
    msg = shared_lcd_buffer;

    // --- RIGHT-ALIGNED STRING BUILDING ---
    int j;
    for (j = 0; j < 6; j++)
        msg[j] = ' ';
    msg[6] = '\0';

    float abs_val = (val < 0) ? -val : val;
    uint32_t units = (uint32_t)(abs_val * 10.0f + 0.5f);
    uint32_t int_part = units / 10;
    uint32_t dec_part = units % 10;

    char *p = &msg[5];
    *p-- = (char)(dec_part) + '0';
    *p-- = '.';

    if (int_part == 0)
    {
        *p-- = '0';
    }
    else
    {
        while (int_part > 0 && p >= &msg[1])
        {
            *p-- = (int_part % 10) + '0';
            int_part /= 10;
        }
    }

    *p = (val < 0) ? '-' : '+';

    // 5. Printing to Hardware
    int16_t Ypos_onScreen = saved_Ypos[LCD_screen_position];
    uint8_t font_size = (numberdisplays > 3) ? 1 : 2;
    uint16_t x_offset = (numberdisplays > 3) ? 72 : 55;

    print(x_offset,
          FindCenterY(Ypos_onScreen, diff_display[numberdisplays], msg, font_size),
          msg, WHITE, BLACK, font_size, font_size, 239);

    // --- FINAL FLAG RESET ---
    // Once we have processed the very last display index, we lower the flag.
    // This ensures every channel got exactly one forced update.
    if (LCD_screen_position == (numberdisplays - 1))
    {
        force_full_redraw = false;
    }

    return true;
}

void ResetLCDMapping()
{

    force_full_redraw = true;

    loop_ticks = 999; // Force the rate limiter to allow the next print
}

void InvalidateLCDCache()
{

    force_full_redraw = true;
}

void ftoa(float x, char *p)
{
    int exp = 0;
    int cc;
    x *= 1.000001;
    if (x < 0.0)
    {
        x *= -1;
        *p++ = '-';
    }
    else
        *p++ = '+';
    while (x >= 10.0)
    {
        exp++;
        x *= 0.1;
    }
    while ((x) && (x < 1.0))
    {
        exp--;
        x *= 10.0;
    }
    *p++ = (char)x + '0';
    x = (x - (int)x) * 10;
    *p++ = '.';
    for (cc = 0; cc < 5; cc++)
    {
        *p++ = (char)x + '0';
        x = (x - (int)x) * 10;
    }
    *p++ = 'E';
    if (exp < 0)
    {
        exp *= -1;
        *p++ = '-';
    }
    else
        *p++ = '+';
    *p++ = (char)exp / 10 + '0';
    *p++ = (char)exp % 10 + '0';
    *p++ = '\0';
}

char *putfloat_n(float x, int n)
{
    char *p = buffer;
    int exp = 0;
    int cc;
    x *= 1.00001;
    if (!x)
    {
        // cputs(zerostr);
        return buffer;
    }
    x *= 1.00001;
    if (x < 0.0)
    {
        x *= -1;
        *p++ = '-';
    }
    else
        *p++ = '+';
    while (x >= 10.0)
    {
        exp++;
        x *= 0.1;
    }
    while ((x) && (x < 1.0))
    {
        exp--;
        x *= 10.0;
    }
    *p++ = (char)x + '0';
    x = (x - (int)x) * 10;
    *p++ = '.';
    for (cc = 0; cc < n; cc++)
    {
        *p++ = (char)x + '0';
        x = (x - (int)x) * 10;
    }
    *p++ = 'E';
    if (exp < 0)
    {
        exp *= -1;
        *p++ = '-';
    }
    else
        *p++ = '+';
    *p++ = (char)exp / 10 + '0';
    *p++ = (char)exp % 10 + '0';
    *p++ = '\0';
    // cputs(buffer);
    // return(x);
    return buffer;
}

#define cputs(x) printf("%s", x)
void float_print(const char *f, const float fx) /* f ="%t.df" t-before, d -after dot */
{
    int exp = 0;
    int cc;
    float fl;
    char sign;
    int t;
    int d;
    int k;
    char *error = "FrmtERR";
    // char ostr[20];
    char *pf;
    long i; /* i is integer part of float x */
    float x = fx;

    ostr[0] = ' ';
    if (*f++ != '%')
    {
        cputs(error);
        return;
    }
    t = *f++ - '0'; /* meaning position before dot*/
    if (t < 2)
        t = 2; /* make space for at least sign and one digit position */
    if (*f++ != '.')
    {
        cputs(error);
        return;
    }
    d = *f++ - '0'; /* meaning position after dot*/
    if ((*f != 'f') && (*f != 'F'))
    {
        cputs(error);
        return;
    }

    if (x < 0.0)
    {
        x *= -1;
        sign = '-';
    }
    else
        sign = '+';
    fl = x;
    while (x >= 10.0f)
    {
        exp++;
        x *= 0.1f;
    }
    while ((x) && (x < 1.0f))
    {
        exp--;
        x *= 10.0f;
    }
    cc = exp;
    if (t <= cc)
        t = cc + 1;
    x = fl;
    pf = &ostr[0];
    pf = pf + t + 1;
    *pf-- = '\0';
    *pf-- = '.'; /* put dot on its position */
    i = (long)x; /* i is integer part of float x */
    x = x - i;   /* x now is only fraction part of float x */

    for (k = 1; k <= t; k++)
    {
        int c;
        if (pf >= &ostr[0]) /*check boundary*/
        {
            if (i > 0) /* if integer part > 0*/
            {
                c = i - (i / 10) * 10; /* remaining - last digit*/
                i = i / 10;
                *pf = (char)c + '0'; /* write digits from the dec.dot to the left*/
            }
            else /*integer part=0 */
            {
                if (cc <= 0 && k == 1)
                {
                    *pf-- = '0';
                    k++;
                } /* if 0 < float < -1 put leader 0 */
                *pf = sign;
                sign = ' '; /* write sign and then spaces*/
            }
            pf--;
        }
        else
        {
            k = t + 1; /* go out from "for" cycle*/
            pf = &ostr[0];
            *pf = '#'; /* put error sign */
            /* t - number of positions is too small - int part doesn't fit*/
        }
    } /* end for */
    pf = &ostr[0] + t + 1;
    x *= 10;
    for (cc = 0; cc <= d; cc++)
    {
        *pf = (char)x + '0';
        x = (x - (int)x) * 10;
        pf++;
    } /* function took an extra last char for rounding */
    pf--;

    /* rounding */
    if (*pf > '5') /* round previous position to the next bigger digit */
    {
        pf--;
        *pf += 1;
        while (*pf == ':' && pf > &ostr[0]) /* symbol':' is next symbol after '9'*/
        {
            *pf = '0'; /* instead 9 put '0' and increase previous digit */
            pf--;
            if (pf == &ostr[0])
                ostr[0] = '%'; /* ERROR ROUNDING, does not fit*/
            /*has been reached beginning of string */

            if (*pf == '.')
                pf--;                     /* bypass decimal dot */
            if (*pf == '+' || *pf == '-') /* shift sign only when case +99.99 */
            {
                char s = *pf--;
                *pf++ = s;
                *pf = '0'; /* zero will become '1' at the end of cycle*/
            }
            *pf += 1;
        }
    }
    ostr[t + d + 1] = '\0';
    cputs(ostr);
    return;
}

char *putfloat(float x)
{
    if (!x)
    {
        return (zerostr);
        // return x;
    }
    ftoa(x, buffer);
    //    cputs(buffer);
    return (buffer);
}

// placeholder for itoa. function already declared in stdlib for R-pi

void floatToString(float num, char *buffer, int decimalPlaces)
{
    int i;
    float fractionalPart;
    char sign;
    // Extract the integer part
    if (num >= 0)
        sign = '+';
    else
    {
        sign = '-';
        num = -num;
    }

    int integerPart = (int)num;
    // Extract the fractional part
    // float
    fractionalPart = num - (float)integerPart;

    // Convert integer part to string
    char intPartStr[20];
    itoa(integerPart, intPartStr, 10); // Convert integer part to string

    buffer[0] = sign;
    // Add integer part to the buffer
    strcpy(buffer + 1, intPartStr);

    // If we need fractional part
    if (decimalPlaces > 0)
    {
        strcat(buffer, "."); // Add decimal point

        // Scale fractional part to get the correct number of decimal places
        for (i = 0; i < decimalPlaces; i++)
        {                         // Declare i here
            fractionalPart *= 10; // Move the decimal point to the right
        }

        // Convert fractional part to integer and then to string
        int fracPart = (int)fractionalPart;
        char fracStr[10];
        itoa(fracPart, fracStr, 10);

        // Add fractional part to the buffer
        strcat(buffer, fracStr);
    }
}

//------------------------------------------------------------------------------------------Command Processing-----------------------------------------------------------------------------------------------------------