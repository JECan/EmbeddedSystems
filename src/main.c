#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/portpins.h>
#include <avr/pgmspace.h>
#include <io.c>
#include <bit.h>

//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "keypad.h"

//usart files
#include "usart_ATmega1284.h"
//--------------------------------------

void A2D_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: Enables analog-to-digital conversion
	// ADSC: Starts analog-to-digital conversion
	// ADATE: Enables auto-triggering, allowing for constant
	//        analog to digital conversions.
}

//////////////////////Setting different adc values///////////////////

// Pins on PORTA are used as input for A2D conversion
//    The default channel is 0 (PA0)
// The value of pinNum determines the pin on PORTA
//    used for A2D conversion
// Valid values range between 0 and 7, where the value
//    represents the desired pin for A2D conversion
void Set_A2D_Pin(unsigned char pinNum) {
	ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
	// Allow channel to stabilize
	static unsigned char i = 0;
	for ( i=0; i<30; i++ ) { asm("nop"); }
}

// Sets PA0 to be input pin for A2D conversion
//Set_A2D_Pin(0x00);

// Sets PA7 to be input pin for A2D conversion
//Set_A2D_Pin(0x01);

// Invalid option. Input pin for A2D remains unchanged

//global variable
unsigned short input_mic;
unsigned char upFlag = 0;
short police[] = {600, 700, 800};
unsigned char downFlag = 0;
unsigned char enter;
unsigned char exits;
unsigned int people = 0;
unsigned short MAX = 96;
//unsigned short MAX = 120;
unsigned silent_alarm = 0x00;
unsigned char flash_green = 0x00;

enum Demo_States {init, wait, input, waitInput, inputCheck} demo_state;

void Demo_Init(){
	demo_state = init;

}

// global Variables

//////////////////////////
//Keypad Password Global Variables
/////////////////////////
unsigned char output = 0;
unsigned char x;
char arr [2];
char pwd[3][2] = {{'1','2'}, {'2','3'}, {'3','4'}};
int index = 0;
int pwdIndex  = 0;
int timer = 0;
int numWrong = 0;
/////////////////////////

//////////////////////////
//LCD GLobal VAriables
//////////////////////////
char text[] = "Alert State: 0  Occupancy: 0";
int numPpl = 0;
int disNumPpl = 0;
//////////////////////////

//////////////////////////
//Keypad Password State machine
//////////////////////////
void Demo_Tick() {

	// Transitions
	switch (demo_state) {
		
		case init:
			if(GetKeypadKey()){
				x = GetKeypadKey();
				demo_state = wait;
			}
			else{
				demo_state = init;
			}
			break;
		case wait:
			if(GetKeypadKey()){
				demo_state = wait;
			}
			else{
				demo_state = input;
			}
			break;
		case input:
			if(x == '#'){
				demo_state = inputCheck;
			}
			else if(x != '#'){
				demo_state = waitInput;
			}
			break;
		case waitInput:
			if(!GetKeypadKey()){
				demo_state = waitInput;
			}
			else if(GetKeypadKey()){
				x = GetKeypadKey();
				demo_state = wait;
			}
			break;
		case inputCheck:
			if(GetKeypadKey()){
				demo_state = inputCheck;
			}
			else{
				demo_state = init;
			}
			break;
		default:
			demo_state = init;
			break;
	}
	
	// Actions
	switch (demo_state){
		case init:
			index = 0;
			break;
		case wait:
			break;
		case input:
			arr[index] = x;
			index++;
			break;
		case waitInput:
			break;
		case inputCheck:
			
			if((index-1) > 2){
				//PORTB = PORTB & 0x3F;
				//PORTB = PORTB | 0x80;
				numWrong++;
			}
			else{
				for(int i = 0; i < 2; i++){
					if(arr[i] != pwd[pwdIndex][i]){
						//PORTB = PORTB & 0x3F;
						//PORTB = PORTB | 0x80;
						numWrong++;
						break;
					}
					else{
						//PORTB = PORTB & 0x3F;
						//PORTB = PORTB | 0x40;
						numWrong = 0;
					}
				}
			}
			
			break;
		default:
		break;
	}
}
///////////////////////////////////////////

////////////////////////////
//Rotate password state machine
///////////////////////////
enum Timer_States {timerInit} timer_state;

