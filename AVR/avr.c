/*
 * base.c
 *
 * Created: 4/10/2021 3:25:23 PM
 *  Author: ehbjork
 */ 

// Imports
#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>


// Define constants

#define BAUD 4800
#define F_CPU 1000000
#define MYUBRR F_CPU/16/BAUD-1

#define int_scale 6
#define filter_len 32

int Kp = 16;
int Ki = 4;
	    
// Define ports

#define LEDC1 PC5
#define LEDB1 PB6
#define LEDB2 PB7

#define ANA1 PC4
#define ANA2 PC3
#define ANA3 PC2

#define DIGINT1 PC1
#define DIGINT2 PC0

#define DIG1 PB2
#define DIG2 PB1

#define PWM1 PD5
#define PWM2 PD6

// Define variables

unsigned char command;

unsigned char speed = 0;
unsigned char speed_setpoint = 0;

int cycles[filter_len] =  {0};
int cycles_pointer = 0;
int32_t cycles_total = 0;

int i = 0;
int p = 0;

bool flag_recieved_interrupt = false;
bool flag_update_controller = false;

char snum[5];

int tune;


void initUSART(unsigned int);
void USART_Transmit(unsigned char data);
unsigned char USART_Receive( void );
void USART_sendString(char *str);



// Interrupt handlers

ISR(PCINT1_vect){

	int temp = TCNT1;
	if(temp > 250){	
		TCNT1 = 0;
		cycles[cycles_pointer] = temp;
		if(cycles_pointer < filter_len-1){
			++cycles_pointer;
		}else{
		cycles_pointer = 0;
		}
		flag_recieved_interrupt = true;
	}	
}

ISR(TIMER2_COMPA_vect){	//Controller should be synchronous. Does that mean that the PI should update the output in the same rate as the encoder gives inputs?
	flag_update_controller = true;
	TCNT2 = 0;
}



// Inits

void initLED(void){

    DDRC |= 0b00100000; //(1 << LEDC1);
	DDRB |= 0b11000000; //(1 << LEDB1) | (1 << LEDB2);
}

void initANA(void){
    DDRC &= 0b11111011;

    ADMUX = 0b01100010;
    ADCSRA = 0b10000111;

}

void initPWM(void){

    DDRD |= 0b01100000; //((1 << PWM1) | (1 << PWM2));		//Digital out at PCINT21,22 PORTD bit 5,6.
	TCCR0A |= 0b00100011;		//Bit7-6=11 => inverting mode OC0A. Bit5-4=11 => inverting mode OC0B. (Need to have inverting mode because otherwise the avr cannot send out 0 (but amplifier inverts too so no). Now it cant send out 255 but thats okay)Bit 3,2 not used. Bit 1-0 = 11 => Fast pwm, 255 bits precision, fs= 1/(255us). 
	TCCR0B |= 0b00000001;		//Bit 7-6=00, when in PWM mode. Bit 5-4 reserved. Bit 3=0 => Fast pwm, 255 bits precision, fs= 1/(255us). Bit 2-0=001 => No prescaler
}

void initINT(void){

    DDRC &= 0b11111100;

	PCICR |= 0b00000010; // Select PCINT 8-14.
	PCMSK1 = 0b00000011; // Select PCINT 9 and 8.
    
    SREG |= 0b10000000;	
}

void initTIMER(void){

    TCCR1A = 0b00000000;  //Normal mode
	TCCR1B |= 0b00000010;  //Normal mode, Bit 2-0=010 => prescaler 1/8
	TCNT1 = 0;
	
 	TCCR2B |= 0b00000111;  //Normal mode, Bit 2-0=111 => prescaler 1/1024
	
	TCCR2A = (1 << WGM01);
	OCR2A =	98;  // 98 => h=0.1
	TIMSK2 = (1 << OCIE2A);
}

void init(void){
	initLED();
    initANA();
	initPWM();
    initUSART(MYUBRR);
    initINT();
    initTIMER();
}



void toggleLED(int led){
	if(led == 0){
		PORTC = PORTC ^ (1 << LEDC1);
	}
	if(led == 1){
        PORTB = PORTB ^ (1 << LEDB1);
	}
	if(led == 2){
        PORTB = PORTB ^ (1 << LEDB2);	
	}
}



