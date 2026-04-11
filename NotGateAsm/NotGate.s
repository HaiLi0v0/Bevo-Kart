// ****************** NoteGate.s ***************
// Assembly project implementing a not gate, Program 2.3.5
// Author: Jonathan Valvano
// Last Modified: Jan 27, 2025 (by Ramesh Yerraballi)
// Brief description of the program:
// This is a simple assembly language project that creates a not gate
// SW input on PB0 input, LED output on PB1
// If SW is pressed, the LED is off.
// If SW is not pressed, the LED is on.
// I/O port addresses
    .include "../inc/msp.s"

       .data
       .align 2
// Declare global variables here if needed
// with the .space assembly directive
      .equ   SIZE,32
Buf:  .space SIZE
DBuf: .space SIZE
TBuf: .space SIZE*2
Cnt:  .space 4
 .global LaunchPad_Init

// Instructions go in FLASH ROM
       .text
       .thumb
       .align 2

       .text
       .thumb
       .align 2

       .global main
main:
//1) Initialize input pin and output pin
  BL  Init
  LDR R6,=GPIOB_DIN31_0
  LDR R7,=GPIOB_DOUT31_0
  MOVS R5,#1 // bit 0
  MOVS R4,#2 // bit 1
loop12:
  LDR  R1,[R6]  // all of GPIOB inputs
  ANDS R1,R1,R5 // select just PB0 input
  EORS R1,R1,R5 // flip bit 0, performing the NOT
  LSLS R1,R1,#1 // shift into bit1 position
  LDR  R2,[R7]  // 1) read previous output
  BICS R2,R2,R4 // 2) clear bit 1
  ORRS R1,R1,R2 // 3) combine previous 31-2,0 with new value in bit1
  STR  R1,[R7]  // 4) write back to output
  B    loop12

// make PB0 input, PB1 output
// turn on PortB, PB1 output
Init: // PB0 input, PB1 output
  PUSH {LR}
  BL   LaunchPad_Init
  MOVS R1,#0x81
  LDR  R0,=IOMUXPB1
  STR  R1,[R0]  // PB1 is out
  LDR  R1,=0x00040081
  LDR  R0,=IOMUXPB0
  STR  R1,[R0]  // PB0 is in
  LDR  R0,=GPIOB_DOE31_0
  LDR  R1,[R0]
  MOVS R2,#0x02 // PB1
  ORRS R1,R1,R2
  STR  R1,[R0]  // enable out
  POP  {PC}

    .end          // end of file
