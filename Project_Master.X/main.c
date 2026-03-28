#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"
#include "keypad.h"

//configs
#define UNO_ADDR 0x10 // TWI slave address (UNO)
#define LED_READY PG1
#define LED_MOVING PG2
#define LED_DOOR PD7
#define LDR_CHANNEL 7
#define LDR_THRESHOLD 400 // ADC threshold for obstacle detection

// Timing constants
#define MOVE_DELAY 1000 // ms per floor
#define DOOR_OPEN_TIME 3000 // ms
#define DOOR_CLOSE_TIME 2000 // ms
#define ADC_POLL_DELAY 50 // ms

//states
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
uint8_t cmd = 0; // 1 = obstacle, 0 = no obstacle

// Initialize TWI in master mode
void TWI_init(){
    TWBR = 0x03; // Bit rate for fast TWI
    TWSR = 0x00; // Prescaler = 1
    TWCR |= (1 << TWEN);
}

// Send single byte to slave
void TWI_send_byte(uint8_t data){

    // START
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Check START transmitted
    if ((TWSR & 0xF8) != 0x08) return;

    // Send address + write
    TWDR = (UNO_ADDR << 1);
    TWCR = (1<<TWINT)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Check ACK
    if ((TWSR & 0xF8) != 0x18) return;

    // Send data
    TWDR = data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Check data ACK
    if ((TWSR & 0xF8) != 0x28) return;

    // STOP
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

// Initialize ADC
void ADC_init(){
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);
}

// Read ADC channel
uint16_t ADC_read(uint8_t ch){
    ADMUX = (ADMUX & 0xF0) | ch;
    ADCSRA |= (1<<ADSC);
    while(ADCSRA & (1<<ADSC));
    return ADC;
}

int main(void){
    // Configure LEDs as outputs
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
            // Reset LEDs
            PORTG &= ~(1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            PORTG |= (1<<LED_READY);

            lcd_clrscr();
            lcd_puts("Enter floor:");
            input_floor = 0;
            input_active = 1;
            // Read keypad input (blocking)
            while(input_active){
                key = KEYPAD_GetKey();
                
                // Build number from digits
                if(key >= '0' && key <= '9') {
                    uint8_t digit = key - '0';
                    
                    // Limit to 2 digits (0-99)
                    if(input_floor <= 9){
                        input_floor = input_floor * 10 + digit;
                        lcd_clrscr();
                        sprintf(buffer,"Floor:%d",input_floor);
                        lcd_puts(buffer);
                    }
                    _delay_ms(200);
                    // Wait for key release
                    while(KEYPAD_GetKey() != 'z') _delay_ms(10);
                }
                // Reset input
                if(key == '#'){
                    input_floor = 0;
                    lcd_clrscr();
                    lcd_puts("Enter floor:");
                }
                // Confirm input
                if(key == '*'){
                    target_floor = input_floor;
                    input_active = 0;
                }
            }
            // Decide next state
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
            _delay_ms(MOVE_DELAY);
            
            if(current_floor == target_floor){
                state = DOOR_OPENING;
            }
        break;

        case GOING_DOWN:
            PORTG = (PORTG & ~(1<<LED_READY)) | (1<<LED_MOVING);
            PORTD &= ~(1<<LED_DOOR);
            current_floor--;
            lcd_clrscr();
            sprintf(buffer,"Floor:%d",current_floor);
            lcd_puts(buffer);
            _delay_ms(MOVE_DELAY);
            
            if(current_floor == target_floor){
                state = DOOR_OPENING;
            }

        break;

        case DOOR_OPENING:
            PORTD |= (1<<LED_DOOR);
            lcd_clrscr();
            lcd_puts("Door open");
            _delay_ms(DOOR_OPEN_TIME);
            state = DOOR_CLOSING;
        break;

        case DOOR_CLOSING:{
            PORTD |= (1<<LED_DOOR);
            lcd_clrscr();
            lcd_puts("Door closing");
            uint16_t close_time = 0;

            // Monitor obstacle continuously during closing
            while(close_time < DOOR_CLOSE_TIME) {
                uint16_t ldr_val = ADC_read(LDR_CHANNEL);
                // Obstacle detected (low light ? LDR drop)
                if(ldr_val < LDR_THRESHOLD){
                    cmd = 1;
                    TWI_send_byte(cmd);
                    state = OBSTACLE;
                    break;
                }
                _delay_ms(ADC_POLL_DELAY);
                close_time += ADC_POLL_DELAY;
            }
            // No obstacle -> return to IDLE
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
            // Activate buzzer via slave
            cmd = 1;
            TWI_send_byte(cmd);
            // Wait until obstacle removed
            while(ADC_read(LDR_CHANNEL) < LDR_THRESHOLD) {
                _delay_ms(100);
            }
            // Stop buzzer
            cmd = 0;
            TWI_send_byte(cmd);
            // Reopen doors for safety
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