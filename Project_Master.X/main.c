#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#include "lcd.h"
#include "keypad.h"
#define UNO_ADDR 0x10
#define LED_READY PG1
#define LED_MOVING PG2
#define LED_DOOR PD7
#define LDR_CHANNEL 7
#define LDR_THRESHOLD 400

typedef enum {
    IDLE,
    GOING_UP,
    GOING_DOWN,
    DOOR_OPENING,
    DOOR_CLOSING,
    OBSTACLE,
    FAULT
} 

state_t;
state_t state = IDLE;
uint8_t current_floor = 0;
uint8_t target_floor = 0;
uint8_t input_floor = 0;
uint8_t input_active = 0;
char buffer[32];
char key;
uint8_t cmd = 0;

void TWI_init() {
    TWSR = 0x00;
    TWBR = 72; 
    TWCR = (1<<TWEN);
}

void TWI_start() {
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
}

void TWI_stop() {
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
    _delay_us(10);
}

void TWI_write(uint8_t data) {
    TWDR = data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
}

void send_command(uint8_t data) {
    TWI_start();
    TWI_write(UNO_ADDR << 1);
    TWI_write(data);
    TWI_stop();
}

void UART_init() {
    uint16_t ubrr = 103;
    UBRR0H = (ubrr >> 8);
    UBRR0L = ubrr;
    UCSR0B = (1<<TXEN0);
    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}

void UART_send(char c) {
    while(!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}

void UART_print(char *str) {
    while(*str)
        UART_send(*str++);
}

void ADC_init() {
    ADMUX = (1<<REFS0);
    ADCSRA =
        (1<<ADEN) |
        (1<<ADPS2) |
        (1<<ADPS1);
}

uint16_t ADC_read(uint8_t ch) {
    ADMUX = (ADMUX & 0xF0) | ch;
    ADCSRA |= (1<<ADSC);
    while(ADCSRA & (1<<ADSC));
    return ADC;
}

int main(void) {
    DDRG |= (1<<LED_READY) | (1<<LED_MOVING);
    DDRD |= (1<<LED_DOOR);
    UART_init();
    UART_print("System start\r\n");
    lcd_init(LCD_DISP_ON);
    lcd_clrscr();
    KEYPAD_Init();
    ADC_init();
    TWI_init();
    
    while(1)
    {
        switch(state) {
        case IDLE:
            UART_print("STATE: IDLE\r\n");
            PORTG &= ~(1<<LED_READY);
            PORTG &= ~(1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            PORTG |= (1<<LED_READY);
            lcd_clrscr();
            lcd_puts("Enter floor:");
            input_floor = 0;
            input_active = 1;
            
            while(input_active)
            {
                key = KEYPAD_GetKey();
                if(key >= '0' && key <= '9') {
                    uint8_t digit = key - '0';
                    if(input_floor <= 9) {
                        input_floor = input_floor * 10 + digit;
                        lcd_clrscr();
                        sprintf(buffer,"Floor:%d",input_floor);
                        lcd_puts(buffer);
                    }
                    _delay_ms(200);
                    while(KEYPAD_GetKey() != 'z')
                        _delay_ms(10);
                } if(key == '#') {
                    input_floor = 0;
                    lcd_clrscr();
                    lcd_puts("Enter floor:");
                } if(key == '*') {
                    target_floor = input_floor;
                    input_active = 0;
                }
            } if(target_floor == current_floor){
                state = FAULT;
            } else if(target_floor > current_floor){
                state = GOING_UP;
            } else {
                state = GOING_DOWN;
            }
        break;

        case GOING_UP:
            PORTG &= ~(1<<LED_READY);
            PORTG &= ~(1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            PORTG |= (1<<LED_MOVING);
            current_floor++;
            lcd_clrscr();
            sprintf(buffer,"Floor:%d",current_floor);
            lcd_puts(buffer);
            _delay_ms(1000);
            if(current_floor == target_floor){
                state = DOOR_OPENING;
            }
        break;

        case GOING_DOWN:
            PORTG &= ~(1<<LED_READY);
            PORTG &= ~(1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            PORTG |= (1<<LED_MOVING);
            current_floor--;
            lcd_clrscr();
            sprintf(buffer,"Floor:%d",current_floor);
            lcd_puts(buffer);
            _delay_ms(1000);
            if(current_floor == target_floor){
                state = DOOR_OPENING;
            }
        break;

        case DOOR_OPENING:
            PORTG &= ~(1<<LED_READY);
            PORTG &= ~(1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            PORTD |= (1<<LED_DOOR);
            lcd_clrscr();
            lcd_puts("Door open");
            _delay_ms(3000);
            state = DOOR_CLOSING;
        break;

        case DOOR_CLOSING:
            PORTG &= ~(1<<LED_READY);
            PORTG &= ~(1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            PORTD |= (1<<LED_DOOR);
            lcd_clrscr();
            lcd_puts("Door closing");
            uint16_t close_time = 0;
            while(close_time < 2000) {
                uint16_t ldr_val = ADC_read(LDR_CHANNEL);
                if(ldr_val < LDR_THRESHOLD) {
                    cmd = 1;
                    send_command(cmd);
                    state = OBSTACLE;
                    break;
                }
                _delay_ms(50);
                close_time += 50;
            } if(state != OBSTACLE) {
                cmd = 0;
                send_command(cmd);
                state = IDLE;
            }
        break;
        
        case OBSTACLE:
            lcd_clrscr();
            lcd_puts("Obstacle!");
            cmd = 1;
            send_command(cmd);    
            while(ADC_read(LDR_CHANNEL) < LDR_THRESHOLD) {
                _delay_ms(100);
            }
            cmd = 0;
            send_command(cmd);
            state = DOOR_OPENING;
        break;

        case FAULT:
            PORTG &= ~(1<<LED_READY);
            PORTG &= ~(1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            lcd_clrscr();
            lcd_puts("Same floor");
            _delay_ms(2000);
            state = IDLE;
        break;
        }
    }
}