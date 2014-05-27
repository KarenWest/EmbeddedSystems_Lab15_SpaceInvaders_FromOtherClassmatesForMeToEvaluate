// SpaceInvaders.c
// Runs on LM4F120/TM4C123
// Tony R Larson - April 2014 - for edX Lab 15
// 
// includes starter and helper code by:
//
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the edX Lab 15
// In order for other students to play your game
// 1) You must leave the hardware configuration as defined
// 2) You must not add/remove any files from the project
// 3) You must add your code only this this C file
// I.e., if you wish to use code from sprite.c or sound.c, move that code in this file
// 4) It must compile with the 32k limit of the free Keil

// April 10, 2014
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Required Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PE2/AIN1
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1  //no special function programmed (TRL)
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4  // No LED action programmed (TRL)
// LED on PB5

// Blue Nokia 5110
// ---------------
// Signal        (Nokia 5110) LaunchPad pin
// Reset         (RST, pin 1) connected to PA7
// SSI0Fss       (CE,  pin 2) connected to PA3
// Data/Command  (DC,  pin 3) connected to PA6
// SSI0Tx        (Din, pin 4) connected to PA5
// SSI0Clk       (Clk, pin 5) connected to PA2
// 3.3V          (Vcc, pin 6) power
// back light    (BL,  pin 7) not connected, consists of 4 white LEDs which draw ~80mA total
// Ground        (Gnd, pin 8) ground

// Red SparkFun Nokia 5110 (LCD-10168)
// -----------------------------------
// Signal        (Nokia 5110) LaunchPad pin
// 3.3V          (VCC, pin 1) power
// Ground        (GND, pin 2) ground
// SSI0Fss       (SCE, pin 3) connected to PA3
// Reset         (RST, pin 4) connected to PA7
// Data/Command  (D/C, pin 5) connected to PA6
// SSI0Tx        (DN,  pin 6) connected to PA5
// SSI0Clk       (SCLK, pin 7) connected to PA2
// back light    (LED, pin 8) not connected, consists of 4 white LEDs which draw ~80mA total

#include "..//tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "Random.h"
#include "TExaS.h"


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Timer2_Init(unsigned long period);
void Delay100ms(unsigned long count); // time delay in 0.1 seconds
unsigned long TimerCount;  // "Count" conflicted so changed name to TimerCount)
unsigned long Semaphore;
unsigned long M=1;         // Random number seed
// The following global variables set the speed and timing of events in
// the game and change the fun and difficulty of game play significantly.
// Shorter numbers correspond to faster game play, while higher numbers
// slow things down.
// Timer2 is used to provide an 80Hz clock that most game events are
// activated by.
unsigned long time2Laser = 0x100, time2Bonus = 0x100;  // wait a while for lasers and bonus ships
unsigned char displayTCount=2, displayTFlag=0, displayTGoal=3; // update screen every 3/80 sec
unsigned char enemyTCount=79, enemyTFlag=0, enemyTGoal=80; // initially update invaders every sec
unsigned char shipTCount=3, shipTFlag=0, shipTGoal=4; // update player ship every 1/20 sec
unsigned char fireTCount=79, fireTFlag=0, fireTGoal=80; // allow missiles once/sec
unsigned char laserTCount=4, laserTFlag=0, laserTGoal=5; // update falling lasers every 5/80 sec
unsigned char bonusTCount=5, bonusTFlag=0, bonusTGoal=6; // update bonus ship every 6/80 sec
unsigned char numEnemys=15;  // there are 3 rows of 5 columns of invaders initially
unsigned char LEDcount;      // LED timer

///////////////////////////////////////
// from ADC.c  much from Lab 14
///////////////////////////////////////
// Runs on LM4F120/TM4C123
// Provide functions that initialize ADC0 SS3 to be triggered by
// software and trigger a conversion, wait for it to finish,
// and return the result. 
// Daniel Valvano
// April 8, 2014


// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// Max sample rate: <=125,000 samples/second
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: software trigger
// SS3 1st sample source: Ain1 (PE2)
// SS3 interrupts: flag set on completion but no interrupt requested

void ADC0_Init(void){ volatile unsigned long localdelay;
  SYSCTL_RCGC2_R |= 0x00000010;   // 1) activate clock for Port E
  localdelay = SYSCTL_RCGC2_R;    //    allow time for clock to stabilize
  GPIO_PORTE_DIR_R &= ~0x04;      // 2) make PE2 input
  GPIO_PORTE_AFSEL_R |= 0x04;     // 3) enable alternate function on PE2
  GPIO_PORTE_DEN_R &= ~0x04;      // 4) disable digital I/O on PE2
  GPIO_PORTE_AMSEL_R |= 0x04;     // 5) enable analog function on PE2
  SYSCTL_RCGC0_R |= 0x00010000;   // 6) activate ADC0 
  localdelay = SYSCTL_RCGC2_R;    //    allow time for clock to stabilize     
  SYSCTL_RCGC0_R &= ~0x00000300;  // 7) configure for 125K 
  ADC0_SSPRI_R = 0x0123;          // 8) Sequencer 3 is highest priority
  ADC0_ACTSS_R &= ~0x0008;        // 9) disable sample sequencer 3
  ADC0_EMUX_R &= ~0xF000;         // 10) seq3 is software trigger
  ADC0_SSMUX3_R = (ADC0_SSMUX3_R&0xFFFFFFF0)+1; // 11) channel Ain1 (PE2)
  ADC0_SSCTL3_R = 0x0006;         // 12) no TS0 D0, yes IE0 END0
  ADC0_ACTSS_R |= 0x0008;         // 13) enable sample sequencer 3
}


