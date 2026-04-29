#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

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
char FW_Date[] = "29-Apr-2026";
char FW_PartNumber[] = "866-501-A";
uint8 wrk_str[HOST_XMT_BUFF_LEN]; // building & sending msg

uint16 CopyConstString(const char *str_f_ptr, char *dest);
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
void Print_Help(void) {
    // We use 'static const' so this string stays in Flash memory, 
    // saving precious RAM on the RP2350.
    static const char *test_menu = 
        "\r\nvers Get FW Version\r\n"
        "menu Call Menu\r\n"
        "init Re-initialize\r\n"
        "dflt Set to Default Values\r\n"
        "save Save Params\r\n"
        "echo Enable/Disable Echo\r\n"
        "phas Get/Set Phase\r\n"
        "test Connect Channel to Display\r\n"
        "adch Get Readings of All Channels\r\n"
        "baud Get/Set Baud Rate\r\n"
        "unit Get/Set Unit\r\n"
        "rbut See Button Status\r\n"
        "pwmo Change PWM Channel Selection\r\n"
        "rlcd Refresh LCD Screen\r\n";

    // On the RP2350, we replace the manual while-loop and register checks
    // with a single call to printf. The SDK handles the buffering for us.
    printf("%s", test_menu);
    
    // Optional: Ensure the buffer is pushed out to the USB immediately
    fflush(stdout);
}

void Print_FW_Version(void)
{
    // On RP2350, we skip the intermediate tmpBuf[256] entirely.
    // This saves 256 bytes of stack space, which is safer for deep function calls.
    
    // We use printf directly. The Pico SDK handles the "Wait for TX" 
    // and character streaming automatically.
    printf("\r\nBattery Monitor SW %s Ver %3.1f @ %s\r\n", 
            FW_PartNumber, 
            (double)FW_ver_float, 
            FW_Date);

    // Ensure the message hits the terminal immediately
    fflush(stdout);
}

void Print_menu(void) {
    // We use 'static const' to keep the string in Flash memory.
    // Note: I swapped \n\r to \r\n to match standard modern terminal behavior,
    // which prevents the "staircase" effect in many serial monitors.
    static const char *test_menu = "\r\nmenu command selected...\r\n";

    // Replaces the manual while-loop and UCA0 register checks.
    printf("%s", test_menu);

    // Forces the SDK to push the string out to the USB/UART immediately.
    fflush(stdout);
}

void Print_init(void) {
    // Kept as 'static const' to save RAM by keeping the string in Flash memory.
    static const char *init_menu = "\r\nInitialization Selection:\r\n";

    // Direct replacement for the while loop and UCA0TXBUF logic.
    printf("%s", init_menu);

    // Critical here: ensures the message is visible BEFORE 
    // the system potentially hangs or delays during initialization.
    fflush(stdout);
}

void Print_dflt(void) {
    // Keep the string in Flash memory to save RAM.
    static const char *test_menu = "\r\ndflt command selected...\r\n";

    // Direct replacement for the manual MSP430 bit-banging loop.
    printf("%s", test_menu);

    // Push the string out to the USB CDC interface immediately.
    fflush(stdout);
}

void Print_save(void) {
    // Standardizing to \r\n for modern terminal compatibility
    static const char *test_menu = "\r\nsave command selected...\r\n";

    // Direct replacement for the MSP430 hardware loop
    printf("%s", test_menu);

    // CRITICAL: Push this message out NOW. 
    // If your following 'save' logic pauses the CPU to write to Flash, 
    // the user needs to see this message first.
    fflush(stdout);
}

void SetGetPhase(void) {
    // 1. Pointer Setup: CommStr is your shared buffer pointer
    Uchar* temp_inp_str = CommStr;
    
    // 2. Extract the Parameter (the text after "phas>")
    // We look 5 bytes in (4 for 'phas' + 1 for '>')
    uint32 param = Convert_4_ASCII_to_Uint32(&temp_inp_str[CMD_LEN + 1]);

    // 3. SET Logic: If the user typed 'phas>'
    if (temp_inp_str[CMD_LEN] == '>') {
        
        // Match "3-ph" (Packed as a 32-bit word)
        if (param == 0x68702D33) { // '3', '-', 'p', 'h' packed
            setBit(SysData.NV_UI.SavedStatusWord, SinglePhase_eq0_3ph_eq1_Bit);
            // On RP2350, we use your new Send_comment for feedback
            Send_comment("Set to 3-Phase");
        }
        // Match "1-ph"
        else if (param == 0x68702D31) { // '1', '-', 'p', 'h' packed
            clearBit(SysData.NV_UI.SavedStatusWord, SinglePhase_eq0_3ph_eq1_Bit);
            Send_comment("Set to 1-Phase");
        }
        else {
            Send_RCI_Param_Error("1-ph or 3-ph");
        }
    }
    // 4. GET Logic: If the user just typed 'phas'
    else {
        Put_CMD_as_chars(); // Prints "phas"
        
        if (testBit(SysData.NV_UI.SavedStatusWord, SinglePhase_eq0_3ph_eq1_Bit)) {
            PutStr(">3-ph");
        } else {
            PutStr(">1-ph");
        }
        
        Send_verbose_comment("Phase configuration status");
    }
}

