#define F_CPU		16000000UL						//CPU Clock Frequency
#define BAUD		9600							// BAUDRATE
#define UBBR_VALUE  (((F_CPU / (BAUD * 16UL))- 1))  //BAUDRATE REGISTER VALUE 

//LCD PINS MACROS 
#define D0			eS_PORTD0
#define D1			eS_PORTD1
#define D2			eS_PORTD2
#define D3			eS_PORTD3
#define D4			eS_PORTD4
#define D5			eS_PORTD5
#define D6			eS_PORTD6
#define D7			eS_PORTD7
#define RS			eS_PORTB4
#define EN			eS_PORTB5

//import libraries
#include <avr/io.h>
#include "lcd/lcd.h"
#include <avr/interrupt.h>


// door status and usart variable flags
volatile unsigned char openStatus =		0;
volatile unsigned char closeStatus =	0;
const unsigned char check =				1;

//function declarations
void openDoor();
void closeDoor();
void USART_init();
void myDelay();
void serialCheck();
void delayServor();
unsigned char USART_receive();
void servo(int x);
void printLCD(char* statement, int line);

// Interrupt Service Routine (ISR) for External Interrupt INT1
ISR(INT1_vect){
	if (openStatus == 0){
		openStatus = 1;						//set flag on interrupt
	}
}

int main(void)
{
	// PORTD , PORTB IO Configuration
	DDRD |= ((1 << PIND7) | (1 << PIND6) | (1 << PIND5) | (1 << PIND4)); 
	DDRB |= ((1 << PINB5) | (1 << PINB4) | (1 << PINB1)); 
	
	//USART initialization
	USART_init(); 
	
	char* statement = "Automatic Door";
	char* statement2 = "    Project     ";
	Lcd4_Init();							// LCD initialization
	printLCD(statement,1);					// Print Statement Line1
	printLCD(statement2,2);					// Print Statement2 Line2
	Lcd4_Clear();							// clear screen
	EICRA = 0x0C;							// rising edge INT1
	EIMSK = 0x02;							// enable external interrupt INT1
	sei ();									// Global interrupt enable
	myDelay(7813);
	
	while (1)
	{
		if (openStatus ==1 ){
			openDoor();				//open door on flag set
		}
		if (closeStatus == 1){
			closeDoor();			//close door on flag set
		}
		if (check)
		{	
			serialCheck();			//check for data on flag set
		} 	
	}
}

// LCD print function - Input variable: character string and LCD print Line
void printLCD(char* statement, int line) {
	if (line == 1){							// if line 1 
		Lcd4_Clear();						// Clear LCD screen
		Lcd4_Set_Cursor(1,0);				// set cursor
		Lcd4_Write_String(statement);		// print statement
		myDelay(7813);						// delay 0.5s
	} else  {								// if line 2 
		Lcd4_Set_Cursor(2,0);				// set cursor
		Lcd4_Write_String(statement);		// print statement
		myDelay(7813);						// delay 0.5s 
	}
}

//delay function
void myDelay(int x){									// input variable - OCR1A compare value
	OCR1A = x;											// set OCR1 to input variable value
	TCCR1A |= (1<<WGM12)|(1<<COM1A1)|(1<<COM1A0);		// set compare match CTC mode  TCCR1A control register
	TCCR1B |= (1<<CS10)|(1<<CS12);						// prescale clock to 1/1024 * 16MHz =
	while ((TIFR1&(1<<OCF1A))==0)						// while interrupt flag not set do nothing
	{}
	TCCR1A = 0;											// set register 0
	TCCR1B = 0;											// stop timer
	TIFR1 = 1<<OCF1A;									// clear interrupt flag register
}

// delay function servo
void delayServor(){		
	OCR0A = 255;										//set output compare register
	TCCR0A |= (1<<WGM01)|(1<<COM0A1)|(1<<COM0A0);		// set CTC, compare match mode
	TCCR0B |= (1<<CS02)|(1<<CS00);						// prescaler 1024
	while((TIFR0&(1<<OCF0A))==0)						// while interrupt flag not set do nothing
	{}
	TCCR0A = 0;											// set register 0
	TCCR0B = 0;											// stop timer
	TIFR0 = (1<<OCF0A);									// clear interrupt flag register
}

//open door function
void openDoor(){
	// LCD print characters
	char* statement = "Opening Door";
	char* statement2 = "Door Open";
	printLCD(statement, 1);								// LCD print
	servo(4000);										// Door open,servo move
	printLCD(statement2, 1);							// LCD print
	for (int x = 0; x < 2; x++){
		myDelay(62500);									// 8 secs delay before door starts closing
	}
	openStatus = 0;										// change open flag
	closeStatus = 1;									// change close flag
}

// door close
void closeDoor(){
	// LCD print characters
	char* statement = "Closing Door";
	char* statement2 = "Door Closed";
	printLCD(statement, 1);								// LCD print
	servo(2000);										// Door close,servo move
	printLCD(statement2, 1);							// LCD print
	closeStatus = 0;									// change close flag
}

// servo pwm movement
void servo(int x) {
	TCCR1A |= (1 << WGM11) | (1 << COM1A1);					// clear OC1  compare match mode , fast pwm
	TCCR1B |= (1 << WGM13) | (1 << WGM12) |(1 << CS11);		// prescaler 8, fast pwm
	ICR1 = 40000;											// set top
	
	// slow movement of servo
	if(x == 2000) {
		for (int x = 5000; x >= 1000; x= x-25){				// loop through duty cycles
			
			OCR1A = x;
			for (int y = 0; y < 2; y++){
				delayServor();								// delay servo 0.02sec
			}
		}
		} 
	else {
		for (int x = 1000; x <= 5000; x= x+25){				// loop through duty cycles
			OCR1A =x;
			for (int y = 0; y < 2; y++){
				delayServor();								// delay servo 0.02sec
			}
		}
	}
}

// USART initialize
// Reference - Available at ATmega48A/PA/88A/PA/168A/PA/328/P Datasheet, page 185, USART Initialization
void USART_init(void){
	//set baud rate, low and high byte
	UBRR0H = (uint8_t)(UBBR_VALUE>>8); 
	UBRR0L = (uint8_t)(UBBR_VALUE);
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);							//enable transmit
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);						//set 8-bit 
}

// USART receive
unsigned char USART_receive(void){
	if(!(UCSR0A & (1<<RXC0))){								// if no data
		return "NO COM";
	}else {													// when data received
		return UDR0;										//  Read data from UDR
	} 
}

// serial Communication check
void serialCheck(){
	unsigned char ReceivedChar = USART_receive();			//call USART receive
	if (ReceivedChar == 'O'){								// if 'o' open door
		char* statement = "PC Open Door";
		printLCD(statement, 1);
		servo(4000);
	}
	else if(ReceivedChar == 'C'){							// if 'c' close door
		char* statement = "PC Close Door";
		printLCD(statement, 1);
		servo(2000);
	}
}