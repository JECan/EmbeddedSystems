#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#define buttonQ1 GetBit(~PINA, 0)
#define buttonQ2 GetBit(~PINA, 1)
#define buttonQ3 GetBit(~PINA, 2)
#define buttonQ4 GetBit(~PINA, 3)
#define button1player GetBit(~PINA, 4)
#define button2player GetBit(~PINA, 5)
#define buttonStart GetBit(~PINA, 6)
#define buttonReset GetBit(~PINA, 7)


/////////////TIMER START---------------------------------------------------------------------------
volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

/////////////TIMER END-----------------------------------------------------------------------------

unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c; 
	while(1)
	{
		c = a % b ;
		if(c == 0)
		{
			return b;
		}
		a = b;
		b = c;
	}
	return 0;
}

typedef struct _task 
{
	/*Tasks should have members that include: state, period,
	a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int);//Task tick function
} task;


/////////////GLOBAL START-----------------------------------------------------------------------------
volatile int game_array_g[10]; //array which we use for the game
volatile int compare_array_g[10];//compare validity

//strings which the LCD will display
unsigned char LCD_string_g[17];
unsigned char main_menu_g[] = "SIMON SAYS - Select Game Mode 1P or 2P or view high score";
unsigned char victory_g[] = "CONGRATULATIONS!!! YOU WIN!!! Returning to main menu";
unsigned char defeat_seq_g[] = "ERROR! Wrong sequence. Returning to main menu";
unsigned char defeat_time_g[] = "Too much time! Returning to main menu";
unsigned char high_score_1_g[] = "HIGH SCORE: 1";
unsigned char high_score_2_g[] = "HIGH SCORE: 2";
unsigned char high_score_3_g[] = "HIGH SCORE: 3";
unsigned char high_score_4_g[] = "HIGH SCORE: 4";
unsigned char high_score_5_g[] = "HIGH SCORE: 5";
unsigned char high_score_6_g[] = "HIGH SCORE: 6";
unsigned char high_score_7_g[] = "HIGH SCORE: 7";
unsigned char high_score_8_g[] = "HIGH SCORE: 8";
unsigned char high_score_9_g[] = "HIGH SCORE: 9";
unsigned char high_score_10_g[] = "HIGH SCORE: 10";

unsigned char req_g = 0;
unsigned char ack_g = 0;

unsigned char which_button_g;
volatile int current_level_g = 0;
int errorflag = 0;
int level1pass = 0;//must be 1 to pass
int level2pass = 0;
int level3pass = 0;
int level4pass = 0;
int level5pass = 0;
int level6pass = 0;
int level7pass = 0;
int level8pass = 0;
int level9pass = 0;
int level10pass = 0;

unsigned char twoplayer = 0;
int whodat = 0;

int highscore;
uint8_t memVal;

/////////////GLOBAL END-------------------------------------------------------------------------------

void transmit_data_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTD = 0x08;
		// set SER = next bit of data to be sent.
		PORTD |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTD |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTD |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTD = 0x00;
}


void transmit_data_green(unsigned char data) {
	int i;
	for (i = 0; i <8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTC = 0x08;
		// set SER = next bit of data to be sent.
		PORTC |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTC |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTC |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTC = 0x00;
}

void transmit_data_red(unsigned char data) {
	int i;
	for (i = 0; i <8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}
void transmit_data_blue(unsigned char data) {
	//data = data << 4;
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}




/*
fills in game_array_g with random integer values of 0-3
The values correspond to the led matrix as follows:
________________________________________________________________________
|                                  |                                   |
|                                  |                                   |
|            x % 4 = 0             |             x % 4 = 1             |
|                                  |                                   |
|                                  |                                   |
|                                  |                                   |
|__________________________________|___________________________________|
|                                  |                                   |
|                                  |                                   |
|            x % 4 = 2             |             x % 4 = 3             |
|                                  |                                   |
|                                  |                                   |
|                                  |                                   |
|__________________________________|___________________________________|
*/
void randomizeArray()
{
	for(int i = 0; i < 10; i++)
	{
		game_array_g[i] = rand() % 4 + 1;
	}
}


	

enum SM1_States {SM1_init};
int SMTick1(int state)
{
	switch(state)
	{
		case -1:
		break;
		default:
		state = -1;
		break;
	}

	switch(state)
	{
		case -1:
		break;
		default:
		break;
	}
	return state;
}
enum SM2_States {SM2_init};
int SMTick2(int state)
{
	switch(state)
	{
		case -1:
		break;
		default:
		state = -1;
		break;
	}

	switch(state)
	{
		case -1:
		break;
		default:
		break;
	}
	return state;
}
enum SM3_States {SM3_init};
int SMTick3(int state)
{
	switch(state)
	{
		case -1:
		break;
		default:
		state = -1;
		break;
	}

	switch(state)
	{
		case -1:
		break;
		default:
		break;
	}
	return state;
}

//////////////////////////////////////////////////////////

unsigned int getinput()
{
	if(buttonQ1) {return 1;}
	if(buttonQ2) {return 2;}
	if(buttonQ3) {return 3;}
	if(buttonQ4) {return 4;}
	return 0;							
}

//////////////////////////////////////////////////////////
void clear_led()
{
	transmit_data_data(0x00);
	transmit_data_red(0x00);
}
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

void play_total()
{
	//randomizeArray();
	//current_level_g;
	clear_led();
	int i;

	_delay_ms(1000);	
	for(i = 0; i < current_level_g; i++)
	{
		clear_led();
		if(game_array_g[i] == 1){
				_delay_ms(1000);
				transmit_data_data(0xF0);
				transmit_data_red(0xF0);
				_delay_ms(1000);
			
		}
		if(game_array_g[i] == 2){
				_delay_ms(1000);
				transmit_data_data(0x0F);
				transmit_data_red(0xF0);
				_delay_ms(1000);			
		}
		if(game_array_g[i] == 3){
				_delay_ms(1000);	
				transmit_data_data(0xF0);
				transmit_data_red(0x0F);
				_delay_ms(1000);					
		}		
		if(game_array_g[i] == 4){
				_delay_ms(1000);
				transmit_data_data(0x0F);
				transmit_data_red(0x0F);
				_delay_ms(1000);										
		}
	}
	
}

void debug_me()
{
	transmit_data_data(0xFF);
	transmit_data_red(0xFF);
}
void player1victory()
{
	transmit_data_data(0x18);
	transmit_data_red(0xFF);
}
void player2victory()
{
	transmit_data_data(0x66);
	transmit_data_red(0xFF);
}
void AA()
{
	transmit_data_data(0xAA);
	transmit_data_red(0xFF);
}
//////////////////////////////////////////////////////////
enum thegame_1player {wait_game, displaySequence, 
	enter_sequence, end_game, victory, hold_up,
	index_two, index_three, index_four,
	index_five, index_six, index_seven, index_eight,
	index_nine,
	x_display, x_player1, x_player2,
	y_display,
	high_score_show
	} one_player_game;
void tick_one_player_game(){
	int playaround;
	int userInput;
	int userInput2;
	int userInput3;
	int userInput4;
	int userInput5;
	int userInput6;
	int userInput7;
	int userInput8;
	int userInput9;
	int j;

	long long periodsPassed = 0;
	switch(one_player_game)
	{
		case wait_game:
//		if(buttonStart){one_player_game = displaySequence;}
		if(buttonStart){one_player_game = y_display;}
		if(button1player){one_player_game = x_display;}
		if(button2player){one_player_game = high_score_show;}
			break;
			
		case high_score_show:
			if(buttonReset) {one_player_game = wait_game;}
			break;
		case x_display:
			one_player_game = displaySequence;
			break;
		case y_display:
			one_player_game = displaySequence;
			break;
		case x_player1:
			if(buttonReset) {one_player_game = wait_game;}	
			break;
		case x_player2:
			if(buttonReset) {one_player_game = wait_game;}
			break;			
		case displaySequence:
			if(buttonReset) {one_player_game = wait_game;}				
			//if(current_level_g > 10) {one_player_game = victory;}
			if(current_level_g < 10) {one_player_game = enter_sequence;}
			if(current_level_g == 9 && level9pass == 1) {one_player_game = victory;}
			if(current_level_g < 10 && twoplayer == 1) {one_player_game = enter_sequence;}
			break;
		case enter_sequence:
			//if(periodsPassed > 1){one_player_game = end_game;}
			if(buttonReset) {one_player_game = wait_game;}
			if(button1player){one_player_game = displaySequence;}
			if(current_level_g == 1 && level1pass == 1) {one_player_game = displaySequence;}
			if(current_level_g == 2 && level1pass == 0) {one_player_game = enter_sequence;}	
			if(current_level_g == 2 && level1pass == 1) {one_player_game = index_two;}	
			if(current_level_g == 3 && level1pass == 0) {one_player_game = enter_sequence;}
			if(current_level_g == 3 && level1pass == 1) {one_player_game = index_two;}
			if(current_level_g == 4 && level1pass == 0) {one_player_game = enter_sequence;}
			if(current_level_g == 4 && level1pass == 1) {one_player_game = index_two;}
			if(current_level_g == 5 && level1pass == 0) {one_player_game = enter_sequence;}
			if(current_level_g == 5 && level1pass == 1) {one_player_game = index_two;}
			if(current_level_g == 6 && level1pass == 0) {one_player_game = enter_sequence;}
			if(current_level_g == 6 && level1pass == 1) {one_player_game = index_two;}
			if(current_level_g == 7 && level1pass == 0) {one_player_game = enter_sequence;}
			if(current_level_g == 7 && level1pass == 1) {one_player_game = index_two;}
			if(current_level_g == 8 && level1pass == 0) {one_player_game = enter_sequence;}
			if(current_level_g == 8 && level1pass == 1) {one_player_game = index_two;}
			if(current_level_g == 9 && level1pass == 0) {one_player_game = enter_sequence;}
			if(current_level_g == 9 && level1pass == 1) {one_player_game = index_two;}						
			if(errorflag == 1) {one_player_game = end_game;}	
				//////
			if(twoplayer == 1)
			{
							if(buttonReset) {one_player_game = wait_game;}
							if(button1player){one_player_game = displaySequence;}
							if(current_level_g == 1 && level1pass == 1) {one_player_game = displaySequence;}
							if(current_level_g == 2 && level1pass == 0) {one_player_game = enter_sequence;}
							if(current_level_g == 2 && level1pass == 1) {one_player_game = index_two;}
							if(current_level_g == 3 && level1pass == 0) {one_player_game = enter_sequence;}
							if(current_level_g == 3 && level1pass == 1) {one_player_game = index_two;}
							if(current_level_g == 4 && level1pass == 0) {one_player_game = enter_sequence;}
							if(current_level_g == 4 && level1pass == 1) {one_player_game = index_two;}
							if(current_level_g == 5 && level1pass == 0) {one_player_game = enter_sequence;}
							if(current_level_g == 5 && level1pass == 1) {one_player_game = index_two;}
							if(current_level_g == 6 && level1pass == 0) {one_player_game = enter_sequence;}
							if(current_level_g == 6 && level1pass == 1) {one_player_game = index_two;}
							if(current_level_g == 7 && level1pass == 0) {one_player_game = enter_sequence;}
							if(current_level_g == 7 && level1pass == 1) {one_player_game = index_two;}
							if(current_level_g == 8 && level1pass == 0) {one_player_game = enter_sequence;}
							if(current_level_g == 8 && level1pass == 1) {one_player_game = index_two;}
							if(current_level_g == 9 && level1pass == 0) {one_player_game = enter_sequence;}
							if(current_level_g == 9 && level1pass == 1) {one_player_game = index_two;}
							if(errorflag == 1 && whodat % 2 == 1) {one_player_game = x_player2;}
							if(errorflag == 1 && whodat % 2 == 0) {one_player_game = x_player1;}
			}
			break;
		case end_game:
			if(buttonReset) {one_player_game = wait_game;}
			break;
		case victory:
			if(buttonReset) {one_player_game = wait_game;}
			break;
		case index_two:
			//if(periodsPassed > 1){one_player_game = end_game;}
			if(buttonReset) {one_player_game = wait_game;}
			if(button1player){one_player_game = displaySequence;}
			if(level2pass == 1 && current_level_g == 2) {one_player_game = displaySequence;}
			if(level2pass == 1 && current_level_g != 2) {one_player_game = index_three;}
			if(errorflag == 1) {one_player_game = end_game;}
			if(twoplayer == 1)
			{
							if(buttonReset) {one_player_game = wait_game;}
							if(button1player){one_player_game = displaySequence;}
							if(level2pass == 1 && current_level_g == 2) {one_player_game = displaySequence;}
							if(level2pass == 1 && current_level_g != 2) {one_player_game = index_three;}
							if(errorflag == 1 && whodat % 2 == 1) {one_player_game = x_player2;}
							if(errorflag == 1 && whodat % 2 == 0) {one_player_game = x_player1;}
			}
			break;	
		case index_three:
			if(buttonReset) {one_player_game = wait_game;}
			if(button1player){one_player_game = displaySequence;}
			if(level3pass == 1 && current_level_g == 3) {one_player_game = displaySequence;}
			if(level3pass == 1 && current_level_g > 3) {one_player_game = index_four;}
			if(errorflag == 1) {one_player_game = end_game;}
			if(twoplayer == 1)
			{
							if(buttonReset) {one_player_game = wait_game;}
							if(button1player){one_player_game = displaySequence;}
							if(level3pass == 1 && current_level_g == 3) {one_player_game = displaySequence;}
							if(level3pass == 1 && current_level_g > 3) {one_player_game = index_four;}
							if(errorflag == 1 && whodat % 2 == 1) {one_player_game = x_player2;}
							if(errorflag == 1 && whodat % 2 == 0) {one_player_game = x_player1;}
			}
			break;
		case index_four:
			if(buttonReset) {one_player_game = wait_game;}
			if(button1player){one_player_game = displaySequence;}
			if(level4pass == 1 && current_level_g == 4) {one_player_game = displaySequence;}
			if(level4pass == 1 && current_level_g > 4) {one_player_game = index_five;}
			if(errorflag == 1) {one_player_game = end_game;}
			if(twoplayer == 1)
			{
							if(buttonReset) {one_player_game = wait_game;}
							if(button1player){one_player_game = displaySequence;}
							if(level4pass == 1 && current_level_g == 4) {one_player_game = displaySequence;}
							if(level4pass == 1 && current_level_g > 4) {one_player_game = index_five;}
							if(errorflag == 1 && whodat % 2 == 1) {one_player_game = x_player2;}
							if(errorflag == 1 && whodat % 2 == 0) {one_player_game = x_player1;}
			}
			break;
		case index_five:
			if(buttonReset) {one_player_game = wait_game;}
			if(button1player){one_player_game = displaySequence;}
			if(level5pass == 1 && current_level_g == 5) {one_player_game = displaySequence;}
			if(level5pass == 1 && current_level_g > 5) {one_player_game = index_six;}
			if(errorflag == 1) {one_player_game = end_game;}
			if(twoplayer == 1)
			{
							if(buttonReset) {one_player_game = wait_game;}
							if(button1player){one_player_game = displaySequence;}
							if(level5pass == 1 && current_level_g == 5) {one_player_game = displaySequence;}
							if(level5pass == 1 && current_level_g > 5) {one_player_game = index_six;}
							if(errorflag == 1 && whodat % 2 == 1) {one_player_game = x_player2;}
							if(errorflag == 1 && whodat % 2 == 0) {one_player_game = x_player1;}
			}
			break;
		case index_six:
			if(buttonReset) {one_player_game = wait_game;}
			if(button1player){one_player_game = displaySequence;}
			if(level6pass == 1 && current_level_g == 6) {one_player_game = displaySequence;}
			if(level6pass == 1 && current_level_g > 6) {one_player_game = index_seven;}
			if(errorflag == 1) {one_player_game = end_game;}
			if(twoplayer == 1)
			{
							if(buttonReset) {one_player_game = wait_game;}
							if(button1player){one_player_game = displaySequence;}
							if(level6pass == 1 && current_level_g == 6) {one_player_game = displaySequence;}
							if(level6pass == 1 && current_level_g > 6) {one_player_game = index_seven;}
							if(errorflag == 1 && whodat % 2 == 1) {one_player_game = x_player2;}
							if(errorflag == 1 && whodat % 2 == 0) {one_player_game = x_player1;}
			}
			break;
		case index_seven:
			if(buttonReset) {one_player_game = wait_game;}
			if(button1player){one_player_game = displaySequence;}
			if(level7pass == 1 && current_level_g == 7) {one_player_game = displaySequence;}
			if(level7pass == 1 && current_level_g > 7) {one_player_game = index_eight;}
			if(errorflag == 1) {one_player_game = end_game;}
			if(twoplayer == 1)
			{
				
							if(buttonReset) {one_player_game = wait_game;}
							if(button1player){one_player_game = displaySequence;}
							if(level7pass == 1 && current_level_g == 7) {one_player_game = displaySequence;}
							if(level7pass == 1 && current_level_g > 7) {one_player_game = index_eight;}
							if(errorflag == 1 && whodat % 2 == 1) {one_player_game = x_player2;}
							if(errorflag == 1 && whodat % 2 == 0) {one_player_game = x_player1;}
			}
			break;
		case index_eight:
			if(buttonReset) {one_player_game = wait_game;}
			if(button1player){one_player_game = displaySequence;}
			if(level8pass == 1 && current_level_g == 8) {one_player_game = displaySequence;}
			if(level8pass == 1 && current_level_g > 8) {one_player_game = index_nine;}
			if(errorflag == 1) {one_player_game = end_game;}
			if(twoplayer == 1)
			{
				
							if(buttonReset) {one_player_game = wait_game;}
							if(button1player){one_player_game = displaySequence;}
							if(level8pass == 1 && current_level_g == 8) {one_player_game = displaySequence;}
							if(level8pass == 1 && current_level_g > 8) {one_player_game = index_nine;}
							if(errorflag == 1 && whodat % 2 == 1) {one_player_game = x_player2;}
							if(errorflag == 1 && whodat % 2 == 0) {one_player_game = x_player1;}
			}
			break;
		case index_nine:
			if(buttonReset) {one_player_game = wait_game;}
			if(button1player){one_player_game = displaySequence;}
			if(level9pass == 1 && current_level_g == 9) {one_player_game = displaySequence;}
			if(level9pass == 1 && current_level_g > 9) {one_player_game = index_six;}
			if(errorflag == 1) {one_player_game = end_game;}
				if(twoplayer == 1)
				{
								if(buttonReset) {one_player_game = wait_game;}
								if(button1player){one_player_game = displaySequence;}
								if(level9pass == 1 && current_level_g == 9) {one_player_game = displaySequence;}
								if(level9pass == 1 && current_level_g > 9) {one_player_game = index_six;}
							if(errorflag == 1 && whodat % 2 == 1) {one_player_game = x_player2;}
							if(errorflag == 1 && whodat % 2 == 0) {one_player_game = x_player1;}
				}
			break;
		default:
			one_player_game = wait_game;
			break;
	}
	
	switch(one_player_game)
	{
		case wait_game:
			whodat = 0;
			level1pass = 0;//must be 1 to pass
			level2pass = 0;
			level3pass = 0;
			level4pass = 0;
			level5pass = 0;
			level6pass = 0;
			level7pass = 0;
			level8pass = 0;
			level9pass = 0;
			level10pass = 0;
			errorflag = 0;
			current_level_g = 0;
			periodsPassed = 0;
			randomizeArray();
			LCD_DisplayString(1,"SELECT MODE");
			clear_led();
			playaround = getinput();
			if(playaround == 1){
				transmit_data_data(0xF0);
				transmit_data_red(0xF0);
			}
			if(playaround == 2){
				transmit_data_data(0x0F);
				transmit_data_red(0xF0);
			}
			if(playaround== 3){
				transmit_data_data(0xF0);
				transmit_data_red(0x0F);
			}
			if(playaround == 4){
				transmit_data_data(0x0F);
				transmit_data_red(0x0F);
			}
			break;
		case high_score_show:
			if(highscore == -1)
			{
				
				LCD_DisplayString(1, "0: NO SCORE");
			}
			else if(highscore == 0)
			{
				
				LCD_DisplayString(1, "0:NO SCORE");
			}
			else if(highscore == 1)
			{
				
				LCD_DisplayString(1, high_score_1_g);
			}
			else if(highscore == 2)
			{
				
				LCD_DisplayString(1, high_score_1_g);
			}
			else if(highscore == 3)
			{
				
				LCD_DisplayString(1, high_score_2_g);
			}
			else if(highscore == 4)
			{	
				LCD_DisplayString(1, high_score_3_g);
			}
			else if(highscore == 5)
			{
				LCD_DisplayString(1, high_score_4_g);
			}
			else if(highscore == 6)
			{
				LCD_DisplayString(1, high_score_5_g);
			}
			else if(highscore == 7)
			{
				LCD_DisplayString(1, high_score_6_g);
			}
			else if(highscore == 8)
			{
				LCD_DisplayString(1, high_score_7_g);
			}
			else{				LCD_DisplayString(1, "FIXME");}
			
		break;
		case x_display:
			twoplayer = 1;
			break;
		case y_display:
			twoplayer = 0;
			break;
		case x_player1:
			LCD_DisplayString(1,"Player1 Win");
			player1victory();
			break;
		case x_player2:
			LCD_DisplayString(1,"Player2 Win");
			player2victory();
			break;
		case displaySequence:
		
			level1pass = 0;//must be 1 to pass
			level2pass = 0;
			level3pass = 0;
			level4pass = 0;
			level5pass = 0;
			level6pass = 0;
			level7pass = 0;
			level8pass = 0;
			level9pass = 0;
			level10pass = 0;
			j = 0;
			LCD_DisplayString(1,"DISPLAYING");
			whodat++;
			current_level_g++;
			if(current_level_g > highscore) {highscore = current_level_g;			memVal = highscore ;	}
//			memVal = highscore -1;	
			eeprom_update_byte((uint8_t *) 46, memVal);
			play_total();
			clear_led();
			if(current_level_g > 10){current_level_g = 0;}
			break;
		case enter_sequence:
			if(current_level_g > 0)
			{
				LCD_DisplayString(1,"ENTER SEQUENCE");
				userInput = getinput();
				if(userInput == 1){
					if(userInput != game_array_g[0]) {errorflag = 1;}
					if(userInput == game_array_g[0]) {level1pass = 1;}
					transmit_data_data(0xF0);
					transmit_data_red(0xF0);
				}
				if(userInput == 2){
					if(userInput != game_array_g[0]) {errorflag = 1;}
					if(userInput == game_array_g[0]) {level1pass = 1;}
					transmit_data_data(0x0F);
					transmit_data_red(0xF0);
				}
				if(userInput== 3){
					if(userInput != game_array_g[0]) {errorflag = 1;}
					if(userInput == game_array_g[0]) {level1pass = 1;}
					transmit_data_data(0xF0);
					transmit_data_red(0x0F);
				}
				if(userInput == 4){
					if(userInput != game_array_g[0]) {errorflag = 1;}
					if(userInput == game_array_g[0]) {level1pass = 1;}
					transmit_data_data(0x0F);
					transmit_data_red(0x0F);
				}
				if(errorflag == 0 && level1pass == 1){break;}
			}
			break;
		case end_game:
			LCD_DisplayString(1,"YOU LOSE!!!");	
			debug_me();
			break;
		case victory:
			LCD_DisplayString(1,"YOU WIN!!!!");		
			break;
		case index_two:
				clear_led();
				if(current_level_g >= 2)
				{
					level2pass = 0;
					LCD_DisplayString(1,"Level 2");
					userInput = getinput();
					if(userInput == 1){
						if(userInput != game_array_g[1]) {errorflag = 1;}
						if(userInput == game_array_g[1]) {level2pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0xF0);
					}
					if(userInput == 2){
						if(userInput != game_array_g[1]) {errorflag = 1;}
						if(userInput == game_array_g[1]) {level2pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0xF0);
					}
					if(userInput== 3){
						if(userInput != game_array_g[1]) {errorflag = 1;}
						if(userInput == game_array_g[1]) {level2pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0x0F);
					}
					if(userInput == 4){
						if(userInput != game_array_g[1]) {errorflag = 1;}
						if(userInput == game_array_g[1]) {level2pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0x0F);
					}
					if(errorflag == 0 && level2pass == 1){break;}
				}
		break;
		case index_three:
				clear_led();
				if(current_level_g >= 3)
				{
					level3pass = 0;
					LCD_DisplayString(1,"Level 3");
					userInput = getinput();
					if(userInput == 1){
						if(userInput != game_array_g[2]) {errorflag = 1;}
						if(userInput == game_array_g[2]) {level3pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0xF0);
					}
					if(userInput == 2){
						if(userInput != game_array_g[2]) {errorflag = 1;}
						if(userInput == game_array_g[2]) {level3pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0xF0);
					}
					if(userInput== 3){
						if(userInput != game_array_g[2]) {errorflag = 1;}
						if(userInput == game_array_g[2]) {level3pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0x0F);
					}
					if(userInput == 4){
						if(userInput != game_array_g[2]) {errorflag = 1;}
						if(userInput == game_array_g[2]) {level3pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0x0F);
					}
					if(errorflag == 0 && level3pass == 1){break;}
				}
				break;
		case index_four:
				clear_led();
				if(current_level_g >= 4)
				{
					level4pass = 0;
					LCD_DisplayString(1,"Level 4");
					userInput = getinput();
					if(userInput == 1){
						if(userInput != game_array_g[3]) {errorflag = 1;}
						if(userInput == game_array_g[3]) {level4pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0xF0);
					}
					if(userInput == 2){
						if(userInput != game_array_g[3]) {errorflag = 1;}
						if(userInput == game_array_g[3]) {level4pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0xF0);
					}
					if(userInput== 3){
						if(userInput != game_array_g[3]) {errorflag = 1;}
						if(userInput == game_array_g[3]) {level4pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0x0F);
					}
					if(userInput == 4){
						if(userInput != game_array_g[3]) {errorflag = 1;}
						if(userInput == game_array_g[3]) {level4pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0x0F);
					}
					if(errorflag == 0 && level4pass == 1){break;}
				}
				break;
		case index_five:
				clear_led();
				if(current_level_g >= 5)
				{
					level5pass = 0;
					LCD_DisplayString(1,"Level 5");
					userInput = getinput();
					if(userInput == 1){
						if(userInput != game_array_g[4]) {errorflag = 1;}
						if(userInput == game_array_g[4]) {level5pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0xF0);
					}
					if(userInput == 2){
						if(userInput != game_array_g[4]) {errorflag = 1;}
						if(userInput == game_array_g[4]) {level5pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0xF0);
					}
					if(userInput== 3){
						if(userInput != game_array_g[4]) {errorflag = 1;}
						if(userInput == game_array_g[4]) {level5pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0x0F);
					}
					if(userInput == 4){
						if(userInput != game_array_g[4]) {errorflag = 1;}
						if(userInput == game_array_g[4]) {level5pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0x0F);
					}
					if(errorflag == 0 && level5pass == 1){break;}
				}
				break;
		case index_six:
				clear_led();
				if(current_level_g >= 6)
				{
					level6pass = 0;
					LCD_DisplayString(1,"Level 6");
					userInput = getinput();
					if(userInput == 1){
						if(userInput != game_array_g[5]) {errorflag = 1;}
						if(userInput == game_array_g[5]) {level6pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0xF0);
					}
					if(userInput == 2){
						if(userInput != game_array_g[5]) {errorflag = 1;}
						if(userInput == game_array_g[5]) {level6pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0xF0);
					}
					if(userInput== 3){
						if(userInput != game_array_g[5]) {errorflag = 1;}
						if(userInput == game_array_g[5]) {level6pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0x0F);
					}
					if(userInput == 4){
						if(userInput != game_array_g[5]) {errorflag = 1;}
						if(userInput == game_array_g[5]) {level6pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0x0F);
					}
					if(errorflag == 0 && level6pass == 1){break;}
				}
				break;
		case index_seven:
				clear_led();
				if(current_level_g >= 7)
				{
					level7pass = 0;
					LCD_DisplayString(1,"Level 7");
					userInput = getinput();
					if(userInput == 1){
						if(userInput != game_array_g[6]) {errorflag = 1;}
						if(userInput == game_array_g[6]) {level7pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0xF0);
					}
					if(userInput == 2){
						if(userInput != game_array_g[6]) {errorflag = 1;}
						if(userInput == game_array_g[6]) {level7pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0xF0);
					}
					if(userInput== 3){
						if(userInput != game_array_g[6]) {errorflag = 1;}
						if(userInput == game_array_g[6]) {level7pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0x0F);
					}
					if(userInput == 4){
						if(userInput != game_array_g[6]) {errorflag = 1;}
						if(userInput == game_array_g[6]) {level7pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0x0F);
					}
					if(errorflag == 0 && level7pass == 1){break;}
				}
				break;
		case index_eight:
				clear_led();
				if(current_level_g >= 8)
				{
					level8pass = 0;
					LCD_DisplayString(1,"Level 8");
					userInput = getinput();
					if(userInput == 1){
						if(userInput != game_array_g[7]) {errorflag = 1;}
						if(userInput == game_array_g[7]) {level8pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0xF0);
					}
					if(userInput == 2){
						if(userInput != game_array_g[7]) {errorflag = 1;}
						if(userInput == game_array_g[7]) {level8pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0xF0);
					}
					if(userInput== 3){
						if(userInput != game_array_g[7]) {errorflag = 1;}
						if(userInput == game_array_g[7]) {level8pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0x0F);
					}
					if(userInput == 4){
						if(userInput != game_array_g[7]) {errorflag = 1;}
						if(userInput == game_array_g[7]) {level8pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0x0F);
					}
					if(errorflag == 0 && level8pass == 1){break;}
				}
				break;
		case index_nine:
				clear_led();
				if(current_level_g >= 9)
				{
					level8pass = 0;
					LCD_DisplayString(1,"Level 9");
					userInput = getinput();
					if(userInput == 1){
						if(userInput != game_array_g[8]) {errorflag = 1;}
						if(userInput == game_array_g[8]) {level9pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0xF0);
					}
					if(userInput == 2){
						if(userInput != game_array_g[8]) {errorflag = 1;}
						if(userInput == game_array_g[8]) {level9pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0xF0);
					}
					if(userInput== 3){
						if(userInput != game_array_g[8]) {errorflag = 1;}
						if(userInput == game_array_g[8]) {level9pass = 1;}
						transmit_data_data(0xF0);
						transmit_data_red(0x0F);
					}
					if(userInput == 4){
						if(userInput != game_array_g[8]) {errorflag = 1;}
						if(userInput == game_array_g[8]) {level9pass = 1;}
						transmit_data_data(0x0F);
						transmit_data_red(0x0F);
					}
					if(errorflag == 0 && level9pass == 1){break;}
				}
				break;
		default:
			break;
	}
	periodsPassed++;
}
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
int main(void)
{
//	memVal = 0x01;
//	eeprom_write_byte((uint8_t *) 46,memVal);
	memVal = eeprom_read_byte((uint8_t *) 46);
	highscore = memVal;
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	unsigned long int SM1_tick = 250;
	unsigned long int SM2_tick = 100;
	unsigned long int SM3_tick = 1000;

	unsigned long int tempGCD;
	tempGCD = findGCD(SM1_tick, SM2_tick);
	tempGCD = findGCD(tempGCD, SM3_tick);

	unsigned long int GCD = tempGCD;

	unsigned long int SM1_period = SM1_tick/GCD;
	unsigned long int SM2_period = SM2_tick/GCD;
	unsigned long int SM3_period = SM3_tick/GCD;

	static task task1, task2, task3;
	task *tasks[] = {&task1, &task2, &task3};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	task1.state = -1;
	task1.period = SM1_period;
	task1.elapsedTime = SM1_period;
	task1.TickFct = &tick_one_player_game;

	task2.state = -1;
	task2.period = SM1_period;
	task2.elapsedTime = SM1_period;
	task2.TickFct = &SMTick2;//tick_one_player_enter;

	task3.state = -1;
	task3.period = SM1_period;
	task3.elapsedTime = SM1_period;
	task3.TickFct = &SMTick3;

	TimerSet(125);
	TimerOn();
	LCD_init();
	LCD_DisplayString(1,"SIMON SAYS");
	randomizeArray();
	
	one_player_game = wait_game;
	//onePlayerInput = wait_input_1p;

	while(1)
	{
//		tick_one_player_enter();
		tick_one_player_game();

		while(!TimerFlag){};
		TimerFlag = 0;
	}
}
