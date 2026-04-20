// Lab9Main.c
// Runs on MSPM0G3507
// Lab 9 ECE319K
// Your name
// Last Modified: January 12, 2026
// game_main.c
// Runs on MSPM0G3507
// BevoKart - ECE319K
// 2-player racing game with UART, obstacles, and difficulty selection

// ---- Uncomment for UT board, comment out for A&M board ----
#define PLAYER1

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
#include "../inc/Arabic.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/BKsprites/images.h"

typedef enum { START, PLAY, WIN, LOSE } GameState_t;
typedef enum { EASY, MEDIUM, HARD }    Difficulty_t;
typedef enum { ENGLISH, SPANISH }      Language_t;

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
sprite_t *mycar;     // points to this board's car
sprite_t *othercar;  // points to the other board's car

// Game global vars
uint32_t PB1Pressed, PB4Pressed;
volatile uint8_t gameReady = 0;
volatile uint8_t animFrame = 0;
int32_t  bombActive   = 0;
uint32_t spawnTimer   = 0;
uint32_t spawnInterval;   // ticks between bomb spawns
int32_t  obstacleSpeed;   // pixels/tick for obstacles

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

// UART1 driver 
//PA8=TX, PA9=RX
#define PA8INDEX 18
#define PA9INDEX 19

void UART1_Init(void){
  UART1->GPRCM.RSTCTL = 0xB1000003;
  UART1->GPRCM.PWREN  = 0x26000001;
  Clock_Delay(24);
  IOMUX->SECCFG.PINCM[PA8INDEX] = 0x00000082;  // PA8 = UART1 TX
  IOMUX->SECCFG.PINCM[PA9INDEX] = 0x00040082;  // PA9 = UART1 RX
  UART1->CLKSEL = 0x08;
  UART1->CLKDIV = 0x00;
  UART1->CTL0 &= ~0x01;
  UART1->CTL0  = 0x00020018;
  // 80MHz bus -> ULPCLK 40MHz -> /16 = 2.5MHz; 2.5MHz/115200 = 21.701 -> IBRD=21 FBRD=45
  UART1->IBRD = 21;
  UART1->FBRD = 45;
  UART1->LCRH = 0x00000030;  // 8-bit, 1 stop, no parity
  UART1->CPU_INT.IMASK = 0;
  UART1->IFLS = 0x0422;
  UART1->CTL0 |= 0x01;
}

char UART1_InChar(void){
  while((UART1->STAT & 0x04) == 0x04){};  // wait while RX FIFO empty
  return (char)(UART1->RXDATA);
}

void UART1_OutChar(char data){
  while((UART1->STAT & 0x80) == 0x80){};  // wait while TX FIFO full
  UART1->TXDATA = data;
}

// UART1_InChar waits on STAT & 0x04 (bit 2 = RXFE), so Available checks same bit
#define UART_RXFE 0x04
static uint8_t UART1_Available(void){
  return !(UART1->STAT & UART_RXFE);
}

// Flag set by ISR when a lose condition occurs — main loop sends the packet
volatile uint8_t SendLoseFlag = 0;

// Packet types
#define PKT_POS  0xFF   // position update: FF x y
#define PKT_LOSE 0xFE   // sender lost the game: FE 00 00

// Difficulty 
void SetDifficulty(Difficulty_t d){
  switch(d){
    case EASY:   obstacleSpeed = 1; spawnInterval = 150; break; // 5s at 30Hz
    case MEDIUM: obstacleSpeed = 2; spawnInterval = 90;  break; // 3s
    case HARD:   obstacleSpeed = 3; spawnInterval = 45;  break; // 1.5s
  }
  sqr.vx = obstacleSpeed;
  sqr.vy = obstacleSpeed;
  bombs.vy = obstacleSpeed;
}

static Difficulty_t ReadDifficulty(void){
  uint32_t adc = ADCin();
  if(adc < 1366) return EASY;
  if(adc < 2731) return MEDIUM;
  return HARD;
}

// Sprite functions
void SpawnBomb(void){
  bombs.x = Random(70) + 20;
  bombs.y = bombs.h - 1;   // top of screen
  bombs.vx = 0;
  bombs.vy = obstacleSpeed;
  bombs.needDraw = 1;
  bombActive = 1;
}

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

