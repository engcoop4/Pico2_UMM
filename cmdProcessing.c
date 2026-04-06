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
Uchar* CommStr;
uint32 ErrorStatus = sizeof(SysData);

// unit command
#define UNIT_24V    24
#define UNIT_48V    48
#define UNIT_125V   125
#define UNIT_250V   250

// pwmo command
extern uint pwm_channel;

// Serial command processing
char * strOut;                              // not found in project
char FW_Date[] = "16-Jun-2025";
char FW_PartNumber[] = "866-501-A";
uint8 wrk_str[HOST_XMT_BUFF_LEN];           // building & sending msg

uint16 CopyConstString( char FL * str_f_ptr, char* dest);
char CMD_index;

// calibration commands
char FL * CalNames[8] = {// IK20250130 be careful with the length, I reserved only 40 bytes for a temporary string in stack in function SetGetCalParam(void) - char Cal_Name[40];
    "BatteryVolts",     // Y1 X1 Y2 X2 Battery Voltage calibration
    "FaultVolts",       // Y1 X1 Y2 X2 Fault Voltage calibration
    "MinusGndVolts",    // Y1 X1 Y2 X2 Minus Grnd voltage correction factor info
    "RippleVolts1ph",   // Y1 X1 Y2 X2 single phase ripple voltage calibration
    "RippleVolts3ph",   // Y1 X1 Y2 X2 three phase ripple voltage calibration
    "RippleCurr1ph",    // Y1 X1 Y2 X2 single phase ripple current calibration
    "RippleCurr3ph",    // Y1 X1 Y2 X2 three phase ripple current calibration
    "CurrentOut_I420",  // NU X1 NU X2 current loop calibration, value in PWM register to get 4 mA or 20 mA
};

static char RCI_message[128];                      //for output via UART from RCI
//float ghost = FW_ver_float;  // confirmed 3.001

int lcd_change = 0;