#include <ti/devices/msp/msp.h>
#include "../inc/EdgeTriggered.h"
#include "../inc/LaunchPad.h"

void EdgeTriggered_Init(void){
  LaunchPad_Init();

  // PB1, PB4 = Player 1 (UT car): left, right
  IOMUX->SECCFG.PINCM[PB1INDEX]  = 0x00050081;
  IOMUX->SECCFG.PINCM[PB4INDEX]  = 0x00050081;

  // PB12, PB17 = Player 2 (A&M car): left, right
  IOMUX->SECCFG.PINCM[PB12INDEX] = 0x00050081;
  IOMUX->SECCFG.PINCM[PB17INDEX] = 0x00050081;

  // All four pins as inputs
  GPIOB->DOE31_0 &= ~((1<<1)|(1<<4)|(1<<12)|(1<<17));

  // Rising edge for PB1, PB4, PB12 
  GPIOB->POLARITY15_0 |= ((1<<9)|(1<<8)|(1<<3)|(1<<2)|(1<<25)|(1<<24));

  // Rising edge for PB17
  GPIOB->POLARITY31_16 |= ((1<<3)|(1<<2));

  // Clear any pending flags, then arm all four pins
  GPIOB->CPU_INT.ICLR  = (1<<1)|(1<<4)|(1<<12)|(1<<17);
  GPIOB->CPU_INT.IMASK = (1<<1)|(1<<4)|(1<<12)|(1<<17);

  // Set priority and enable GROUP1 in NVIC
  NVIC->IP[0] = (NVIC->IP[0]&(~0x0000FF00))|2<<14;
  NVIC->ISER[0] = 1<<1;
}