//------------ADC0_In------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
unsigned long ADC0_In(void){  
	unsigned long result;
  ADC0_PSSI_R = 0x0008;            // 1) initiate SS3
  while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done
  result = ADC0_SSFIFO3_R&0xFFF;   // 3) read result
  ADC0_ISC_R = 0x0008;             // 4) acknowledge completion
  return result;
}

////////////////////////////////////////////
// from DAC.c
////////////////////////////////////////////
// Runs on LM4F120 or TM4C123, 
// edX lab 13 
// Implementation of the 4-bit digital to analog converter
// Daniel Valvano, Jonathan Valvano
// March 13, 2014
// Port B bits 3-0 have the 4-bit DAC  correction bits 5-0 have 6-bit DAC



// **************DAC_Init*********************
// Initialize 4-bit DAC   no 6-bit DAC
// Input: none
// Output: none
void DAC_Init(void){ // set up PB0-5 (0-3 for dac, 4-5 for LEDs)
	unsigned long junk;
	SYSCTL_RCGC2_R |= 0x00000002; // (a) activate clock for port B
	  junk = SYSCTL_RCGC2_R;      // just waste a little time for the clock to start
		junk++;
  GPIO_PORTB_DIR_R |= 0x3F;    // (c) make PB0-3 out
  GPIO_PORTB_AFSEL_R &= ~0x3F;  //     disable alt funct on PB0-3
  GPIO_PORTB_DEN_R |= 0x3F;     //     enable digital I/O on PB0-3   
  GPIO_PORTB_PCTL_R &= ~0x00FFFFFF; // configure PB0-3 as GPIO
  GPIO_PORTB_DR8R_R |= 0x3F;     // configure PB0-3 as 8mA strong drivers
  GPIO_PORTB_AMSEL_R = 0;       //     disable analog functionality on PB
}

#define LED_PORTB_DATA_R        (*((volatile unsigned long *)0x400050C0))
	
void Write_LEDS(unsigned char leds)
{ 
   LED_PORTB_DATA_R = leds<<4;  // friendly write to LEDs
}

// **************DAC_Out*********************
// output to DAC
// Input: 4-bit data, 0 to 15 
// Output: none

#define DAC_PORTB_DATA_4BITS    (*((volatile unsigned long *)0x4000503C))

void DAC_Out(unsigned long data){
  	data &= 0x0F;
	  DAC_PORTB_DATA_4BITS = data;   // store data to remaining bits (friendly bit addressing)
}


////////////////////////////////////////////////////////
// Timer0.c
//////////////////////////////////////////////////////////
// Runs on LM4F120/TM4C123
// Use TIMER0 in 32-bit periodic mode to request interrupts at a periodic rate
// Daniel Valvano
// March 20, 2014

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013
  Program 7.5, example 7.6

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */


void (*PeriodicTask)(void);   // user function

// ***************** Timer0_Init ****************
// Activate TIMER0 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq)
// Outputs: none
void Timer0_Init(void(*task)(void), unsigned long period){
  SYSCTL_RCGCTIMER_R |= 0x01;   // 0) activate TIMER0
  PeriodicTask = task;          // user function
  TIMER0_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER0_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER0_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER0_TAILR_R = period-1;    // 4) reload value
  TIMER0_TAPR_R = 0;            // 5) bus clock resolution
  TIMER0_ICR_R = 0x00000001;    // 6) clear TIMER0A timeout flag
  TIMER0_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
  NVIC_EN0_R = 1<<19;           // 9) enable IRQ 19 in NVIC
  TIMER0_CTL_R = 0x00000001;    // 10) enable TIMER0A
}

void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER0A timeout
  (*PeriodicTask)();                // execute user task
}

#include "Subfile1.c"

///////////////////////////////////////////////////
// Subfile1.c includes  ===> Sound.c <===
///////////////////////////////////////////////////
// Runs on any computer
// Sound assets based off the original Space Invaders and basic
// functions to play them.  Copy these functions and constants
// into your SpaceInvaders.c for ease of sharing your game!
// Jonathan Valvano
// November 19, 2012


//////////////////////////////////////////////////
// from Switch.c
//////////////////////////////////////////////////

void Init_Switches(void)
{ unsigned long delay;
  SYSCTL_RCGC2_R |= 0x10;           // Port E clock
  delay = SYSCTL_RCGC2_R;           // wait 3-5 bus cycles
	delay++;
  GPIO_PORTE_AMSEL_R &= ~0x03;      // 3) disable analog function on PE1-0
  GPIO_PORTE_PCTL_R &= ~0x000000FF; // 4) enable regular GPIO
  GPIO_PORTE_DIR_R &= ~0x03;        // 5) inputs on PE1-0
  GPIO_PORTE_AFSEL_R &= ~0x03;      // not alternative
  GPIO_PORTE_AMSEL_R &= ~0x03;      // no analog
  GPIO_PORTE_DEN_R |= 0x03;         // enable PE1, PE0
}

unsigned Read_Switches(void)
{ 
   return (GPIO_PORTE_DATA_R & 0x03);
}



/******************** Random Number Generator ***************************
********************* see Valvano pg 127 *******************************/
unsigned long myRandom(void) {
    M = 1664525*M+1013904223;
	  M = M >> (M & 0x00000007);
    return(M);
    }	


		#include "Subfile2.c"

