/**
 * @file TouchScreeninit.h
 * @author engcoop#4 RW
 * @brief Header file for TouchScreeninit.c. Imported from CCS.
 * @version 1.0.0
 * @date 2026-04-15
 * @copyright Copyright (c) 2026
 */

#ifndef TOUCHSCREENINIT_H_
#define TOUCHSCREENINIT_H_

#include <stdint.h>
#include <stdbool.h>

#define MARGIN 600 // 150 * 4
// Converted constants to macros for better readability and maintainability
#define Y_PLUS 26
#define X_PLUS 27
#define Y_MINUS 22
#define X_MINUS 2

#define NUMBER_OF_TOUCH_CHANNELS 4

#define CALIBRATE_SAMPLES 16

#define CALI_BOUNDS_MIN_X 1000  // Low threshold for calibration (near 0V)
#define CALI_BOUNDS_MIN_Y 1000
#define CALI_BOUNDS_MAX_X 3000 // High threshold for calibration (near max voltage)
#define CALI_BOUNDS_MAX_Y 3500

#define CAPTURE_CALI_COORDS_SAMPLES 16

#define STABILIZATION_TIMEOUT 5  // if value doesnt stabilize after 5 tries, just return the current value (exit), prevents getting stuck in loop
#define DIFF_STABLE_THRESHOLD 80 // (20 * 4) = 80
#define STABLE_COUNT_MIN 2

#define CHECK_RELEASE 20
#define AVERAGE_SAMPLES_RELEASE 4

#define MIN_RAW 4096            // change based on ADC resolution 2^n
#define AVERAGE_READ_SAMPLES 18 // must be greater than 2 to allow for outlier removal

#define CALI_OFFSET_X 15
#define DISP_WIDTH 240
#define X_COMPENSATION 180 // for 10-bit ADC, 45 was about 14 pixels, so it must be scaled up to 180 (?)

#define CALI_OFFSET_Y 15
#define DISP_HEIGHT 320
#define Y_COMPENSATION 180 // for 10-bit ADC, 45 was about 14 pixels, so it must be scaled up to 180 (?)

extern volatile int screenTouched;
extern int touch_init;

void TouchScreeninit(void);

void TouchScreen_deinit(void);

void TouchInterrupt(unsigned int, uint32_t);

void CalibrateTouch(void);

uint16_t CalculateTouch(void);

// formerly bool CaliBoundsCheckTS(void)
bool CaliBoundsCheckTouch(void);

// formerly CaptureCaliCoordsTS(Void)
void CaptureCaliCoordsTouch(void);

uint16_t CalculateTouch_Stable(void);

// formerly void WaitForReleaseTS(void)
void WaitForTouchRelease(void);

void TouchScreenReset(void);

uint16_t ReadTouchX(void);

uint16_t ReadTouchX_Raw(void);

uint16_t ReadTouchY(void);

uint16_t ReadTouchY_Raw(void);

uint8_t Display_Bounds_Check(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t);
#endif /* TOUCHSCREENINIT_H_ */
