/**
 * @file LCDProcessing.c
 * @author engcoop#4 RW
 * @brief Handles the drawing states and buffer mapping for the Adafruit LCD. Imported from CCS.
 * @version 1.0.0
 * @date 2026-04-02
 * @copyright Copyright (c) 2026
 */

#include "LCDProcessing.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
// #include "TouchScreeninit.h"
#include "AdafruitDisplayInits.h"
#include "Global.h"

// touch_init = 0 while initial testing done
// REMOVE THIS WHEN TOUCH SCREEN TESTING IS DESIRED
// IN ADDITION, RESET_LCD_MAPPING COMMENTED OUT OF FUNCTIONS PRESETCONFIGS AND UPDATENUMBEROFDISPLAYS. RESTORE THESE BEFORE OFFICIAL TESTING IS EXECUTED
int touch_init = 0;

uint8_t LCD_ch_source[7] = {1, 2, 3, 4, 5, 6, 7};

/************************************************************************************************************/
// LCD Graphical Setup
/*+++++++++++++++Private Logic Variables+++++++++++++++*/
// Used in ChannelSelection to create boxes displaying chosen channels
static int j_idx = 1;   // used to index through the displays in ChannelSelection
static int title_index; // used to index title on final display screen (resetting cursor_position caused title to always default to "1-Phase Power AC")
int idx;                // used to index variables of UpdateChannelSelection
int num_boxes;
int x;
int y;
int sync_chan;
extern int lcd_change;
int cali;

extern uint16_t touch_baseline;
extern volatile uint16_t touch_triggered;
extern volatile uint8_t return_request_flag;

static int lastB1 = 0;     // last button state SW1 (UP)
static int lastB2 = 0;     // last button state SW2 (DOWN)
static int lastEnter = 0;  // last button state SW4 (ENTER)
static int lastReturn = 0; // last button state SW3 (RETURN)

extern volatile uint8_t entry_method;

extern int calidone;
// X cord can be set to uint8_t because 2^6 is 256, and 240 will never go above this threshold
uint32_t X_Cord = 0;
// Y cord must be set to uint16_t because 320 will get clamped to 256
uint32_t Y_Cord = 0;
//+++++++++++++++Shared Buffers+++++++++++++++
// Used for string formatting within UpdateChannelSelection, UpdateNumberOfDisplays, and DisplayChannels
static char display[10];
static char header[10];
char conv;

//+++++++++++++++Look-Up Tables+++++++++++++++
// More easily modifiable than re-entering text for FindCenterX, FindCenterY, and print statements
// ButtonLayout()
const char *LEDs[4] = {
    "AUTO",
    "ALARM",
    "PULSE ON",
    "T/R"};

// ControlsDisplay()
const char *buttons[4] = {
    "UP",
    "ENTER",
    "DOWN",
    "RETURN"};

// OperatingMode()
const char *title[5] = {
    "1-Phase Power AC",
    "3-Phase Power AC",
    "Wattmeter",
    "Frequency Meter",
    "CUSTOM"};
uint32_t box_color[5] = {GREY, DARK_GREY, PURPLE, MAROON, MAGENTA};

// ChannelSelection()
const char *syncbox[6] = {
    "A: ",
    "B: ",
    "C: ",
    "D: ",
    "E: ",
    "F: "};
const int xcord[6] = {6, 133, 6, 133, 6, 133};
const int ycord[6] = {179, 179, 226, 226, 273, 273};

// DisplayChannels()
int16_t saved_Ypos[7];
int diff_display[8] = {31, 100, 90, 80, 66, 51, 42, 35};
int additional_offset[7] = {79, 34, 9, 0, 0, 0, 0};
uint32_t header_color[7] = {RED, GREEN, BLUE, MAGENTA, BROWN, SKY_BLUE, ORANGE};
char const *display_title[5] = {
    "1-Phase Power AC",
    "3-Phase Power AC",
    "Wattmeter",
    "Frequency",
    "CUSTOM DISPLAY"};

//------------------------------------------------------------------------------------------LCD GRAPHIC DISPLAY-----------------------------------------------------------------------------------------------------------
void LCDSetup(void)
{
    RSUP;
    RESETUP;

    Lcd_Init();
    LCD_Clear(BLACK);
}

void ButtonLayout(void)
{
    // Controlled by SW1
    Rectf(BUT_X_START_A, BUT_Y_START_A, BUT_W, BUT_H, YELLOW);
    print(FindCenterX(BUT_X_START_A, BUT_W, buttons[0], FONT_1), FindCenterY(BUT_Y_START_A, BUT_H, buttons[0], FONT_1), buttons[0], BLACK, YELLOW, FONT_1, FONT_1, SCREEN_EDGE_X);

    // Controlled by SW2
    Rectf(BUT_X_START_B, BUT_Y_START_A, BUT_W, BUT_H, GREEN);
    print(FindCenterX(BUT_X_START_B, BUT_W, buttons[1], FONT_1), FindCenterY(BUT_Y_START_A, BUT_H, buttons[1], FONT_1), buttons[1], BLACK, GREEN, FONT_1, FONT_1, SCREEN_EDGE_X_HEAD);

    // Controlled by SW4
    Rectf(BUT_X_START_A, BUT_Y_START_B, BUT_W, BUT_H, WHITE);
    print(FindCenterX(BUT_X_START_A, BUT_W, buttons[2], FONT_1), FindCenterY(BUT_Y_START_B, BUT_H, buttons[2], FONT_1), buttons[2], BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X);

    // Controlled by SW3
    Rectf(BUT_X_START_B, BUT_Y_START_B, BUT_W, BUT_H, RED);
    print(FindCenterX(BUT_X_START_B, BUT_W, buttons[3], FONT_1), FindCenterY(BUT_Y_START_B, BUT_H, buttons[3], FONT_1), buttons[3], BLACK, RED, FONT_1, FONT_1, SCREEN_EDGE_X_HEAD);

    return;
}