void testLCD(void) {
    Uchar* temp_inp_str = CommStr;
    // We keep this param extraction for the 'dflt' check
    uint32 param = Convert_4_ASCII_to_Uint32(&temp_inp_str[CMD_LEN + 1]);

    if (temp_inp_str[CMD_LEN] == '>') 
    {
        // Path A: The specific "test>a#d$" assignment
        if (temp_inp_str[CMD_LEN + 1] == 'a') 
        {
            // --- KEEPING YOUR EXACT SWITCH LOGIC ---
            switch (temp_inp_str[CMD_LEN + 2]) 
            {
                case '0': SD24_index = 0; break;
                case '1': SD24_index = 1; break;
                case '2': SD24_index = 2; break;
                case '3': SD24_index = 3; break;
                case '4': SD24_index = 4; break;
                case '5': SD24_index = 5; break;
                case '6': SD24_index = 6; break;
                default: goto format_error;
            }

            if (temp_inp_str[CMD_LEN + 3] == 'd') 
            {
                switch (temp_inp_str[CMD_LEN + 4]) 
                {
                    case '0': LCD_CH_index = 0; break;
                    case '1': LCD_CH_index = 1; break;
                    case '2': LCD_CH_index = 2; break;
                    default: goto format_error;
                }
            }
            // --- END OF SWITCH LOGIC ---

            char display_selection = (char)(LCD_CH_index + 65); // 0,1,2 -> A,B,C

            // R-Pi Optimization: Remove tmpBuf[256] and manual UCA0TXBUF loop.
            // printf handles formatting and the USB stack in one shot.
            printf("\r\nSelected Channel: %d, Selected Display: %c\r\n", SD24_index, display_selection);
            fflush(stdout);
        }
        // Path B: Resetting to default (test>dflt)
        else if (param == 0x746C6664) // 'd', 'f', 'l', 't' packed for 32-bit
        {
            ResetLCDMapping();
            Send_comment("LCD Mapping Reset to Default");
        }
        else 
        {
            goto format_error;
        }
    } 
    else 
    {
        format_error:
        // Use your existing PutStr wrapper which is already ported to the Pico SDK
        PutStr("\r\nFormat Error. Please enter in form test>a#d$ where # is [0..6] and $ is [0..2]\r\n");
    }
}

void Acquire_ADC_raw_counts() {
    // We no longer need char tmpBuf[64] because we print directly to the stream.
    
    int i;
    for (i = 0; i < 7; i++) {
        // The RP2350 FPU handles this multiplication natively.
        // I've kept your exact precision constant.
        float voltage = (0.0000001788139343261719f) * ((float)(ADCbuffer[i]));

        // Direct replacement for sprintf + manual UART loop.
        // We use %ld for ADCbuffer assuming it's a long (int32_t) on the R-Pi.
        printf("\r\nSD24 CH%d read: %ld -> %f V\r\n", i, (long)ADCbuffer[i], (double)voltage);
    }
    
    // Ensure all 7 channels are sent to the terminal before moving on.
    fflush(stdout);
}

void SetGetBaudRate() {
    Uchar* temp_inp_str = CommStr;
    // Extract first 4 chars after "baud=" for identification
    uint32 param = Convert_4_ASCII_to_Uint32(&temp_inp_str[CMD_LEN + 1]);

    // CHANGE 1: Use your global variable instead of calling a 'get' function.
    // This removes the "implicitly declared" error.
    uint32 current_baud = UART_BAUD;

    if (temp_inp_str[CMD_LEN] == '=') 
    {
        if (Is_Numeric(&temp_inp_str[CMD_LEN + 1]) == 1) 
        {
            uint32 target_baud = 0;

            if (param == 0x32353131) { // "1152"
                if (current_baud == 115200) {
                    PutStr("\r\nBaud Rate Already Selected\r\n");
                    return;
                }
                target_baud = 115200;
            }
            else if (param == 0x30303639) { // "9600"
                if (current_baud == 9600) {
                    PutStr("\r\nBaud Rate Already Selected\r\n");
                    return;
                }
                target_baud = 9600;
            }
            else goto format_error;

            printf("\r\n%lu baud rate selected\r\n"
                   "Re-launch terminal with new baud rate.\r\n"
                   "Press Enter Key with new baud rate to Resume\r\n\n", (unsigned long)target_baud);
            
            fflush(stdout);
            uart_tx_wait_blocking(UART_ID); // Using your UART_ID define

            // CHANGE 2: Capture the actual baud rate returned by the SDK.
            // This ensures UART_BAUD matches the physical hardware timing perfectly.
            UART_BAUD = uart_set_baudrate(UART_ID, target_baud);

            // Acknowledge the switch
            (void)getchar(); 
        }
        else goto format_error;
    }
    else if (temp_inp_str[CMD_LEN] == '\0') 
    {
        // Use current_baud (which we pulled from UART_BAUD at the start)
        printf("\r\nCurrent Baud Rate: %lu\r\n", (unsigned long)current_baud);
        fflush(stdout);
    }
    else 
    {
        format_error:
        Send_RCI_Param_Error("115200, 9600");
    }
}

