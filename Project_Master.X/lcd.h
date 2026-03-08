#ifndef LCD_H
#define LCD_H

#include <inttypes.h>
#include <avr/pgmspace.h>

/* display configuration */

#define LCD_LINES 2
#define LCD_DISP_LENGTH 16
#define LCD_LINE_LENGTH 0x40

#define LCD_START_LINE1 0x00
#define LCD_START_LINE2 0x40
#define LCD_START_LINE3 0x14
#define LCD_START_LINE4 0x54

#define LCD_WRAP_LINES 0

/* use IO mode */
#define LCD_IO_MODE 1


/* ---------- LCD PIN CONFIGURATION ---------- */
/* mapping for Arduino Mega 2560 */

#define LCD_DATA0_PORT PORTC
#define LCD_DATA0_PIN  7   // D30 (LCD D4)

#define LCD_DATA1_PORT PORTC
#define LCD_DATA1_PIN  5   // D32 (LCD D5)

#define LCD_DATA2_PORT PORTC
#define LCD_DATA2_PIN  3   // D34 (LCD D6)

#define LCD_DATA3_PORT PORTC
#define LCD_DATA3_PIN  1   // D36 (LCD D7)

#define LCD_RS_PORT PORTC
#define LCD_RS_PIN  4      // D33

#define LCD_RW_PORT PORTC
#define LCD_RW_PIN  2      // D35

#define LCD_E_PORT PORTC
#define LCD_E_PIN  0       // D37


/* ---------- DELAYS ---------- */

#define LCD_DELAY_BOOTUP 16000
#define LCD_DELAY_INIT 5000
#define LCD_DELAY_INIT_REP 64
#define LCD_DELAY_INIT_4BIT 64
#define LCD_DELAY_BUSY_FLAG 4
#define LCD_DELAY_ENABLE_PULSE 1


/* ---------- LCD COMMANDS ---------- */

#define LCD_CLR               0
#define LCD_HOME              1

#define LCD_ENTRY_MODE        2
#define LCD_ENTRY_INC         1
#define LCD_ENTRY_SHIFT       0

#define LCD_ON                3
#define LCD_ON_DISPLAY        2
#define LCD_ON_CURSOR         1
#define LCD_ON_BLINK          0

#define LCD_MOVE              4
#define LCD_MOVE_DISP         3
#define LCD_MOVE_RIGHT        2

#define LCD_FUNCTION          5
#define LCD_FUNCTION_8BIT     4
#define LCD_FUNCTION_2LINES   3
#define LCD_FUNCTION_10DOTS   2

#define LCD_CGRAM             6
#define LCD_DDRAM             7

#define LCD_BUSY              7


#define LCD_ENTRY_DEC            0x04
#define LCD_ENTRY_DEC_SHIFT      0x05
#define LCD_ENTRY_INC_           0x06
#define LCD_ENTRY_INC_SHIFT      0x07

#define LCD_DISP_OFF             0x08
#define LCD_DISP_ON              0x0C
#define LCD_DISP_ON_BLINK        0x0D
#define LCD_DISP_ON_CURSOR       0x0E
#define LCD_DISP_ON_CURSOR_BLINK 0x0F

#define LCD_MOVE_CURSOR_LEFT     0x10
#define LCD_MOVE_CURSOR_RIGHT    0x14
#define LCD_MOVE_DISP_LEFT       0x18
#define LCD_MOVE_DISP_RIGHT      0x1C

#define LCD_FUNCTION_4BIT_1LINE  0x20
#define LCD_FUNCTION_4BIT_2LINES 0x28

#define LCD_MODE_DEFAULT ((1<<LCD_ENTRY_MODE) | (1<<LCD_ENTRY_INC))


/* ---------- FUNCTION PROTOTYPES ---------- */

extern void lcd_init(uint8_t dispAttr);
extern void lcd_clrscr(void);
extern void lcd_home(void);
extern void lcd_gotoxy(uint8_t x, uint8_t y);
extern void lcd_putc(char c);
extern void lcd_puts(const char *s);
extern void lcd_puts_p(const char *progmem_s);
extern void lcd_command(uint8_t cmd);
extern void lcd_data(uint8_t data);

#define lcd_puts_P(__s) lcd_puts_p(PSTR(__s))

#endif