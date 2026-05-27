/**
 * @file LCDProcessing.h
 * @author engcoop#4 RW
 * @brief Header file for LCDProcessing.c. Imported from CCS.
 * @version 1.0.0
 * @date 2026-04-02
 * @copyright Copyright (c) 2026
 */

#ifndef LCDPROCESSING_H_
#define LCDPROCESSING_H_

#include <stdbool.h>
#include <stdint.h>

#define ENTRY_CUSTOM 1
#define ENTRY_PRESET 2

/* ---------- GRAPHICS PIXEL VALUES ---------- */
/* ---------- KEY ---------- */
/* F = FACTOR */
/* BUT = BUTTON */
/* CONT = CONTROLS */
/* T = TOUCH */
/* NT = NO TOUCH */
/* HEAD = HEADER */
/* W = WIDTH*/
/* H = HEIGHT */
/* CALI = CALIBRATION */
/* ENAB = ENABLE */
/* DIS = DISABLE */
/* R = RADIUS */
/* OPER = OPERATING */
/* NUMD = NUMBER OF DISPLAYS */
/* TRI = TRIANGLE */
/* U = UPDATE */
/* CHANSEL = CHANNEL SELECTION */
/* OFF = OFFSET */
/* SBOX = SMALL BOX */
/* DISPCHAN = DISPLAY CHANNELS*/
/* WFI = WAITFORINPUT */
/* THRESH = THRESHOLD */
/* NP = NOT PRESSED */
/* P = PRESSED */
/* HI = HIGH */
/* L = LOW */
/* RIGH = RIGHT */
/* LEFT = LEFT */
/* DEC = DECREASE */
/* INC = INCREASE */
/* TOP = TOP */
/* BOT = BOTTOM */
/* BOUND = BOUNDARY */
/* -------- END KEY -------- */

// General
// Font here in case font is changed, and thus sizing changes
#define FONT_3 3
#define FONT_2 2
#define FONT_1 1
#define SCREEN_EDGE_X 239
#define SCREEN_EDGE_X_HEAD 231
#define SCREEN_EDGE_Y 319

// ButtonLayout
#define BUT_X_START_A 6
#define BUT_X_START_B 133
#define BUT_Y_START_A 178
#define BUT_Y_START_B 255
#define BUT_W 100
#define BUT_H 25

// ControlsDisplay
#define CONT_HEAD_X 6
#define CONT_HEAD_Y 0
#define CONT_HEAD_W 227
#define CONT_HEAD_H 25

#define CONT_LED_X_START_A 6
#define CONT_LED_X_START_B 133
#define CONT_LED_Y_START_A 30
#define CONT_LED_Y_START_B 70
#define CONT_LED_W 100
#define CONT_LED_H 25

// TouchScreenDecision
#define T_HEAD_X 6
#define T_HEAD_Y 6
#define T_HEAD_W 227
#define T_HEAD_H 72

#define T_PROMPT_X 0
#define T_PROMPT_Y 78
#define T_PROMPT_W 239
#define T_PROMPT_H 86

#define T_ENAB_X 6
#define T_ENAB_Y 164
#define T_ENAB_W 227
#define T_ENAB_H 52

#define T_DIS_X 6
#define T_DIS_Y 245
#define T_DIS_W 227
#define T_DIS_H 52

// TouchCalibration
#define CALI_PROMPT_X 0
#define CALI_SCREEN_EDGE 319
#define CALI_CIRCLE_ONE_X 15
#define CALI_CIRCLE_ONE_Y 15
#define CALI_CIRCLE_ONE_R 15

#define CIRCLE_ONE_ERASE_X_START 0
#define CIRCLE_ONE_ERASE_Y_START 0
#define CIRCLE_ONE_ERASE_W 35
#define CIRCLE_ONE_ERASE_H 35

#define CALI_CIRCLE_TWO_X 223 // 239 - 16 (0 to 15 is 16 pixels)
#define CALI_CIRCLE_TWO_Y 303 // 319 - 16 (0 to 15 is 16 pixels)
#define CALI_CIRCLE_TWO_R 15

#define CALI_SCREEN_ERASE_X 0
#define CALI_SCREEN_ERASE_Y 130
#define CALI_SCREEN_ERASE_W 239
#define CALI_SCREEN_ERASE_H 190

// OperatingMode
#define OPER_MODE_HEAD_X 6
#define OPER_MODE_HEAD_Y 6
#define OPER_MODE_HEAD_W 227
#define OPER_MODE_HEAD_H 50

#define NUMBER_OF_MODES 5

#define OPER_MODE_BOX_X 6
#define OPER_MODE_BOX_Y_STARTING_OFF 60
#define VARIABLE_FOR_BOX_Y_OFF 52
#define OPER_MODE_BOX_W 227
#define OPER_MODE_BOX_H 48