void SetGetVoltageRange(void) {
    Uchar* temp_Inp_str = CommStr;
    uint32 param = Convert_4_ASCII_to_Uint32(&temp_Inp_str[CMD_LEN + 1]);
    uint8 index;

    if (temp_Inp_str[CMD_LEN] == '>') {
        // Hex literals updated for RP2350 32-bit packing (Little Endian)
        if (param == 0x76343230)      // "024v" packed
            SysData.NV_UI.unit_type = UNIT_24V;
        else if (param == 0x76383430) // "048v" packed
            SysData.NV_UI.unit_type = UNIT_48V;
        else if (param == 0x76353231) // "125v" packed
            SysData.NV_UI.unit_type = UNIT_125V;
        else if (param == 0x76303532) // "250v" packed
            SysData.NV_UI.unit_type = UNIT_250V;
        else 
            Send_RCI_Param_Error("024v 048v 125v 250v only");
    }
    /*
    SysData.NV_UI.unit_index = index;
    SysData.NV_UI.unit_type = UnitTypes[index];
    SysData.NV_UI.low_bat_threshold_V_f = Alarm_Limits[index].s.low_bat_threshold_V_f[DefSet];
    SaveToEE(SysData.NV_UI.low_bat_threshold_V_f);      // Store_Parameter(LOW_BAT, SysData.NV_UI.low_bat_threshold_V_f); //store new value

    SysData.NV_UI.high_bat_threshold_V_f = Alarm_Limits[index].s.hi_bat_threshold_V_f[DefSet];
    SaveToEE(SysData.NV_UI.high_bat_threshold_V_f);     // Store_Parameter(HIGH_BAT, SysData.NV_UI.high_bat_threshold_V_f); //store new value

    SysData.NV_UI.minus_gf_threshold_V_f = Alarm_Limits[index].s.minus_gf_threshold_V_f[DefSet];
    SaveToEE(SysData.NV_UI.minus_gf_threshold_V_f);     // Store_Parameter(MINUS_GF, SysData.NV_UI.minus_gf_threshold_V_f); //store new value

    SysData.NV_UI.plus_gf_threshold_V_f = SysData.NV_UI.minus_gf_threshold_V_f;     // IK20240206 copy plus threshold from minus when changing unit type from menu
    SaveToEE(SysData.NV_UI.plus_gf_threshold_V_f);      // Store_Parameter(PLUS_GF, SysData.NV_UI.plus_gf_threshold_V_f);               // store new value
    */
    else {
        // RP2350 Upgrade: Replace sprintf buffer and manual UART loop with direct printf
        printf("\r\nunit>%03dv\r\n", (int)SysData.NV_UI.unit_type);
        fflush(stdout);

        Send_verbose_comment("Unit type");
    }
}

void Echo_Enab_Disab(void) {
    // Convert_4_ASCII_to_Uint32 reads 4 chars starting at '>'
    uint32 param = Convert_4_ASCII_to_Uint32(&CommStr[CMD_LEN]);

    // ">ena" (first 4 of ">enable")
    // Packed as: 0x616E653E ('a' 'n' 'e' '>')
    if (param == 0x616E653E) {
        setBit(rt.Host, CharEchoFlag);
        clearBit(rt.Host, CmdVerboseResponse);
    }
    // ">ver" (first 4 of ">verbose")
    // Packed as: 0x7265763E ('r' 'e' 'v' '>')
    else if (param == 0x7265763E) {
        setBit(rt.Host, (CharEchoFlag | CmdVerboseResponse));
    }
    // ">dis" (first 4 of ">disable")
    // Packed as: 0x7369643E ('s' 'i' 'd' '>')
    else if (param == 0x7369643E) {
        clearBit(rt.Host, (CharEchoFlag | CmdVerboseResponse));
    }
    else {
        // "Get" Logic: Display current status
        Put_CMD_as_chars(); 
        PutChar('>');

        if (rt.Host & CmdVerboseResponse) {
            printf("Verbose");
        }
        else if (rt.Host & CharEchoFlag) {
            printf("Enabled");
        }
        else {
            printf("Disabled");
        }
        
        // Ensure the data is pushed to the terminal
        fflush(stdout);
    }
}

