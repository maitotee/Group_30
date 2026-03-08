#ifndef _KEYPAD_H
#define _KEYPAD_H

#include <avr/io.h>
#include "stdutils.h"

/* -------- KEYPAD PORT CONFIGURATION -------- */

/* Rows = PK7 PK6 PK5 PK4  (A15 A14 A13 A12)
   Cols = PK3 PK2 PK1 PK0  (A11 A10 A9 A8) */

#define M_RowColDirection DDRK
#define M_ROW PORTK
#define M_COL PINK

/* rows output, columns input */
#define C_RowOutputColInput_U8 0xF0

/* -------- FUNCTION PROTOTYPES -------- */

void KEYPAD_Init(void);
void KEYPAD_WaitForKeyRelease(void);
void KEYPAD_WaitForKeyPress(void);
uint8_t KEYPAD_GetKey(void);

#endif