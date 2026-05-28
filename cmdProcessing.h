/*
 * cmdProcessingTest.h
 *
 *  Created on: Feb 12, 2025
 *      Author: engcoop#3
 */

#ifndef CMDPROCESSING_H_
#define CMDPROCESSING_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"

typedef int8_t Schar;
typedef uint8_t Uchar;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef unsigned int uint; // Standard 'uint' is 32-bit on RP2350
typedef int16_t Int16;
typedef int32_t Int32;
typedef char Boolean;
typedef uint8_t uint8;
typedef uint32_t DWORD;
typedef int BOOL;

// --- 2. BIT MANIPULATION MACROS ---
#define testBitNo(var, bit_no) (var & (1u << bit_no))
#define setBitNo(var, bit_no) (var |= (1u << bit_no))
#define clearBitNo(var, bit_no) (var &= ~(1u << bit_no))
#define updateBitNo(var, bit_no, src) (var = (var & (~(1u << bit_no))) | (src & (1u << bit_no)))
#define writeBitNo(var, bit_no, val) (var = (var & (~(1u << bit_no))) | (val << bit_no))

#define setBit(var, bit_value) (var |= (bit_value))
#define clearBit(var, bit_value) (var &= ~(bit_value))
#define toggleBit(var, bit_value) (var ^= (bit_value))
#define testBit(var, bit_value) (var & (bit_value))

// --- 3. BIT DEFINITIONS (0-31) ---
// Using 'u' suffix to ensure unsigned 32-bit constants
#define BIT0 (0x00000001u)
#define BIT1 (0x00000002u)
#define BIT2 (0x00000004u)
#define BIT3 (0x00000008u)
#define BIT4 (0x00000010u)
#define BIT5 (0x00000020u)
#define BIT6 (0x00000040u)
#define BIT7 (0x00000080u)
#define BIT8 (0x00000100u)
#define BIT9 (0x00000200u)
#define BITA (0x00000400u)
#define BITB (0x00000800u)
#define BITC (0x00001000u)
#define BITD (0x00002000u)
#define BITE (0x00004000u)
#define BITF (0x00008000u)

#define BIT10 BITA
#define BIT11 BITB
#define BIT12 BITC
#define BIT13 BITD
#define BIT14 BITE
#define BIT15 BITF

// Extended 32-bit range for RP2350
#define BIT16 (1u << 16)
#define BIT17 (1u << 17)
#define BIT18 (1u << 18)
#define BIT19 (1u << 19)
#define BIT20 (1u << 20)
#define BIT21 (1u << 21)
#define BIT22 (1u << 22)
#define BIT23 (1u << 23)
#define BIT24 (1u << 24)
#define BIT25 (1u << 25)
#define BIT26 (1u << 26)
#define BIT27 (1u << 27)
#define BIT28 (1u << 28)
#define BIT29 (1u << 29)
#define BIT30 (1u << 30)
#define BIT31 (1u << 31)

// --- 4. APPLICATION LOGIC BITS ---
#define SinglePhase_eq0_3ph_eq1_Bit BIT11
#define ADC0_DISPLAY1_BIT BIT12
#define ADC0_DISPLAY2_BIT BIT13
#define ADC0_DISPLAY3_BIT BIT14

#define Command_Executing_eq1_Bit BIT24
#define ButtonTest_eq1_Bit BIT0

// Button/LED Bits (Mapped to Port 5 logic)
#define Button1_Bit BIT0
#define Button2_Bit BIT1
#define Button3_Bit BIT2
#define Button4_Bit BIT3
#define LED1_Bit BIT4
#define LED2_Bit BIT5
#define LED3_Bit BIT6
#define LED4_Bit BIT7

// --- 5. SYSTEM CONSTANTS ---
#define CMD_LEN 4
#define HOST_RX_BUFF_LEN 256
#define HOST_XMT_BUFF_LEN 256
#define CaRet 0x0D
#define CharAvailableFlag BIT0
#define CharEchoFlag BIT2
#define CmdAvailFlag BIT4

// Error Codes
#define OK 0x00
#define NO_ERROR OK
#define SIO_CMD_ERROR 0x01
#define BAD_SIO_CMD_ERR SIO_CMD_ERROR
#define PARAM_ERROR 0x02
#define BAD_VALUE_ERR PARAM_ERROR

// Output Macros
#define PutChar putchar
#define CPUTS PutStr
#define cputs CPUTS

// Calibration/Point Indices
#define Y1_low_point 0
#define X1_low_point 1
#define Y2_high_point 2
#define X2_high_point 3

#define SHOW_LONG 0x10
#define SHOW_Qfloat 0

