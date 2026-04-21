// Lab9Main.c
// Runs on MSPM0G3507
// Lab 9 ECE319K
// Your name
// Last Modified: January 12, 2026
// game_main.c
// Runs on MSPM0G3507
// BevoKart - ECE319K
// 2-player racing game with UART, obstacles, and difficulty selection

// Board identity is selected at runtime via button press on startup

#include <stdio.h>
#include <stdint.h>
#include "../inc/EdgeTriggered.h"
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "../inc/DAC5.h"
#include  "../inc/Arabic.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/BKsprites/images.h"
#include "../inc/DAC.h"

typedef enum { START, 
  PLAY,
  WIN, 
  LOSE 
} GameState_t;

typedef enum { 
  EASY, 
  MEDIUM, 
  HARD 
} Difficulty_t;

typedef enum { 
  ENGLISH, 
  SPANISH 
} Language_t;

//initializing
volatile GameState_t State;
Difficulty_t Difficulty;
Language_t   Language = ENGLISH;

struct sprite {
  int32_t x, y, xold, yold, vx, vy;
  const unsigned short *image;
  int32_t w, h;
  uint32_t needDraw;
};
typedef struct sprite sprite_t;

// Sprites
sprite_t utc, amc, sqr, bombs;

// Game global vars
uint32_t PB1Pressed, PB4Pressed;
volatile uint8_t gameReady = 0;
volatile uint8_t animFrame = 0;
int32_t  bombActive   = 0;
uint32_t spawnTimer   = 0;
uint32_t spawnInterval;   // ticks between bomb spawns
int32_t  obstacleSpeed;   // pixels/tick for obstacles
volatile uint32_t score = 0;

// Clock
void PLL_Init(void){
  Clock_Init80MHz(0);
}

// Random number generator
uint32_t M = 1;

uint32_t Random32(void){ 
  M = 1664525*M + 1013904223; 
  return M; 
}

uint32_t Random(uint32_t n){ 
  return (Random32()>>16) % n; 
}

//==================================================
// Player 2 buttons — PB12 = left, PB17 = right
#define PB12INDEX 28
#define PB17INDEX 42

uint32_t PB12Pressed, PB17Pressed;

//==================================================
// Difficulty 
void SetDifficulty(Difficulty_t d){
  switch(d){
    case EASY:   
      obstacleSpeed = 1; 
      spawnInterval = 150; 
      break; // 5s at 30Hz
    case MEDIUM: 
      obstacleSpeed = 2; 
      spawnInterval = 90;  
      break; // 3s
    case HARD:   
      obstacleSpeed = 3; 
      spawnInterval = 45;  
      break; // 1.5s
  }
  
  // update variables
  sqr.vx = obstacleSpeed;
  sqr.vy = obstacleSpeed;
  bombs.vy = obstacleSpeed;
}

// needed to flip ADC logic due to orientation
static Difficulty_t ReadDifficulty(void){
  uint32_t adc = ADCin();
  if(adc < 1366) return HARD;
  if(adc < 2731) return MEDIUM;
  return EASY;
}

//==================================================
// Sprite functions

/*
SpawnBomb:
bomb spawns at a random x position and travels down the screen at a random velocity
*/
void SpawnBomb(void){
  bombs.x = Random(70) + 20;
  bombs.y = bombs.h - 1;   // top of screen
  bombs.vx = 0;
  bombs.vy = obstacleSpeed;
  bombs.needDraw = 1;
  bombActive = 1;
}

