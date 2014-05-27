// SpaceInvaders.c
// Runs on LM4F120/TM4C123
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
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
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

// ---------------------------------------------------------------------
// Game inspired by "Defender" (1980)
// The game scrolls horizontally.
// The player ship is on the left side of the screen and is controlled vertically by slide pot.
// The terrain below is generate randomly.
// Fire button (PE0) shots a rocket against enemies.
// Special weapon fire button (PE1) destroys alla enemies on the screen (only once per game)
// The are three levels, with increasing difficulty. The player must destroy all
//     enemies in every level to win the game.
// Player loses game if he's killed four times.
// Player is killed if:
// 				-- hit by enemy rocket
//        -- hit by an enemy
//        -- hits the terrain
//        -- an enemy reaches the left edge of the screen
// ---------------------------------------------------------------------

#include "tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "Random.h"
#include "TExaS.h"

#include "SubFile1.c"
#include "SubFile2.c"
#include "SubFile3.c"


//-------------------------------
int main(void){
	
	unsigned long collis;
	unsigned char i,j, k;
	
	FrameNum = 0;

        TExaS_Init(SSI0_Real_Nokia5110_Scope);  
	Nokia5110_Init();
	ADC0_Init();
	Switch_Init();
	Sound_Init();
	Led_Init();
	
	// initialize SysTick to generate an interrupt
	// at 30 Hz	
	SysTick_Init(0x0028B0AA);
	
	// starts SysTick without interrupts
	// to initialize random seed
	SysTick_Start_Noint();
	
	// infinite loop
	while (1) {
		
		Game_Init(); // initialize game parameters
		
		FlashLeds(10000000,64000000,0);
		
		// Starting game message
		ScreenMessage(4);
		WaitFire1Pressed(); // wait for fire pressing
		delay_1ms(100);
		
		FlashLeds(14000000,64000000,1);
		
		// random seed: to make seed more casual,
		// it's set after player has pressed fire button
		Rnd = NVIC_ST_CURRENT_R;
		Random_Init(Rnd);
		SysTick_Stop();
	
		//----------------------------------
		// game loop: ends when all levels completed
		// or no player lives left
		while (PlayerLifesLeft && CurrentLevel<=2) {
			
			Level_Init();

			// start interrupt at 30 Hz
			Debounce = 3;
			SysTick_Start();
			
			//----------------------------------
			// level loop: ends when all enemies killed or player killed;
		  // in any case, finishes explosions animation and sounds for more realistic result;
			// 		check collisions
			// 		generate new sprites
			// 		display sprites on Nokia5110
			while ((GetSpriteLife(&Player) 
							&& Levels[CurrentLevel].EnemyTotNumber)
							|| (ExplLives()) 
							|| Sound_Active()) {
		
				// wait for next frame (SysTick interrupt)
				while (!Semaphore){}
				Semaphore = 0;
			
					
				//----------------------------------
				// check player collisions
				//----------------------------------
					
				// collision with enemies
				for (i=0; i<MAXENEMY; i++) {
					collis = CheckSpritesCollision(&(Enemies[i]),&Player);
					if (collis) {
						KillPlayer(); // player killed
					}
				}
				
				// collision with enemy rockets
				for (i=0; i<MAXENEMYROCKETS; i++) {
					collis = CheckSpritesCollision(&(Erockets[i]),&Player);
					if (collis) {
						KillPlayer(); // player killed
					}
				}
				
				// collision with sky and terrain
				collis = CheckSpriteWorldCollision(&Player,&Terrain);
				
				 // collision with ground
				if (collis == 1) {
					KillPlayer();  // player killed
				}
				
				 // collision with sky
				if (collis == 2) {
					SetDirY(&Player,2); // stop player upward movement
				}
				
				//----------------------------------
				// check player rockets collisions
				//----------------------------------
		
				for (i=0; i<MAXPLAYERROCKETS; i++) {
					
					// collisions with enemies
					for (j=0; j<MAXENEMY; j++) {
						if (CheckSpritesCollision(&(Prockets[i]),&(Enemies[j]))) {
							KillEnemy(j); // enemy dead
							SetSpriteLife(&(Prockets[i]),0);  // rocket dead
						}
					}
					
					// collisions with enemy rockets
					for (j=0; j<MAXENEMYROCKETS; j++) {
						if (CheckSpritesCollision(&(Prockets[i]),&(Erockets[j]))) {
							SetSpriteLife(&(Erockets[j]),0);  // rocket dead
							SetSpriteLife(&(Prockets[i]),0);  // rocket dead
						}
					}
				
					// collisions with terrain or edges
					collis = CheckSpriteWorldCollision(&(Prockets[i]),&Terrain);
					if (collis == 1 || collis == 3) {
						SetSpriteLife(&(Prockets[i]),0);  // rocket dead
					}
				}

				//----------------------------------
				// check enemies collisions
				//----------------------------------
				
				for (i=0; i<MAXENEMY; i++) {
					
					// collision with other enemies
					for (j=i+1; j<MAXENEMY; j++) {
						if (CheckSpritesCollision(&(Enemies[i]),&(Enemies[j]))) {				
							// change Y direction of sprites
							SetDirY(&(Enemies[j]),GetDirY(&(Enemies[j])) ^ 0x01);
						}
					}
					
					// collisions with terrain or edges
					collis = CheckSpriteWorldCollision(&(Enemies[i]),&Terrain);
					
					// terrain
					if (collis == 1) {
						SetDirY(&(Enemies[i]),DIRUP); // upward Y direction
					}
					
					// sky
					if (collis == 2) {
						SetDirY(&(Enemies[i]),DIRDOWN); // downward Y direction
					}
					
					// enemy has reached left edge of the screen
					if (collis == 3) {
						KillPlayer(); // player dead
					}
					
				}
				
				//----------------------------------
				// check enemy rockets collisions
				//----------------------------------
				
				for (i=0; i<MAXENEMYROCKETS; i++) {
					
					// collisions with terrain or edges
					collis = CheckSpriteWorldCollision(&(Erockets[i]),&Terrain);
					if (collis) {
						SetSpriteLife(&(Erockets[i]),0); // rocket dead
					}		
				}
				
				//----------------------------------
				// enemies random creation
				//----------------------------------
				Rnd = Random();
				
				// generate an enemy with a probability
				// 1 / EnemyRndGen
				if (!((Rnd>>16)%Levels[CurrentLevel].EnemyRndGen)) {
					
					for (i=0; i<MAXENEMY; i++) {
						
						// search for an empty bucket in enemies array
						if (!(GetSpriteLife(&(Enemies[i])))) {
							
							// random initial Y direction
//							Rnd = Random();
//							j = (Rnd>>16)%1;
							
							// random initial Y position
							// within sky and terrain
							Rnd = Random();
							k = (Rnd>>16)%(Terrain.landscape[SCREENWIDTH-1]);
							
							// initial Y direction
							if (k > (SCREENHEIGHT /2)) {
								j = DIRUP;
							} else {
								j = DIRDOWN;
							}
							
							// random enemy type
							Rnd = Random();
							if ((Rnd>>16)%2) {
								CreateEnemy(&(Enemies[i]),(const unsigned char *) Enemy1Images, ENEMY1FRAMES, ENEMY1FRAMESIZE, 
														Levels[CurrentLevel].EnemySpeedX, Levels[CurrentLevel].EnemySpeedY, k, j);
							} else {
								CreateEnemy(&(Enemies[i]),(const unsigned char *) Enemy2Images, ENEMY2FRAMES, ENEMY2FRAMESIZE, 
														Levels[CurrentLevel].EnemySpeedX, Levels[CurrentLevel].EnemySpeedY, k, j);
							}
							
							// if new enemy collides with other enemies, doesn't create it
							for (j=0; j<MAXENEMY; j++) {
								if (CheckSpritesCollision(&(Enemies[i]),&(Enemies[j])) && i!=j) {				
									SetSpriteLife(&(Enemies[i]),0);
									break;
								}
							}
							
							// if new enemy collides with terrain or sky, doesn't create it
							if (CheckSpriteWorldCollision(&(Enemies[i]),&Terrain)) {	
								SetSpriteLife(&(Enemies[i]),0);
							}
							break;
						}
					}
				}

				
				//----------------------------------
				// enemy rockets random creation
				//----------------------------------
				
				for (i=0; i<MAXENEMY; i++) {
					if (GetSpriteLife(&(Enemies[i]))) {
						
						// for every enemy, generate a rocket with a probability
						// 1 / EnemyRocketRndGen
						Rnd = Random();
						if (!((Rnd>>16)%Levels[CurrentLevel].EnemyRocketRndGen)) {
							for (j=0; j<MAXENEMYROCKETS; j++) {
								if (!(GetSpriteLife(&(Erockets[j])))) {
									CreateEnemyRocket(&(Erockets[j]),&(Enemies[i]),(const unsigned char *) EnemyRocketImages, ENEMYROCKETFRAMES,
															ENEMYROCKETFRAMESSIZE, ENEMYROCKETSPEED);
									break;
								}
							}
						} 	
					}
				}
				
				//---------------------------------------
				// graphic objects display on screen
				//---------------------------------------
				
				Nokia5110_ClearBuffer();
				
				// display terrain
				LandToBMP(Terrain.landscape);
				DisplaySprite(ScreenBMP,0,SCREENHEIGHT-1);
				
				// display player
				if (GetSpriteLife(&Player)) {
					Nokia5110_DisplaySprite(GetCurFrame(&Player), GetPosX(&Player), GetPosY(&Player));
				}
				
				// display player rockets
				for (i=0; i<MAXPLAYERROCKETS; i++) {
					if (GetSpriteLife(&(Prockets[i]))) {
						Nokia5110_DisplaySprite(GetCurFrame(&(Prockets[i])), GetPosX(&(Prockets[i])), GetPosY(&Prockets[i]));
					}					
				}
				
				// display enemies
				for (i=0; i<MAXENEMY; i++) {
					if (GetSpriteLife(&(Enemies[i]))) {
						Nokia5110_DisplaySprite(GetCurFrame(&(Enemies[i])), GetPosX(&(Enemies[i])), GetPosY(&Enemies[i]));
					}					
				}
				
				// display enemy rockets
				for (i=0; i<MAXENEMYROCKETS; i++) {
					if (GetSpriteLife(&(Erockets[i]))) {
						Nokia5110_DisplaySprite(GetCurFrame(&(Erockets[i])), GetPosX(&(Erockets[i])), GetPosY(&Erockets[i]));
					}					
				}
				
				// animate explosions
				for (i=0; i<MAXEXPLOSIONS; i++) {
					if (GetSpriteLife(&(Explosions[i]))) {
						Nokia5110_DisplaySprite(GetCurFrame(&(Explosions[i])), GetPosX(&(Explosions[i])), GetPosY(&Explosions[i]));
					}					
				}
				
				// flicker screen as a visual effect
//				if (InvertScreen) {
//					if ((InvertScreen--) & 0x02) {
//						Nokia5110_InvertBuffer();
//					}
//				}
				Nokia5110_DisplayBuffer();
				
			} // while (GetSpriteLife(&Player) && Levels[CurrentLevel].EnemyTotNumber) || (ExplLives))
			
			// stop interrupt at 30 Hz
			SysTick_Stop();
			
			InvertScreen = 0;
			
			// player dead
			if (!GetSpriteLife(&Player)) {
				
				// message "Killed"
				ScreenMessage(5);
				
				if (!PlayerLifesLeft) {
					
					// no player lifes left: display message player has loosed
					ScreenMessage(2);
					WaitFire1Pressed();
				} 
				
			// player alive
			} else {
				
				if (CurrentLevel == 2) {
						// player win: display message
						CurrentLevel++;
						ScreenMessage(1);
						WaitFire1Pressed();
				} else { 
					// starts next level: display message
					CurrentLevel++;
					ScreenMessage(3);
					WaitFire1Pressed();	
				}
			}
			
		} // while (PlayerLifesLeft && CurrentLevel<=2)
		
  } // while (1)
	
} // Main