///////////////////////////////////////////////////
// Subfile2.c includes  ===> Images <===
///////////////////////////////////////////////////
// *************************** Images ***************************

		
// *************************** Capture image dimensions out of BMP**********
#define BUNKERW     ((unsigned char)Bunker0[18])
#define BUNKERH     ((unsigned char)Bunker0[22])
#define ENEMY30W    ((unsigned char)SmallEnemy30PointA[18])
#define ENEMY30H    ((unsigned char)SmallEnemy30PointA[22])
#define ENEMY20W    ((unsigned char)SmallEnemy20PointA[18])
#define ENEMY20H    ((unsigned char)SmallEnemy20PointA[22])
#define ENEMY10W    ((unsigned char)SmallEnemy10PointA[18])
#define ENEMY10H    ((unsigned char)SmallEnemy10PointA[22])
#define ENEMYBONUSW ((unsigned char)SmallEnemyBonus0[18])
#define ENEMYBONUSH ((unsigned char)SmallEnemyBonus0[22])
#define LASERW      ((unsigned char)Laser0[18])
#define LASERH      ((unsigned char)Laser0[22])
#define MISSILEW    ((unsigned char)Missile0[18])
#define MISSILEH    ((unsigned char)Missile0[22])
#define PLAYERW     ((unsigned char)PlayerShip0[18])
#define PLAYERH     ((unsigned char)PlayerShip0[22])
	
// We allow up to MAXLASERS active weapons to be falling from
// the invaders at one time.
// The lasers will be dropped at random times less than LASERDELAY 
// cycles long once they have started falling.
// The bonus ships will be deplayed at random times less than BONUSDELAY 
// cycles long once they have started flying.
#define MAXLASERS   5
#define LASERDELAY  0x7F
#define BONUSDELAY  0x2FF

// Screen and ship related constants.
// These could be calculated in a function to be more portable
// to other systems and screen sizes, but are just entered as
// constants here to save code space.
#define Ship0MaxX 65  // right screen address - ship width (83 - 18)
#define ADC2Ship 63   // 2^12(4095)/Ship0MaxX conversion from ADC to ship position

unsigned char Ship0XPos = 32;  // X position (in pixels) of the player's ship
unsigned char ControlPos = 32; // X position of the ADC slide potentiometer (in screen pixels)
unsigned char EnemyX=100, EnemyY=0, EnemyDir=1, Xoffset=100;  // Xoffset lets all arithmatic be positive with frame offsets

// The enemy ships (invaders) are placed in rows and columns in a frame.
// The frame is then moved around the screen and carries the invaders with it.
// As invaders get shot, the frame size may shrink. The following keep track of
// the frame extents to allow the program to know how far the frame may be moved.
unsigned char frameLeft, frameRight, frameTop, frameBot=0;
unsigned char missilephase=0;  // alternates 0-1-0 to allow the missile to 'wiggle'
unsigned char won = 0, lost = 0;
unsigned points = 0; // Global game score
unsigned char numLasers=0, nextLaser=0, playerLives=2; // number of active lasers and extra lives
unsigned char bunkerExploding=0, bonusExploding=0, oldBonus;

struct spriteinfo {   // This is the structure used for sprite objects
	unsigned char frameX;  // Location in the frame (or on the screen)
  unsigned char	frameY;
	unsigned char condition; // 0 = dead, 1 = live, 2... various levels of health
	unsigned char width;
	unsigned char hight;
	const unsigned char *figure1;  // Image to draw when alive
	const unsigned char *figure2;
};

typedef struct spriteinfo Objects;

Objects EShip[3][5];	     // 15 enemy ships (3 rows, 5 columns)
Objects Missile;           // 1 Missile, shot by player up towards the invaders
Objects Bunker[3];         // 3 Bunkers protecting the player 
Objects Laser[MAXLASERS];  // up to 4 laser bombs at once may be active (dropped by invaders)
Objects Bonus;             // 1 small fast bonus enemy ship

	
void Green_LED (unsigned char greenTime)
{
		LEDcount=greenTime;
		Write_LEDS(0x01); // turn on green LED to indicate a hit or win
}

void Red_LED (unsigned char redTime)
{
		LEDcount=redTime;
		Write_LEDS(0x02); // turn on red LED to indicate a bunker or player hit
}

void FireLaser(unsigned char x, unsigned char y) // Invader drops laser bomb from x, y on screen
{   if (numLasers > (MAXLASERS-1))
	     return;
    Sound_Highpitch();
   	Laser[nextLaser].frameX = x; 
		Laser[nextLaser].frameY = y + LASERH/2; // middle of laser starts at invader coords.
	// TBD should this be offset by invader width/2
		Nokia5110_PrintBMP(Laser[nextLaser].frameX, Laser[nextLaser].frameY, Laser0, 0);
   	Laser[nextLaser].condition = 1;      // make laser 1 = alive, (0 = gone)
   	Laser[nextLaser].width = LASERW;
   	Laser[nextLaser].hight = LASERH;
   	Laser[nextLaser].figure1 = Laser0;
   	Laser[nextLaser].figure2 = Laser1; 
	
	  nextLaser++;  // set up so that the next call to FireLaser grabs the correct bomb
	  if (nextLaser > (MAXLASERS-1))
			nextLaser = 0;
		numLasers++;
}

