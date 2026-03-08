#include "keypad.h"
#include "delay.h"

static uint8_t keypad_ScanKey();

/* ---------- INIT ---------- */

void KEYPAD_Init()
{
    DDRK = 0xF0;      // PK7?PK4 rows output
    PORTK = 0x0F;     // PK3?PK0 columns pullup
}

/* ---------- WAIT RELEASE ---------- */

void KEYPAD_WaitForKeyRelease()
{
    uint8_t key;

    do
    {
        do
        {
            M_ROW = 0xF0;
            key = M_COL & 0x0F;
        }while(key != 0x0F);

        DELAY_ms(1);

        M_ROW = 0xF0;
        key = M_COL & 0x0F;

    }while(key != 0x0F);
}

/* ---------- WAIT PRESS ---------- */

void KEYPAD_WaitForKeyPress()
{
    uint8_t key;

    do
    {
        do
        {
            M_ROW = 0xF0;
            key = M_COL & 0x0F;
        }while(key == 0x0F);

        DELAY_ms(1);

        M_ROW = 0xF0;
        key = M_COL & 0x0F;

    }while(key == 0x0F);
}

/* ---------- GET KEY ---------- */

uint8_t KEYPAD_GetKey()
{
    uint8_t key;

    KEYPAD_WaitForKeyRelease();
    DELAY_ms(1);

    KEYPAD_WaitForKeyPress();

    key = keypad_ScanKey();

    switch(key)
    {
        case 0xE7: key='*'; break;
        case 0xEB: key='7'; break;
        case 0xED: key='4'; break;
        case 0xEE: key='1'; break;

        case 0xD7: key='0'; break;
        case 0xDB: key='8'; break;
        case 0xDD: key='5'; break;
        case 0xDE: key='2'; break;

        case 0xB7: key='#'; break;
        case 0xBB: key='9'; break;
        case 0xBD: key='6'; break;
        case 0xBE: key='3'; break;

        case 0x77: key='D'; break;
        case 0x7B: key='C'; break;
        case 0x7D: key='B'; break;
        case 0x7E: key='A'; break;

        default: key='z'; break;
    }

    return key;
}

/* ---------- SCAN KEY ---------- */

static uint8_t keypad_ScanKey()
{
    uint8_t scan = 0xEF;
    uint8_t i, key;

    for(i=0;i<4;i++)
    {
        M_ROW = scan;
        DELAY_ms(1);

        key = M_COL & 0x0F;

        if(key != 0x0F)
            break;

        scan = ((scan<<1) + 0x01);
    }

    key = key + (scan & 0xF0);

    return key;
}