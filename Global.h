/*
 * Global.h
 *
 *  Created on: Jun 16, 2025 (Imported to Visual Studio Code on 4/2/26)
 *      Author: engcoop#3
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/timer.h"

// changing to lower value changes display quicker but consumes CPU resources more
// changing to higher value changes display slower but frees up CPU more
// recommended "sweet spot" is 50, but can be modified later down the line
#define LCD_UPDATE_INTERVAL 50

// switch between BOARD_TYPE_ADAFRUIT and BOARD_TYPE_NEWHAVEN depending on which device is wired
//#define BOARD_TYPE_ADAFRUIT
#define BOARD_TYPE_NEWHAVEN

#if defined(BOARD_TYPE_ADAFRUIT) && defined(BOARD_TYPE_NEWHAVEN)
    #error "Multiple display boards defined! Please define only one in Global.h."
#endif

#if !defined(BOARD_TYPE_ADAFRUIT) && !defined(BOARD_TYPE_NEWHAVEN)
    #error "No display board defined! Please define BOARD_ADAFRUIT or BOARD_NEWHAVEN in Global.h."
#endif

#define GUI_OUTPUT_LINES 7 // max number of displays is 7
#define NUM_SD24_ADC_CHANNELS 7

// extern volatile int32_t ADCbuffer[NUM_SD24_ADC_CHANNELS];

#define TP1_high (P3OUT |= BIT2) // MM20250611 umm pins for synchro
#define TP1_low (P3OUT &= ~BIT2)

#define TP2_high (P3OUT |= BIT3) // MM20250611 umm pins for synchro
#define TP2_low (P3OUT &= ~BIT3)

// function pointer declaration
typedef void (*MainScreenFunction)(void);

#define NUM_MAIN_SCREENS (sizeof(Screen_Options) / sizeof(Screen_Options[0]))
extern const MainScreenFunction Screen_Options[9];

#define WFI_SCREENS (sizeof(Screen_Changes) / sizeof(Screen_Changes[0]))
typedef void (*WFI_ScreenFunction)(void);

extern const WFI_ScreenFunction Screen_Changes[9];

typedef enum
{ // enumeration - used to assign meaningful names to integer values
    Screen_ControlsDisplay = 0,
    Screen_TouchDecision,
    Screen_TouchCalibration,
    Screen_OperatingMode,
    Screen_NumberDisplays,
    Screen_ChannelSelection,
    Index_PresetConfigs,
    Screen_DisplayChannels,
    Screen_Metering
} State_of_Screen;

extern volatile State_of_Screen current_screen;
extern volatile bool force_redraw;
extern volatile bool timer_return_flag;

typedef enum
{
    One_Phase_AC = 0, // 0
    Three_Phase_AC,   // 1
    Wattmeter,        // 2
    FreqMeter         // 3
} DisplaySelection;

#endif