void adc0_acquire(void)
{
    // Calculation: 1.5V ref / (2^23 - 1)
    // The RP2350 FPU handles this float math natively and fast.
    float voltage = ((float)ADCbuffer[0]) * (0.0000001788139343261719f);

    // Direct output using the Pico SDK's buffered printf.
    // Casting ADCbuffer[0] to long to ensure %ld works correctly on 32-bit ARM.
    printf("\r\nSD24 CH0 read: %ld -> %f V\r\n", (long)ADCbuffer[0], (double)voltage);

    // Push the string out to the USB/Serial terminal immediately.
    fflush(stdout);
}

void SetGet_param(int float_offset, float minValue, float maxValue, float* Qf_var_ptr, char* verb_msg)
{
    float temp_float;
    char* temp_Inp_str = CommStr; // pointer to RxBuff
    // Resolve the string pointer based on command length and your offset mask
    int str_ptr = CMD_LEN + (float_offset & (SHOW_LONG - 1));

    if (temp_Inp_str[str_ptr] == '=')
    {
        str_ptr++;
        // Use your existing Is_Numeric validation
        if (Is_Numeric(&temp_Inp_str[str_ptr]) != true) goto par_error;

        // RP2350 uses the standard C library atof, which is fast on Cortex-M33
        temp_float = (float)atof(&temp_Inp_str[str_ptr]);

        // Range checking
        if ((temp_float > maxValue) || (temp_float < minValue))
            goto par_error;

        // Logic check: determine if we save as a precise float or a long integer
        if (float_offset < SHOW_LONG) {
            *Qf_var_ptr = temp_float;            // save as float
        } else {
            *Qf_var_ptr = (float)((long)(temp_float)); // save as 'long' casted back to float
        }

        return;
    }
    else // Query mode (e.g., "volt?")
    {
        // RP2350 Upgrade: PutStr handles the command echo, printf handles the value
        PutStr(temp_Inp_str);
        temp_float = *Qf_var_ptr;

        if (float_offset < SHOW_LONG)
        {
            // Dynamic formatting based on value magnitude for better precision display
            const char* frmt = "=%3.3f";
            if (temp_float < 10.0f)      frmt = "=%1.7f";
            else if (temp_float < 100.0f) frmt = "=%2.5f";
            
            printf(frmt, (double)temp_float);
        }
        else
        {
            // Rounding for "long" display (+0.5f)
            printf("=%3.0f", (double)(temp_float + 0.5f));
        }

        fflush(stdout); // Ensure the value is sent to terminal
        Send_verbose_comment(verb_msg);
        return;
    }

par_error:
    ErrorStatus = PARAM_ERROR;
    // Using %g for range display—it's smart about scientific vs fixed decimal notation
    printf(">~ERR VALUE %s, range [%g..%g]", temp_Inp_str, (double)minValue, (double)maxValue);
    fflush(stdout);
}

void SetGetCalParam(void) {
    Uchar* temp_Inp_str = CommStr;
    CalPtr CalStructurePtr;
    char Cal_Name[40];

    // RP2350 handles these index conversions very quickly
    int index1 = ASCIItoHexChar(temp_Inp_str[CMD_LEN]);
    int index2 = ASCIItoHexChar(temp_Inp_str[CMD_LEN + 1]); 

    if ((index1 < 0) || (index1 > 7)) {
        goto error_param;
    }

    // Copy calibration name from Flash to RAM
    CopyConstString(CalNames[index1], Cal_Name);

    // Pointer math: SysData.BatteryVolts is the start of an array of calibration structures
    CalStructurePtr = (CalPtr)&SysData.BatteryVolts;
    CalStructurePtr += index1; 

    // RCI_message is likely a global buffer; we use snprintf for safety on ARM
    // to prevent buffer overflows if Cal_Name is unexpectedly long.
    
    if (index2 == X1_low_point) // == 1
    {
        snprintf(RCI_message, sizeof(RCI_message), "Point %c, X coordinate, 0..32767, %s", '1', Cal_Name);
        SetGet_param(2 + SHOW_LONG, 0.0f, 32767.0f, &CalStructurePtr->Coord[X1_low_point], RCI_message);
    }
    else if (index2 == Y1_low_point) // == 0
    {
        snprintf(RCI_message, sizeof(RCI_message), "Point %c, Y coordinate, 0..300000, %s", '1', Cal_Name);
        SetGet_param(2 + SHOW_Qfloat, 0.0f, 300000.0f, &(CalStructurePtr->Coord[Y1_low_point]), RCI_message);
    }
    else if (index2 == X2_high_point) // == 3
    {
        snprintf(RCI_message, sizeof(RCI_message), "Point %c, X coordinate, 0..32767, %s", '2', Cal_Name);
        SetGet_param(2 + SHOW_LONG, 0.0f, 32767.0f, &CalStructurePtr->Coord[X2_high_point], RCI_message);
    }
    else if (index2 == Y2_high_point) // == 2
    {
        snprintf(RCI_message, sizeof(RCI_message), "Point %c, Y coordinate, 0..300000, %s", '2', Cal_Name);
        SetGet_param(2 + SHOW_Qfloat, 0.0f, 300000.0f, &CalStructurePtr->Coord[Y2_high_point], RCI_message);
    }
    else
    {
        error_param:
        // Use a safe copy for the error message
        CopyConstString("arg *# where # is: 0-Y1,1-X1,2-Y2,3-X2", Cal_Name);
        Send_RCI_Param_Error(Cal_Name);
        return;
    }
}