void ControlsDisplay(void)
{

    LCD_Clear(BLACK);

    // Header
    Rectf(CONT_HEAD_X, CONT_HEAD_Y, CONT_HEAD_W, CONT_HEAD_H, WHITE);
    print(FindCenterX(CONT_HEAD_X, CONT_HEAD_W + CONT_HEAD_X, "MENU - CONTROLS", FONT_1), FindCenterY(CONT_HEAD_Y, CONT_HEAD_H, "MENU - CONTROLS", FONT_1), "MENU - CONTROLS", BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X_HEAD);

    // LED Labels
    Rectf(CONT_LED_X_START_A, CONT_LED_Y_START_A, CONT_LED_W, CONT_LED_H, WHITE);
    print(FindCenterX(CONT_LED_X_START_A, CONT_LED_W, LEDs[0], FONT_1), FindCenterY(CONT_LED_Y_START_A, CONT_LED_H, LEDs[0], FONT_1), LEDs[0], BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X);

    Rectf(CONT_LED_X_START_B, CONT_LED_Y_START_A, CONT_LED_W, CONT_LED_H, WHITE);
    print(FindCenterX(CONT_LED_X_START_B, CONT_LED_W, LEDs[1], FONT_1), FindCenterY(CONT_LED_Y_START_A, CONT_LED_H, LEDs[1], FONT_1), LEDs[1], BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X_HEAD);

    Rectf(CONT_LED_X_START_A, CONT_LED_Y_START_B, CONT_LED_W, CONT_LED_H, WHITE);
    print(FindCenterX(CONT_LED_X_START_A, CONT_LED_W, LEDs[2], FONT_1), FindCenterY(CONT_LED_Y_START_B, CONT_LED_H, LEDs[2], FONT_1), LEDs[2], BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X);

    Rectf(CONT_LED_X_START_B, CONT_LED_Y_START_B, CONT_LED_W, CONT_LED_H, WHITE);
    print(FindCenterX(CONT_LED_X_START_B, CONT_LED_W, LEDs[3], FONT_1), FindCenterY(CONT_LED_Y_START_B, CONT_LED_H, LEDs[3], FONT_1), LEDs[3], BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X_HEAD);

    // ButtonLayout();
}

// Remain on touchscreen decision until ENABLE or DISABLE chosen. Hitting buttons will highlight boxes to indicate selection
// (not yet implemented aside from graphically - touch screen must be set-up)
void TouchScreenDecision(void)
{
    LCD_Clear(BLACK);

    // Header
    Rectf(T_HEAD_X, T_HEAD_Y, T_HEAD_W, T_HEAD_H, WHITE);
    print(FindCenterX(T_HEAD_X, T_HEAD_W, "SCREEN SETTINGS", FONT_1), FindCenterY(T_HEAD_Y, T_HEAD_H, "SCREEN SETTINGS", FONT_1), "SCREEN SETTINGS", BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X);

    print(FindCenterX(T_PROMPT_X, T_PROMPT_W, "Touch Screen?", FONT_1), FindCenterY(T_PROMPT_Y, T_PROMPT_H, "Touch Screen?", FONT_1), "Touch Screen?", WHITE, BLACK, FONT_1, FONT_1, SCREEN_EDGE_X);

    Rectf(T_ENAB_X, T_ENAB_Y, T_ENAB_W, T_ENAB_H, GREEN);
    print(FindCenterX(T_ENAB_X, T_ENAB_W, "ENABLE", FONT_1), FindCenterY(T_ENAB_Y, T_ENAB_H, "ENABLE", FONT_1), "ENABLE", BLACK, GREEN, FONT_1, FONT_1, SCREEN_EDGE_X);

    Rectf(T_DIS_X, T_DIS_Y, T_DIS_W, T_DIS_H, RED);
    print(FindCenterX(T_DIS_X, T_DIS_W, "DISABLE", FONT_1), FindCenterY(T_DIS_Y, T_DIS_H, "DISABLE", FONT_1), "DISABLE", BLACK, RED, FONT_1, FONT_1, SCREEN_EDGE_X);

    // Initial highlight based on current cursor_position
    if (!cursor_position)
    {
        Rect(T_ENAB_X, T_ENAB_Y - 1, T_ENAB_W, T_ENAB_H + 1, WHITE);
        Rect(T_DIS_X, T_DIS_Y - 1, T_DIS_W, T_DIS_H + 1, BLACK);
    }
    else
    {
        Rect(T_DIS_X, T_DIS_Y - 1, T_DIS_W, T_DIS_H + 1, WHITE);
        Rect(T_ENAB_X, T_ENAB_Y - 1, T_ENAB_W, T_ENAB_H + 1, BLACK);
    }
    UpdateTouchHighlight();
}

// Controls highlight updates after initial screen created
void UpdateTouchHighlight(void)
{

    if (!cursor_position)
    {
        Rect(T_ENAB_X, T_ENAB_Y - 1, T_ENAB_W, T_ENAB_H + 1, WHITE);
        Rect(T_DIS_X, T_DIS_Y - 1, T_DIS_W, T_DIS_H + 1, BLACK);
    }
    else
    {
        Rect(T_DIS_X, T_DIS_Y - 1, T_DIS_W, T_DIS_H + 1, WHITE);
        Rect(T_ENAB_X, T_ENAB_Y - 1, T_ENAB_W, T_ENAB_H + 1, BLACK);
    }
}