// Buffer Sizes
#define HOST_XMT_BUFF_LEN 256
#define HOST_RX_BUFF_LEN 256
// Functions declared elsewhere (LCDProcessing.h)
void DisplayChannels(void);
void LCDSetup(void);

//---------------------------------------------------------------------------------Update LCD by ADC-----------------------------------------------------------------------------------------------------------
/**
 * @brief  Increments a counter each full completion of cycling through
 * channels. Calls @ref Output_string_on_LCD_by_index for each selected
 * channel (decided by @ref numberdisplays).
 *
 * @return true always
 * @see    loop_ticks
 * @see    numberdisplays
 * @see    ch
 */
bool UpdateLCD_byADCvalue(void);

/**
 * @brief  Displays the function with the raw float input (not converted value).
 * Used primarily for testing.
 * @param[in] inp Used to determine if value is out of bounds
 * @param[in] LCD_screen_position The index of the screen element to update.
 * @return true always
 * @note   Currently no uses in the code, may be ultimately removed.
 * @see    LCD_screen_position
 * @see    Ypos_onScreen
 */
bool Output_string_on_LCD(float inp, int16_t LCD_screen_position);

/**
 * @brief Formats and renders a specific ADC channel value to the LCD.
 *
 * This function handles:
 * 1. Checking if the update interval has elapsed.
 * 2. Fetches raw ADC data and scales it using constant 0.0000001788139343261719.
 * 3. Applies a 0.05V hysteresis check to prevent screen flicker (only udpates value if change > 0.05V)
 * 4. Manually builds a right-aligned, signed string (e.g., "+ 12.3") in a shared buffer.
 * 5. Dynamically adjusts font size and positioning based on the total number of displays.
 *
 * @param[in] LCD_screen_position The index of the screen element to update (0 to numberdisplays-1).
 * @return true always (indicating completion or skipped frame).
 * @note Global Dependencies:
 * - Uses @ref loop_ticks for timing control.
 * - Reads from @ref ADCbuffer and @ref LCD_ch_source.
 * - Modifies @ref shared_lcd_buffer and @ref last_displayed_values.
 * @warning This function resets @ref loop_ticks to 0 when the final display index is processed.
 */
bool Output_string_on_LCD_by_index(int16_t LCD_screen_position);

/**
 * @brief  Resets the LCD channel and ADC mapping to the default assignment originally
 * created by @ref LCD_ch_source.
 *
 * @note    Performs this reset by setting the value to an infeasible value, such as -9999.9
 * so that the value is out of bounds and will default to the pre-determined array.
 * @see    LCD_ch_source
 * @see    loop_ticks
 * @see    last_displayed_values
 */
void ResetLCDMapping(void);

/**
 * @brief  Resets the LCD channel and ADC mapping so that the @ref diff hysteresis check can
 * be bypassed upon a screen refresh or flip.
 *
 * @note    Performs this reset by setting the value to an infeasible value, such as -9999.9
 * so that the @diff will always be greater than 0.05, causing the value to be printed.
 * @warning This is meant to be used primarily for rlcd and flip commands. If the code is
 * modified to be constantly outputting instead of using a counter, the value of -9999.9
 * could potentially be printed to the screen.
 * @see    last_displayed_values
 */
void InvalidateLCDCache(void);

/**
 * @brief  Converts float to a null-terminated string in scientific notation.
 * @param[in]  x The floating point value.
 * @param[out]  Pointer to a character buffer where the result is stored.
 * @note    Used in putfloat and nowhere else. Precision to 5 decimal places.
 * @warning  Buffer must be at least 13 bytes long.
 * @see    putfloat
 */
void ftoa(float x, char *p);

/**
 * @brief  Converts float to a string and sends floating value out.
 * @param[in] x The floating point value.
 * @param[in] n The number of decimal places to include.
 * @return A pointer to the start of @ref buffer
 * @note    Not currently implemented anywhere in the code.
 * @see    ftoa()
 */
char *putfloat_n(float x, int n);

/**
 * @brief  Print on screen float in format tttttt.dddd.
 * t - number of positions before decimal dot (integer part), including sign,
 * d - number of positions after dot (fraction part)
 *  Function performs rounding
 * If format t too small for integer part - on the first position
 *    function prints error mark '#' and integer part is incorrect
 * If format t too small after rounding for integer part +999.999->+1000.000
 *  - on the first position function prints error mark '&'
 *    and integer part is incorrect
 * If format string is incorrect function prints error message "FrmtERR"
 * @param[in] f  Format string. Must strictly follow the pattern "%t.df" (or %t.dF).
 * - 't': Digit (0-9) defining positions *before* the decimal point.
 * (Values < 2 are automatically clamped to 2 for sign/digit).
 * - 'd': Digit (0-9) defining positions *after* the decimal point.
 * @param[in] fx The floating-point number to be processed and displayed.
 * @return void
 * @note    Not currently implemented anywhere in the code.
 * @warning TOTAL OUTPUT STRING LENGTH <= 19, Max allowed %9.9f
 * where *f defines passed string "%t.df" or "%t.dF"
 */
