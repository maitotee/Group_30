#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"
#include "keypad.h"
#define UNO_ADDR 0x10//TWI address of Arduino UNO (slave)
#define LED_READY PG1//ready LED
#define LED_MOVING PG2//moving LED
#define LED_DOOR PD7//door LED
#define LDR_CHANNEL 7//ADC channel for LDR
#define LDR_THRESHOLD 400//threshold for obstacle detection

//elevator states
typedef enum{
    IDLE,
    GOING_UP,
    GOING_DOWN,
    DOOR_OPENING,
    DOOR_CLOSING,
    OBSTACLE,
    FAULT
} state_t;
state_t state = IDLE;
uint8_t current_floor = 0;
uint8_t target_floor = 0;
uint8_t input_floor = 0;
uint8_t input_active = 0;
char buffer[32];
char key;
uint8_t cmd = 0;//command sent to slave (0 or 1)
uint8_t twi_status;
char test_char_array[16];
//initialize TWI (master mode)

void TWI_init(){
    TWBR = 0x03;//set SCL frequency (~400 kHz)
    TWSR = 0x00;//prescaler = 1
    TWCR |= (1 << TWEN);//enable TWI
}

//send one byte to slave using TWI
void TWI_send_byte(uint8_t data){
    //START condition
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
    
    //send slave address + write
    TWDR = (UNO_ADDR << 1);
    TWCR = (1<<TWINT)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
    
    //send data
    TWDR = data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    //STOP condition
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

//initialize ADC
void ADC_init(){
    ADMUX = (1<<REFS0);//AVcc reference
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);//enable ADC + prescaler
}

//read ADC value from selected channel
uint16_t ADC_read(uint8_t ch){
    ADMUX = (ADMUX & 0xF0) | ch;
    ADCSRA |= (1<<ADSC);//start conversion
    while(ADCSRA & (1<<ADSC));//wait
    return ADC;
}

int main(void){
    //set LED pins as outputs
    DDRG |= (1<<LED_READY) | (1<<LED_MOVING);
    DDRD |= (1<<LED_DOOR);
    lcd_init(LCD_DISP_ON);
    lcd_clrscr();
    KEYPAD_Init();
    ADC_init();
    TWI_init();

    while(1)
    {
        switch(state) {
        case IDLE:
            //reset LEDs
            PORTG &= ~(1<<LED_READY);
            PORTG &= ~(1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);

            PORTG |= (1<<LED_READY);//show ready state
            lcd_clrscr();
            lcd_puts("Enter floor:");
            input_floor = 0;
            input_active = 1;
            //read keypad input
            while(input_active){
                key = KEYPAD_GetKey();
                if(key >= '0' && key <= '9') {
                    uint8_t digit = key - '0';
                    if(input_floor <= 9){
                        input_floor = input_floor * 10 + digit;
                        lcd_clrscr();
                        sprintf(buffer,"Floor:%d",input_floor);
                        lcd_puts(buffer);
                    }
                    _delay_ms(200);
                    while(KEYPAD_GetKey() != 'z') _delay_ms(10);
                }
                if(key == '#'){
                    input_floor = 0;
                    lcd_clrscr();
                    lcd_puts("Enter floor:");
                }
                if(key == '*'){
                    target_floor = input_floor;
                    input_active = 0;
                }
            }
            //decide direction
            if(target_floor == current_floor) state = FAULT;
            else if(target_floor > current_floor) state = GOING_UP;
            else state = GOING_DOWN;
        break;

        case GOING_UP:
            PORTG = (PORTG & ~(1<<LED_READY)) | (1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            current_floor++;
            lcd_clrscr();
            sprintf(buffer,"Floor:%d",current_floor);
            lcd_puts(buffer);
            _delay_ms(1000);
            if(current_floor == target_floor) state = DOOR_OPENING;
        break;

        case GOING_DOWN:
            PORTG = (PORTG & ~(1<<LED_READY)) | (1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            current_floor--;
            lcd_clrscr();
            sprintf(buffer,"Floor:%d",current_floor);
            lcd_puts(buffer);
            _delay_ms(1000);
            if(current_floor == target_floor) state = DOOR_OPENING;
        break;

        case DOOR_OPENING:
            PORTD |= (1<<LED_DOOR);
            lcd_clrscr();
            lcd_puts("Door open");
            _delay_ms(3000);
            state = DOOR_CLOSING;
        break;

        case DOOR_CLOSING:{
            PORTD |= (1<<LED_DOOR);
            lcd_clrscr();
            lcd_puts("Door closing");
            uint16_t close_time = 0;

            //monitor LDR during closing
            while(close_time < 2000) {
                uint16_t ldr_val = ADC_read(LDR_CHANNEL);
                
                //sobstacle detected
                if(ldr_val < LDR_THRESHOLD){
                    cmd = 1;
                    TWI_send_byte(cmd);
                    state = OBSTACLE;
                    break;
                }
                _delay_ms(50);
                close_time += 50;
            }
            //no obstacle -> normal close
            if(state != OBSTACLE){
                cmd = 0;
                TWI_send_byte(cmd);
                state = IDLE;
            }
        }
        break;

        case OBSTACLE:
            lcd_clrscr();
            lcd_puts("Obstacle!");
            cmd = 1;
            TWI_send_byte(cmd);
            //wait until obstacle removed
            while(ADC_read(LDR_CHANNEL) < LDR_THRESHOLD) {
                _delay_ms(100);
            }
            cmd = 0;
            TWI_send_byte(cmd);
            state = DOOR_OPENING;
        break;

        case FAULT:
            lcd_clrscr();
            lcd_puts("Same floor");
            _delay_ms(2000);
            state = IDLE;
        break;
        }
    }
}