void SetGetButtonStateMan(void)
{
    uint32 param = Convert_4_ASCII_to_Uint32(&CommStr[CMD_LEN]);

    if (param == 0x7365743E) // ">test"
    {
        setBit(rt.OperStatusWord, ButtonTest_eq1_Bit);
    }
    else if (param == 0x6F74733E) // ">stop"
    {
        clearBit(rt.OperStatusWord, ButtonTest_eq1_Bit);
    }
    else
    {
        // 1. Select the ADC channel for your ladder (e.g., ADC0 is GPIO 26)
        adc_select_input(0); 
        uint16_t raw_adc = adc_read();

        // 2. Determine button states based on voltage windows
        // Assumes 12-bit ADC (0-4095). Adjust these thresholds based on your resistor values!
        int stateB1man = (raw_adc > 500 && raw_adc < 1500); 
        int stateB2man = (raw_adc > 1500 && raw_adc < 2500);
        int stateB3man = (raw_adc > 2500 && raw_adc < 3500);
        int stateB4man = (raw_adc > 3500); 

        printf("\r\nADC Raw: %u | B1=%d B2=%d B3=%d B4=%d\r\n", 
                raw_adc, stateB1man, stateB2man, stateB3man, stateB4man);
        fflush(stdout);
    }
}

void UpdateDutyChannel() {
    // CommStr[CMD_LEN] is likely '>', so [CMD_LEN + 1] is the channel digit
    char param = CommStr[CMD_LEN + 1];
    int channel = param - '0';

    // Validate channel against your Universal Meter's ADC count
    if (channel >= NUM_SD24_ADC_CHANNELS || channel < 0) {
        // RP2350 Upgrade: Direct printf replaces the manual UART TX loop
        printf("Incorrect command format. Try 'pwmo>[channel#]'");
        fflush(stdout);
        return;
    }

    // Update the global PWM channel variable
    pwm_channel = channel;
}

void RefreshLCDScreen() {
    // 1. Force a hardware reset
    // Replace 7 with your actual Reset (RST) GPIO pin number on the Pico
    gpio_put(7, 0); 
    sleep_ms(10);   // Standard SDK delay is much easier than counting cycles
    gpio_put(7, 1); 
    sleep_ms(20);   // Give the controller (likely ILI9341 or similar) time to stabilize

    // 2. Re-initialize the LCD driver registers
    lcd_change = 1;
    LCDSetup();     // Ensure this function now uses your new SPI/I2C Pico drivers

    // 3. Redraw the UI
    if(current_screen == InitializationDone) {
        // Force the code to ignore previous cached values
        InvalidateLCDCache();
        DisplayChannels();
    }
}