// screen for user to set-up touch calibration
void TouchCalibration(void)
{
    LCD_Clear(BLACK);
    cali = 0;

    print_centered(FindCenterY(CALI_PROMPT_X, CALI_SCREEN_EDGE, "PRESS CIRCLE TO CALIBRATE", FONT_1),
                   "PRESS CIRCLE TO CALIBRATE", WHITE, BLACK, FONT_1, FONT_1, SCREEN_EDGE_X);

    // first calibration circle
    Circlef(CALI_CIRCLE_ONE_X, CALI_CIRCLE_ONE_Y, CALI_CIRCLE_ONE_R, RED);
    Circle(CALI_CIRCLE_ONE_X, CALI_CIRCLE_ONE_Y, CALI_CIRCLE_ONE_R, WHITE);
}

void UpdateTouchCalibration(void)
{
    cali = 1;
    Rectf(CIRCLE_ONE_ERASE_X_START, CIRCLE_ONE_ERASE_Y_START, CIRCLE_ONE_ERASE_W, CIRCLE_ONE_ERASE_H, BLACK);

    // second calibration circle
    Circlef(CALI_CIRCLE_TWO_X, CALI_CIRCLE_TWO_Y, CALI_CIRCLE_TWO_R, RED);
    Circle(CALI_CIRCLE_TWO_X, CALI_CIRCLE_TWO_Y, CALI_CIRCLE_TWO_R, WHITE);

    // TouchScreenReset();
}

void FinishTouchCalibration(void)
{

    Rectf(CALI_SCREEN_ERASE_X, CALI_SCREEN_ERASE_Y, CALI_SCREEN_ERASE_W, CALI_SCREEN_ERASE_H, BLACK);

    print_centered(FindCenterY(CALI_PROMPT_X, CALI_SCREEN_EDGE, "CALIBRATION COMPLETE", FONT_1),
                   "CALIBRATION COMPELTE", WHITE, BLACK, FONT_1, FONT_1, SCREEN_EDGE_X);
}

// 4 pixels between boxes, 48 pixels per box for even spacing
void OperatingMode(void)
{
    uint8_t i;

    LCD_Clear(BLACK);

    // Draw Header
    Rectf(OPER_MODE_HEAD_X, OPER_MODE_HEAD_Y, OPER_MODE_HEAD_W, OPER_MODE_HEAD_H, WHITE);
    print(FindCenterX(OPER_MODE_HEAD_X, OPER_MODE_HEAD_W, "Operating Mode", FONT_1),
          FindCenterY(OPER_MODE_HEAD_Y, OPER_MODE_HEAD_H, "Operating Mode", FONT_1),
          "Operating Mode", BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X);

    // Draw the 5 Operating Mode Boxes
    for (i = 0; i < NUMBER_OF_MODES; i++)
    {
        Rectf(OPER_MODE_BOX_X, (OPER_MODE_BOX_Y_STARTING_OFF + (i * VARIABLE_FOR_BOX_Y_OFF)), OPER_MODE_BOX_W, OPER_MODE_BOX_H, box_color[i]);
        print(FindCenterX(OPER_MODE_BOX_X, OPER_MODE_BOX_W, title[i], FONT_1),
              FindCenterY((OPER_MODE_BOX_Y_STARTING_OFF + (i * VARIABLE_FOR_BOX_Y_OFF)), OPER_MODE_BOX_H, title[i], FONT_1),
              title[i], WHITE, box_color[i], FONT_1, FONT_1, SCREEN_EDGE_X);
    }

    // Draw the cursor highlight at its current position
    Rect(OPER_MODE_BOX_X, (OPER_MODE_BOX_Y_STARTING_OFF + (VARIABLE_FOR_BOX_Y_OFF * cursor_position)), OPER_MODE_BOX_W, OPER_MODE_BOX_H, WHITE);
}

void UpdateOperatingModeSelection(void)
{
    int i;
    // 1. Draw a "Neutral" state (Black boxes) over the highlight areas
    // This effectively "erases" the old white selection border
    for (i = 0; i < NUMBER_OF_MODES; i++)
    {

        Rect(OPER_MODE_BOX_X, (OPER_MODE_BOX_Y_STARTING_OFF + (VARIABLE_FOR_BOX_Y_OFF * i)), OPER_MODE_BOX_W, OPER_MODE_BOX_H, BLACK);
    }

    Rect(OPER_MODE_BOX_X, (OPER_MODE_BOX_Y_STARTING_OFF + (VARIABLE_FOR_BOX_Y_OFF * cursor_position)), OPER_MODE_BOX_W, OPER_MODE_BOX_H, WHITE);
    title_index = cursor_position;
}

