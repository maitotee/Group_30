/*
 *recieved commands from master meaningns
 *1 = obstacle detected = blink LED + play alarm
 *0 = obstacle cleared = turn off LED and buzzer
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define SLAVE_ADDR 0x10 // TWI slave address (UNO)

#define BUZZER PB5 // buzzer pin
#define LED PB4    // LED pin

// note frequencies (Hz)
#define NOTE_C5 523
#define NOTE_E5 659
#define NOTE_G5 784

volatile uint8_t obstacle_active = 0;// 1 = obstacle detected, 0 = clear

uint8_t twi_status; // TWI status register value
uint8_t data;       // received data byte

// Generate a tone on the buzzer
void play_tone(uint16_t freq, uint16_t duration){
    uint32_t period = 1000000UL / freq;// cycle length in us
    uint32_t cycles = (duration * 1000UL) / period;// number of cycles

    for(uint32_t i=0;i<cycles;i++){
        PORTB |= (1<<BUZZER);// high half-cycle
        for(uint32_t j=0;j<period/2;j++) _delay_us(1);

        PORTB &= ~(1<<BUZZER);// low half-cycle
        for(uint32_t j=0;j<period/2;j++) _delay_us(1);
    }
}

// Play a short alarm melody
void play_alarm(){
    play_tone(NOTE_C5,150);
    play_tone(NOTE_E5,150);
    play_tone(NOTE_G5,150);
    play_tone(NOTE_E5,150);
    play_tone(NOTE_C5,200);
}

// Initialize TWI in slave mode
void TWI_slave_init(){
    TWAR = (SLAVE_ADDR << 1);// set slave address
    TWCR = (1<<TWEA) | (1<<TWEN);// enable TWI and ACK
}

int main(void){
    DDRB |= (1<<BUZZER) | (1<<LED);// set buzzer and LED as outputs

    TWI_slave_init();// start TWI slave

    while(1){
        // Check if TWI interrupt flag is set
        if (TWCR & (1<<TWINT)){
            twi_status = (TWSR & 0xF8);

            if (twi_status == 0x60){
                // 0x60, SLA+W received, ACK returned
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }

            else if (twi_status == 0x80){
                // 0x80, data byte received, ACK returned
                data = TWDR;

                if(data == 1) obstacle_active = 1;
                else if(data == 0) obstacle_active = 0;

                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }

            else if (twi_status == 0xA0){
                // 0xA0, STOP or repeated START received
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }

            else{
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }
        }

        // React to obstacle status
        if(obstacle_active){
            // NOTE, play_alarm() blocks about 800ms during which TWI messages are not handled
            PORTB ^= (1<<LED);// blink LED
            play_alarm();// sound alarm
            _delay_ms(300);
        }else{
            PORTB &= ~(1<<LED);// turn off LED
            PORTB &= ~(1<<BUZZER);// turn off buzzer
        }
    }
}
