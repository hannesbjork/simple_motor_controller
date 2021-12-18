/*
 * base.c
 *
 * Created: 4/10/2021 3:25:23 PM
 *  Author: ehbjork
 */ 

#include <avr/io.h> // This contains the definitions of the terms used
#include <avr/interrupt.h>
#include <stdbool.h>
	    
	
#define LEDC1 PC5
#define LEDB1 PB6
#define LEDB2 PB7

#define ANA1 PC4
#define ANA2 PC3
#define ANA3 PC2

#define INT1 PC1
#define INT2 PC0

#define DIG1 PB2
#define DIG2 PB1

#define PWM1 PD5
#define PWM2 PD6

int32_t led_brightness = 0;

int main()
{
	
	//Init PWM
	DDRD |=0b01100000;
	TCCR0A |= 0b10110011;
	TCCR0B|=0b00000001;
	
	//Init test LEDs on port C
	DDRC |= (0x01 << LEDC1) | (0x01 << LEDC2); //Configure the PORTD2 as output
	PORTC = 0x00;
	
	//Init test LEDs on port B
	DDRB |= (0x01 << LEDB1) | (0x01 << LEDB2); //Configure the PORTD2 as output
	PORTB = 0x00;
	
	//Set timer vars
	TCNT1 = 1;   // for 1 sec at 16 MHz
	//TCCR1B = (1<<CS10) | (1<<CS12);; //Prescaler
	
	//Init timer
	TCCR1B = (1 << CS10);  // Timer mode
	TIMSK1 = (1 << TOIE1) ;   // Enable timer1 overflow interrupt(TOIE1)
	sei();        // Enable global interrupts by setting global interrupt enable bit in SREG
	
	while(1){
	}
}

int run()
{
	//Flicker all test LEDs, on/off on tick
	//PORTC ^= (1 << LEDC1) | (1 << LEDC2);
	//PORTB ^= (1 << LEDB1) | (1 << LEDB2);
			
	//Fade up light with PWM
	//led_brightness = led_brightness + 2;
	//if(led_brightness>70){
	//	led_brightness = 0;
	//}
	//OCR0A = led_brightness;
			
}

ISR (TIMER1_OVF_vect)    // Timer1 ISR
{
	run();
}

//void USART0_sendChar(char c) 
//{
//    while (!(USART0.STATUS & USART_DREIF_bm))
//    {
//        ;
//    }        
//    USART0.TXDATAL = c;
//}
