
// This initialization function sets up the ADC 
// Max sample rate: <=125,000 samples/second
// SS3 triggering event: software trigger
// SS3 1st sample source:  channel 1
// SS3 interrupts: enabled but not promoted to controller
void ADC0_Init(void){
  volatile unsigned long delay;	
  SYSCTL_RCGC2_R |= 0x00000010;   // 1) activate clock for Port E
  delay = SYSCTL_RCGC2_R;         //    allow time for clock to stabilize
  GPIO_PORTE_DIR_R &= ~0x04;      // 2) make PE2 input
  GPIO_PORTE_AFSEL_R |= 0x04;     // 3) enable alternate function on PE2
  GPIO_PORTE_DEN_R &= ~0x04;      // 4) disable digital I/O on PE2
  GPIO_PORTE_AMSEL_R |= 0x04;     // 5) enable analog function on PE2
  SYSCTL_RCGC0_R |= 0x00010000;   // 6) activate ADC0
  delay = SYSCTL_RCGC2_R;        
  SYSCTL_RCGC0_R &= ~0x00000300;  // 7) configure for 125K
  ADC0_SSPRI_R = 0x0123;          // 8) Sequencer 3 is highest priority
  ADC0_ACTSS_R &= ~0x0008;        // 9) disable sample sequencer 3
  ADC0_EMUX_R &= ~0xF000;         // 10) seq3 is software trigger
  ADC0_SSMUX3_R &= ~0x000F;       // 11) clear SS3 field
  ADC0_SSMUX3_R += 1;             //    set channel Ain1 (PE2)
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

//------------ADC0_InMean------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
// sampled 5 times and averaged
unsigned long ADC0_Mean(void){ 
	
		unsigned long i, sum;
	
		sum = 0;
		for (i=0;i<5;i++){
			sum += ADC0_In();
		}
		return sum / 5;
}

// initialize SysTick with interrupts
void SysTick_Init(unsigned long period){
  NVIC_ST_CTRL_R = 0;           // disable SysTick during setup
  NVIC_ST_RELOAD_R = period;    // period
  NVIC_ST_CURRENT_R = 0;        // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
}

// start SysTick
void SysTick_Start(void){
    NVIC_ST_CURRENT_R = 0;        // any write to current clears it
	  NVIC_ST_CTRL_R = 0x00000007;  // enable with core clock and interrupts
}

// start SysTick no interrupt
void SysTick_Start_Noint(void){
    NVIC_ST_CURRENT_R = 0;        // any write to current clears it
	  NVIC_ST_CTRL_R = 0x00000005;  // enable with core clock and interrupts
}

// stop SysTick
void SysTick_Stop(void){
	  NVIC_ST_CTRL_R = 0x00000000;
}


// standard header for a 84 x 48 bitmap
const unsigned char Header[0x76] = {
 0x42, 0x4D, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00
};

// 118 + (44x48) = 2112 + 1 = 2231
unsigned char ScreenBMP[2231];

//------------------------------------------------------------------------
// convert terrain data into screen buffer
// Given a Screen buffer componenti Screen[i],
// coords x,y opixel (x,y) of screen (origin upper left) is given by:
// Screen
//------------------------------------------------------------------------
void LandToBMP(unsigned char *landscape){
	
	unsigned char i, j;
	unsigned long k;
	
	// initialize BMP header
	for (i=0; i<0x76; i++) {
		ScreenBMP[i] = Header[i];
	}
	
	k = 0x76;

	for (j=SCREENHEIGHT; j>0 ; j--) {
		for (i=0; i<SCREENWIDTH ; i=i+2) {
			if (i==(SCREENWIDTH-2) && j == 0x2C) {
				k = k;
			}
			ScreenBMP[k] = 0;
			if (*(landscape+i) == (j-1)) {	
				ScreenBMP[k] |= 0xF0;
			}
			if (*(landscape+i+1) == (j-1)) {	
				ScreenBMP[k] |= 0x0F;
			}
			k++;
		}
		// padding extra bytes at the end of a row (32 bit aligned)
		ScreenBMP[k] = 0;
		k++;
		ScreenBMP[k] = 0;
		k++;
	}
	ScreenBMP[2230] = 0xFF;
}
	
//------------------------------------------------------------------------
// display a sprite
//------------------------------------------------------------------------
void DisplaySprite(unsigned char *image, unsigned char xpos, unsigned char ypos){
	
	Nokia5110_PrintBMP(xpos, ypos, image, 0);
	
}

// initialize general parameters for single game
void Game_Init(void){
	
	// level parameters;
	// change this values to make levels
	// more simpler or harder
	Levels[0].Number = 1;
	Levels[0].EnemyRndGen = 65;
	Levels[0].EnemyRocketRndGen = 100;
	Levels[0].EnemySpeedX = 4;
	Levels[0].EnemySpeedY = 10;
	Levels[0].EnemyTotNumber = 15;
	
	Levels[1].Number = 2;
	Levels[1].EnemyRndGen = 50;
	Levels[1].EnemyRocketRndGen = 80;
	Levels[1].EnemySpeedX = 2;
	Levels[1].EnemySpeedY = 6;
	Levels[1].EnemyTotNumber = 20;
	
	Levels[2].Number = 3;
	Levels[2].EnemyRndGen = 40;
	Levels[2].EnemyRocketRndGen = 30;
	Levels[2].EnemySpeedX = 1;
	Levels[2].EnemySpeedY = 3;
	Levels[2].EnemyTotNumber = 25;
	
	CurrentLevel = 0;
	PlayerLifesLeft = PLAYERMAXLIVES;
	Armageddon = 0;
	
}

// initialize general parameters for single level
void Level_Init(){
	
	unsigned char i;
	
	// initialize array of sprite:
	// initially no enemy, no rockets, no explosions
	for (i=0; i<MAXENEMY ; i++) {
		SetSpriteLife(&(Enemies[i]),0);
	}
	for (i=0; i<MAXPLAYERROCKETS ; i++) {
		SetSpriteLife(&(Prockets[i]),0);
	}
	for (i=0; i<MAXENEMYROCKETS ; i++) {
		SetSpriteLife(&(Erockets[i]),0);
	}
	for (i=0; i<MAXEXPLOSIONS ; i++) {
		SetSpriteLife(&(Explosions[i]),0);
	}
	
	// create initial terrain
	 CreateLand(&Terrain, LANDSPEED);
	
	// create player
	CreatePlayer(&Player, (const unsigned char *)PlayerImages,
	               PLAYERFRAMES, PLAYERFRAMESSIZE, PLAYERSPEED);
	
}

// player control:
// converts slider pot ADC value to corresponding 
// y screen coordinate
unsigned char ConvertCoordYPlayer(unsigned long fromAdc){
	
	unsigned long a;
	
	if (fromAdc > LOWSLIDER) {
		return SCREENHEIGHT;
	}
	if (fromAdc < HIGHSLIDER) {
		return 0;
	}
	a = (fromAdc - HIGHSLIDER);
	a = a / STEPSLIDER;
	return a;
}

// returns true if any explosion currently in action
unsigned char ExplLives(void){
	
	unsigned char i;
	
	for (i=0; i<MAXEXPLOSIONS ; i++) {
		if (GetSpriteLife(&(Explosions[i]))) {
			return 1;
		}
	}
	
	return 0;
}

// kill current player sprite with explosion
void KillPlayer(void){
	
	unsigned char k;
	
	if (!UNBREAKABLE) {
	
		SetSpriteLife(&Player,0);
		if (PlayerLifesLeft) {
			PlayerLifesLeft--;
		}
		
		// create new explosion
		for (k=0; k<MAXEXPLOSIONS ; k++) {
			if (!(GetSpriteLife(&(Explosions[k])))) {
				CreateExplosion(&(Explosions[k]),&Player,(const unsigned char *) BigExplosionImages,BIGEXPLFRAMES,
							BIGEXPLFRAMESFRAMESSIZE,BIGEXPLFRAMESSPEED);
				Sound_Explosion();
				InvertScreen = 30; // screen flickering
				break;
			}
		}
	}
	
}

// kill an enemy with explosion
void KillEnemy(unsigned char indenemy){
	
	unsigned char k;
	
	SetSpriteLife(&(Enemies[indenemy]),0); // enemy dead
	Levels[CurrentLevel].EnemyTotNumber--;
	
	// create new explosion
	for (k=0; k<MAXEXPLOSIONS ; k++) {
		if (!(GetSpriteLife(&(Explosions[k])))) {
			CreateExplosion(&(Explosions[k]),&(Enemies[indenemy]),(const unsigned char *) BigExplosionImages,BIGEXPLFRAMES,
						BIGEXPLFRAMESFRAMESSIZE,BIGEXPLFRAMESSPEED);
			Sound_Killed();
			FlashLeds(5000000,5000000,5);
			break;
		}
	}
}

// simple delay function
// which delays 1 millisecond
// assuming 80 MHz clock
void delay_1ms(unsigned long delay){

		unsigned long i,j;
    
    for (j=0;j<delay;j++) {
      i = 12500;
      while(i > 0){
        i = i - 1;
      }
    }
}

// display various text messages on screen
void ScreenMessage(unsigned char mess){
	
	Nokia5110_Clear();
	switch(mess) {
		case 1: Nokia5110_SetCursor(1,1);
						Nokia5110_OutString((char *)MSG_WIN);
						delay_1ms(1000);
						Nokia5110_SetCursor(1,4);
						Nokia5110_OutString((char *)MSG_KEY);
						break;
		case 2: Nokia5110_SetCursor(1,1);
						Nokia5110_OutString((char *)MSG_OVER);
						delay_1ms(1000);
						Nokia5110_SetCursor(1,4);
						Nokia5110_OutString((char *)MSG_KEY);
						break;
		case 3: Nokia5110_SetCursor(2,1);
						Nokia5110_OutString((char *)MSG_LVL);
						Nokia5110_SetCursor(9,1);
						Nokia5110_OutChar((CurrentLevel+1) + 0x030);
						delay_1ms(1000);
						Nokia5110_SetCursor(1,4);
						Nokia5110_OutString((char *)MSG_KEY);
						break;
		case 4: Nokia5110_SetCursor(2,1);
						Nokia5110_OutString((char *)MSG_START);
						delay_1ms(1000);
						Nokia5110_SetCursor(1,4);
						Nokia5110_OutString((char *)MSG_KEY);
						break;
		case 5: Nokia5110_SetCursor(3,1);
						Nokia5110_OutString((char *)MSG_KILLED);
						Nokia5110_SetCursor(0,4);
						Nokia5110_OutChar(PlayerLifesLeft + 0x030);
						Nokia5110_SetCursor(2,4);
						Nokia5110_OutString((char *)MSG_LIVES);
						delay_1ms(1000);
						break;
	}
	
}

// Systick interrupt (30 Hz)
// 		Generate screen frames
// 		Check if buttons pressed
// 		Read slide pot position
//    Moves all sprites
void SysTick_Handler(void){
	
		unsigned char but, i;
		unsigned long cursor;
		
		//-------------------------------------
		// read ADC and move player
		//-------------------------------------
		cursor = ADC0_In();
		MoveSpriteMan(&Player,-1,ConvertCoordYPlayer(cursor));
		
		//-------------------------------------
		// check button pressed
		//-------------------------------------
		if (!(Debounce--)) {
			Debounce = 3;
			but = Switched();
			
			// fire button
			if (but == FIRE1) {
				for (i=0; i < MAXPLAYERROCKETS; i++) {
					
					// create player rocket if there is room
					if (!GetSpriteLife(&(Prockets[i]))) {
						CreatePlayerRocket(&(Prockets[i]), &Player, (const unsigned char *)PlayerRocketImages, PLAYERROCKETFRAMES, 
												 PLAYERROCKETFRAMESSIZE, PLAYERROCKETSPEED);
						Sound_Shoot();
						break;
					}
				}
			}
			
			// armageddon button:
			// destroy all enemy objects on screen
			if (but == FIRE2 && !Armageddon) {
					for (i=0; i<MAXENEMY ; i++) {
						if (GetSpriteLife(&(Enemies[i]))) {
							KillEnemy(i);
						}
					}
					for (i=0; i<MAXENEMYROCKETS ; i++) {
						SetSpriteLife(&(Erockets[i]),0);
					}
					Armageddon++;
					Sound_Armageddon();
			}
		}
		
		// enemies movements
		for (i=0; i<MAXENEMY; i++) {
			MoveSpriteAuto(&(Enemies[i]));
		}
		
		// enemy rockets movements
		for (i=0; i<MAXENEMYROCKETS; i++) {
			MoveSpriteAuto(&(Erockets[i]));
		}

		// player rockets movements
		for (i=0; i<MAXPLAYERROCKETS; i++) {
			MoveSpriteAuto(&(Prockets[i]));
		}
		
		// explosions animations
		for (i=0; i<MAXEXPLOSIONS; i++) {
			AnimateExplosion(&(Explosions[i]));
		}
		
		//terrain movement
		Rnd = Random();
		if (!(FrameNum%5)) {
			Range = Random(); // used to make terrain creation more random
		}
		MoveLand(&Terrain, Rnd, Range);

		Semaphore = 1;
}
