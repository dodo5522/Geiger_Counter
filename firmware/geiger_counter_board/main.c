/*
    9-1-09
    Aaron Weiss for SparkFun Electronics.
    
    Geiger Counter
	
    v13: displays counts per second, every second
	
    Internal 8MHz: lfuse = 0xE2, hfuse = 0xDF
    
*/
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

//load timer with a value to optimize for 1 second, (256/8MHz)*(65536bits-34286)~=1.0s
#define TCNT1_1SEC (34286)

#define FOSC 8000000
#define BAUD 9600
#define MYUBRR 51 

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

#define STATUS_LED 5

volatile unsigned long sec = 0;
volatile unsigned long cps = 0;
volatile unsigned long cpm = 0;

///============Initialize Prototypes=====================///////////////////////
void ioinit(void);      // initializes IO
static int uart_putchar(char c, FILE *stream);
uint8_t uart_getchar(void);
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
void delay_ms(uint16_t x); // general purpose delay
/////===================================================////////////////////////

ISR (INT0_vect) 
{
	++cps;
	cbi(PORTC, STATUS_LED);
}

ISR(TIMER1_OVF_vect)
{
    TCNT1 = TCNT1_1SEC;
    //printf("%lu \r", sec);

    if (sec < 60) {
        cpm += cps;
        ++sec;
    }

    cps = 0;
}

//=========MAIN================/////////////////////////////////////////////////
int main(void)
{
	ioinit(); //Setup IO pins and defaults
	
	delay_ms(1200); //wait to settle
	
	while(1)
	{	
		sbi(PORTC, STATUS_LED);
		delay_ms(30);

        if (sec >= 60) {
            //printf("cpm:%lu          \r", cpm);
            printf("cpm:%lu          \n", cpm);
            cpm = 0;
            sec = 0;
        }
	}
	
	cli(); // Disables all interrupts by clearing the global interrupt mask.
}

////////////////////////////////////////////////////////////////////////////////
///==============Initializations=======================================/////////
////////////////////////////////////////////////////////////////////////////////
void ioinit (void)
{
    //1 = output, 0 = input
    DDRB = 0b11101111; //PB4 = MISO 
    DDRC = 0b00110001; //Output on PORTC0, PORTC4 (SDA), PORTC5 (SCL), all others are inputs
    DDRD = 0b11110010; //PORTD (RX on PD0), input on PD2
	
    //USART Baud rate: 9600
    UBRR0H = MYUBRR >> 8;
    UBRR0L = MYUBRR;
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);    
	
    stdout = &mystdout; //Required for printf init
	
    //pin change interrupt on INT0
    EICRA = (1<<ISC01);//falling edge generates interrupt
    EIMSK = (1<<INT0);
	
    // Setting Timer 1:
    // normal mode
    TCCR1A = 0x00;
    // Set to clk/256 
    TCCR1B |= (1<<CS12);
    //enable overflow interrupt
    TIMSK1 |= (1<<TOIE1);
    //load timer with a value to optimize for 1 second
    TCNT1 = TCNT1_1SEC;
	
    sei(); //turn on global interrupts
}

static int uart_putchar(char c, FILE *stream)
{
    if (c == '\n') uart_putchar('\r', stream);
  
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
    
    return 0;
}

uint8_t uart_getchar(void)
{
    while( !(UCSR0A & (1<<RXC0)) );
    return(UDR0);
}

//General short delays
void delay_ms(uint16_t x)
{
  uint8_t y, z;
  for ( ; x > 0 ; x--){
    for ( y = 0 ; y < 40 ; y++){
      for ( z = 0 ; z < 40 ; z++){
        asm volatile ("nop");
      }
    }
  }
}