void float_print(const char *f, const float fx);

/**
 * @brief  Put floating value out over UART.
 * @param[in] x Float value for the output over UART.
 * @return void
 * @note    Not currently implemented anywhere in the code.
 * @see    ftoa
 * @see    zerostr
 */
char *putfloat(float x);

/**
 * @brief Converts an integer to a null-terminated string using a specified base.
 * This function converts the integer @p num into its string representation in
 * @p base. It handles bases from 2 up to 36 (mapping values 10-35 to 'a'-'z').
 * The conversion is performed by extracting digits in reverse order and then
 * reversing the string in-place.
 * @param[in]  num   The integer value to be converted.
 * @param[out] str   Pointer to the destination buffer. Must be pre-allocated.
 * @param[in]  base  The numerical base for conversion (e.g., 10 for decimal, 16 for hex).
 * @return void
 * @note
 * - Negative numbers are only handled (prefixed with '-') if @p base is **10**.
 * - For other bases, @p num is treated as an unsigned value or converted
 * using its absolute value without a sign.
 * @warning
 * - The caller must ensure @p str has sufficient space to hold the result,
 * including the null terminator. For base 2, an `int` could require up to 34 bytes.
 * @see float_print
 * @see putfloat_n
 */
// void itoa(int num, char* str, int base);

/**
 * @brief Converts a floating-point number to a signed decimal string.
 * This function decomposes a float into its integer and fractional components.
 * It uses @ref itoa to convert these components into strings and concatenates
 * them into the provided @p buffer with a leading sign and an optional decimal point.
 * @param[in]  num           The float value to convert.
 * @param[out] buffer        Pointer to the destination char array.
 * @param[in]  decimalPlaces The number of digits to extract after the decimal point.
 * @return void
 * @note
 * - Always prefixes the output with a '+' or '-' sign.
 * - The fractional part is truncated, not rounded (e.g., 0.99 with 1 decimal place becomes "0.9").
 * @warning
 * - **Buffer Safety:** The @p buffer must be large enough to hold the sign, integer digits,
 * decimal point, fractional digits, and the null terminator.
 * - Relies on the external @ref itoa function for integer conversion.
 * @see itoa
 * @see putfloat_n
 */
void floatToString(float num, char *buffer, int decimalPlaces);

/**
 * @brief Converts a string to an integer.
 * Parses the C-string @p str and interprets its content as a numeric value.
 * The function stops at the first non-digit character it encounters.
 * @param[in] str The null-terminated ASCII string to be converted.
 * @return The converted integer value. Returns 0 if no valid digits were found
 * at the start of the string.
 * @note
 * - This implementation performs the conversion using the formula:
 * $res = res \times 10 + (*str - '0')$.
 * - It does not handle leading whitespace or a leading '+' or '-' sign.
 * @warning
 * - Does not check for integer overflow. If the number in the string exceeds
 * the maximum value of a signed 16-bit or 32-bit int (depending on your architecture),
 * the result will wrap around.
 */
int atoi(const char *str);

//---------------------------------------------------------------------------------Command Processing-----------------------------------------------------------------------------------------------------------
/**
 * @brief  Prints a test menu of the various commands that can be used.
 *
 * @return void
 */
void Print_Help(void);

/**
 * @brief  Prints the product number, version, and date.
 *
 * @return void
 */
void Print_FW_Version(void);

/**
 * @brief  Prints test message for version selection, "version command selected..."
 *
 * @return void
 * @note    Not currently implemented anywhere in the code.
 */
void Print_vers(void);

/**
 * @brief  Prints test message for menu selection, "menu command selected..."
 *
 * @return void
 */
void Print_menu(void);

/**
 * @brief  Prints test message for initialization, "Initialization Selection:"
 *
 * @return void
 * @note    Eventually plan to implement as an initialize function (see ATMEL).
 */
void Print_init(void);

/**
 * @brief  Prints test message for default, "dflt command selected..."
 *
 * @return void
 * @note    Eventually plan to implement a default settings function (see ATMEL).
 */
void Print_dflt(void);