//initializing the graphics before the main function
void graphics_init(void){
  utc.x = 30;   utc.y = 110;
  utc.xold = utc.x; utc.yold = utc.y;
  utc.vx = 0; utc.vy = 0;
  utc.image = ut_car1;
  utc.w = 15; utc.h = 27;
  utc.needDraw = 1;

  amc.x = 80;   amc.y = 110;
  amc.xold = amc.x; amc.yold = amc.y;
  amc.vx = 0; amc.vy = 0;
  amc.image = am_car1;
  amc.w = 15; amc.h = 27;
  amc.needDraw = 1;

  sqr.x = 50;   sqr.y = 20;
  sqr.xold = sqr.x; sqr.yold = sqr.y;
  sqr.vx = obstacleSpeed;
  sqr.vy = obstacleSpeed;
  sqr.image = sq_r1;
  sqr.w = 20; sqr.h = 10;
  sqr.needDraw = 1;

  bombs.x = 50; bombs.y = 9;
  bombs.xold = bombs.x; bombs.yold = bombs.y;
  bombs.vx = 0; bombs.vy = obstacleSpeed;
  bombs.image = bomb;
  bombs.w = 10; bombs.h = 10;
  bombs.needDraw = 0;
  bombActive = 0;

  spawnTimer = 0;
}

// function to draw the sprites
// >> allows for cleaner code 
void draw(sprite_t *s){
  if(!s->needDraw){
    return;
  }
  ST7735_DrawBitmap(s->x, s->y, s->image, s->w, s->h);
  s->xold = s->x;
  s->yold = s->y;
}

//==================================================
// displays
// Draws animated track as background, then overlays centered text each call
// 128x160 display, SmallFont 6x8px -> 21 cols x 20 rows
// Center col = (21 - charcount) / 2
void DrawStartScreen(Difficulty_t d){
  static uint8_t bgFrame = 0;
  bgFrame ^= 1;
  ST7735_DrawBitmap(0, 160, bgFrame ? track_2 : track_1, 128, 160);

  ST7735_SetCursor(6, 1);    // "BEVOKART" 8 chars -> col 6
  ST7735_OutString(" BEVOKART ");

  // Language row — PB4 toggles
  ST7735_SetCursor(4, 4);    // "Language:PB4" 12 chars -> col 4
  if(Language == ENGLISH){
    ST7735_OutString("Lang(Right):  ");
    ST7735_SetCursor(7, 5);  // "ENGLISH" 7 chars -> col 7
    ST7735_OutString("ENGLISH");
  } else {
    ST7735_OutString("Idioma(Bien):");
    ST7735_SetCursor(7, 5);  // "ESPANOL" 7 chars -> col 7
    ST7735_OutString(" ESPANOL");
  }

  // Difficulty row — slide pot
  ST7735_SetCursor(5, 8);
  if(Language == ENGLISH) {
    ST7735_OutString("Difficulty:");  // 11 chars -> col 5
  } else{
    ST7735_OutString("Dificultad:");  // 11 chars -> col 5
  }
  
  ST7735_SetCursor(7, 10);   // difficulty value ~7 chars -> col 7
  
  if(Language == ENGLISH){
    if(d == EASY){
      ST7735_OutString("  EASY  ");
    } else if(d == MEDIUM) {
      ST7735_OutString(" MEDIUM ");
    } else{
      ST7735_OutString("  HARD  ");
    }
    
  } else {
    if(d == EASY){
      ST7735_OutString("  FACIL  ");
    } else if(d == MEDIUM){
      ST7735_OutString("  MEDIO  ");
    } else{
      ST7735_OutString(" DIFICIL ");
    }
  }

  // Start prompt
  ST7735_SetCursor(4, 14);
  if(Language == ENGLISH){ 
    ST7735_OutString(" left to start ");  // 12 chars -> col 4
  } else{
    ST7735_SetCursor(2, 14);
    ST7735_OutString("izquierda pa jugar");  // 12 chars -> col 4
  } 
}