// Shows number of displays and implements UP as a +1 and DOWN as a -1. Minimum is 1, maximum is 7
// Holding SW1 for 3 seconds maxs to 7, holding SW2 for 3 seconds decrements to 1 immediately (not implemented yet)
void NumberOfDisplays(void)
{
    LCD_Clear(BLACK);

    Rectf(NUMD_HEAD_X, NUMD_HEAD_Y, NUMD_HEAD_W, NUMD_HEAD_H, WHITE);
    print(FindCenterX(NUMD_HEAD_X, NUMD_HEAD_W, "Number of Displays", FONT_1),
          FindCenterY(NUMD_HEAD_Y, NUMD_HEAD_H, "Number of Displays", FONT_1),
          "Number of Displays", BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X);

    if (touch_init)
    {
        Rectf(NUMD_T_X, NUMD_T_Y, NUMD_T_W, NUMD_T_H, WHITE);
        // RETURN
        Rectf(NUMD_T_RETURN_X, NUMD_T_RETURN_Y, NUMD_T_BUT_W, NUMD_T_BUT_H, RED);
        print(FindCenterX(NUMD_T_RETURN_X, NUMD_T_BUT_W, buttons[3], FONT_1), FindCenterY(NUMD_T_RETURN_Y, NUMD_T_BUT_H, buttons[3], FONT_1), buttons[3], BLACK, RED, FONT_1, FONT_1, SCREEN_EDGE_X);

        // ENTER
        Rectf(NUMD_T_ENTER_X, NUMD_T_ENTER_Y, NUMD_T_BUT_W, NUMD_T_BUT_H, GREEN);
        print(FindCenterX(NUMD_T_ENTER_X, NUMD_T_BUT_W, buttons[1], FONT_1), FindCenterY(NUMD_T_ENTER_Y, NUMD_T_BUT_H, buttons[1], FONT_1), buttons[1], BLACK, GREEN, FONT_1, FONT_1, SCREEN_EDGE_X);

        /*
        // UP AND DOWN ARROWS (+1 UP, -1 DOWN)
        Trianglef(119, 65, 99, 85, 139, 85, RED);
        Trianglef(119, 244, 99, 224, 139, 224, RED);
        */

        // LEFT AND RIGHT ARROWS (+1 RIGHT, -1 LEFT)
        Trianglef(NUMD_LEFT_TRI_X1, NUMD_LEFT_TRI_Y1, NUMD_LEFT_TRI_X2, NUMD_LEFT_TRI_Y2, NUMD_LEFT_TRI_X3, NUMD_LEFT_TRI_Y3, RED);
        Trianglef(NUMD_RIGHT_TRI_X1, NUMD_RIGHT_TRI_Y1, NUMD_RIGHT_TRI_X2, NUMD_RIGHT_TRI_Y2, NUMD_RIGHT_TRI_X3, NUMD_RIGHT_TRI_Y3, RED);
    }
    else
    {
        Rectf(NUMD_NT_X, NUMD_NT_Y, NUMD_NT_W, NUMD_NT_H, WHITE);
    }

    UpdateNumberOfDisplays();
}

void UpdateNumberOfDisplays(void)
{
    int y;
    // ResetLCDMapping();

    // if touch enabled, screen is shorter to account for ENTER/RETURN buttons
    if (touch_init)
    {
        y = U_NUMD_CLEAR_H_T;
    }
    else
        y = U_NUMD_CLEAR_H_NT;

    // need to make the find enter y dynamic
    sprintf(display, "%d", numberdisplays);
    print(FindCenterX(U_NUMD_CLEAR_X, U_NUMD_CLEAR_W, "1", FONT_3),
          FindCenterY(U_NUMD_CLEAR_Y, y, "1", FONT_3),
          display, RED, WHITE, FONT_3, FONT_3, SCREEN_EDGE_X);
}

// modify to have condensed screen if touch_init initialized (use touch_init * [factor]) to adjust bounds ?, 0 means no bounds adjustment, 1 means bounds adjustment)
void ChannelSelection(void)
{
    LCD_Clear(BLACK);

    lcd_change = 1;

    Rectf(CHANSEL_HEAD_X, CHANSEL_HEAD_Y, CHANSEL_HEAD_W, CHANSEL_HEAD_START_H - (touch_init * CHANSEL_HEAD_T_F), WHITE);
    Rectf(CHANSEL_BOX_X, CHANSEL_BOX_Y_STARTING_OFF - (touch_init * CHANSEL_BOX_T_F), CHANSEL_BOX_W, CHANSEL_BOX_START_H - (touch_init * CHANSEL_BOX_T_F), WHITE);

    print(FindCenterX(CHANSEL_BOX_X, CHANSEL_BOX_W, "Channel Selection", FONT_1),
          FindCenterY(CHANSEL_BOX_X, CHANSEL_HEAD_START_H - (touch_init * CHANSEL_HEAD_T_F), "Channel Selection", FONT_1),
          "Channel Selection", BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X);

    num_boxes = numberdisplays - 1;
    for (sync_chan = 0; sync_chan < num_boxes; sync_chan++)
    {
        int col = sync_chan % 2;
        int row = sync_chan / 2;

        x = startX + (col * (W + CHANSEL_W_OFF));
        y = startY + (row * (H + CHANSEL_H_OFF));

        /* Controls small boxes at bottom of screen */
        Rect(x, y - (touch_init * CHANSEL_SBOX_Y_T_F), CHANSEL_SBOX_W, CHANSEL_SBOX_START_H - (touch_init * CHANSEL_SBOX_H_T_F), WHITE);
        print(x + CHANSEL_PRINT_OFF_X, FindCenterY(y - (touch_init * CHANSEL_SBOX_Y_T_F), CHANSEL_SBOX_START_H - (touch_init * CHANSEL_SBOX_H_T_F), syncbox[sync_chan], FONT_1),
              syncbox[sync_chan], header_color[sync_chan], BLACK, FONT_1, FONT_1, SCREEN_EDGE_X);
    }

    // Enter and Return buttons on bottom of screen for touch
    if (touch_init)
    {
        Rectf(CHANSEL_TOUCH_RETURN_X, CHANSEL_TOUCH_RETURN_Y, CHANSEL_TOUCH_BUTTON_W, CHANSEL_TOUCH_BUTTON_H, RED);
        Rectf(CHANSEL_TOUCH_ENTER_X, CHANSEL_TOUCH_ENTER_Y, CHANSEL_TOUCH_BUTTON_W, CHANSEL_TOUCH_BUTTON_H, GREEN);
        print(FindCenterX(CHANSEL_TOUCH_RETURN_X, CHANSEL_TOUCH_BUTTON_W, "RETURN", FONT_1), FindCenterY(CHANSEL_TOUCH_RETURN_Y, CHANSEL_TOUCH_BUTTON_H, "RETURN", FONT_1), "RETURN", BLACK, RED, FONT_1, FONT_1, SCREEN_EDGE_X);
        print(FindCenterX(CHANSEL_TOUCH_ENTER_X, CHANSEL_TOUCH_BUTTON_W, "ENTER", FONT_1), FindCenterY(CHANSEL_TOUCH_ENTER_Y, CHANSEL_TOUCH_BUTTON_H, "ENTER", FONT_1), "ENTER", BLACK, GREEN, FONT_1, FONT_1, SCREEN_EDGE_X);
    }

    if (touch_init)
    {
        Trianglef(CHANSEL_LEFT_TRI_X1, CHANSEL_LEFT_TRI_Y1, CHANSEL_LEFT_TRI_X2, CHANSEL_LEFT_TRI_Y2, CHANSEL_LEFT_TRI_X3, CHANSEL_LEFT_TRI_Y3, RED);
        Trianglef(CHANSEL_RIGHT_TRI_X1, CHANSEL_RIGHT_TRI_Y1, CHANSEL_RIGHT_TRI_X2, CHANSEL_RIGHT_TRI_Y2, CHANSEL_RIGHT_TRI_X3, CHANSEL_RIGHT_TRI_Y3, RED);
    }

    UpdateChannelSelection();
}