void GenerateBunkers(void)  // Place 3 protective bunkers above the player's ship.
{   unsigned char i;
    for (i=0; i<3; i++)
			{
      	Bunker[i].frameX = 11 + i * 21;
				Bunker[i].frameY = 47 - PLAYERH;
				Nokia5110_PrintBMP(Bunker[i].frameX, 47 - PLAYERH, Bunker0, 0);
      	Bunker[i].condition = 1; // 1 = alive, 0 = gone, 2 = hit, 3 = damaged
      	Bunker[i].width = BUNKERW;
      	Bunker[i].hight = BUNKERH;
      	Bunker[i].figure1 = Bunker1;
      	Bunker[i].figure2 = Bunker2; 
			}
}

void LaunchBonus(void)
{   
	      Sound_Fastinvader1();
      	Bonus.frameX = 1;
	     	Bonus.frameY = ENEMYBONUSH;
				Nokia5110_PrintBMP(Bonus.frameX, Bonus.frameY, SmallEnemyBonus0, 0);
      	Bonus.condition = 1; // 1 = alive, 0 = gone
      	Bonus.width = ENEMYBONUSW;
      	Bonus.hight = ENEMYBONUSH;
      	Bonus.figure1 = SmallEnemyBonus0;
      	Bonus.figure2 = SmallEnemyBonus0; 
}

void UpdateBonus(void)  // move the small bonus ship across, erase if it died
{ 
	     if (Bonus.condition == 1)
			 { if ( Bonus.frameX < (84 - ENEMYBONUSW))
				 { Bonus.frameX++;
					 oldBonus = Bonus.frameX;
    			 Nokia5110_PrintBMP(Bonus.frameX, Bonus.frameY, SmallEnemyBonus0, 0);
					 if ((Bonus.frameX % 8) == 2)  // update the sound as it moves across
						 Sound_Fastinvader2();
					 else if ((Bonus.frameX % 8) == 4)
						 Sound_Fastinvader3();
					 else if ((Bonus.frameX % 8) == 6)
						 Sound_Fastinvader4();
					 else if ((Bonus.frameX % 8) == 0)
						 Sound_Fastinvader1();
				 }
				 else
				 { Nokia5110_PrintBMP(Bonus.frameX, Bonus.frameY+2, SmallExplosion1, 0);
					 Nokia5110_PrintBMP(Bonus.frameX+2, Bonus.frameY+2, SmallExplosion1, 0);
					 Bonus.condition = 0;
 					 Bonus.frameX = 0;
				 }
			 }
			 
			 if (bonusExploding)
			 { if (bonusExploding > 1)
				   { Nokia5110_PrintBMP(oldBonus, Bonus.frameY+2, SmallExplosion0, 0);
			  	 }
					else 
			  	 { Nokia5110_PrintBMP(oldBonus, Bonus.frameY+2, SmallExplosion1, 0);
						 Nokia5110_PrintBMP(oldBonus+2, Bonus.frameY+2, SmallExplosion1, 0);
             Bonus.condition = 0;
					   Bonus.frameX = 0;
			  	 }
			 }
}

void UpdateBunkers(void)  // Replace solid bunkers with
	                        // tattered ones as thier condition deteriorates
{   unsigned char i;
    for (i=0; i<3; i++)
			{
				if (Bunker[i].condition == 1)
	  			Nokia5110_PrintBMP(Bunker[i].frameX, 47 - PLAYERH, Bunker0, 0);
				else if (Bunker[i].condition == 2)
	  			Nokia5110_PrintBMP(Bunker[i].frameX, 47 - PLAYERH, Bunker1, 0);
				else if (Bunker[i].condition == 3)
	  			Nokia5110_PrintBMP(Bunker[i].frameX, 47 - PLAYERH, Bunker2, 0);
				else 
	  			Nokia5110_PrintBMP(Bunker[i].frameX, 47 - PLAYERH, Bunker3, 0);
				
			if ((bunkerExploding>1) & (Bunker[i].frameY == 1))
				Nokia5110_PrintBMP(Bunker[i].frameX, 47 - PLAYERH, BigExplosion0, 0);

			}
}
	
void GenerateMissile(void) // Initialize the missile object (TBD reduce to condition only?)
{   Missile.frameX = Ship0XPos + PLAYERW / 2 - MISSILEW / 2;
    Missile.frameY = 47 - PLAYERH;
  	Missile.condition = 0; // 1 = alive, 0 = gone, 2 = hit, 3 = damaged
  	Missile.width = MISSILEW;
  	Missile.hight = MISSILEH;
  	Missile.figure1 = Missile0;
  	Missile.figure2 = Missile1; 
}

void GenerateEnemys(void) // Initialize the 15 enemies and place them on the "frame"
{ unsigned char row, col;
	numEnemys=0;
	for (row = 0; row < 3; row++)
	  for (col = 0; col < 5; col++)
	  { EShip[row][col].frameX = col * (ENEMY10W-1);
			EShip[row][col].frameY = row * 9 + (ENEMY10H-1);
			EShip[row][col].condition = 1; // 1 = alive, 0 = gone, 2 = hit, 3 = damaged
			EShip[row][col].width = ENEMY10W;
			EShip[row][col].hight = ENEMY10H;
			if (row==0)
		  {	EShip[row][col].figure1 = SmallEnemy30PointB; EShip[row][col].figure2 = SmallEnemy30PointA; }
			else if (row==1)
		  {	EShip[row][col].figure1 = SmallEnemy20PointA; EShip[row][col].figure2 = SmallEnemy20PointB; }
			else if (row==2)
		  {	EShip[row][col].figure1 = SmallEnemy10PointB; EShip[row][col].figure2 = SmallEnemy10PointA; }
	    numEnemys++;
		}
}

