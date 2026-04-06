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

extern int diff_display[8];
extern int16_t saved_Ypos[7];
extern uint8_t LCD_ch_source[7];

void LCDSetup(void);
void ButtonLayout(void);
void ControlsDisplay(void);
void TouchScreenDecision(void);
void UpdateTouchHighlight(void);
void TouchCalibration(void);
void OperatingMode(void);
void UpdateOperatingModeSelection(void);
void NumberOfDisplays(void);
void UpdateNumberOfDisplays(void);
void ChannelSelection(void);
void UpdateChannelSelection(void);
void PresetConfigs(void);
void DisplayChannels(void);
void InitYPositions(void);
void WaitForInput(void);

#endif /* LCDPROCESSING_H_ */