void Timer_Init(){
	timer_state = timerInit;

}

void Timer_Tick() {

	// Transitions
	switch (timer_state) {
		
		case timerInit:
			break;
		default:
			timer_state = timerInit;
			break;
	}
	
	// Actions
	switch (timer_state){
		case timerInit:
			timer++;
			if((timer%10) == 0){
				pwdIndex++;
				timer = 0;
				if(pwdIndex > 2){
					pwdIndex = 0;
				}
			}
			break;
		default:
		break;
	}
}

///////////////////////////
//LCD State Machine
///////////////////////////

enum LCD_States {LCDInit, display, LCDWait, updateAlert, updatePpl, waitAlert, waitPpl} LCD_state;

void LCD_Init(){
	LCD_state = LCDInit;

}


//Enter Pin: ~PINC = 0x10;
//Exit Pin: ~PINC = 0x20;

bool pwAlert = false;
bool boomAlert = false;
bool boomSet = false;

void LCD_Tick() {
	switch (LCD_state) {
		case LCDInit:
		LCD_state = display;
		break;
		case display:
		LCD_state = LCDWait;
		break;
		case LCDWait:
		//if((~PINA & 0x10) || (~PINA & 0x08)){
		if(disNumPpl != numPpl){
			LCD_state = updatePpl;
		}
		else if(((numWrong >= 3) && (pwAlert == false) && (!boomSet))){
			LCD_state = updateAlert;
		}
		else if((boomAlert) && (!boomSet)){
			LCD_state = updateAlert;
		}
		/*
		else if((~PINB & 0x01) || (~PINB & 0x02) || (~PINB & 0x04) || (~PINB & 0x08)){
			LCD_state = updateAlert;
		}
		*/
		else{
			LCD_state = LCDWait;
		}
		break;
		case updateAlert:
		//LCD_state = waitAlert;
		LCD_state = LCDWait;
		break;
		case updatePpl:
		//LCD_state = waitPpl;
		LCD_state = LCDWait;
		break;
		case waitAlert:
		if(!(~PINB & 0x01) && !(~PINB & 0x02) && !(~PINB & 0x04) && !(~PINB & 0x08)){
			LCD_state = LCDWait;
		}
		else{
			LCD_state = waitAlert;
		}
		break;
		case waitPpl:
		if(!(~PINA & 0x10) && !(~PINA & 0x08)){
			LCD_state = LCDWait;
		}
		else{
			LCD_state = waitPpl;
		}
		break;
		default:
		LCD_state = LCDInit;
		break;
	}
	
	switch(LCD_state){
		case display:
		LCD_ClearScreen();
		LCD_DisplayString(1,text);
		break;
		case LCDWait:
		break;
		case updateAlert:
		if(boomAlert){
			boomSet = true;
			LCD_Cursor(14);
			LCD_WriteData(2 + '0');
		}
		else if(numWrong >= 3){
			pwAlert = true;
			LCD_Cursor(14);
			LCD_WriteData(1 + '0');
		}
		else if(numWrong < 3){
			pwAlert = false;
			LCD_Cursor(14);
			LCD_WriteData(0 + '0');
		}
		/*
		if(~PINB & 0x01){
			LCD_Cursor(14);
			LCD_WriteData(1 + '0');
		}
		else if(~PINB & 0x02){
			LCD_Cursor(14);
			LCD_WriteData(2 + '0');
		}
		else if(~PINB & 0x04){
			LCD_Cursor(14);
			LCD_WriteData(3 + '0');
		}
		else if(~PINB & 0x08){
			LCD_Cursor(14);
			LCD_WriteData(0 + '0');
		}
		*/
		break;
		case updatePpl:
		//if(~PINA & 0x10){
		if(disNumPpl < numPpl){
			if(disNumPpl < 9){
				LCD_Cursor(28);
				disNumPpl = disNumPpl + 1;
				LCD_WriteData(disNumPpl + '0');
			}
		}
		else if(disNumPpl > numPpl){
		//else if(~PINA & 0x08){
			if(disNumPpl != 0){
				LCD_Cursor(28);
				disNumPpl = disNumPpl - 1;
				LCD_WriteData(disNumPpl + '0');
			}
		}
		break;
		case waitPpl:
		break;
		default:
		break;
	}
	
}