int invertSignal(int value){
	if(value > 255){
		value = 255;
	} else if(value < 0){
		value = 0;
	}
	value = 255 - value;
	return value;
}

void setPWM(int value){		
	value = invertSignal(value);		//Since inverting mode is activated. (but amplifier inverts as well)
	
	OCR0B = value;
}



int readAIn(void){
    // Init analogue in
    ADCSRA |= (1<< ADSC);

    while(ADCSRA & (1 << ADSC)); 
	
    return ADCH;
}



int32_t scaledDiv(int32_t n, int32_t d){		//Scaled down(normal) numbers as arguments.
	return (n << int_scale) / d;
}
  
int32_t scaledMul(int32_t f1, int32_t f2){	//Scaled up numbers as arguments.
	   return ((f1*f2) >> int_scale); // << 6;//;
}

int32_t scaleDown(int32_t x){
	return x >> int_scale;
}

int32_t scaleUp(int32_t x){
	return x << int_scale;
}




void updateSpeed(){
	if(flag_recieved_interrupt){ 
		for(int i=0; i<filter_len; ++i){
			cycles_total += cycles[i];
		}
		
  
        speed = (unsigned char)scaleDown(scaledDiv(78125, scaleDown(scaledDiv(cycles_total, filter_len)))) *2;

        cycles_total = 0;
		
        flag_recieved_interrupt = false;
	}															

}

unsigned char getCommand(){
	return USART_Receive();
}

unsigned char executeCommand(unsigned char com){
	switch(com){

		case 'a':  //Get speed
            sprintf(snum, "%d", speed);
			USART_sendString(snum);
			break;
		
		case 'b':  //Set speed
			speed_setpoint = (uint8_t)USART_Receive();
            int local_setpoint = speed_setpoint + tune;
            if(local_setpoint < 0){
                local_setpoint = 0;
            }
            sprintf(snum, "%d", local_setpoint);
			USART_sendString(snum);
			break;
						
		case 'c':	//Test
            sprintf(snum, "%d", tune);
			USART_sendString(snum);
            //USART_Transmit((readAIn());
			break;

        default:
            break;
			
	}
	return 0;
}



void speedPI(unsigned char setpoint){
    toggleLED(1);

	if(flag_update_controller){
		if(setpoint <= 0){
			setPWM(0);
            speed = 0;

		}else{

			int error = setpoint - speed;

			if(error < 0){
				p = 0;
			}else{
				//p =  (( Kp * (error << int_scale)) >> int_scale);
                p = scaledMul(Kp, scaleUp(error));
            }

			if(error < 0){

                i -= scaledMul(Ki, scaleUp(-error));
            }else{

                i += scaledMul(Ki, scaleUp(error));
            }
			
            int scaled_max = (255 << int_scale);

            if(i > scaleUp(255) - p){
				i = scaleUp(255)-p;

			} else if(i<-scaleUp(255)-p){
				i = -scaleUp(255)-p;
			}
			
			setPWM((unsigned char)scaleDown(p + i));
		}
		flag_update_controller = false;
	}
}

int fineTuning(){
    return scaleDown(scaledDiv(readAIn()*21,256))-10;
}



int main(void){
    init();

    toggleLED(2);

    setPWM(0);

    while(1){

        toggleLED(0);
		command = getCommand();
        toggleLED(0);

        updateSpeed();

        command = executeCommand(command);

        tune = fineTuning();
		speedPI(speed_setpoint + tune);
	}

    toggleLED(2);
	
	return 0;

}




void initUSART( unsigned int ubrr)
{
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;

	UCSR0B = (1<<RXEN0)|(1<<TXEN0);

	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void USART_Transmit( unsigned char data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
		;
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

unsigned char USART_Receive( void )
{
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) );

	/* Get and return received data from buffer */
	return UDR0;
}

void USART_sendString(char *str)
{
    int len = (int)strlen(str);

    if(len < 3){

        for(int i = 0; i < (3 - len); i++)
        {
            USART_Transmit('0');
        }       
    }

    for(int i = 0; i < len; i++)
    {
        USART_Transmit(str[i]);
    }
}