void UpdateChannelSelection(void)
{
    static int last_num = -1;
    static int last_idx = -1;
    int i;

    if (lcd_change)
    {
        last_num = -1;
        last_idx = -1;
    }

    int refresh_required = lcd_change;
    int screen_changed = ((j_idx != last_idx) || refresh_required);
    lcd_change = 0;

    int number_changed = (numberchannels != last_num);

    idx = j_idx - 1;
    conv = j_idx + U_CHANSEL_J_IDX_OFF;

    // handles main labels, screen_changed modified to include refresh clause
    if (screen_changed)
    {
        char prefix[4] = {(char)conv, ':', ' ', '\0'};
        print(U_CHANSEL_PRINT_X - (touch_init * U_CHANSEL_PRINT_X_T_F),
              U_CHANSEL_PRINT_Y - (touch_init * U_CHANSEL_PRINT_Y_T_F),
              prefix, header_color[idx], WHITE, FONT_3 - (touch_init * U_CHANSEL_FONT_X_T_F),
              FONT_3 - (touch_init * U_CHANSEL_FONT_Y_T_F), SCREEN_EDGE_X);

        last_idx = j_idx;
    }

    // handles large number in middle
    if (number_changed || screen_changed)
    {
        char num_str[12];
        itoa(numberchannels, num_str, 10);
        print(FindCenterX(U_CHANSEL_NUMBER_PRINT_X, U_CHANSEL_NUMBER_PRINT_W, "0", FONT_3) * touch_init + U_CHANSEL_NUMBER_PRINT_X_T_F - (U_CHANSEL_NUMBER_PRINT_X_T_F * touch_init),
              FindCenterY(U_CHANSEL_NUMBER_PRINT_Y - (touch_init * U_CHANSEL_NUMBER_PRINT_Y_T_F), U_CHANSEL_NUMBER_PRINT_H - (touch_init * U_CHANSEL_NUMBER_PRINT_Y_T_F), "0", FONT_3),
              num_str, RED, WHITE, FONT_3, FONT_3, SCREEN_EDGE_X);

        if (idx >= 0 && idx < 7)
        {
            LCD_ch_source[idx] = (uint8_t)numberchannels;
        }
        last_num = numberchannels;
    }

    // needs to compare max number of boxes (1 less than display) to current index
    // to determine how many stored values from array must be called without
    // displaying a ghost number
    int max_boxes = numberdisplays - 1;

    if (refresh_required)
    {
        // only draw boxes up to max_boxes, and only if the index hasn't exceeded it
        int limit = (j_idx < max_boxes) ? j_idx : max_boxes;

        for (i = 0; i < limit; i++)
        {
            char chan_sel[4];
            int val = (int)LCD_ch_source[i];
            if (val == 0)
                val = 1;

            itoa(val, chan_sel, 10);

            print(FindCenterX(xcord[i], U_CHANSEL_SBOX_PRINT_W, "0", FONT_1),
                  FindCenterY(ycord[i] - (touch_init * U_CHANSEL_SBOX_PRINT_Y_T_F), U_CHANSEL_SBOX_PRINT_H - (touch_init * U_CHANSEL_SBOX_PRINT_H_T_F), "0", FONT_1),
                  chan_sel, WHITE, BLACK, FONT_1, FONT_1, SCREEN_EDGE_X);
        }
    }

    else if ((number_changed || screen_changed) && idx < max_boxes && idx >= 0)
    {
        char chan_sel[4];
        itoa(numberchannels, chan_sel, 10);
        print(FindCenterX(xcord[idx], U_CHANSEL_SBOX_PRINT_W, "0", FONT_1),
              FindCenterY(ycord[idx] - (touch_init * U_CHANSEL_SBOX_PRINT_Y_T_F), U_CHANSEL_SBOX_PRINT_H - (touch_init * U_CHANSEL_SBOX_PRINT_H_T_F), "0", FONT_1),
              chan_sel, WHITE, BLACK, FONT_1, FONT_1, SCREEN_EDGE_X);
    }
}

