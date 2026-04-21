// Sound.c
// Runs on MSPM0
// Sound assets in sounds/sounds.h
// your name
// your data
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "Sound.h"
#include "sounds/sounds.h"
#include "../inc/DAC.h"
#include "../inc/Timer.h"

static const uint8_t *SoundPt    = 0;
static uint32_t       SoundCount = 0;

void SysTick_IntArm(uint32_t period, uint32_t priority){
  SysTick->CTRL = 0;
  SysTick->LOAD = period - 1;
  SysTick->VAL  = 0;
  // SysTick priority byte on Cortex-M0+ is at 0xE000ED23 (bits 7:6)
  *((volatile uint8_t *)0xE000ED23) = priority << 6;
  SysTick->CTRL = 0x07;  // CLK_SRC=processor, TICKINT=1, ENABLE=1
}

void Sound_Init(void){
  SoundPt    = 0;
  SoundCount = 0;
  DAC_Init();
  SysTick_IntArm(7273, 2);  // 80MHz / 11000 = 7273 -> 11kHz
}

void SysTick_Handler(void){
  if(SoundCount > 0){
    DAC_Out((uint32_t)(*SoundPt) << 4);  // scale 8-bit to 12-bit
    SoundPt++;
    SoundCount--;
  } else {
    DAC_Out(2048);  // silence at midpoint
  }
}

void Sound_Start(const uint8_t *pt, uint32_t count){
  SoundPt    = pt;
  SoundCount = count;
}

void Sound_Shoot(void){
  Sound_Start(shoot, 4080);
}

void Sound_Killed(void){
  Sound_Start(invaderkilled, 3377);
}

void Sound_Explosion(void){
  Sound_Start(explosion, 2000);
}

void Sound_Fastinvader1(void){
  Sound_Start(fastinvader1, 982);
}

void Sound_Fastinvader2(void){
  Sound_Start(fastinvader2, 1042);
}

void Sound_Fastinvader3(void){
  Sound_Start(fastinvader3, 1054);
}

void Sound_Fastinvader4(void){
  Sound_Start(fastinvader4, 1098);
}

void Sound_Highpitch(void){
  Sound_Start(highpitch, 1802);
}

void Sound_SquirrelDeath(void){
  Sound_Start(invaderkilled, 3377);
}

void Sound_Bomb(void){
  Sound_Start(explosion, 2000);
}