void draw(sprite_t *s){
  if(!s->needDraw) return;
  ST7735_DrawBitmap(s->x, s->y, s->image, s->w, s->h);
  s->xold = s->x;
  s->yold = s->y;
}

// displays
// Draws animated track as background, then overlays centered text each call
// 128x160 display, SmallFont 6x8px -> 21 cols x 20 rows
// Center col = (21 - charcount) / 2
void DrawStartScreen(Difficulty_t d){
  static uint8_t bgFrame = 0;
  bgFrame ^= 1;
  ST7735_DrawBitmap(0, 160, bgFrame ? track_2 : track_1, 128, 160);

  ST7735_SetCursor(6, 1);    // "BEVOKART" 8 chars -> col 6
  ST7735_OutString("BEVOKART");

  // Language row — PB4 toggles
  ST7735_SetCursor(4, 4);    // "Language:PB4" 12 chars -> col 4
  if(Language == ENGLISH){
    ST7735_OutString("Lang(PB4):  ");
    ST7735_SetCursor(7, 5);  // "ENGLISH" 7 chars -> col 7
    ST7735_OutString("ENGLISH");
  } else {
    ST7735_OutString("Idioma(PB4):");
    ST7735_SetCursor(7, 5);  // "ESPANOL" 7 chars -> col 7
    ST7735_OutString("ESPANOL");
  }

  // Difficulty row — slide pot
  ST7735_SetCursor(5, 8);
  if(Language == ENGLISH) ST7735_OutString("Difficulty:");  // 11 chars -> col 5
  else                    ST7735_OutString("Dificultad:");  // 11 chars -> col 5

  ST7735_SetCursor(7, 10);   // difficulty value ~7 chars -> col 7
  if(Language == ENGLISH){
    if(d == EASY)        ST7735_OutString("  EASY  ");
    else if(d == MEDIUM) ST7735_OutString(" MEDIUM ");
    else                 ST7735_OutString("  HARD  ");
  } else {
    if(d == EASY)        ST7735_OutString("  FACIL  ");
    else if(d == MEDIUM) ST7735_OutString("  MEDIO  ");
    else                 ST7735_OutString(" DIFICIL ");
  }

  // Start prompt
  ST7735_SetCursor(4, 14);
  if(Language == ENGLISH) ST7735_OutString("PB1 to start");  // 12 chars -> col 4
  else                    ST7735_OutString("PB1 pa jugar");  // 12 chars -> col 4
}

void DrawWinScreen(void){
  static uint8_t bgFrame = 0;
  bgFrame ^= 1;
  ST7735_DrawBitmap(0, 160, bgFrame ? track_2 : track_1, 128, 160);
  ST7735_SetCursor(6, 7);
  if(Language == ENGLISH) ST7735_OutString(" YOU WIN! ");
  else                    ST7735_OutString(" GANASTE! ");
  ST7735_SetCursor(3, 10);
  if(Language == ENGLISH) ST7735_OutString(" PB1 to replay ");
  else                    ST7735_OutString("  PB1 rejugar  ");
}

void DrawLoseScreen(void){
  static uint8_t bgFrame = 0;
  bgFrame ^= 1;
  ST7735_DrawBitmap(0, 160, bgFrame ? track_2 : track_1, 128, 160);
  ST7735_SetCursor(6, 7);
  if(Language == ENGLISH) ST7735_OutString(" YOU LOSE ");
  else                    ST7735_OutString(" PERDISTE ");
  ST7735_SetCursor(3, 10);
  if(Language == ENGLISH) ST7735_OutString(" PB1 to replay ");
  else                    ST7735_OutString("  PB1 rejugar  ");
}

// UART code
void UART_SendPos(int32_t x, int32_t y){
  UART1_OutChar((char)PKT_POS);
  UART1_OutChar((char)(x & 0xFF));
  UART1_OutChar((char)(y & 0xFF));
}

void UART_SendLose(void){
  UART1_OutChar((char)PKT_LOSE);
  UART1_OutChar(0x00);
  UART1_OutChar(0x00);
}