/**
 * @brief  Prints test message for save, "save command selected..."
 *
 * @return void
 * @note    Eventually plan to implement a save setting function (see ATMEL). This will involve INFOA-INFOD and saving to FLASH.
 */
void Print_save(void);

/**
 * @brief  Used to set the phase/get the phase depending on syntax.
 * "phas" will get the phase for the user and print it in the terminal.
 * "phas>1-ph" will set the phase to 1-phase.
 * "phas>3-ph" will set the phase to 3-phase.
 *
 * @return void
 * @note    Screen will need to reflect changes made, such as switching between 1-phase and 3-phase power.
 * @see    Put_CMD_as_chars()
 */
void SetGetPhase(void);

/**
 * @brief  Allows for channels to be connected to specific displays. Uses
 * syntax test>a#d$ where # represents channel number (0-6) and $ represents display (0-2).
 *
 * @return void
 * @note    After overhaul of @ref UpdateLCD_byADCvalue, only test>dflt currently works.
 * @todo    Possibly eliminate switch-case logic and make easier to read/less resource intensive.
 * @warning Current implementation only allows for selection between 3 displays, not all 7 (due to previous display).
 */
void testLCD(void);

/**
 * @brief  Prints the ADC value from each channel in the serial terminal.
 *
 * @return void
 * @note    Only gives readings after set-up is complete since DMA set-up occurs after menu
 * setup has finished.
 */
void Acquire_ADC_raw_counts(void);

/**
 * @brief Sets or retrieves the UART baud rate via command string parsing.
 * This function parses @ref CommStr to either update the system baud rate
 * (if '=' is present) or report the current rate. It directly manipulates
 * MSP430-style UART registers (UCA0BRW, UCA0MCTLW).
 * @return void
 * @note
 * - **Baud Selection:** Supports 9600 and 115200.
 * - **String Evaluation:** Uses a unique packed-character comparison for the
 * @p param (e.g., `'1' + 256 * '1'`...) to identify the baud rate string.
 * - **Blocking Behavior:** When changing baud rates, the function blocks execution
 * until the user presses a key at the new rate to synchronize the terminal.
 * @warning
 * - Directly modifies @ref UCA0CTLW0, @ref UCA0BRW, and @ref UCA0MCTLW.
 * - Changing the baud rate will terminate the current terminal session
 * until the host client (e.g., PuTTY) is reconfigured.
 * @see Convert_4_ASCII_to_Uint32
 * @see Is_Numeric
 * @see Send_RCI_Param_Error
 */
void SetGetBaudRate(void);

/**
 * @brief  Allows the user to either set or retrieve the current unit
 * setting for the product. The options include 24V, 48V, 125V, and 250V.
 *
 * @return void
 * @note    Actually reflecting the unit changes not yet implemented in the code, and
 * since there have been no default configurations created, using the command before
 * setting will result in a reading of 000.0V.
 * @warning The chosen voltage values reflect those of ATMEL and standard. If
 * a different voltage monitoring was desired, new HI/LOW/etc. values would
 * need to be created/determined.
 * @todo    Set-up default unit configuration and modify the code to reflect
 * the actual unit changes (HI/LOW adjustment).
 */
void SetGetVoltageRange(void);

/**
 * @brief  Allows user to enable/disable character ECHO in PuTTY terminal. This
 * is unrelated to PuTTY's local echo setting option.
 *
 * @return void
 * @note    Not currently implemented other than allowing selection and
 * telling the user what the current selection is. It does not actually
 * reflect the changes made based on the user input. This most likely has to do with
 * one of the commands in COMMAND PROCESSING SUPPORT.
 */
void Echo_Enab_Disab(void);

/**
 * @brief  Prints the ADC value for channel 0 specifically in the serial terminal.
 *
 * @return void
 * @note    Number printed in terminal may not exactly match the one displayed on screen due to how quickly
 * the ADC is taking values and the LCD is displaying them.
 * @see    Acquire_ADC_raw_counts
 */
void adc0_acquire(void);

/**
 * @brief  SetGet_param() updates / shows EITHER 'Qfloat' OR 'long', can add "verbose" short help
 * "float_offset" == 0...15 is where search for float string should begin.
 * offset in command parameter string where float starts (i.e, after "="),delay= 6th index, 7th position
 * It is additional string position starting from CMD_LEN == 4
 * ALSO, if float offset is < 16, show 'Qfloat', otherwise, show 'long'
 * @param[in]     float_offset  A bitmask/offset used for parsing logic:
 * - Determines the start position in @ref CommStr.
 * - If < @ref SHOW_LONG, the value is handled as a float.
 * - If >= @ref SHOW_LONG, the value is cast to a long.
 * @param[in]     minValue      The minimum allowable value for the parameter.
 * @param[in]     maxValue      The maximum allowable value for the parameter.
 * @param[in,out] Qf_var_ptr    Pointer to the global variable being modified or queried.
 * Can point to either a @c float or a @c long based on @p float_offset.
 * @param[in]     verb_msg      A descriptive "verbose" string sent to the user during
 * a query to explain what the parameter represents.
 * @return void
 * @note    Not yet implemented. Not entirely sure how to implement to UMM from ATMEL.
 * @see    SetGetCalParam
 */
