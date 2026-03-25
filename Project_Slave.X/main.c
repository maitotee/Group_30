#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define SLAVE_ADDR 0x10 // slave address (UNO)

#define BUZZER PB5 // buzzer pin
#define LED PB4    // LED pin

// note frequencies
#define NOTE_C5 523
#define NOTE_E5 659
#define NOTE_G5 784

volatile uint8_t obstacle_active = 0;//1 = obstacle, 0 = no obstacle

uint8_t twi_status;//TWI status
uint8_t data;//received data

//generate tone on buzzer
void play_tone(uint16_t freq, uint16_t duration){
    uint32_t period = 1000000UL / freq;
    uint32_t cycles = (duration * 1000UL) / period;
    
    for(uint32_t i=0;i<cycles;i++){
        PORTB |= (1<<BUZZER);//ON
        for(uint32_t j=0;j<period/2;j++) _delay_us(1);
        
        PORTB &= ~(1<<BUZZER);//OFF
        for(uint32_t j=0;j<period/2;j++) _delay_us(1);
    }
}

// play alarm sound
void play_alarm(){
    play_tone(NOTE_C5,150);
    play_tone(NOTE_E5,150);
    play_tone(NOTE_G5,150);
    play_tone(NOTE_E5,150);
    play_tone(NOTE_C5,200);
}

// init TWI slave
void TWI_slave_init(){
    TWAR = (SLAVE_ADDR << 1);//set address
    TWCR = (1<<TWEA) | (1<<TWEN);//enable TWI + ACK
}

int main(void){
    DDRB |= (1<<BUZZER) | (1<<LED);//outputs

    TWI_slave_init();//start TWI

    while(1){
        //check if data or address received
        if (TWCR & (1<<TWINT)){
            twi_status = (TWSR & 0xF8);

            if (twi_status == 0x60){
                //address received
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }

            else if (twi_status == 0x80){
                //data received
                data = TWDR;

                if(data == 1) obstacle_active = 1;
                else if(data == 0) obstacle_active = 0;

                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }

            else if (twi_status == 0xA0){
                //stop condition
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }

            else{
                //other cases
                TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
            }
        }

        //react to obstacle
        if(obstacle_active){
            PORTB ^= (1<<LED);//blink LED
            play_alarm();//sound
            _delay_ms(300);
        }else{
            PORTB &= ~(1<<LED);
            PORTB &= ~(1<<BUZZER);
        }
    }
}