// Call from main loop (not ISR) — non-blocking
void UART_Receive(void){
  if(!UART1_Available()) return;
  char sync = UART1_InChar();

  if((uint8_t)sync == PKT_LOSE){
    if(UART1_Available()) UART1_InChar();  // drain x
    if(UART1_Available()) UART1_InChar();  // drain y
    State = WIN;
    return;
  }

  if((uint8_t)sync != PKT_POS) return;  // unknown byte, discard

  if(!UART1_Available()) return;
  char rx = UART1_InChar();
  if(!UART1_Available()) return;
  char ry = UART1_InChar();

  othercar->x = (int32_t)(uint8_t)rx;
  othercar->y = (int32_t)(uint8_t)ry;
}

// Collision
// y is the bottom row of sprite
int32_t collides(sprite_t *a, sprite_t *b){
  return (a->x < b->x + b->w) && (a->x + a->w  > b->x) && (a->y - a->h + 1 <= b->y)&&(a->y >= b->y - b->h + 1);
}

// Game logic 
void UpdateMyPlayer(void){
  if(PB1Pressed){
    PB1Pressed = 0;
    mycar->x -= 5;
    if(mycar->x < 20){
      mycar->x = 20;
    }
      
  }
  if(PB4Pressed){
    PB4Pressed = 0;
    mycar->x += 5;
    if(mycar->x > 90){
      mycar->x = 90;
    }
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
    }
  }

  // Periodically spawn a new bomb
  spawnTimer++;
  if(spawnTimer >= spawnInterval){
    spawnTimer = 0;
    if(!bombActive) SpawnBomb();
  }

  // Squirrel collision
  if(collides(mycar, &sqr)){
    mycar->y += 5;
    if(mycar->y > 160){ // pushed off the screen
      SendLoseFlag = 1;
      State = LOSE;
      return;
    }
  }

  // Bomb collision
  if(bombActive && collides(mycar, &bombs)){
    SendLoseFlag = 1;
    State = LOSE;
  }
}

// Interrupt handlers
void TIMG12_IRQHandler(void){
  if((TIMG12->CPU_INT.IIDX) == 1){
    GPIOB->DOUTTGL31_0 = GREEN;

    if(State == PLAY){
      UpdateMyPlayer();
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
  if(status & (1<<1)) PB1Pressed = 1;
  if(status & (1<<4)) PB4Pressed = 1;
  GPIOB->CPU_INT.ICLR = (1<<1)|(1<<4);
}

// Main code
int main(void){
  //initializing
  __disable_irq();
  PLL_Init();
  LaunchPad_Init();
  EdgeTriggered_Init();
  IOMUX->SECCFG.PINCM[43] = 0x00000000;  // PB18 analog mode for slide pot (no digital function)
  ADCinit();
  UART1_Init();
  ST7735_InitPrintf(INITR_REDTAB);
  PB1Pressed = 0;
  PB4Pressed = 0;

  // Assign cars based on board identity
#ifdef PLAYER1
  mycar    = &utc;
  othercar = &amc;
#else
  mycar    = &amc;
  othercar = &utc;
#endif

  __enable_irq();

  while(1){  //main game loop

    //Reset state for new game
    State = START;
    PB1Pressed = 0;
    PB4Pressed = 0;
    SendLoseFlag = 0;
    gameReady = 0;
    animFrame = 0;

    // start screen
    while(State == START){
      Difficulty_t d = ReadDifficulty();
      if(PB4Pressed){ PB4Pressed = 0; Language = (Language == ENGLISH) ? SPANISH : ENGLISH; }
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
        if(SendLoseFlag){ SendLoseFlag = 0; UART_SendLose(); }
        UART_SendPos(mycar->x, mycar->y);
        UART_Receive();
        ST7735_DrawBitmap(0, 160, animFrame ? track_2 : track_1, 128, 160);
        draw(&sqr);
        draw(&utc);
        draw(&amc);
        if(bombActive) draw(&bombs);
      }
    }

    // end screen
    __disable_irq();
    if(SendLoseFlag){ 
      SendLoseFlag = 0; 
      UART_SendLose(); 
    }
    
    PB1Pressed = 0;
    
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
    

    
      // wait for button to restart
    
  }
}
