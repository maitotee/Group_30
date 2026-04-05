/*
 * Receives TWI command from master and controls all LEDs:
 * 0 = CMD_IDLE -> ready LED on
 * 1 = CMD_OBSTACLE -> obstacle LED blink + buzzer alarm
 * 2 = CMD_MOVING -> moving LED on
 * 3 = CMD_DOOR -> door LED on
 *
 * D12 (PB4) = obstacle LED
 * D11 (PB3) = ready LED
 * D10 (PB2) = moving LED
 * D9  (PB1) = door LED
 * D13 (PB5) = buzzer
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define SLAVE_ADDR 0x10 // TWI slave address

#define BUZZER       PB5 // buzzer pin (D13)
#define LED_OBSTACLE PB4 // obstacle LED (D12)
#define LED_READY    PB3 // ready LED (D11)
#define LED_MOVING   PB2 // moving LED (D10)
#define LED_DOOR     PB1 // door LED (D9)

// TWI command values
#define CMD_IDLE     0
#define CMD_OBSTACLE 1
#define CMD_MOVING   2
#define CMD_DOOR     3

// note frequencies (Hz)
#define NOTE_C5 523
#define NOTE_E5 659
#define NOTE_G5 784

volatile uint8_t current_cmd = CMD_IDLE; // current command from master

uint8_t twi_status; // TWI status register value
uint8_t data;       // received data byte

// Generate a tone on the buzzer
void play_tone(uint16_t freq, uint16_t duration){
    uint32_t period = 1000000UL / freq; // cycle length in us
    uint32_t cycles = (duration * 1000UL) / period; // number of cycles

    for(uint32_t i=0;i<cycles;i++){
        PORTB |= (1<<BUZZER); // high half-cycle
        for(uint32_t j=0;j<period/2;j++) _delay_us(1);

        PORTB &= ~(1<<BUZZER); // low half-cycle
        for(uint32_t j=0;j<period/2;j++) _delay_us(1);
    }
}

// Play a short alarm melody (C5-E5-G5-E5-C5)
void play_alarm(){
    play_tone(NOTE_C5,150);
    play_tone(NOTE_E5,150);
    play_tone(NOTE_G5,150);
    play_tone(NOTE_E5,150);
    play_tone(NOTE_C5,200);
}

// Turn off all LEDs and buzzer
void leds_off(){
    PORTB &= ~((1<<LED_OBSTACLE)|(1<<LED_READY)|(1<<LED_MOVING)|(1<<LED_DOOR)|(1<<BUZZER));
}

// Initialize TWI in slave mode
void TWI_slave_init(){
    TWAR = (SLAVE_ADDR << 1); // set slave address
    TWCR = (1<<TWEA) | (1<<TWEN); // enable TWI and ACK
}

int main(void){
    // Set all LED and buzzer pins as outputs
    DDRB |= (1<<BUZZER)|(1<<LED_OBSTACLE)|(1<<LED_READY)|(1<<LED_MOVING)|(1<<LED_DOOR);

    TWI_slave_init(); // start TWI slave

    while(1){
        // Check if TWI interrupt flag is set (data or address received)
        if (TWCR & (1<<TWINT)){
            twi_status = (TWSR & 0xF8);

            if (twi_status == 0x60){
                // 0x60: SLA+W received, ACK returned (slave selected for write)
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }

            else if (twi_status == 0x80){
                // 0x80: data byte received, ACK returned
                data = TWDR;
                if(data <= CMD_DOOR) current_cmd = data;
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }

            else if (twi_status == 0xA0){
                // 0xA0: STOP or repeated START received
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }

            else{
                // other TWI states: reset and re-enable
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }
        }

        // Control LEDs and buzzer based on current command
        switch(current_cmd){
            case CMD_IDLE:
                leds_off();
                PORTB |= (1<<LED_READY); // ready LED on
                break;

            case CMD_OBSTACLE:
                leds_off();
                PORTB ^= (1<<LED_OBSTACLE); // blink obstacle LED
                play_alarm(); // sound alarm
                _delay_ms(300);
                break;

            case CMD_MOVING:
                leds_off();
                PORTB |= (1<<LED_MOVING); // moving LED on
                break;

            case CMD_DOOR:
                leds_off();
                PORTB |= (1<<LED_DOOR); // door LED on
                break;
        }
    }
}