void DrawWinScreen(void){
  static uint8_t bgFrame = 0;
  bgFrame ^= 1;
  ST7735_DrawBitmap(0, 160, bgFrame ? track_2 : track_1, 128, 160);
  ST7735_SetCursor(5, 5);
  
  if(Language == ENGLISH){
    ST7735_OutString(" UT WINS! ");
  }else {
    ST7735_OutString(" UT GANA! ");
  }
  
  ST7735_SetCursor(4, 8);
  if(Language == ENGLISH){
    ST7735_OutString("Score:");
  } else{
    ST7735_OutString("Pts:");
  }
  
  ST7735_OutUDec(score);
  ST7735_SetCursor(3, 12);
  if(Language == ENGLISH){
    ST7735_OutString(" left to replay ");
  } else{
    ST7735_SetCursor(2, 12);
    ST7735_OutString("izquierda rejugar");
  }
}

void DrawLoseScreen(void){
  static uint8_t bgFrame = 0;
  bgFrame ^= 1;
  ST7735_DrawBitmap(0, 160, bgFrame ? track_2 : track_1, 128, 160);
  ST7735_SetCursor(4, 5);
  if(Language == ENGLISH){
    ST7735_OutString(" A&M WINS! ");
  }else {
    ST7735_OutString(" A&M GANA! ");
  }
  
  ST7735_SetCursor(6, 8);
  if(Language == ENGLISH) ST7735_OutString("Score:");
  else                    ST7735_OutString("Pts:");
  ST7735_OutUDec(score);
  ST7735_SetCursor(3, 12);
  if(Language == ENGLISH){
    ST7735_OutString(" left to replay ");
  }else {
    ST7735_SetCursor(2, 12);
    ST7735_OutString("izquierda rejugar");
  }
}


// =================================================
// Collision
// y is the bottom row of sprite
int32_t collides(sprite_t *a, sprite_t *b){
  return (a->x < b->x + b->w) && (a->x + a->w  > b->x) && (a->y - a->h + 1 <= b->y)&&(a->y >= b->y - b->h + 1);
}

// Game logic
void UpdateMyPlayer(void){
  if(PB1Pressed){
    PB1Pressed = 0;
    amc.x -= 5;
    if(amc.x < 20) amc.x = 20;
  }
  if(PB4Pressed){
    PB4Pressed = 0;
    amc.x += 5;
    if(amc.x > 90) amc.x = 90;
  }
}

void UpdateOtherPlayer(void){
  if(PB12Pressed){
    PB12Pressed = 0;
    utc.x -= 5;
    if(utc.x < 20) utc.x = 20;
  }
  if(PB17Pressed){
    PB17Pressed = 0;
    utc.x += 5;
    if(utc.x > 90) utc.x = 90;
  }
}

void UpdateSprites(void){
  // Move squirrel diagonally, bounce off track walls
  sqr.x += sqr.vx;
  sqr.y += sqr.vy;
  if(sqr.x < 20){ 
    sqr.x = 20;
    sqr.vx = -sqr.vx; 
  }
  
  if(sqr.x + sqr.w > 90){ 
    sqr.x = 90 - sqr.w;
    sqr.vx = -sqr.vx;
  }
  
  if(sqr.y > 160){
    sqr.y = sqr.h - 1; // wrap top
    score++;
  }
  
  if(sqr.y < sqr.h - 1){
    sqr.y = 160; // wrap bottom
  }

  // Move bomb down
  if(bombActive){
    bombs.y += bombs.vy;
    //despawn the bomb
    if(bombs.y > 160){
      bombActive = 0;
      bombs.needDraw = 0;
      score++;
    }
  }

  // Periodically spawn a new bomb
  spawnTimer++;
  if(spawnTimer >= spawnInterval){
    spawnTimer = 0;
    if(!bombActive) SpawnBomb();
  }

  // UT car collisions
  if(collides(&utc, &sqr)){
    Sound_SquirrelDeath();
    utc.y += 5;
    if(utc.y > 160){
      State = LOSE;
      return;
    }
  }
  if(bombActive && collides(&utc, &bombs)){
    Sound_Bomb();
    State = LOSE;
    return;
  }

  // A&M car collisions
  if(collides(&amc, &sqr)){
    Sound_SquirrelDeath();
    amc.y += 5;
    if(amc.y > 160){
      State = WIN;
      return;
    }
  }
  if(bombActive && collides(&amc, &bombs)){
    Sound_Bomb();
    State = WIN;
  }
}