void FlipScreen() {
    lcd_change = 1;
    
    // Toggle the flip state
    FLIP = !FLIP;
    
    // Clear the software cache to ensure a full redraw in the new orientation
    InvalidateLCDCache();
    
    // Re-initialize the LCD. 
    // Your LCDSetup() must check the FLIP variable to send the 
    // correct MADCTL (Memory Access Control) register value.
    LCDSetup();
    
    if(current_screen == InitializationDone) {
        DisplayChannels();
    }
}
//******************************************************************************
// COMMANDS TEST TABLE *********************************************************
//******************************************************************************
const t_rci_commands rci[] = {
    // --- System / Help Commands ---
    { 0x706C6568, &Print_Help },            // "help"
    { 0x0000003F, &Print_Help },            // "?" (Padded with nulls)
    { 0x73726576, &Print_FW_Version },      // "vers"
    { 0x756E656D, &Print_menu },            // "menu"
    { 0x74696E69, &Print_init },            // "init"
    { 0x746C6664, &Print_dflt },            // "dflt"
    { 0x65766173, &Print_save },            // "save"
    
    // --- Configuration / Settings ---
    { 0x73616870, &SetGetPhase },           // "phas"
    { 0x64756162, &SetGetBaudRate },        // "baud"
    { 0x74696E75, &SetGetVoltageRange },    // "unit" (UMM Voltage Range)
    { 0x6F686365, &Echo_Enab_Disab },       // "echo"
    { 0x72617063, &SetGetCalParam },        // "cpar"
    
    // --- ADC & Hardware Tests ---
    { 0x74736574, &testLCD },               // "test"
    { 0x68636461, &Acquire_ADC_raw_counts }, // "adch"
    { 0x30636461, &adc0_acquire },          // "adc0"
    { 0x74756272, &SetGetButtonStateMan },  // "rbut"
    { 0x6F6D7770, &UpdateDutyChannel },     // "pwmo"
    
    // --- Display Control ---
    { 0x64636C72, &RefreshLCDScreen },      // "rlcd"
    { 0x70696C66, &FlipScreen },            // "flip"

#ifdef PC
    { 0x0D736C63, &ClearConsole },          // "cls\r" (Legacy PC support)
#endif

    { 0, NULL }                             // Table Terminator (CRITICAL)
};

void Send_comment(char *comment)
{
    printf(" // %s", comment);
}

void Send_verbose_comment(char *comment)
{
    if (testBit(rt.Host, CmdVerboseResponse))
        Send_comment(comment);
}

void Put_CMD_as_chars(void)
{
    // Use uint32_t to match the 32-bit word size of the RP2350
    uint32_t word = (rci[CMD_index].cmd_code);

    // We can use your PutChar macro here
    PutChar((char)word);         // LSB (Byte 0)
    PutChar((char)(word >> 8));  // Byte 1
    PutChar((char)(word >> 16)); // Byte 2
    PutChar((char)(word >> 24)); // MSB (Byte 3)
}

void Send_RCI_Param_Error(char *valid_msg)
{
    ErrorStatus = PARAM_ERROR;

    // The Pico SDK handles the "Waiting for TX" logic internally.
    // %s will pull from your buffers just like before.
    printf("\r\n>~ERR BAD param %s; valid: %s\r\n\n", rt.HostRxBuff, valid_msg);
}

uint16 PutStr(char *Str)
{
    uint16 len = (uint16)printf("%s", Str);
    fflush(stdout);
    sleep_ms(3);
    return len;
}

char *ToUpper(char *in_str)
{
    char *chr_ptr = in_str;

    // We can tighten this up for the 32-bit RP2350
    while (*chr_ptr != '\0')
    {
        // Use the standard library toupper (it's very fast on ARM)
        *chr_ptr = (char)toupper((unsigned char)*chr_ptr);
        chr_ptr++;
    }
    return in_str;
}

int Is_Numeric(char *strp)
{
    char *stringStartAddress = strp;
    int CharInString;
    int DigitDetected = false; // Now uses the definition from your header

    do
    {
        CharInString = (int)*strp;

        if (CharInString == ' ')
        {
            if (DigitDetected == false)
                stringStartAddress = strp + 1; // POINT TO NEXT: Ensures "only return" check works
        }
        else if (CharInString == CaRet || CharInString == '\n') // ADDED: \n for modern terminals
        {
            if (strp == stringStartAddress)
                return false;
            else
                return true;
        }
        else
        {
            // Check for valid numeric characters
            if (((CharInString < '0') || (CharInString > '9')) &&
                ((CharInString != '.') && (CharInString != 'e') && (CharInString != 'E')) &&
                ((CharInString != '-') && (CharInString != '+')))
            {
                return false; // Found a non-numeric character
            }
            else
            {
                DigitDetected = true;
            }
        }
        strp++;
    } while (*strp != 0);

    return DigitDetected; // Logic fix: ensure at least one number/sign was seen
}

uint32 toLower(Uchar ch)
{
    // On the RP2350, 'int' is 32-bit.
    // We can remove 'register' as the compiler handles this automatically.
    int ch1 = ch;
    int ch2 = ch1 + 32; // Standard ASCII shift: 'a' - 'A' = 32

    /*
     * Logic:
     * We subtract 'a' from our shifted value.
     * If the result is greater than ('z' - 'a'), it means the original
     * character was NOT an uppercase letter (A-Z).
     * In that case, we return the original ch1.
     */
    return ((uint32)(ch2 - 'a') > (uint32)('z' - 'a')) ? (uint32)ch1 : (uint32)ch2;
}

