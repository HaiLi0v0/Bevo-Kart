#include <ti/devices/msp/msp.h>
#include "../inc/EdgeTriggered.h"
#include "../inc/LaunchPad.h"

void EdgeTriggered_Init(void){
  LaunchPad_Init();

  // Initialize PB1 as GPIO input with pull-down (positive logic)
  IOMUX->SECCFG.PINCM[PB1INDEX] = 0x00050081;

  // Initialize PB4 as GPIO input with pull-down (positive logic)
  IOMUX->SECCFG.PINCM[PB4INDEX] = 0x00050081;

  // Set PB1 and PB4 as inputs (0 = input in DOE register)
  GPIOB->DOE31_0 &= ~((1<<1)|(1<<4));

  GPIOB->POLARITY15_0 &= ~((1<<9)|(1<<8)|(1<<3)|(1<<2));  // clears bits 
  // Rising edge trigger (positive logic = high when pressed)
  GPIOB->POLARITY15_0 |= ((1<<8)|(1<<2));  // 0 = rising edge <<<<

  // Clear any pending flags, then arm both pins
  GPIOB->CPU_INT.ICLR  = (1<<1)|(1<<4);
  GPIOB->CPU_INT.IMASK = (1<<1)|(1<<4);

  // Set priority and enable GROUP1 in NVIC
  NVIC->IP[0] = (NVIC->IP[0]&(~0x0000FF00))|2<<14;
  NVIC->ISER[0] = 1<<1;
}
