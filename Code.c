#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Define LCD pins
#define LCD_RS PB0
#define LCD_E PB1
#define LCD_D4 PB2
#define LCD_D5 PB3
#define LCD_D6 PB4
#define LCD_D7 PB5



#define RELAY_PIN PD7
#define RepeatProcessButton PD2


volatile uint8_t TempRead=0;
volatile uint8_t AutoTurnOff = 0;
volatile uint16_t Timer = 0;

void LCDcommand(uint8_t CommandByte)
{

PORTB = (PORTB & 0x0F) | (CommandByte & 0xF0);// least 4 bits
PORTB &= ~(1 << LCD_RS);// data being sent is a command not data
PORTB |= (1 << LCD_E);//Sets the LCD_Enable pin, indicating that data is present and ready to be read.
_delay_us(1);
PORTB &= ~(1 << LCD_E);// enabling again and execute the command
_delay_us(200);
PORTB= (PORTB & 0x0F) | (CommandByte << 4);
	//Clears the upper 4 bits of PORTD and sets them according to
	//the lower 4 bits of the command byte shifted left by 4 bits
PORTB |= (1 << LCD_E);
_delay_us(2);
//Sets the LCD_Enable pin again to execute the command.

PORTB &= ~(1 << LCD_E);
_delay_us(2);



}

void LCDdata(uint8_t data) {
PORTB = (PORTB & 0x0F) | (data & 0xF0); // data bits on most 4 bits

PORTB |= (1 << LCD_RS);// Sets the LCD_Reset pin high, indicating that data is being sent.
PORTB |= (1 << LCD_E);//Sets the LCD_Enable pin high, indicating that data is present and ready to be read.

_delay_us(1);
PORTB &= ~(1 << LCD_E);//Clears the LCD_Enable pin to toggle it and execute the data transmission.
_delay_us(200);

PORTB= (PORTB & 0x0F) | (data << 4);//Clears the upper 4 bits of PORTD and sets them according to
//the lower 4 bits of the data byte shifted left by 4 bits.

PORTB |= (1 << LCD_E);
_delay_us(2);

PORTB &= ~(1 << LCD_E);
_delay_us(2);


}

void LCDinit() {

DDRB = 0xFF;
_delay_us(2);

LCDcommand(0x02);//Initialize the LCD to 4-bit mode.
LCDcommand(0x28);// 4-bit data, 2-line display, 5x8 font.
LCDcommand(0x0c);//Display on, cursor off, blink off.
LCDcommand(0x06);//Entry mode: Increment cursor position, no display shift.
LCDcommand(0x01);// Clear display.

_delay_us(2);


}

void LCDClear()
{
LCDcommand (0x01);
_delay_us(2);

LCDcommand (0x80);
}

void LCDprint(char *str) {
int ci;
for(ci=0;str[ci]!=0;ci++)
{
LCDdata (str[ci]);
_delay_us(2000);
}

}


void LCDSetCursor(uint8_t row, uint8_t col) {
uint8_t pos;
if(row==0)
{
	pos= 0x80 +col;
}
else
{
	pos= 0xC0 +col;
}
// If row is 0, the cursor position is calculated as 0x80 + col,
//indicating the starting address for the first line of the LCD.
//If row is not 0, the cursor position is calculated as 0xC0 + col
//indicating the starting address for the second line of the LCD.
LCDcommand(pos);
//This line calls the LCDcommand function to send a command to the LCD controller
//to set the cursor position to the calculated value pos.

}



void ADCinit() {
ADMUX &= ~(1 << REFS1); // Clear REFS1 bit
ADMUX &= ~(1 << REFS0); // Clear REFS0 bit
//By clearing both REFS0 and REFS1,
//the ADC reference voltage is set to the AREF, external voltage reference.
ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1)| (1 << ADPS0);
//sets the pre scaler to divide the system clock frequency by 64,
//providing an appropriate ADC clock frequency for most applications.


}
uint16_t ADCread(uint8_t ch)//takes 8 bit unsigned integer: ch, Reads an analog value from the specified ADC channel.
{
ch &= 0b00000111; // last 3 bit only are used
ADMUX = (ADMUX & 0xF8) | ch;
// clear the last3 bits in ADMUX, set ADMUX register
_delay_ms(100);
ADCSRA |= (1 << ADSC);// start conversion

while (ADCSRA & (1 << ADSC)); //Starts the ADC conversion and waits for it to complete.
return ADC; //Returns the 10-*bit result from the ADC data registers.
}

void TurnOnAC()
{
PORTD |= (1 << PORTD7);
AutoTurnOffFeature();
}


void TurnOffAC()
{
PORTD &= ~(1 << PORTD7);
}

void TIMER0init() {
TCCR0A |= (1 << WGM01);  //Configure Timer 0 to CTC mode.
//TCCR0A is a Timer/Counter Control Register for Timer 0.
//WGM01 is a bit in the TCCR0A register used to set the Waveform Generation Mode.

OCR0A = 156; //Set the compare match value to 156
//This line sets the value for the compare match.
// When the timer counter reaches 156, it will trigger the compare match event.
 
TIMSK0 |= (1 << OCIE0A); //Enable the Output Compare A Match interrupt.
// When the timer counter matches the value in OCR0A (i.e., 156),
// an interrupt will be triggered if global interrupts are enabled.

TCCR0B |= (1 << CS00) |(1 << CS02) ; //Set the timer pre scaler to 1024.
//configures the timer to use a pre scaler of 1024.
//This means the timer frequency is reduced to the CPU clock frequency divided by 1024.
}


void AutoTurnOffFeature() { 
AutoTurnOff=1;
Timer++;
if (Timer >= 10)
{
TurnOffAC();
AutoTurnOff = 0;
if(!(PIND & (1<<RepeatProcessButton))){
RepeatProcess();
}
}
}

void RepeatProcess(){
		DDRD &= ~((1 << RepeatProcessButton));
		PORTD |= (1 << RepeatProcessButton);
		EICRA |= (1 << ISC01) | (1 << ISC00);
		EIMSK |= (1 << INT0);
		sei();
			unsigned int temperature = (ADCread(0)  / 1024.0) * 500.0;
		if (temperature >= 30.0) {
			TurnOnAC();
		}
		else {
			TurnOffAC();
			Timer=0;
		}
}

ISR(INT0_vect)
{
	char temp_str[16];
	unsigned int temperature = (ADCread(0)  / 1024.0) * 500.0;
			if (temperature >= 30.0) {
			TurnOnAC();
		}
		else {
			TurnOffAC();
			Timer=0;
		}
		ADCread(0);
		_delay_ms(1000);
}


int main(void) {

DDRB = 0xFF;
DDRD |= (1 << PORTD7);
DDRD &= ~((1 << RepeatProcessButton));
PORTD |= (1 << RepeatProcessButton);

ADCinit();
LCDinit();
TIMER0init();
char temp_str[16];

while (1) {
unsigned int temperature = (ADCread(0)  / 1024.0) * 500.0;
	
	

if(TempRead==1){
snprintf(temp_str, 16 , "Temperature: %uC", temperature);
LCDSetCursor(0, 0);
LCDprint(temp_str);

if (temperature >= 30.0) {
TurnOnAC();
}
else {
TurnOffAC();
Timer=0;

     }
ADCread(0);
_delay_ms(1000);
     
}
else{
	LCDSetCursor(0, 0);
	LCDprint("      Reading Temperature");
	
	_delay_ms(1000);
	LCDClear();
}
TempRead = 1;
ADCread(0);



}
return 0;
}
