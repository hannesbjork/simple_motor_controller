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



#define BAUD 4800

#define F_CPU 1000000

#define MYUBRR F_CPU/16/BAUD-1


	    
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


#define int_scale 6

#define mov_avg_size 32


unsigned char command = 0;
unsigned char speed = 0;
unsigned char pos = 0;
unsigned char speedSetpoint = 0;
unsigned char posSetpoint = 0;
int32_t speedOutput = 0;
unsigned char test = 0;
int cycles[mov_avg_size] =  {0};
int cyclePos = 0;
int32_t total = 0;
int i = 0;

bool newPulse = false;
bool updatePI = false;

char snum[5];
char* cmdstr;
volatile char count;


void initUSART(unsigned int);
void USART_Transmit(unsigned char data);
unsigned char USART_Receive( void );
void USART_sendString(char *str);



// Interrupt handlers

ISR(PCINT1_vect){

	int temp = TCNT1;
	if(temp > 250){		//>520 ger att allt över 150rpm ignoreras.=> för lågt värde. Men datan kanske är normalfördelad så måste ta med båda sidor?
		TCNT1 = 0;
		cycles[cyclePos] = temp;
		if(cyclePos < mov_avg_size-1){
			++cyclePos;
		}else{
		cyclePos = 0;
		}
		newPulse = true;
	}	
}

ISR(TIMER2_COMPA_vect){	//Controller should be synchronous. Does that mean that the PI should update the output in the same rate as the encoder gives inputs?
	updatePI = true;
	TCNT2 = 0;
}

// Inits

void initLED(void){

    DDRC |= 0b00100000; //(1 << LEDC1);
	DDRB |= 0b11000000; //(1 << LEDB1) | (1 << LEDB2);
}

void initANA(void){

    ADMUX = 0b01100010;
    ADCSRA = 0b10000111;
}

void initPWM(void){

    DDRD |= 0b01100000; //((1 << PWM1) | (1 << PWM2));		//Digital out at PCINT21,22 PORTD bit 5,6.
	TCCR0A |= 0b00100011;		//Bit7-6=11 => inverting mode OC0A. Bit5-4=11 => inverting mode OC0B. (Need to have inverting mode because otherwise the avr cannot send out 0 (but amplifier inverts too so no). Now it cant send out 255 but thats okay)Bit 3,2 not used. Bit 1-0 = 11 => Fast pwm, 255 bits precision, fs= 1/(255us). 
	TCCR0B |= 0b00000001;		//Bit 7-6=00, when in PWM mode. Bit 5-4 reserved. Bit 3=0 => Fast pwm, 255 bits precision, fs= 1/(255us). Bit 2-0=001 => No prescaler
}

void initINT(void){

    DDRC &= 0b11111101;

	PCICR |= 0b00000010; // Select PCINT 8-14.
	PCMSK1 = 0b00000010; // Select PCINT 9.
    
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
    //initANA();
	initPWM();
    initUSART(MYUBRR);
    initINT();
    initTIMER();
}



void switchLED(int led){
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
	ADMUX = 0b01100010;
    ADCSRA |= (1<< ADSC);

    while(ADCSRA & (1 << ADSC)); //funkar för jonte
    // while((ADCSRA >> ADSC) && 1);
	
    return ADCH;
}

int32_t fixedDiv(int32_t n, int32_t d){		//Scaled down(normal) numbers as arguments.
	  return ((int32_t)n * (1 << 6)) / d;
}
  
int fixedMulti(int32_t f1, int32_t f2){	//Scaled up numbers as arguments.
	   return (f1*f2) / (1 << 6);// << 6;//;
}

unsigned char scaleDown(int x){
	return x >> 6;
}

int scaleDownI(int32_t x){
	return x >> 6;
}

int32_t scaleUp(int x){
	return x << 6;
}


void updateSpeed(){
	if(newPulse){ 
		for(int i=0; i<mov_avg_size; ++i){
			total +=cycles[i];
		}
		speed = scaleDown(fixedDiv(78125, scaleDownI(fixedDiv(total, mov_avg_size))))*2;
		total = 0;
		//speed = scaleDown((fixedMulti(61, scaleUp(speed)) + fixedMulti(3, fixedDiv(78125, cycles)))); //0.95*2^6=61, 0.05*2^6=3 (sum =2^6)
													//1MHz, 96 pulses per rev, 8 prescaler => 1000000*60/(96*8) =78125
		newPulse = false;
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
			speedSetpoint = (uint8_t)USART_Receive();
            sprintf(snum, "%d", speedSetpoint);
			USART_sendString(snum);
			break;
						
		case 'c':	//Test
			USART_sendString("test\n\r");
			break;

        default:
            break;
			
	}
	return 0;
}

void speedPI2(unsigned char setpoint){
    switchLED(1);

	if(updatePI){
		if(setpoint == 0){
			setPWM(0);
		}else{
			int Kp = 16;
			int p = 0;
			int error = setpoint - speed;
			if(error < 0){
				p = 0;
				}else{
				p = fixedMulti(Kp, scaleUp(error));
			}
			int Ki = 4;
			if(error < 0){
				i -= fixedMulti(Ki, scaleUp(-error));
				}else{
				i += fixedMulti(Ki, scaleUp(error));
			}
			if(i > scaleUp(255)- p){
				i = scaleUp(255)-p;
				} else if(i<-scaleUp(255)-p){
				i = -scaleUp(255)-p;
			}
			
			setPWM(scaleDown(p + i));
		}
		updatePI = false;
	}
}

int fineTuning(){
	int tune = scaleDownI(fixedDiv(readAIn()*21,256))-10;
	test = scaleDownI(fixedDiv(readAIn()*21,256));
	return tune;
}



int main(void){
    init();

    switchLED(2);

    setPWM(0);

    while(1){

        switchLED(0);
		command = getCommand();
        switchLED(0);

        updateSpeed();

        command = executeCommand(command);

		speedPI2(speedSetpoint);// + tune);
	}

    switchLED(2);
	
	return 0;

}



void initUSART( unsigned int ubrr)
{
	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	/*Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);

	/* Set frame format: 8data, 2stop bit */
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