uint32 Convert_4_ASCII_to_Uint32(Uchar *pstr)
{
    uint32 ssss;

    /*
     * Logic:
     * Byte 0: Shift 0  (Original: + toLower(*pstr))
     * Byte 1: Shift 8  (Original: * 256)
     * Byte 2: Shift 16 (Original: * 65536)
     * Byte 3: Shift 24 (Original: * 65536 * 256)
     */

    ssss = ((uint32)toLower(pstr[0])) |
           ((uint32)toLower(pstr[1]) << 8) |
           ((uint32)toLower(pstr[2]) << 16) |
           ((uint32)toLower(pstr[3]) << 24);

    return ssss;
}

int ASCIItoHexChar(char in_char)
{
    // Use standard int (32-bit on RP2350). 'register' is ignored by GCC.
    int in_int = toupper((unsigned char)in_char);

    // Handle 0-9
    if ((in_int >= '0') && (in_int <= '9'))
    {
        return (in_int - '0'); // '0' is 0x30
    }

    // Handle A-F
    if ((in_int >= 'A') && (in_int <= 'F'))
    {
        return (in_int - ('A' - 10)); // ('A' - 10) is 0x37. Result: A=10, B=11...
    }

    // Handle Error case
    ErrorStatus = BAD_VALUE_ERR;
    return -1;
}

void PutTwoChars(int TwoChars)
{
    // On RP2350, 'int' is 32-bit.
    // We extract the high byte (bits 15-8) first.
    putchar((char)(TwoChars >> 8));

    // Then extract the low byte (bits 7-0).
    // The cast to (char) handles the & 0xFF masking automatically.
    putchar((char)TwoChars);
}

void SendCrLf(void)
{
    PutTwoChars(256 * '\r' + '\n'); // CRLF;
}

void processChar(void)
{
    char tmp_char; // 'register' is unnecessary on ARM

    // Safety check: ensure we don't process if the buffer is empty
    if (rt.HostRxBuffPtr == rt.EchoRxBuffPtr)
    {
        goto exit_ProcessChar;
    }

    // 1. Initial Backspace/Delete Check (at start of buffer)
    tmp_char = rt.HostRxBuff[rt.EchoRxBuffPtr];
    if ((tmp_char == '\b') || (tmp_char == 0x7F))
    {
        // If the very first char is a backspace, just reset
        if (rt.HostRxBuffPtr == 0)
        {
            rt.EchoRxBuffPtr = 0;
        }
    }

    // 2. Processing Loop
    while (rt.EchoRxBuffPtr != rt.HostRxBuffPtr)
    {
        tmp_char = rt.HostRxBuff[rt.EchoRxBuffPtr];

        if (tmp_char == 0)
        {
            if (testBit(rt.Host, CharEchoFlag))
                SendCrLf();
            goto exit_ProcessChar;
        }
        // 3. Handle Backspace ('\b') or Delete (0x7F)
        else if ((tmp_char == '\b') || (tmp_char == 0x7F))
        {
            if (rt.HostRxBuffPtr > 0)
            {
                // Remove the backspace char AND the char before it
                rt.HostRxBuffPtr--;
                if (rt.HostRxBuffPtr > 0)
                    rt.HostRxBuffPtr--;

                rt.HostRxBuff[rt.HostRxBuffPtr] = 0;

                if (testBit(rt.Host, CharEchoFlag))
                {
                    // Standard terminal backspace sequence: Back, Space, Back
                    cputs("\b \b");
                }
            }
            // Move echo pointer past the backspace command
            rt.EchoRxBuffPtr++;
        }
        // 4. Echo valid characters
        else if (testBit(rt.Host, CharEchoFlag))
        {
            // Echo the character that was just received
            PutChar(tmp_char);
            rt.EchoRxBuffPtr++;
        }
        else
        {
            rt.EchoRxBuffPtr++;
        }

        // 5. Circular Buffer Wrap-around
        // HOST_RX_BUFF_LEN must be a power of 2 (e.g., 256) for this mask to work
        rt.EchoRxBuffPtr &= (HOST_RX_BUFF_LEN - 1);
    }

exit_ProcessChar:
    clearBit(rt.Host, CharAvailableFlag);
}

void ClearRxBuffer(void) {
    // Reset pointers to start
    rt.HostRxBuffPtr = 0;
    rt.EchoRxBuffPtr = 0;

    // Optimized memory clear
    // Note: sizeof(rt.HostRxBuff) works perfectly here as long as HostRxBuff 
    // is a fixed-size array (e.g., char HostRxBuff[256])
    memset((void*)rt.HostRxBuff, 0, sizeof(rt.HostRxBuff));

    /* * IMPORTANT FIX: Bitwise OR vs Addition
     * In your header, CmdAvailFlag and CharAvailableFlag are likely BIT4 and BIT0.
     * While (BIT4 + BIT0) works, using (BIT4 | BIT0) is the safer 'ARM way' 
     * to ensure you aren't accidentally carrying bits in 32-bit space.
     */
    clearBit(rt.Host, (CmdAvailFlag | CharAvailableFlag));
}