// Can alter preset configurations
void PresetConfigs(void)
{
    // Use cursor_position as index for
    // Preset configs used only to update variables that DisplayChannels will use, does not execute its own screen so goes immediately into DisplayChannels using current_screen indexing
    selected_display = cursor_position;
    switch (selected_display)
    {
    case One_Phase_AC:
        LCD_ch_source[0] = 1;
        LCD_ch_source[1] = 4;
        LCD_ch_source[2] = 5;
        unit_index = 0;
        numberdisplays = 3;
        break;
    case Three_Phase_AC:
        // specifically for three phase power, need default LCD mapping
        // ResetLCDMapping();
        unit_index = 0;
        numberdisplays = 7;
        break;
    case Wattmeter:
        unit_index = 4;
        numberdisplays = 1;
        break;
    case FreqMeter:
        unit_index = 5;
        numberdisplays = 1;
        break;
    }
    // REMOVED FOR INITIAL TESTING. RESTORE EVENTUALLY
    // current_screen = Screen_DisplayChannels;
    // force_redraw = true;
}

// The various operating modes can set values like numberdisplays an input string for title, and desired unit
// the main differences will be which channel goes to which display
void DisplayChannels(void)
{
    int i;
    LCD_Clear(BLACK);
    InitYPositions();

    Rectf(DISPCHAN_HEAD_X, DISPCHAN_HEAD_Y, DISPCHAN_HEAD_W, DISPCHAN_HEAD_H, WHITE); // Rectangle fill
    // implement command to rename custom display ?
    print(FindCenterX(DISPCHAN_HEAD_X, DISPCHAN_HEAD_W, display_title[title_index], FONT_1) - (FindCenterX(DISPCHAN_HEAD_X, DISPCHAN_HEAD_W, display_title[title_index], FONT_1) * touch_init) + (DISPCHAN_HEAD_T_F * touch_init),
          FindCenterY(DISPCHAN_HEAD_Y, DISPCHAN_HEAD_H, display_title[title_index], FONT_1), display_title[title_index], BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X_HEAD);

    // determine number of rectangles needed based on numberdisplays

    for (i = 0; i < numberdisplays; i++)
    { // Execute draw rectangle based on how many displays there are
        char conv = i + DISPCHAN_CONV_F;
        sprintf(header, "%c:", conv);

        // need to offset by 31 to keep minimum 6 spaces from start, and also must offset by pixel value so that boxes have enough space each new iteration
        // additional offset added to screens with 1-3 displays to center them (looks nicer)
        int16_t Y_start = ((additional_offset[numberdisplays - 1]) * (i + 1) + (diff_display[0] + (diff_display[numberdisplays] + DISPCHAN_DIFF_OFF) * (i))); // offset of 6 (title and spacing of 6) +

        Rect(DISPCHAN_HEAD_X, Y_start, DISPCHAN_HEAD_W, diff_display[numberdisplays], WHITE);

        print(DISPCHAN_PRINT_HEAD_X, FindCenterY(Y_start, diff_display[numberdisplays], "A:", ((numberdisplays > 5) ? FONT_1 : FONT_2)), header, header_color[i], BLACK, ((numberdisplays > 5) ? FONT_1 : FONT_2), ((numberdisplays > 5) ? FONT_1 : FONT_2), SCREEN_EDGE_X);

        print(DISPCHAN_PRINT_READING_X, ((Y_start + diff_display[numberdisplays]) - DISPCHAN_PRINT_READING_Y_OFF), display_unit[unit_index], WHITE, BLACK, FONT_1, FONT_1, SCREEN_EDGE_X);
    }

    // "x" for return on display screen
    if (touch_init)
    {
        print(DISPCHAN_EXIT_T_X, DISPCHAN_EXIT_T_Y, "x", BLACK, WHITE, FONT_1, FONT_1, SCREEN_EDGE_X);
        Circle(DISPCHAN_EXIT_CIRCLE_T_X, DISPCHAN_EXIT_CIRCLE_T_Y, DISPCHAN_EXIT_CIRCLE_R, BLACK);
        // used to test centering
        // draw_pixel(219, 12, RED);
    }

    // current_screen = InitializationDone;
}

void InitYPositions(void)
{
    int i;
    for (i = 0; i < Y_POS_POSSIBILITES; i++)
    {
        saved_Ypos[i] = ((additional_offset[numberdisplays - 1]) * (i + 1) +
                         (diff_display[0] + (diff_display[numberdisplays] + Y_POS_OFF) * (i)));
    }
}

