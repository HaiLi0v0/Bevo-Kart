// Lab9Main.c
// Runs on MSPM0G3507
// Lab 9 ECE319K
// Your name
// Last Modified: January 12, 2026

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


typedef enum {
  START,
  PLAY,
  P1_WIN,
  P2_WIN
} GameState_t;

GameState_t State;
// extern volatile uint8_t PB1Pressed;
// extern volatile uint8_t PB4Pressed;
uint32_t PB1Pressed, PB4Pressed;

struct sprite {
  int32_t x;      // x coordinate
  int32_t y;      // y coordinate
  int32_t xold, yold;
  int32_t vx,vy;  // pixels/30Hz
  const unsigned short *image; // ptr->image
  //const unsigned short *black;
  //status_t life;        // dead/alive
  int32_t w; // width
  int32_t h; // height
  uint32_t needDraw; // true if need to draw
};
typedef struct sprite sprite_t;

  sprite_t utc;
  sprite_t amc;
  sprite_t bombs;
  sprite_t sqr;


  
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}
// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz
void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}

uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}

// // games  engine runs at 30Hz
// void TIMG12_IRQHandler(void){uint32_t pos,msg;
//   if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
//     GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
//     GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
// // game engine goes here
//     // 1) sample slide pot
//     // 2) read input switches
//     // 3) move sprites
//     // 4) start sounds
//     // 5) set semaphore
//     // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
//     GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
//   }
// }

// initialize the graphics on the screen
void graphics_init(void){
  //UT car
  utc.x = 30;
  utc.y = 90;
  utc.xold = utc.x;
  utc.yold = utc.y;
  utc.vx = 0;
  utc.vy = 0;
  utc.image = ut_car1;
  utc.w = 15;
  utc.h = 27;
  utc.needDraw = 1;
  
  //A&M car
  amc.x = 80;
  amc.y = 90;
  amc.xold = amc.x;
  amc.yold = amc.y;
  amc.vx = 0;
  amc.vy = 0;
  amc.image = am_car1;
  amc.w = 15;
  amc.h = 27;
  amc.needDraw = 1;
  
  //bomb
  bombs.x = 50;
  bombs.y = 110;
  bombs.xold = bombs.x;
  bombs.yold = bombs.y;
  bombs.vx = 0;
  bombs.vy = 0;
  bombs.image = bomb;
  bombs.w = 10;
  bombs.h = 10;
  bombs.needDraw = 0;

  //squirrel
  sqr.x = 50;
  sqr.y = 50;
  sqr.xold = sqr.x;
  sqr.yold = sqr.y;
  sqr.vx = 0;
  sqr.vy = 0;
  sqr.image = sq_r1;
  sqr.w = 20;
  sqr.h = 10;
  sqr.needDraw = 0;
}

void draw(sprite_t *s){
  ST7735_DrawBitmap(s->x, s->y, s->image, s->w, s->h);
  s->xold = s->x;
  s->yold = s->y;
}

//used to update graphics of the game
void up_graphics(void){
  //check for which button is pressed and move according to the button
  
}

void UpdatePlayers(void){
  if(PB1Pressed){
    PB1Pressed = 0;      // clear FIRST before acting
    utc.x -= 5;
    if(utc.x < 20) utc.x = 20;
  }

  if(PB4Pressed){
    PB4Pressed = 0;      // clear FIRST before acting
    utc.x += 5;
    if(utc.x > 90) utc.x = 90;
  }
}

// in TIMG12_IRQHandler — move game logic here

volatile uint8_t gameReady = 0;
volatile uint8_t animFrame = 0;  // 0 or 1, toggles each tick

void TIMG12_IRQHandler(void){
  if((TIMG12->CPU_INT.IIDX) == 1){
    GPIOB->DOUTTGL31_0 = GREEN;

    UpdatePlayers();

    // toggle animation frame each tick
    animFrame ^= 1;

    // update sprite images based on frame
    utc.image  = animFrame ? ut_car2  : ut_car1;
    amc.image  = animFrame ? am_car2  : am_car1;
    sqr.image  = animFrame ? sq_r1    : sq_r2;

    gameReady = 1;
    GPIOB->DOUTTGL31_0 = GREEN;
  }
}



int main(void){ 
  __disable_irq();
  PLL_Init();
  LaunchPad_Init();
  EdgeTriggered_Init();
  PB1Pressed = 0;
  PB4Pressed = 0;
  ST7735_InitPrintf(INITR_REDTAB);
  ST7735_FillScreen(ST7735_BLACK);
  graphics_init();
  TimerG12_IntArm(2666667, 2);  // start 30Hz game tick ← add this
  __enable_irq();   
  
  while(1){
    UpdatePlayers();
    if(gameReady){
      gameReady = 0;                              // reset flag
      ST7735_DrawBitmap(0, 160,animFrame ? track_2 : track_1, 128, 160);
      draw(&sqr);                                 // then sprites on top
      draw(&utc);
      draw(&amc);
      draw(&bombs);
    }
  }
}

void GROUP1_IRQHandler(void){
  
  uint32_t status = GPIOB->CPU_INT.RIS;
  
  //if(status & (1<<1)){
    PB1Pressed = 1;
  //}
  if(status & (1<<4)){
    PB4Pressed = 1;
  }
  GPIOB->DOUTTGL31_0 = RED; // toggle PB26
  GPIOB->CPU_INT.ICLR = 0x00000012; // clear bit 21
}

// game engine
// bounds of the track 20<= x <= 90
int main0(void){ 
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  EdgeTriggered_Init();
  PB1Pressed = 0;
  PB4Pressed = 0;
  TimerG12_IntArm(2666667, 2);
  ST7735_InitPrintf(INITR_REDTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  ST7735_FillScreen(ST7735_BLACK);
  graphics_init();
  __enable_irq();   
  
  while(1)
  {
  ST7735_DrawBitmap(0, 160, track_1, 128, 160);
  utc.image = ut_car1;
  amc.image = am_car1;
  sqr.image = sq_r2;

  UpdatePlayers();
  if(gameReady){
    gameReady = 0; 
    draw(&sqr);
    draw(&utc);
    draw(&amc);
    draw(&bombs);
  }
  
  Clock_Delay1ms(80);
  
  ST7735_DrawBitmap(0, 160, track_2, 128, 160);
  utc.image = ut_car2;
  amc.image = am_car2;
  sqr.image = sq_r1;
  
  UpdatePlayers();

  if (gameReady) {
    gameReady = 0; 
    draw(&sqr);
    draw(&utc);
    draw(&amc);
    draw(&bombs);
  }
  
  Clock_Delay1ms(80);
  }
  
}