// FL is no longer needed, so we define it as empty to keep the code compiling
#define FL 

uint16 CopyConstString(const char* str_f_ptr, char* dest) {
    char* bufptr = dest;
    const char* start_t_ptr = str_f_ptr; // Keep track of the source start

    // Standard copy loop
    while (*str_f_ptr != 0) {
        *bufptr++ = *str_f_ptr++;
    }
    
    // Add the null terminator (the original 'do-while' did this automatically)
    *bufptr = 0;

    // Calculate length: current pos - start pos
    // No need to subtract 1 because bufptr is at the null terminator
    uint16 str_len = (uint16)(str_f_ptr - start_t_ptr); 
    
    return str_len;
}

void SendMsgToPC(const char* Msg)
{
    // 1. Logic Check: Do we even need a buffer?
    // On the RP2350, PutStr (which we defined earlier) is already buffered 
    // and handles the USB/UART stack for us.
    
    // We can replace the entire manual loop with your existing PutStr.
    PutStr((char*)Msg);

    // 2. Ensure the line is finished
    SendCrLf();
}

bool ParseRCI(void)
{
    int i = 0;
    uint32_t cmd_word;
    uint32_t cmd_listed;

    // 1. Set up the pointer to the start of the buffer
    CommStr = (Uchar *)&rt.HostRxBuff[0];

    // 2. Handle character processing (backspaces, echos, etc.)
    if (rt.Host & CharAvailableFlag)
        processChar();

    // 3. Early exit if no full command is ready
    if ((rt.Host & CmdAvailFlag) == 0)
        return false;

    // 4. Pack the first 4 bytes into a uint32 for fast comparison
    cmd_word = Convert_4_ASCII_to_Uint32(CommStr);

    // Prepare for search
    ErrorStatus = 1; // Assuming 1 corresponds to BAD_SIO_CMD_ERR

    const char p_Execution[] = ">~Execution";
    const char p_Unrecognized[] = ">~Unrecognized";

    // 5. Command Search Loop
    int Num_RCI_commands = sizeof(rci) / sizeof(t_rci_commands);
    for (i = 0; i < (Num_RCI_commands); i++)
    {
        // On RP2350, we cast the 4-char array to a uint32 pointer and dereference
        cmd_listed = rci[i].cmd_code;

        if (cmd_word == cmd_listed)
        {
            ErrorStatus = 0;      // NO_ERROR
            rt.HostRxBuffPtr = 0; // Reset buffer pointer for next command
            CMD_index = i;        // Store which command we found

            // Execute the function pointer
            if (rci[i].f_ptr != NULL)
            {
                (rci[i].f_ptr)();
            }
            break;
        }
    }

    SendCrLf();

    // 6. Cleanup and Feedback
    ClearRxBuffer();

    if (ErrorStatus == 0) // NO_ERROR
    {
        if ((rt.OperStatusWord & Command_Executing_eq1_Bit) == 0)
            SendMsgToPC(">~OK\r\n\n");
        else
            SendMsgToPC(">~Doing CMD");
    }
    else
    {
        // 7. Simplified Error Reporting
        // We replace the manual while(!(UCA0IFG...)) loops with standard printf
        if (ErrorStatus == 1)
        { // BAD_SIO_CMD_ERR
            printf("%s Cmd, Error Code = %d\r\n", p_Unrecognized, ErrorStatus);
        }
        else if (ErrorStatus != 2)
        { // Assuming 2 is PARAM_ERROR
            printf("%s Cmd, Error Code = %d\r\n", p_Execution, ErrorStatus);
        }
    }

    wrk_str[0] = 0;
    return true; // command was processed
}

void ServiceSerialHardware(void) {
    // Make sure the pin is an output
    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);

    int c = getchar_timeout_us(0); 
    while (c != PICO_ERROR_TIMEOUT) {
        
        // PHYSICAL DIAGNOSTIC: Toggle GP6 every time a byte arrives
        gpio_put(LED1, !gpio_get(LED1)); 
        
        // Log back to terminal to see what the Pico thinks it's getting
        printf("{Rx:0x%02X}", (uint8_t)c);
        fflush(stdout);

        // Your existing logic
        rt.HostRxBuff[rt.HostRxBuffPtr] = (char)c;
        rt.HostRxBuffPtr = (rt.HostRxBuffPtr + 1) & (HOST_RX_BUFF_LEN - 1);
        setBit(rt.Host, CharAvailableFlag);

        if (c == '\r' || c == '\n') {
            setBit(rt.Host, CmdAvailFlag);
        }
        c = getchar_timeout_us(0);
    }
}