void SetGet_param(int float_offset, float minValue, float maxValue, float *Qf_var_ptr, char *verb_msg);

/**
 * @brief  set/get will define "Calibration PARameter" or "factor" for a particular cal, command takes 2 arguments.
 * "cpar#$=GGG.GGG\r" SET calibration parameter.
 * "cpar#$": returns ->"cpar#$=ggg.gg\r" - GET calibration parameter
 * Command itself takes 4 chars in command string, [0] to [3]
 * calibration allows to use linear interpolation of input value == ADC_counts into Engineerng Unit value, i.e. Voltage, Current, etc.
 * calibration factors are stored as two sets of X,Y coordinates, {X1,Y1} low cal point, {X2,Y2} high cal point, which define linear interpolation
 *
 * @return void
 * @note    Not entirely sure how to implement to UMM from ATMEL. X1 must not be equal to X2, and X1 should be a lower value than X2.
 * @see    SetGet_param
 */
void SetGetCalParam(void);

/**
 * @brief  Command used to determine the status of the buttons (pressed or unpressed).
 * Basis of the command used in various LCD functions to navigate menu screens.
 *
 * @return void
 * @note    The "test" mode built in doesn't seem to do anything (anymore).
 */
void SetGetButtonStateMan(void);

/**
 * @brief  Allows for different channel connections while using pwmo.
 * If a different channel than the one currently selected is set in UpdateDutyChannel,
 * then the oscilloscope will read the wrong signal because the channel is not correctly set.
 *
 * @return void
 * @note    Currently only works with channels 0, 1, and 2.
 */
void UpdateDutyChannel(void);

/**
 * @brief  Can be called on any screen and will reset it, returning the screen to it's
 * exact display before it was refreshed.
 *
 * @return void
 * @see    LCDSetup
 * @see    InvalidateLCDCache
 * @see    DisplayChannels
 */
void RefreshLCDScreen(void);

/**
 * @brief  Flips the screen portrait mode when called.
 *
 * @return void
 * @see    LCDSetup
 * @see    InvalidateLCDCache
 * @see    DisplayChannels
 */
void FlipScreen(void);

//---------------------------------------------------------------------------------COMMAND PROCESSING SUPPORT-----------------------------------------------------------------------------------------------------------
/**
 * @brief Prints a formatted string as a C-style line comment.
 * Prepends the input string with a comment prefix (" // ") and outputs
 * it to the standard output.
 * @param comment Pointer to the null-terminated string to be printed.
 * @return void
 */
void Send_comment(char *comment);

/**
 * @brief Sends a comment only if verbose response mode is enabled.
 * Checks the CmdVerboseResponse bit in the host configuration. If the bit
 * is set, the provided string is passed to Send_comment for output.
 * @param comment Pointer to the null-terminated string to be printed.
 * @return void
 */
void Send_verbose_comment(char *comment);

/**
 * @brief Decomposes a command code into individual characters and outputs them.
 * Retrieves a 32-bit command word from the current command index, then
 * extracts and transmits each of the four bytes sequentially (from least
 * significant to most significant) using PutChar.
 * @note This function assumes a little-endian byte order for the command code
 * mapping and relies on the global rci array and CMD_index.
 */
void Put_CMD_as_chars(void);

/**
 * @brief Sets a parameter error status and transmits a detailed error message via UART.
 * This function updates the global ErrorStatus to PARAM_ERROR and constructs a
 * formatted error string containing the invalid received buffer and the
 * expected valid parameters. The message is transmitted character-by-character
 * by polling the UART (UCA0) TX interrupt flag.
 * @param valid_msg A pointer to a string describing the valid parameter options
 * or expected format.
 * @note This function performs direct hardware register polling (UCA0IFG) and
 * will block execution until the entire error message is shifted out.
 * @warning The internal errorBuf has a fixed size of 256 bytes; ensure combined
 * input lengths do not cause a buffer overflow.
 */
void Send_RCI_Param_Error(char *valid_msg);