unsigned char Check4BonusHit(unsigned char x) //Did the bonus ship get hit?
{	unsigned char hit=0;

	if (Bonus.condition == 1)
  	if (( x > Bonus.frameX ) & ( x < (Bonus.frameX + Bonus.width - 1)))
			if (Missile.frameY < (Bonus.hight + Missile.hight))
    	  {	hit++;
					Nokia5110_PrintBMP(x, 10, SmallExplosion0, 0); //Show hit
					Sound_Explosion();
	      	Bonus.condition = 0;   // kill bonus ship
	      	points += 300;
					bonusExploding = 16;
	      }
	return(hit);
}

void UpdateMissile(void) // Push missile up to the top of the screen, watch for bonus hit
{
  	if (Missile.condition == 1)
		{
			if ((Missile.frameY - MISSILEH)>1)   // not reached top of screen yet
  		{
	  	  Missile.frameY--;
	  		missilephase ^= 0x01;
	  		Nokia5110_PrintBMP(Missile.frameX, Missile.frameY, missilephase ? Missile.figure1 : Missile.figure2, 0);
      }
	   else 
    	{
	    	Missile.condition = 0;  // kill missile when it reaches the top of the screen
	    	Nokia5110_PrintBMP(Missile.frameX, Missile.frameY, Missile2, 0);
    	}
			
   	if (Check4BonusHit(Missile.frameX))	
	  	{ Missile.condition = 0;   // Kill missile if it hits a bonus ship
	    	Nokia5110_PrintBMP(Missile.frameX, Missile.frameY, Missile2, 0);
				UpdateBonus();
	  	}

		}
}

///////////////////~~~UpdateEnemys~~~///////////////////////
// This is where most of the heavy lifting of the game gets done.
//
// Go through the 15 enemy ships, determine the x and y extent of
// the ships that remain to calculate the "frame" size.  This is
// reported on frameLeft...Right...Top...frameBot so that the main
// loop knows how far the invaders should march before reaching the
// edge of the screen (or the bottom).
// Draw each 'live' enemy in its proper phase (arms up or out).
// If a missile is live, check it against each live invader to determine
// a hit.
// Use looking at each live invader as an event to count down the
// (random) time until the next laser bomb gets dropped or the bonus
// ship is launched.  Reset these times when they trigger.
/////////////////////////////////////////////////////////////////////
void UpdateEnemys(unsigned char phase, unsigned x, unsigned y)
{ unsigned char row, col;
	unsigned absx, absy;

	frameLeft = frameTop = 0xff;
	frameRight = frameBot = 0x00;
	
if (numEnemys >0)
{
	if (Bonus.condition != 1)
	{ if (phase)
		  Sound_Fastinvader1();   // comment this out to stop the marching invaders from making noise
//  	else
//	  	Sound_Fastinvader2();  // un-comment this to alternate marching noises by phase
  }
		
	for (row = 0; row < 3; row++)	  // 3 rows and 5 columns of invaders to check
  	for(col=0; col<5; col++)
	   {
  		absx = x + EShip[row][col].frameX;        // Calculate absolute (screen) coordinates
    	absx -= Xoffset;
    	absy = y+EShip[row][col].frameY;

	    if(EShip[row][col].condition == 2)	      // Erase previously damaged ships
			{ EShip[row][col].condition = 0;
	   	  Nokia5110_PrintBMP(absx, absy, SmallExplosion1, 0);
			}
	    else if(EShip[row][col].condition == 1)    // For live ships determine current "frame" size
	  		{  if (frameLeft > EShip[row][col].frameX)
					    frameLeft = EShip[row][col].frameX + 1;
					 if (frameBot < EShip[row][col].frameY)
					    frameBot = EShip[row][col].frameY + 1;
           if (frameRight < (EShip[row][col].frameX + EShip[row][col].width - 1))
					    frameRight = EShip[row][col].frameX + EShip[row][col].width;
					 if (frameTop > (1 + EShip[row][col].frameY - EShip[row][col].hight))
					    frameTop = 1 + EShip[row][col].frameY - EShip[row][col].hight;
					
					 if (phase==0)                            // draw live ships in proper phase
		   	     Nokia5110_PrintBMP(absx, absy, EShip[row][col].figure1, 0);
		    	 else if (phase==1)
		   	     Nokia5110_PrintBMP(absx, absy, EShip[row][col].figure2, 0);
  		  

		  		if (Missile.condition == 1)              // if missile is in transit, look for hits
					 if((Missile.frameX >= absx) & (Missile.frameX < absx+EShip[row][col].width))
  					 if(((Missile.frameY - MISSILEH) <= (absy)) & ((Missile.frameY - MISSILEH) > (absy-EShip[row][col].hight)))
							 { Missile.condition = 0;
								 Green_LED(25);
								 Sound_Killed();
								 Nokia5110_PrintBMP(Missile.frameX, Missile.frameY, Missile2, 0); // erase exploded missile
								 Missile.frameY = MISSILEH;  // move missile to top of screen just in case
								 EShip[row][col].condition = 2;  // indicate enemy ship has been hit
								 points += (10 * (3 - row))*(1 + (45-absy)/7);
								 numEnemys--;
		 		   	     Nokia5110_PrintBMP(absx, absy, SmallExplosion0, 0);
							 }
							 
						
					if (numEnemys < 12)   // Drop laser bombs once 1/3 of the invaders are gone
					{                     // but only from rows 1&2 if there is no invader below
						                    // (added check for row 2 because row0 might drop if row1 dead)
						if (time2Laser == 0)
						{ if ((row < 2) & (EShip[row][col].condition == 1))
							  if (( EShip[row+1][col].condition == 0) & (absy < 40) & ( EShip[2][col].condition == 0))
								{ FireLaser(absx, absy);
									time2Laser = (myRandom() >> 7) & LASERDELAY;
								}
						}
						else
							time2Laser--;
					}
					
					if (y > 5)  // Begin launching bonus ship when there is room for it
					{           // and after a random wait.  It comes more frequently
						          // when there are more ships alive.
						if ((time2Bonus == 0) & (Bonus.condition == 0))
						{ LaunchBonus();
							time2Bonus = (myRandom() >> 8) & BONUSDELAY;
						}
						else if(time2Bonus > 0)
							time2Bonus--;
					}
							 
		  }
		}
	}
}