void WaitForInput(void)
{
    static bool lock_engaged = false; // acts as lock to have 1 button press, tunes out noise/adc readings from falling voltages while the capacitor discharges after a button press
                                      // Only resets when finger fully removed and adc value falls below 600 threshold (Release Gate)

    uint16_t adc_val = adc_read(); // take adc value

    // manually reset buttons on each new waitforinput because theoretically each new waitforinput should be waiting for an input or processing a single one
    int8_t b1_pressed = 0, b2_pressed = 0, enter_pressed = 0, return_pressed = 0;

    // no buttons pressed if falls under 500, "unlocks"
    if (adc_val < 500)
    {
        lock_engaged = false;
    }

    // if lock is not engaged and adc value is above 700, we can say "a button is being pressed"
    if (!lock_engaged && adc_val >= 700)
    {

        // wait for SPI noise to pass (even with filters included, SPI communication causes noise spikes)
        sleep_ms(5);

        // take a confirmation reading to ensure its not just a random spike
        adc_val = adc_read();

        // button value reads at 4071, use 3725 for expected tolerance across parts, if above this threshold, ENTER is being pressed
        if (adc_val >= 3725)
        {
            enter_pressed = 1;
            lock_engaged = true;
        }
        // button value reads at 2780, use 2480 (low) and 2980 (high) for expected tolerance across parts, if between this threshold, RETURN is being pressed
        else if (adc_val >= 2480 && adc_val <= 2980)
        {
            return_pressed = 1;
            lock_engaged = true;
        }
        // button value reads at 2048, use 1737 (low) and 2234 (high) for expected tolerance across parts, if between this threshold, DOWN is being pressed
        else if (adc_val >= 1740 && adc_val <= 2235)
        {
            b2_pressed = 1;
            lock_engaged = true;
        }
        // button value reads at 1365, use 992 (low) and 1489 (high) for expected tolerance across parts, if between this threshold, UP is being pressed
        else if (adc_val >= 990 && adc_val <= 1490)
        {
            b1_pressed = 1;
            lock_engaged = true;
        }
    }

    switch (current_screen)
    {
    // Displays LEDs, sets current_screen to next_screen Screen_TouchDecision, resets force_redraw so mainloop switch case works correctly
    case Screen_ControlsDisplay:
        if (enter_pressed)
        {
            current_screen = Screen_TouchDecision;
            force_redraw = true;
        }
        break;

    // Once screen changed to Screen_TouchDecision, enter UpdateTouchHighlight when a button is pressed (UP/DOWN), or go to next screen (ENTER)
    case Screen_TouchDecision:
        if (b1_pressed)
        {
            cursor_position = 0;
            UpdateTouchHighlight();
        }
        else if (b2_pressed)
        {
            cursor_position = 1;
            UpdateTouchHighlight();
        }

        if (enter_pressed)
        {
            if (cursor_position == 0)
            {
                cali = 0;
                // touch_triggered = 0;

                // in case of Disable -> Return -> Enable
                // P1IE &= ~BIT0;        // Disable interrupt
                // P1IFG &= ~BIT0;       // Clear any "stale" flag from the 'Disabled' period

                current_screen = Screen_TouchCalibration;
                // TouchScreeninit();    // Re-init pins and re-enable P1IE
            }
            else
            {
                current_screen = Screen_OperatingMode;
                // should be disabling touch screen initialization
                // TouchScreen_deinit();
            }
            cursor_position = 0;
            force_redraw = true;
        }
        break;

    // needs to respond to touch ONLY, no button presses for calibration
    // code at bottom will scan for X and Y coordinate values
    // kills interrupt
    case Screen_TouchCalibration:
        /*
            if(touch_triggered) {
                // 1. GATEKEEPER: Is the user touching the right general area?
                if(CaliBoundsCheckTS()) {

                    if(cali == 0) {
                        CaptureCaliCoordsTS();

                        // update cali value AFTER min values captured, update screen to indicate to user to remove finger
                        UpdateTouchCalibration();

                        // 4. WAIT: Don't move on until the finger is gone
                        WaitForReleaseTS();
                        touch_triggered = 0;

                        // 5. RE-ARM: Clean up flags and re-enable interrupt
                        //P1IFG &= ~BIT0;
                        //P1IE |= BIT0;
                    }
                    else if (cali == 1) {
                        // Repeat for the second point
                        CaptureCaliCoordsTS();

                        WaitForReleaseTS();
                        FinishTouchCalibration();
                        current_screen = Screen_OperatingMode;

                        //P1IFG &= ~BIT0;
                        touch_triggered = 0;
                        //P1IE |= BIT0;

                        force_redraw = true;
                    }
                }
                else {
                    // FAILED BOUNDS: User touched the wrong spot.
                    // We must still reset the flag/interrupt so they can try again.
                    WaitForReleaseTS();
                    //P1IFG &= ~BIT0;
                    touch_triggered = 0;
                    //P1IE |= BIT0;
                }
            }
            */
        break;
    // Displays the various operating modes, compares b2 and b1 to see if any button was pressed
    // if it was, move cursor position by the difference, check bounds to ensure cursor never goes past 4 or below 0
    case Screen_OperatingMode:
    {
        /*
        // 1. TOUCH INPUT LOGIC
        if (touch_triggered) {
            // Clear the ISR flag immediately so we don't loop on the same touch
            touch_triggered = 0;

            // Read fresh coordinates (this includes the settling delay internally)
            X_Cord = ReadTouchX();
            Y_Cord = ReadTouchY();

            // Only process if the touch is valid (greater than 0)
            if (X_Cord > 0 && Y_Cord > 0) {
                int i;
                for (i = 0; i < 5; i++) {
                    // Logic: Start at 60Y, each box is 48px high, stepping by 52px
                    uint16_t row_top = 60 + (52 * i);

                    if (Display_Bounds_Check(X_Cord, Y_Cord, 6, row_top, 227, 48)) {
                        cursor_position = i;
                        UpdateOperatingModeSelection();

                        // Trigger the transition immediately
                        enter_pressed = 1;
                        break;
                    }
                }
            }

            // Re-enable Port 1 Interrupt for the next physical touch
            //P1IFG &= ~BIT0;
            //P1IE  |=  BIT0;
        }
        */

        // 2. PHYSICAL BUTTON LOGIC
        // Calculates direction: b2 (Down) is +1, b1 (Up) is -1
        int8_t moved = b2_pressed - b1_pressed;
        if (moved != 0)
        {
            cursor_position += moved;

            // Clamp bounds to the 5 available menu items (0 to 4)
            if (cursor_position < 0)
                cursor_position = 4;
            if (cursor_position > 4)
                cursor_position = 0;

            UpdateOperatingModeSelection();
        }

        // 3. STATE TRANSITION LOGIC
        if (enter_pressed)
        {
            // CRITICAL: Clear the flag so the next screen doesn't "auto-enter"
            enter_pressed = 0;

            if (cursor_position < 4)
            {
                // Path A: One of the 4 Presets was selected
                entry_method = ENTRY_PRESET;
                PresetConfigs(); // This function must set current_screen = InitializationDone
            }
            else
            {
                // Path B: "Custom" (Item 5) was selected
                entry_method = ENTRY_CUSTOM;
                numberdisplays = 1;
                current_screen = Screen_NumberDisplays;
                cursor_position = 0;
                force_redraw = true;
            }
        }
        else if (return_pressed)
        {
            // Clear the flag to prevent "Double Returns"
            return_pressed = 0;

            current_screen = Screen_TouchDecision;
            cali = 0;
            cursor_position = 0;
            force_redraw = true;
        }
        break;
    }

    // Increment number of displays if SW1 pressed, decrement if SW2 pressed
    case Screen_NumberDisplays:
    {
        bool value_changed = false;
        if (b1_pressed)
        {
            numberdisplays++;
            value_changed = true;
        }
        if (b2_pressed)
        {
            numberdisplays--;
            value_changed = true;
        }

        if (value_changed)
        {
            numberdisplays = (numberdisplays > 7) ? 1 : (numberdisplays < 1 ? 7 : numberdisplays);
            UpdateNumberOfDisplays();
        }

        if (enter_pressed)
        {
            j_idx = 1;          // Start at Display A
            numberchannels = 1; // Start at channel 1
            current_screen = Screen_ChannelSelection;
            force_redraw = true;
        }
        else if (return_pressed)
        {
            current_screen = Screen_OperatingMode;
            force_redraw = true;
        }
        break;
    }

    // Increment channel selection if SW1 pressed, decrement if SW2 pressed
    case Screen_ChannelSelection:
    {
        bool value_changed = false;
        if (b1_pressed)
        {
            numberchannels++;
            value_changed = true;
        }
        if (b2_pressed)
        {
            numberchannels--;
            value_changed = true;
        }

        if (value_changed)
        {
            numberchannels = (numberchannels > 7) ? 1 : (numberchannels < 1 ? 7 : numberchannels);
            UpdateChannelSelection();
        }

        // if enter pressed, increment j_idx to go to next display
        if (enter_pressed)
        {
            if (j_idx < numberdisplays)
            {
                j_idx++;
                numberchannels = 1;
                UpdateChannelSelection();
            }
            else
            {
                current_screen = Screen_DisplayChannels;
                force_redraw = true;
            }
        }
        else if (return_pressed)
        {
            if (j_idx > 1)
            {
                j_idx--;
                numberchannels = 1;

                // wrap in conditional statement to prevent random black box being drawn for j_idx = 6 (out of array bounds)
                if (idx < 6)
                {
                    Rectf(FindCenterX(xcord[idx], 100, "0", 1),
                          FindCenterY(ycord[idx] - (touch_init * 40), 42 - (touch_init * 6), "0", 1),
                          14, 20, BLACK);
                }

                UpdateChannelSelection();
            }
            else
            {
                current_screen = Screen_NumberDisplays;
                force_redraw = true;
            }
        }
        break;
    }
    }

    // update the 'previous state' trackers for the next loop pass

    /*
    // LEDs and cursor used for tracking touch, not included in final implementation
    if(current_screen != Screen_TouchCalibration) {
        if (touch_init) {
            if (touch_triggered) {
                // waits for voltage to stabilize before taking reading
                // used because some boards have high resistance/noisier environment and sometimes miss touches (board #6)
                uint16_t current_val = CalculateTouch_Stable();
                uint16_t sensitivity = 500;

                static uint16_t last_X = 0;
                static uint16_t last_Y = 0;
                // finger being dragged (not likely to be used in official implementation, since touch screen acts more as button presses? but maybe)
                static uint8_t is_dragging = 0;
                static uint8_t filter_block_count = 0;

                if (current_val < (touch_baseline - sensitivity)) {
                    //P5OUT &= ~BIT4;   // LED ON

                    uint16_t new_Y = ReadTouchY();
                    uint16_t new_X = ReadTouchX();

                    if (!is_dragging) {
                        X_Cord = new_X;
                        Y_Cord = new_Y;
                        is_dragging = 1;
                        filter_block_count = 0;
                    } else {
                        int16_t dx = (int16_t)new_X - (int16_t)last_X;
                        int16_t dy = (int16_t)new_Y - (int16_t)last_Y;
                        if (dx < 0) dx = -dx;
                        if (dy < 0) dy = -dy;

                        // "self-healing" delta filter
                        // checks to see if new data value is valid (counteracts voltage spikes)
                        if (dx < 25 && dy < 25) {
                            X_Cord = new_X;
                            Y_Cord = new_Y;
                            filter_block_count = 0; // if new coordinate within range, set new reference point
                        } else {
                            // increment counter if jump was too large
                            filter_block_count++;

                            // if jump is blocked more than 8 times in a row but the screen is still being touched
                            // assign new position because the filter has become stuck on a bad reading
                            if (filter_block_count > 8) {
                                X_Cord = new_X;
                                Y_Cord = new_Y;
                                filter_block_count = 0;
                            }
                        }
                    }

                    last_X = X_Cord;
                    last_Y = Y_Cord;
                    Rectf(X_Cord, Y_Cord, 2, 2, CYAN);

                } else {
                    //P5OUT |= BIT4;    // LED OFF
                    is_dragging = 0;        // reset
                    filter_block_count = 0; // clear counter


                    // re-enable interrupt
                    touch_triggered = 0;
                    //P1IFG &= ~BIT0;
                    //P1IE |= BIT0;
                }
            }
        }
    }
    */
}