/**
 * @brief Transmits a null-terminated string via UART and returns the character count.
 * This function iterates through the input string, polling the UART transmission
 * interrupt flag (UCTXIFG) before sending each byte. It utilizes critical section
 * management by disabling interrupts during the actual write to the TX buffer.
 * @param Str Pointer to the null-terminated string to be transmitted.
 * @return uint16 The total number of characters successfully processed and sent.
 * @note This function includes a blocking delay of 48,000 cycles (approximately 3ms
 * at 16MHz) after the final character is queued to allow for hardware transmission
 * completion.
 * @warning This is a blocking call; it will hang if the UART hardware is not
 * properly initialized or if the TX flag never clears.
 */
uint16 PutStr(char *Str);

/**
 * @brief Converts a lowercase character to uppercase.
 * Uses an optimized range check and arithmetic offset to convert characters
 * in the range ['a', 'z'] to ['A', 'Z']. If the input character is not
 * a lowercase letter, it is returned unchanged.
 * @param ch The character to be converted, passed as an integer.
 * @return int The uppercase equivalent if the input was lowercase; otherwise, the original character.
 * @note This implementation avoids branching by using a single unsigned comparison
 * to validate the character range.
 */
// int toupper(int ch);

/**
 * @brief Converts an entire null-terminated string to uppercase in-place.
 * Iterates through the provided string and applies a case conversion to each
 * character using the toupper() utility. The original memory buffer is modified
 * directly.
 * @param in_str Pointer to the null-terminated string to be converted.
 * @return char* Pointer to the beginning of the modified string (in_str).
 * @note This function performs an in-place modification; the caller must
 * ensure the input buffer is writable (e.g., not a string literal in read-only memory).
 */
char *ToUpper(char *in_str);

/**
 * @brief Validates if a string contains a numeric representation.
 * Scans the input string to determine if it represents a valid numeric value,
 * accounting for scientific notation ('e'/'E'), signs ('+', '-'), and decimals ('.').
 * The function ignores leading spaces but will return false if it encounters
 * non-numeric characters that do not belong to a standard float/integer format.
 * @param strp Pointer to the null-terminated string to be validated.
 * @return int Returns true (non-zero) if the string is numeric or ends with
 * a carriage return (CaRet) after valid input; returns false (0) otherwise.
 * @note If the string consists only of a carriage return without any preceding
 * digits, it is considered invalid.
 */
int Is_Numeric(char *strp);

/**
 * @brief Converts an uppercase character to lowercase.
 * Performs an optimized conversion of characters in the range ['A', 'Z'] to
 * lowercase ['a', 'z']. The function uses an unsigned range check to determine
 * if the character is uppercase before applying the bit-distance offset (32).
 * @param ch The character to be converted (expected as a Uchar/unsigned char).
 * @return uint32 The lowercase equivalent if the input was uppercase; otherwise,
 * the original character.
 * @note This implementation leverages a branchless-style comparison by checking
 * if the offset value falls outside the lowercase alphabet range.
 */
uint32_t toLower(Uchar ch);

/**
 * @brief Packs four ASCII characters into a single 32-bit unsigned integer.
 * Each character is first converted to lowercase before being shifted and
 * combined into a 4-byte word. The resulting uint32 is constructed in a
 * little-endian style layout:
 * - Byte 0 (LSB): pstr[0]
 * - Byte 1: pstr[1]
 * - Byte 2: pstr[2]
 * - Byte 3 (MSB): pstr[3]
 * @param pstr Pointer to an array of at least four characters (Uchar).
 * @return uint32 The resulting 32-bit packed integer.
 * @note This function uses explicit multipliers (256, 65536) to perform
 * bit-shifting logic across the word.
 */
uint32_t Convert_4_ASCII_to_Uint32(Uchar *pstr);

/**
 * @brief Converts a single ASCII hexadecimal character to its numeric value.
 * Maps ASCII characters '0'-'9' to 0-9 and 'A'-'F' (case-insensitive) to 10-15.
 * If the input character is not a valid hexadecimal digit, a global error
 * status is set and an error code is returned.
 * @param in_char The ASCII character to be converted (e.g., 'a', 'F', or '5').
 * @return int The numeric value (0-15) of the hex character, or -1 if the
 * input is invalid.
 * @note If an invalid character is detected, the global variable ErrorStatus
 * is updated to BAD_VALUE_ERR.
 */
int ASCIItoHexChar(char in_char);