// NumberOfDisplays
#define NUMD_HEAD_X 6
#define NUMD_HEAD_Y 6
#define NUMD_HEAD_W 227
#define NUMD_HEAD_H 50

#define NUMD_T_X 6
#define NUMD_T_Y 61
#define NUMD_T_W 227
#define NUMD_T_H 188

#define NUMD_T_ENTER_X 133
#define NUMD_T_ENTER_Y 255
#define NUMD_T_BUT_W 100
#define NUMD_T_BUT_H 50

#define NUMD_T_RETURN_X 6
#define NUMD_T_RETURN_Y 255

#define NUMD_NT_X 6
#define NUMD_NT_Y 61
#define NUMD_NT_W 227
#define NUMD_NT_H 257

#define NUMD_LEFT_TRI_X1 10
#define NUMD_LEFT_TRI_Y1 155
#define NUMD_LEFT_TRI_X2 30
#define NUMD_LEFT_TRI_Y2 175
#define NUMD_LEFT_TRI_X3 30
#define NUMD_LEFT_TRI_Y3 135

#define NUMD_RIGHT_TRI_X1 229
#define NUMD_RIGHT_TRI_Y1 155
#define NUMD_RIGHT_TRI_X2 209
#define NUMD_RIGHT_TRI_Y2 175
#define NUMD_RIGHT_TRI_X3 209
#define NUMD_RIGHT_TRI_Y3 135

// UpdateNumberOfDisplays
#define U_NUMD_CLEAR_X 0
#define U_NUMD_CLEAR_Y 61
#define U_NUMD_CLEAR_W 239

#define U_NUMD_CLEAR_H_T 194
#define U_NUMD_CLEAR_H_NT 258

// ChannelSelection
#define W 100
#define H 42
#define startX 6
#define startY 179

#define CHANSEL_HEAD_X 6
#define CHANSEL_HEAD_Y 6
#define CHANSEL_HEAD_W 227
#define CHANSEL_HEAD_START_H 50
#define CHANSEL_HEAD_T_F 20

#define CHANSEL_BOX_X 6
#define CHANSEL_BOX_Y_STARTING_OFF 61
#define CHANSEL_BOX_T_F 20
#define CHANSEL_BOX_W 227
#define CHANSEL_BOX_START_H 113

#define CHANSEL_W_OFF 27
#define CHANSEL_H_OFF 5

#define CHANSEL_SBOX_Y_T_F 40
#define CHANSEL_SBOX_W 100
#define CHANSEL_SBOX_START_H 42
#define CHANSEL_SBOX_H_T_F 6
#define CHANSEL_PRINT_OFF_X 5

#define CHANSEL_TOUCH_RETURN_X 6
#define CHANSEL_TOUCH_RETURN_Y 279
#define CHANSEL_TOUCH_BUTTON_W 100
#define CHANSEL_TOUCH_BUTTON_H 36

#define CHANSEL_TOUCH_ENTER_X 133
#define CHANSEL_TOUCH_ENTER_Y 279

#define CHANSEL_LEFT_TRI_X1 11
#define CHANSEL_LEFT_TRI_Y1 89
#define CHANSEL_LEFT_TRI_X2 31
#define CHANSEL_LEFT_TRI_Y2 109
#define CHANSEL_LEFT_TRI_X3 31
#define CHANSEL_LEFT_TRI_Y3 69

#define CHANSEL_RIGHT_TRI_X1 228
#define CHANSEL_RIGHT_TRI_Y1 89
#define CHANSEL_RIGHT_TRI_X2 208
#define CHANSEL_RIGHT_TRI_Y2 109
#define CHANSEL_RIGHT_TRI_X3 208
#define CHANSEL_RIGHT_TRI_Y3 69

// UpdateChannelSelection
#define U_CHANSEL_J_IDX_OFF 64

#define U_CHANSEL_PRINT_X 40
#define U_CHANSEL_PRINT_X_T_F 32
#define U_CHANSEL_PRINT_Y 96
#define U_CHANSEL_PRINT_Y_T_F 53
#define U_CHANSEL_FONT_X_T_F 2
#define U_CHANSEL_FONT_Y_T_F 2

#define U_CHANSEL_NUMBER_PRINT_X 6
#define U_CHANSEL_NUMBER_PRINT_W 227
#define U_CHANSEL_NUMBER_PRINT_X_T_F 148
#define U_CHANSEL_NUMBER_PRINT_Y 61
#define U_CHANSEL_NUMBER_PRINT_Y_T_F 20
#define U_CHANSEL_NUMBER_PRINT_H 113