void Celebrate(void)  // fireworks and book keeping for victors!
{
	points += 200;  // 200 extra points for eliminating all invaders
	if (EnemyY < 30)
		points += 11*(30-EnemyY); // up to 330 extra points for fast victory
	points += 250 * playerLives;
  Green_LED(80);
	Sound_Highpitch();
	Nokia5110_PrintBMP( 5, 15, SmallExplosion0, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(3);
	Sound_Fastinvader1();
	Nokia5110_PrintBMP(57, 35, SmallExplosion0, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(3);
	Sound_Fastinvader4();
	Nokia5110_PrintBMP(28, 25, SmallExplosion0, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(3);
	Sound_Killed();
	Nokia5110_PrintBMP(18, 40, SmallExplosion0, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(3);
	Sound_Explosion();
	Nokia5110_PrintBMP(40, 12, SmallExplosion0, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(3);
	
 	Sound_Killed();
	Nokia5110_PrintBMP(5, 15, SmallExplosion1, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(2);
	Sound_Killed();
	Nokia5110_PrintBMP(57, 35, SmallExplosion1, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(2);
	Sound_Killed();
	Nokia5110_PrintBMP(28, 25, SmallExplosion1, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(2);
	Sound_Killed();
	Nokia5110_PrintBMP(18, 40, SmallExplosion1, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(2);
	Sound_Killed();
	Nokia5110_PrintBMP(40, 12, SmallExplosion1, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
 	Delay100ms(2);
	
	
}


// If a missile is launched directly under a bunker, it damages the
// bunker and never starts flying.  If a laser bomb hits a bunker
// it explodes and damages the bunker.

unsigned char Check4BunkerHit(unsigned char x)
{	unsigned char i, hit=0;
	
	for (i=0; i<3; i++)
	if (Bunker[i].condition != 0)
  	if (( x > Bunker[i].frameX ) & ( x < (Bunker[i].frameX + Bunker[i].width - 3)))
    	{
				hit++;
				Sound_Explosion();
				Red_LED(30);
				bunkerExploding = 16;
	    	Bunker[i].condition++;   // add damage to the bunker
				Bunker[i].frameY = 1;    // use as flage for explosion
	    	if (Bunker[i].condition > 3)
	    		Bunker[i].condition = 0;
	    }
	UpdateBunkers();
			
	return(hit);
}

void DestroyShip(void)  // destroy the player's ship at the bottom of the screen,
{                       // decrement a life, set lost flage if no more lives.
	Sound_Explosion();
	Red_LED(60);
  Nokia5110_PrintBMP(Ship0XPos, 47, BigExplosion0, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
	Delay100ms(4);
	Nokia5110_PrintBMP(Ship0XPos, 47, BigExplosion1, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
	Delay100ms(5);
	Nokia5110_PrintBMP(Ship0XPos, 47, SmallExplosion1, 0);
  Nokia5110_DisplayBuffer();     // draw buffer
	Delay100ms(6);
	
  if (playerLives > 0)
	{ playerLives--;
    Nokia5110_PrintBMP(Ship0XPos, 47, PlayerShip0, 0);
	}
	else
	{	Sound_Explosion();
		Red_LED(100);
		lost++;
	}
}


void UpdateLaser()  // move a laser bomb and check for its damage to bunkers and players
{ unsigned char i;
	
	if (numLasers > 0)
		for ( i=0; i<MAXLASERS; i++)
	    if ( Laser[i].condition == 1 )
			{ Laser[i].frameY++;
				Nokia5110_PrintBMP(Laser[i].frameX, Laser[i].frameY, Laser[i].figure1, 0);
				
        if ( Laser[i].frameY == 35)       // if laser at top of bunker level -- check for bunker hit
		  		if (Check4BunkerHit(Laser[i].frameX) !=0)
						Laser[i].condition = 0;       // kill laser if it hits a bunker

				if ( Laser[i].frameY == 41)       // if laser at top ship level -- check for ship hit
		  		if ((Laser[i].frameX > Ship0XPos) & (Laser[i].frameX < (Ship0XPos+PLAYERW)))
					{  Laser[i].condition = 0;       // kill laser if it hits a ship
					   DestroyShip();     //					update the hit ship here!!!!!!!!!!!!
					}
					
				if (Laser[i].frameY >= 46) 
					Laser[i].condition = 0;  // kill laser if it hits the screen bottom
					
				if ( Laser[i].condition == 0 )  // Laser recently killed
				{	Nokia5110_PrintBMP(Laser[i].frameX, Laser[i].frameY, Laser[i].figure2, 0);
					numLasers--;
				}

			}
}

void FireMissile(void) // Start a missile from the current player ship location
{ if (fireTFlag == 0) // Timer says it's not time yet
		return;
	else
	{  fireTFlag = 0;
		 fireTCount = 0;  // don't allow another fire for 1/2 second
	}
	
	if (Missile.condition == 0)
	{   Missile.frameX = Ship0XPos + PLAYERW / 2 - MISSILEW / 2;
      Missile.frameY = 44; //  - PLAYERH;
	  	Missile.condition = 1;
		  Sound_Shoot();
	}

  if (Check4BunkerHit(Missile.frameX))	
	{ Missile.condition = 0;   // Kill missile if it is fired into a bunker
		Red_LED(30);
		Sound_Explosion();
	}

}


void UpdateShip0(void)  // update the player's ship at the bottom of the screen
{                       // by following the slide pot location (but only one pixel at a time)
	                      // Fire and update missiles if needed.
	ControlPos =  ADC0_In()/64;
	if (ControlPos > Ship0MaxX)
		ControlPos = Ship0MaxX;
	
	if (ControlPos > Ship0XPos)         // Move one pixel right
		 Ship0XPos++;

	if (ControlPos < Ship0XPos)         // Move one pixel left
		Ship0XPos--;

  Nokia5110_PrintBMP(Ship0XPos, 47, PlayerShip0, 0); //Render player in new location
	UpdateMissile();  // Ship and missile updated at same frequency (move same speed)
	 
	if ((GPIO_PORTE_DATA_R&0x03) > 0)  //either button pressed
		FireMissile();
}

unsigned long highScore = 0;

void ExitScreen(void)
{
  Nokia5110_Clear();               // Display parting message and score
  Nokia5110_SetCursor(1, 0);
  Nokia5110_OutString("GAME OVER");
  Nokia5110_SetCursor(1, 1);
	if (lost)
	{ Red_LED(160);
    Nokia5110_OutString("Nice try,");
	}
	else if (won)
	{ Green_LED(160);
    Nokia5110_OutString("Nice game,");
	}
  Nokia5110_SetCursor(1, 2);
  Nokia5110_OutString("Earthling!");
	if (points > highScore)
  	{ highScore = points;
  		Nokia5110_SetCursor(1, 4);
      Nokia5110_OutString("High Score:");
  	}
	else
  	{ Nokia5110_SetCursor(3, 4);
      Nokia5110_OutString("Score: ");
  	}
  Nokia5110_SetCursor(3, 5);
  Nokia5110_OutUDec(points);
	Delay100ms(30);
  Nokia5110_SetCursor(0, 3);
  Nokia5110_OutString("Press 'Fire'");

}

void SplashScreen(void) // Show at opening of game
{
  Nokia5110_Clear();
	Sound_Fastinvader1();
  Nokia5110_SetCursor(1, 0);
  Nokia5110_OutString("RESISTANCE");
  Nokia5110_SetCursor(4, 1);
  Nokia5110_OutString("IS");
  Nokia5110_SetCursor(2, 2);
  Nokia5110_OutString("FUTILE!!");
  Nokia5110_SetCursor(1, 4);
  Nokia5110_OutString("Earthling");
	Delay100ms(3);              // total delay 2.5 sec
	Sound_Fastinvader2();
	Delay100ms(3);              
	Sound_Fastinvader3();
	Delay100ms(3);              
	Sound_Fastinvader4();
	Delay100ms(3);   
	Sound_Fastinvader1();
	Delay100ms(4);              
	Sound_Fastinvader2();
	Delay100ms(5);              
	Sound_Fastinvader3();
	Delay100ms(6);              
	Sound_Highpitch();
	Delay100ms(7);   

	
}
void GameInit(void) // zero out all of the global variables to start a new game
{ unsigned char i;
  Nokia5110_ClearBuffer();
  GenerateBunkers();	
	GenerateEnemys();
	GenerateMissile();

	Bonus.condition = 0;
	for (i=0; i<MAXLASERS; i++)
	  Laser[i].condition = 0;
	points = 0; // Global game score
  numLasers=0;
	nextLaser=0;
	playerLives=2; 
 	enemyTGoal=80; // initially update invaders every sec
  shipTGoal=4; // update player ship every 1/20 sec
	numEnemys=15;
	won = 0;
	lost = 0;
	time2Laser = 0x100;
	time2Bonus = 0x100;

	EnemyY = 0;            
	EnemyX = Xoffset; 
  Ship0XPos = 32;	
	bunkerExploding=0;
	bonusExploding=0;
	Nokia5110_PrintBMP(Ship0XPos, 47, PlayerShip0, 0); // player ship middle bottom
}

/////////////////////////////////////////////////////
// Main space invaders loop - initialize everything
// update everything repeatedly in a loop when the
// timer flags say to.
////////////////////////////////////////////////////

int main(void){
	unsigned char phase, marchRight=1;
  TExaS_Init(SSI0_Real_Nokia5110_Scope);  // set system clock to 80 MHz
  Random_Init(1);
	Init_Switches();
//	SysTick_Init();  // not needded
	ADC0_Init();   //must include ADC.c in this file to upload
	DAC_Init();    //must include DAC.c in this file to upload
  Nokia5110_Init();
  Nokia5110_ClearBuffer();
	Nokia5110_DisplayBuffer();      // draw buffer
	
	Timer2_Init(999999); // 80hz master clock
	Sound_Init();        // Starts 11.025kHz clock for Valvano sound player
	EnableInterrupts();
	SplashScreen();
	
while(1)  // outside infinite loop 
{	
  	DisableInterrupts();
    GameInit();
	  marchRight=1;
  	phase = 0; 
	  UpdateEnemys(phase, EnemyX, EnemyY);  // initial enemy rendering
    Nokia5110_DisplayBuffer();            // draw buffer to screen
	  Delay100ms(1);
  	EnableInterrupts();  // it's showtime


  while((lost + won)==0) {  // main loop - do until game is over
		if ( enemyTFlag > 0 )  
		{ enemyTFlag = 0;       // Time to update the enemies inside of their 'frame'
			phase ^= 0x01;
  		UpdateEnemys(phase, EnemyX, EnemyY);
			
			if (marchRight)      //  Enemies march (frame location moves left or right)
				EnemyX ++;
			else
				EnemyX--;
			
			if ((EnemyX - Xoffset) >= ( 84 - frameRight))   // Hit right screen edge, now march left
			  {  marchRight = 0;
					 EnemyY++;                                  // drop down on direction change
				}
			else if ( ((EnemyX + frameLeft - 1) <= Xoffset)) // Hit left screen edge, now march right
			  {  marchRight = 1;				
				   EnemyY++;                                   // drop down on direction change
				}
		}

		if (numEnemys == 1)  // Major speed up of ship and missiles when only one invader left
			shipTGoal = 2;
		
		if ( shipTFlag > 0 )
		{ shipTFlag = 0;           // Time to:
  		UpdateShip0();           // draw new ship and missile location
		}
		
		if ( laserTFlag > 0 )
		{ laserTFlag = 0;         // Time to update lasers
  		UpdateLaser(); 
		}
		
		if ( bonusTFlag > 0 )
		{ bonusTFlag = 0;         // Time to update bonus ship
  		UpdateBonus(); 
		}
		

		if ( displayTFlag > 0 )
		{ displayTFlag = 0;              // Time to send out to screen
  	  Nokia5110_DisplayBuffer();     // draw buffer
		}
		
		
		if ( frameBot == 0xff)           // Have we won (lost) yet?
			won = 1;
		else if (numEnemys == 0)
			won = 1;
		else if ((EnemyY + frameBot) > 40)
			lost = 1;
  }

	if (won==1)
		Celebrate();
	else if (lost == 1)
		DestroyShip();
	
	ExitScreen();               // Show score
	
	while (Read_Switches()==0) // fall through and start new game when a button is pressed
	{  Nokia5110_SetCursor(0, 3);
     Nokia5110_OutString("Press 'Fire'");
     Green_LED(10);	//   Write_LEDS(0x03); // blink both LEDs
		 Delay100ms(1);
	   Nokia5110_SetCursor(0, 3);
     Nokia5110_OutString("            ");
		 Red_LED(10);
		 Delay100ms(1);
	}
	while (Read_Switches() != 0) // wait until switch is released so it doesn't act like
		 ;                         // a missile fire in the next game
 }
}


// You can use this timer only if you learn how it works
void Timer2_Init(unsigned long period){ 
  unsigned long volatile delay;
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate timer2
  delay = SYSCTL_RCGCTIMER_R;
  TimerCount = 0;
  Semaphore = 0;
  TIMER2_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = period-1;    // 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN0_R = 1<<23;           // 9) enable IRQ 23 in NVIC
  TIMER2_CTL_R = 0x00000001;    // 10) enable timer2A
}
void Timer2A_Handler(void){ 
  TIMER2_ICR_R = 0x00000001;   // acknowledge timer2A timeout
  TimerCount++;
  Semaphore = 1; // trigger
	                                      ////////~~~Main game timing - 80Hz clock~~~//////
		displayTCount++;                    // update counters and raise flags when        //
		if (displayTCount >= displayTGoal)  // it's time to look at the Display,           //
		{  displayTCount = 0;               // enemys, player ship, lasers, bonus ship,    //
			 displayTFlag = 1;                // fire button, etc.                           //
		}                                   /////////////////////////////////////////////////
		enemyTCount++;
		if (enemyTCount >= enemyTGoal)
		{  enemyTCount = 0;
			 enemyTFlag = 1;
		}

		shipTCount++;	
		if (shipTCount >= shipTGoal)
		{  shipTCount = 0;
			 shipTFlag = 1;
		}

		laserTCount++;	
		if (laserTCount >= laserTGoal)
		{  laserTCount = 0;
			 laserTFlag = 1;
		}

		bonusTCount++;	
		if (bonusTCount >= bonusTGoal)
		{  bonusTCount = 0;
			 bonusTFlag = 1;
		}
		
		fireTCount++;	        // allow fire button only once per second
		if (fireTCount >= fireTGoal)
		{  fireTCount = 0;
			 fireTFlag = 1;
		}

		                           // This could go into the update enemy routine
		                           // but it doesn't take long and works well here
		                           // since it is a timing function.
		
		if (numEnemys < 8)         // speed up invaders as they diminish in number
			enemyTGoal = 2 * numEnemys - 1;
		else if (numEnemys < 12)
			enemyTGoal = (3 * numEnemys) - 8;
		else
			enemyTGoal = (5 * numEnemys) - 30;
		
		if (bunkerExploding)
		{ if (bunkerExploding == 1)
			  UpdateBunkers();
  		bunkerExploding--;	
		}			
		
		if (bonusExploding)
		{ if (bonusExploding == 1)
			  UpdateBonus();
  		bonusExploding--;	
		}			
		
		if (LEDcount)
			{ if (LEDcount == 1)
			   Write_LEDS(0x00);  // turn off any LEDs currently on
  		  LEDcount--;	
		  }			

}

void Delay100ms(unsigned long count){unsigned long volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

// EOF Line  http://youtu.be/wkoryiblxcs    YouTube of this SpaceInvaders