/**
 * @brief Transmits two characters packed within a 16-bit integer.
 * Splits the input integer into two 8-bit characters and outputs them
 * sequentially. The high byte (bits 8-15) is sent first, followed by
 * the low byte (bits 0-7).
 * @param TwoChars The 16-bit integer containing the two characters to be sent.
 * @return void
 * @note This function follows a big-endian transmission order regardless of
 * the host architecture's endianness.
 */
void PutTwoChars(int TwoChars);

/**
 * @brief Transmits a Carriage Return (CR) and Line Feed (LF) sequence.
 * This function constructs a 16-bit word containing the ASCII values
 * for '\r' (0x0D) and '\n' (0x0A) and passes them to PutTwoChars for
 * sequential transmission.
 * @return void
 * @note This is a convenience wrapper for standard EOL (End of Line)
 * termination in serial communication.
 */
void SendCrLf(void);

/**
 * @brief Processes characters from the host receive buffer and handles terminal echoing.
 * This function parses the incoming data in rt.HostRxBuff, managing special control
 * characters such as backspace ('\b') and delete (0x7F). It handles buffer pointer
 * synchronization between the raw host buffer and the echo buffer, performs
 * character echoing to the terminal if the CharEchoFlag is set, and manages
 * basic line editing logic.
 * @return void
 * @note
 * - Handles backspace by physically moving pointers back and sending "\b \b"
 * to clear the character on the terminal.
 * - If a null terminator (0) is encountered, it may send a CRLF and exits.
 * - Uses a bitwise mask (HOST_RX_BUFF_LEN - 1) for the echo pointer, implying
 * the buffer length must be a power of two.
 * - Clears the CharAvailableFlag bit in the rt.Host status register upon completion.
 */
void processChar(void);

/**
 * @brief Resets the host receive buffer and associated status flags.
 * Synchronizes the host and echo buffer pointers to zero and clears the
 * entire contents of the receive buffer using memset. It also clears
 * the command and character availability flags in the host status register
 * to prepare for new incoming data.
 * @return void
 * @note The buffer is cleared before external status signaling to prevent
 * race conditions where new incoming data might be overwritten or
 * misidentified as part of a previous command.
 */
void ClearRxBuffer(void);

/**
 * @brief Formats and transmits a string to the PC via UART.
 * This function takes a string (typically a format string or a constant string
 * from memory), processes it into a local temporary buffer using sprintf, and
 * transmits the result character-by-character over the UCA0 UART interface.
 * @param Msg A pointer to the character string to be transmitted.
 * (Note: Uses the custom 'FL' memory qualifier).
 * @return void
 * @note
 * - The function utilizes a fixed-size local buffer of 256 bytes.
 * - This is a blocking call that polls the UCTXIFG flag until each character is sent.
 * - It provides "Fixed Printf Support" by manually handling the UART TX buffer.
 * @warning Calling this with a string exceeding 255 characters (plus null terminator)
 * will cause a stack-based buffer overflow. Using `sprintf` with a direct
 * user-controlled `Msg` may also pose security risks if format specifiers
 * are present in the input.
 */
void SendMsgToPC(const char *Msg);

/**
 * @brief Main parser for Remote Control Interface (RCI) commands.
 * This function orchestrates the command-line interface logic. It ensures incoming
 * characters are processed, converts the first four characters of the receive
 * buffer into a 32-bit command word, and searches for a match within the global
 * `rci` command table. If a match is found, the associated function pointer
 * is executed.
 * @return Uchar Returns true if a command was processed or the parser reached
 * completion; returns false if no command is currently available in the buffer.
 * @note
 * - Automatically handles terminal echoing and backspacing via processChar().
 * - Sets the global `CMD_index` to the index of the matched command.
 * - Resets the receive buffer and status flags via ClearRxBuffer() after execution.
 * - Provides standardized feedback to the host, such as ">~OK", ">~Doing CMD",
 * or error codes like ">~Unrecognized".
 * @warning This function uses direct function pointer casting and execution
 * `((void(*)())(rci[i].f_ptr))()`. Ensure all functions in the rci table
 * match this `void func(void)` signature to avoid stack corruption.
 */
bool ParseRCI(void);

void ServiceSerialHardware(void);

typedef struct
{
    float low_meas;
    float calptlow;
    float high_meas;
    float calpthigh;
} Calibr2points;

typedef struct
{
    uint32 cmd_code;     // Changed from cmd_id to cmd_code
    void (*f_ptr)(void); // Changed from handler to f_ptr
} t_rci_commands;

extern const t_rci_commands rci[];

