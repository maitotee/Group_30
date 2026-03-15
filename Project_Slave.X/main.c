#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define SLAVE_ADDR 0x10

#define BUZZER PB5   // D13
#define LED PB4      // D12

#define NOTE_C5 523
#define NOTE_E5 659
#define NOTE_G5 784

volatile uint8_t obstacle_active = 0;


void play_tone(uint16_t freq, uint16_t duration) {
    uint32_t period = 1000000UL / freq;
    uint32_t cycles = (duration * 1000UL) / period;
    for(uint32_t i=0;i<cycles;i++)
    {
        PORTB |= (1<<BUZZER);
        for(uint32_t j=0;j<period/2;j++)
            _delay_us(1);
        PORTB &= ~(1<<BUZZER);
        for(uint32_t j=0;j<period/2;j++)
            _delay_us(1);
    }
}

void play_alarm(){
    play_tone(NOTE_C5,150);
    play_tone(NOTE_E5,150);
    play_tone(NOTE_G5,150);
    play_tone(NOTE_E5,150);
    play_tone(NOTE_C5,200);
}

void TWI_slave_init() {
    TWAR = (SLAVE_ADDR << 1);
    TWCR =
        (1<<TWEN) |
        (1<<TWEA) |
        (1<<TWIE);
}

ISR(TWI_vect) {
    switch(TWSR & 0xF8)
    {
        case 0x60:
            TWCR =
                (1<<TWINT) |
                (1<<TWEN) |
                (1<<TWEA) |
                (1<<TWIE);
        break;
        
        case 0x80:
            if(TWDR == 1){
                obstacle_active = 1;
            } if(TWDR == 0){
                obstacle_active = 0;
            }
            TWCR =
                (1<<TWINT) |
                (1<<TWEN) |
                (1<<TWEA) |
                (1<<TWIE);
        break;
        
        default:
            TWCR =
                (1<<TWINT) |
                (1<<TWEN) |
                (1<<TWEA) |
                (1<<TWIE);
        break;
    }
}

int main(void)
{
    DDRB |= (1<<BUZZER) | (1<<LED);
    TWI_slave_init();
    sei();
    while(1){
        if(obstacle_active){
            PORTB ^= (1<<LED);
            play_alarm();
            _delay_ms(300);
        } else {
            PORTB &= ~(1<<LED);
            PORTB &= ~(1<<BUZZER);
        }
    }
}