///////////////////////////
//Photoresistor 1
///////////////////////////
enum Enter_States {enter_init, enter_wait} Enter_state;

void Enter_Init(){
	Enter_state = enter_init;
	
}

////////////////////////Photoresistor 1 //////////////////////////
void Enter_Tick(){
	//Transitions
	switch(Enter_state){
		case enter_init:
		Set_A2D_Pin(0x00);
		enter = ADC;
		
		if(enter < MAX /2 )
		{
			if(numPpl < 9){
				numPpl++;
			}
			//people = people + 1;
			//PORTB = people | silent_alarm;
			Enter_state = enter_wait;
			/*
			if(people >= 10)
			{
				//set_PWM(police[1]);
				//PORTC = 0x01;
			}
			*/
		}
		else
		Enter_state = enter_init;
		break;
		case enter_wait:
		//PORTB = people | silent_alarm;
		Set_A2D_Pin(0x00);		//enter
		enter = ADC;
		if((enter > MAX / 2))
		{
			//recal = enter;
			Enter_state = enter_init;
			//enter
			enter = ADC;
		}
		else
		Enter_state = enter_wait;
		break;
		default:
		Enter_state = enter_init;
		break;
		
	}
	//Actions
	switch(Enter_state){
		case enter_init:
		//PORTB = people| silent_alarm;
		if(people >= 10)
		{
			//PWM_on();
			//set_PWM(police[1]);
			//PORTC = 0x01;
		}
		else
		//PORTC = 0x00;
		break;
		
		case enter_wait:
		Set_A2D_Pin(0x00);		//enter
		enter = ADC;
		//PORTB = people | silent_alarm;
		break;
		
		default:
		break;
	}
	
}

//////////////////////////
//Photoresistor 2
//////////////////////////
enum Exit_States {exit_init, exit_wait} Exit_state;

void Exit_Init(){
	Exit_state = exit_init;
}

////////////////////////Photoresistor 2 //////////////////////////
void Exit_Tick(){
	//Transitions
	switch(Exit_state){
		case exit_init:
		Set_A2D_Pin(0x01);
		exits = ADC;
		
		if(exits < MAX /2 )
		{
			if(numPpl > 0){
				numPpl--;
			}
			//people = people + 1;
			//PORTB = people | silent_alarm;
			Exit_state = exit_wait;
			/*
			if(people >= 10)
			{
				//set_PWM(police[1]);
				//PORTC = 0x01;
			}
			*/
		}
		else
		Exit_state = exit_init;
		break;
		case exit_wait:
		//PORTB = people | silent_alarm;
		Set_A2D_Pin(0x01);		//enter
		exits = ADC;
		if((exits > MAX / 2))
		{
			//recal = enter;
			Exit_state = exit_init;
			//enter
			exits = ADC;
		}
		else
		Exit_state = exit_wait;
		break;
		default:
		Exit_state = exit_init;
		break;
		
	}
	//Actions
	switch(Exit_state){
		case exit_init:
		//PORTB = people| silent_alarm;
		if(people >= 10)
		{
			//PWM_on();
			//set_PWM(police[1]);
			//PORTC = 0x01;
		}
		else
		//PORTC = 0x00;
		break;
		
		case exit_wait:
		Set_A2D_Pin(0x01);		//enter
		exits = ADC;
		//PORTB = people | silent_alarm;
		break;
		
		default:
		break;
	}
	
}

///////////////////////////

//////////////////////////
//Mic
//////////////////////////
enum Mic_States {mic_init, mic_wait} mic_state;

unsigned short micInput;
unsigned short micInput2;

void Mic_Init(){
	mic_state = mic_init;
}

//////////////////////// Mic //////////////////////////
void Mic_Tick(){
	//Transitions
	switch(mic_state){
		case mic_init:
			if(micInput2 & 0x02){
				mic_state = mic_wait;
			}
			else{
				mic_state = mic_init;
			}
			break;
		
		case mic_wait:
			mic_state = mic_wait;
			break;
		
		default:
			mic_state = mic_init;
			break;
		
	}
	//Actions
	switch(mic_state){
		case mic_init:
			Set_A2D_Pin(0x02);
			micInput = abs(ADC - 760);
			micInput2 = micInput/256;
			//micInput /= 512;
			//PORTB = micInput2;
			
			break;
		
		case mic_wait:
			//PORTB = 0xAA;
			boomAlert = true;
			break;
		
		default:
			break;
	}
	
}

