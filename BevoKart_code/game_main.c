// Lab9Main.c
// Runs on MSPM0G3507
// Lab 9 ECE319K
// Your name
// Last Modified: January 12, 2026

#include <stdio.h>
#include <stdint.h>
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
//#include "images.h"

typedef enum{
  START,
  PLAY,
  P1_WIN,
  P2_WIN
} GameState_t;

GameState_t State;

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

// games  engine runs at 30Hz
void TIMG12_IRQHandler(void){uint32_t pos,msg;
  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
// game engine goes here
    // 1) sample slide pot
    // 2) read input switches
    // 3) move sprites
    // 4) start sounds
    // 5) set semaphore
    // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}


// initialize the graphics on the screen
void graphics_init(void){
  
}

//used to update graphics of the game
void up_graphics(void){
  //check for which button is pressed and move according to the button
  
}

// game engine
// bounds of the track 20<= x <= 90
int main(void){ 
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  
  ST7735_InitPrintf(INITR_REDTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  ST7735_FillScreen(ST7735_BLACK);
  graphics_init();
  ST7735_DrawBitmap(50, 110, bomb, 10, 10);
  
  while(1)
  {
    ST7735_DrawBitmap(0, 168, track_1, 128, 160);  
    ST7735_DrawBitmap(50, 50, sq_r2, 20, 10); 
    ST7735_DrawBitmap(20, 90, ut_car1, 15,27); 
    ST7735_DrawBitmap(90, 90, am_car1, 15,27); 
    Clock_Delay1ms(60);
    ST7735_DrawBitmap(0, 160, track_2, 128, 160);  
    ST7735_DrawBitmap(50, 50, sq_r1, 20, 10); 
    ST7735_DrawBitmap(20, 90, ut_car2, 15,27); 
    ST7735_DrawBitmap(90, 90, am_car2, 15,27); 
    Clock_Delay1ms(100);
  }
  
}