// Interrupt handlers
void TIMG12_IRQHandler(void){
  if((TIMG12->CPU_INT.IIDX) == 1){
    GPIOB->DOUTTGL31_0 = GREEN;

    if(State == PLAY){
      UpdateMyPlayer();
      UpdateOtherPlayer();
      UpdateSprites();
      animFrame ^= 1;
      utc.image = animFrame ? ut_car2 : ut_car1;
      amc.image = animFrame ? am_car2 : am_car1;
      sqr.image = animFrame ? sq_r1   : sq_r2;

      gameReady = 1;
    }

    GPIOB->DOUTTGL31_0 = GREEN;
  }
}

void GROUP1_IRQHandler(void){
  uint32_t status = GPIOB->CPU_INT.RIS;
  if(status & (1<<1)){  
    PB1Pressed  = 1;
  }
  
  if(status & (1<<4)){
    PB4Pressed  = 1;
  }
  
  if(status & (1<<12)){
    PB12Pressed = 1;
  }
  
  if(status & (1<<17)){
    PB17Pressed = 1;
  }
  
  GPIOB->CPU_INT.ICLR = (1<<1)|(1<<4)|(1<<12)|(1<<17);
}

// Main code
int main(void){
  //initializing
  __disable_irq();
  PLL_Init();
  LaunchPad_Init();
  EdgeTriggered_Init();
  ADCinit();
  ST7735_InitPrintf(INITR_REDTAB);
  Sound_Init();
  PB1Pressed = 0;
  PB4Pressed = 0;
  PB12Pressed = 0;
  PB17Pressed = 0;
  __enable_irq();

  while(1){  //main game loop

    //Reset state for new game
    State = START;
    PB1Pressed = 0;
    PB4Pressed = 0;
    PB12Pressed = 0;
    PB17Pressed = 0;
    gameReady = 0;
    animFrame = 0;
    score = 0;

    // start screen
    while(State == START){
      Difficulty_t d = ReadDifficulty();
      if(PB4Pressed){ 
        PB4Pressed = 0; 
        Language = (Language == ENGLISH) ? SPANISH : ENGLISH;
      }
      
      DrawStartScreen(d);
      
      if(PB1Pressed){
        PB1Pressed = 0;
        Difficulty = d;
        SetDifficulty(d);
        State = PLAY;
      }
      Clock_Delay1ms(100);  // ~10Hz animation on start screen
    }

    // ---- Init game ----
    __disable_irq();
    graphics_init();
    ST7735_FillScreen(ST7735_BLACK);
    TimerG12_IntArm(2666667, 2);  // 30Hz
    __enable_irq();

    // gameplay loop
    while(State == PLAY){
      if(gameReady){
        gameReady = 0;
        ST7735_DrawBitmap(0, 160, animFrame ? track_2 : track_1, 128, 160);
        draw(&sqr);
        draw(&utc);
        draw(&amc);
        
        if(bombActive){
          draw(&bombs);
        }
        
        ST7735_SetCursor(0, 0);
        
        if(Language == ENGLISH){
          ST7735_OutString("Score:");
        } else{
          ST7735_OutString("Pts:");
        }
        
        ST7735_OutUDec5(score);
      }
    }

    // end screen
    __disable_irq();
    PB1Pressed = 0;
    
    // wait for button to restart
    while(!PB1Pressed){
      if(State == WIN){
        DrawWinScreen();
      }
      
      if(State == LOSE){ 
        DrawLoseScreen();
      }
      Clock_Delay1ms(100);
      __enable_irq();
    }
    
  }
}