#define U_CHANSEL_SBOX_PRINT_W 100
#define U_CHANSEL_SBOX_PRINT_Y_T_F 40
#define U_CHANSEL_SBOX_PRINT_H 42
#define U_CHANSEL_SBOX_PRINT_H_T_F 6

// DisplayChannels
#define DISPCHAN_HEAD_X 6
#define DISPCHAN_HEAD_Y 0
#define DISPCHAN_HEAD_W 228
#define DISPCHAN_HEAD_H 25
#define DISPCHAN_HEAD_T_F 12

#define DISPCHAN_CONV_F 65

#define DISPCHAN_DIFF_OFF 6

#define DISPCHAN_PRINT_HEAD_X 10
#define DISPCHAN_PRINT_READING_X 192
#define DISPCHAN_PRINT_READING_Y_OFF 21

#define DISPCHAN_EXIT_T_X 214
#define DISPCHAN_EXIT_T_Y 3
#define DISPCHAN_EXIT_CIRCLE_T_X 219
#define DISPCHAN_EXIT_CIRCLE_T_Y 12
#define DISPCHAN_EXIT_CIRCLE_R 11

// InitYPositions
#define Y_POS_POSSIBILITES 7
#define Y_POS_OFF 6

// WaitForInput
#define WFI_BUT_THRESH_NP 500
#define WFI_BUT_THRESH_P 700
#define WFI_BUT_ENTER_THRESH_P 3725
#define WFI_BUT_RETURN_THRESH_P_HI 2980
#define WFI_BUT_RETURN_THRESH_P_L 2480
#define WFI_BUT_DOWN_THRESH_P_HI 2235
#define WFI_BUT_DOWN_THRESH_P_L 1740
#define WFI_BUT_UP_THRESH_P_HI 1490
#define WFI_BUT_UP_THRESH_P_L 990

#define MAX_NUMBER_DISPLAYS 7
#define MIN_NUMBER_DISPLAYS 1

#define WFI_IDX_THRESH 6
#define WFI_CHANSEL_ERASER_X_W 100
#define WFI_CHANSEL_ERASER_Y_F 40
#define WFI_CHANSEL_ERASER_START_H 42
#define WFI_CHANSEL_ERASER_Y_H_F 6
#define WFI_CHANSEL_ERASER_W 14
#define WFI_CHANSEL_ERASER_H 20

// NumberDisplays
#define NUMD_THRESH_X_LEFT_BOUND 0
#define NUMD_DEC_THRESH_X_W 65
#define NUMD_TRI_THRESH_Y_TOP 100
#define NUMD_TRI_THRESH_Y_H 120
#define NUMD_INC_THRESH_X_LEFT 195
#define NUMD_INC_THRESH_X_W 60
#define NUM_BUT_THRESH_Y_TOP 250
#define NUM_BUT_THRESH_Y_H 69
#define NUM_RET_THRESH_X_RIGH 106
#define NUM_ENT_THRESH_X_W 106
#define NUM_ENT_THRESH_X_LEFT 133

// Menu Channel Selection
#define NUMC_TRI_THRESH_Y_TOP 60
#define NUMC_TRI_THRESH_Y_H 58
#define NUMC_THRESH_X_LEFT_BOUND 0
#define NUMC_DEC_THRESH_X_W 65
#define NUMC_INC_THRESH_X_LEFT 195
#define NUMC_INC_THRESH_X_W 60 // ideally 239 - 195 would be 44, but due to rounding 44 doesnt give enough leniency for touch detection, boosted to 60

extern int diff_display[8];
extern int16_t saved_Ypos[7];
extern uint8_t LCD_ch_source[7];

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    volatile uint8_t *button_flag; // Points to the global variable flag
} TouchZone;

extern char const *display_unit[6];

void LCDSetup(void);
void ButtonLayout(void);
void MENU_ControlsDisplay(void);
void MENU_TouchScreenDecision(void);
void UpdateTouchHighlight(void);
void MENU_TouchCalibration(void);
void MENU_OperatingMode(void);
void UpdateOperatingModeSelection(void);
void MENU_NumberOfDisplays(void);
void UpdateNumberOfDisplays(void);
void MENU_ChannelSelection(void);
void UpdateChannelSelection(void);
void PresetConfigs(void);
void DisplayChannels(void);
void InitYPositions(void);
void WaitForInput(void);
void ButtonPolling(void);
void TouchDetection(void);
void UIDispatcher(void);
void ControlsDisplay(void);
void TouchDecision(void);
void TouchCalibration(void);
void OperatingMode(void);
void NumberDisplays(void);
void ChannelSel(void);
void CursorFunction(void);
void MeasureTouch(void);
void ActiveMetering(void);

#endif /* LCDPROCESSING_H_ */