// Structs for Settings and System Data
typedef struct
{
    uint16_t high_bat_threshold;
    uint16_t low_bat_threshold;
    uint16_t minus_gf_threshold;
    uint16_t plus_gf_threshold;
    uint16_t ripple_voltage_threshold;
    uint16_t ripple_current_threshold;
    uint16_t time_delay;
    uint16_t meter_address;
    uint16_t host_address;
    uint16_t baud_rate;
    uint16_t SavedStatusWord;
    uint8_t buzzer;
    uint8_t phase;
    uint8_t latch_state;
    uint8_t pulse;
    uint8_t disabled_alarms;
    uint8_t unit_type;
    uint8_t unit_index;
} SettingsStruct;

typedef union
{
    float Coord[4];
    Calibr2points Cal;
} Calibration, *CalPtr;

typedef struct
{
    uint16_t Data_Valid;
    uint16_t FWversion;
    uint16_t meter_address;
    uint16_t host_address;
    uint16_t dll_timeout;
    uint16_t xmt_delay;
    uint16_t baud_rate;
    uint16_t char_gap;

    uint8_t protocol;
    uint8_t protocol_parity;
    uint8_t dll_confirm;
    uint8_t app_confirm;
    uint8_t dll_retries;
    uint8_t extra_bytes[11];

    uint8_t unit_type;
    uint8_t input_type[6];
    uint8_t true_1mA_false_20mA;
    uint16_t V4;
    uint16_t V20;
    uint16_t extra_int16[2];

    uint8_t FrontBoardBytes[16];

    Calibr2points BatteryVolts;
    Calibr2points FaultVolts;
    Calibr2points MinusGndVolts;
    Calibr2points RippleVolts1ph;
    Calibr2points RippleVolts3ph;
    Calibr2points RippleCurr1ph;
    Calibr2points RippleCurr3ph;
    Calibr2points CurrentOut_I420;

    float extra_floats[8];
    SettingsStruct NV_UI;
} SYS_SPECIFIC_DATA;

typedef struct
{
    volatile uint32_t OperStatusWord;
    volatile uint16_t free_running_timer;
    uint8_t FrontInterfaceByte;
    int16_t battery_voltage;
    int16_t fault_voltage;
    int16_t minus_gnd_volts;
    int16_t ripple_current;
    int16_t ripple_voltage;
    int16_t plus_gnd_volts;

    uint8_t _4ma;
    uint8_t _20ma;
    uint8_t operating_protocol;
    uint8_t PulseMow;

    uint8_t registers;
    uint16_t first_register;
    uint16_t device_register;

    volatile uint8_t Host;
    volatile uint8_t HostTx_StrLen;
    volatile uint8_t HostTxPtr_IN;
    volatile uint8_t HostTxPtr_OUT;
    volatile uint16_t HostRxBuffPtr;
    volatile uint16_t EchoRxBuffPtr;
    char HostTxBuff[HOST_XMT_BUFF_LEN];
    char HostRxBuff[HOST_RX_BUFF_LEN];
    char *ParamPtr;
} RealTimeVars;

// Variable Declarations
extern SYS_SPECIFIC_DATA SysData;
extern RealTimeVars rt;
extern uint32_t ErrorStatus;

// Prototypes
uint32_t Convert_4_ASCII_to_Uint32(Uchar *pstr);
// --- Project Constants & Protocol Logic (KEEP THESE) ---
#define ASCII_TESTING
#define FW_VERSION 30
#define FW_ver_float (((float)(FW_VERSION) + 0.1f) / 10.0f)

#define CmdVerboseResponse BIT7

enum Protocols
{
    SETUP = 0x00,
    DNP3 = 0x11,
    MODBUS = 0x22,
    ASCII_CMDS = 0x33,
    ASCII_MENU = 0x44
};

enum Board_Addresses
{
    ALARM_WRITE = 0x10,
    ALARM_READ = 0x11,
    DISPLAY_WRITE = 0x40,
    DISPLAY_READ = 0x41,
    IO_WRITE = 0x82,
    IO_READ = 0x83,
    A_TO_D_WRITE = 0xD0,
    A_TO_D_READ = 0xD1
};

// --- TWI (I2C) Constants (KEEP the logic, DELETE the hardware bits) ---
#define TWI_TIMEOUT_ms 200
#define TWI_MSG_ALARMS 14
#define TWI_BATT_VOLTS 1
#define NOT_DONE 0
#define DONE 10

/* * NOTE: TWINT, TWEA, TWSTA, TWSTO, TWEN, TWIE, and TWSR_STATUS_MASK
 * are DELETED. These are hardware-specific to the old chip's I2C controller.
 * On the RP2350, the I2C peripheral handles these states internally
 * through the Pico SDK i2c_write/read functions.
 */

#endif /* CMDPROCESSING_H_*/