//////////////////////////////////
//USART MASTER
//////////////////////////////////
enum Master_States {mInit, mWait} m_state;

void Master_Init(){
	m_state = mInit;

}

unsigned char mOut = 0;

void Master_Tick() {

	// Local Variables

	// Transitions
	switch (m_state) {
		case mInit:
		if(mOut == 0){
			mOut = 0x85;
			}else{
			mOut = 0x00;
		}
		m_state = mInit;
		break;
		default:
		m_state = mInit;
		break;
	}
	
	// Actions
	switch (m_state) {
		case mInit:
		initUSART(0);
		
		if(USART_HasTransmitted(0)){
			USART_Flush(0);
		}
		
		if(USART_IsSendReady(0)){
			USART_Send(mOut,0);
		}
		
		PORTD = PORTD & 0xFB; 
		//PORTD = PORTD | 0x04;
		PORTD = PORTD | (mOut & 0x04);
		break;
		default:
		break;
	}
	
}

///////////////////////////
//Keypad
void LedSecTask()
{
	Demo_Init();
	for(;;)
	{
		Demo_Tick();
		vTaskDelay(1);
	}
	
}

//Timer
void LedSecTask2()
{
	Timer_Init();
	for(;;)
	{
		Timer_Tick();
		vTaskDelay(1000);
	}
	
}

//LCD
void LedSecTask3()
{
	LCD_Init();
	for(;;)
	{
		LCD_Tick();
		vTaskDelay(1);
	}
	
}

void LedSecTask4()
/*
					Entrance
This state machine is for the photoresistor that acts as an entrance counter. the PH resistor
increments to simulate a person entering the bank, MAX = 96 is the optimal value we found and we divide by 2
to show that the change in resistance means someone entered

*/
{
	Enter_Init();
	for(;;)
	{
		Enter_Tick();
		vTaskDelay(50);
	}
}

void LedSecTask5()
/*
					Exit
This state machine is for the photoresistor that acts as an exit counter. the PH resistor
increments to simulate a person entering the bank, MAX = 96 is the optimal value we found and we divide by 2
to show that the change in resistance means someone entered

*/
{
	Exit_Init();
	for(;;)
	{
		Exit_Tick();
		vTaskDelay(50);
	}
}

void LedSecTask6(){
	Mic_Init();
	for(;;){
		Mic_Tick();
		vTaskDelay(1);
	}
}

//USART
void LedSecTask7()
{
	Master_Init();
	for(;;)
	{
		Master_Tick();
		vTaskDelay(1000);
	}
	
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LedSecTask, (signed portCHAR *)"LedSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void StartSecPulse2(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LedSecTask2, (signed portCHAR *)"LedSecTask2", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void StartSecPulse3(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LedSecTask3, (signed portCHAR *)"LedSecTask3", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void StartSecPulse4(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LedSecTask4, (signed portCHAR *)"LedSecTask4", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void StartSecPulse5(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LedSecTask5, (signed portCHAR *)"LedSecTask5", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void StartSecPulse6(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LedSecTask6, (signed portCHAR *)"LedSecTask6", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void StartSecPulse7(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LedSecTask7, (signed portCHAR *)"LedSecTask7", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void)
{
	DDRC = 0xF0; PORTC = 0x0F; //Keypad
	DDRB = 0xFF; PORTB = 0x00; //LCD Data Lines
	DDRA = 0x00; PORTA = 0xFF; //[0:5] Input, [6:7] LCD control lines
	DDRD = 0xFF; PORTD = 0x00; //Temporarily make PORTD output
	A2D_init();
	LCD_init();
	LCD_ClearScreen();
	LCD_WriteData(1+'0');
	
	//Start Tasks
	//StartSecPulse(4);  //Keypad
	//StartSecPulse2(5); //Timer
	//StartSecPulse3(3); //LCD
	//StartSecPulse4(1); //Enter photoresistor
	//StartSecPulse5(2); //Exit photoresistor
	//StartSecPulse6(6); //Microphone
	StartSecPulse7(7); //USART

	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}