// FIFO1.c
// Runs on any microcontroller
// Provide functions that implement the Software FiFo Buffer
// Last Modified: July 19, 2025
// Student names: change this to your names or look very silly
// Last modification date: change this to the last modification date or look very silly
#include <stdint.h>

#define FIFO_SIZE 500

// Declare state variables for FiFo
//        size, buffer, put and get indexes
uint32_t putI; // where to put next
uint32_t getI; // where to get next
char static FIFOarr[FIFO_SIZE];

// *********** Fifo1_Init**********
// Initializes a software FIFO1 of a
// fixed size and sets up indexes for
// put and get operations
void Fifo1_Init(){
//Complete this
 putI = 0;
 getI = 0;
 //size = 3;
}

// *********** Fifo1_Put**********
// Adds an element to the FIFO1
// Input: data is character to be inserted
// Output: 1 for success, data properly saved
//         0 for failure, FIFO1 is full
uint32_t Fifo1_Put(char data){
  //Complete this routine
  uint32_t newPutI = (putI+1) % FIFO_SIZE;
  if (newPutI == getI) return 0;
  FIFOarr[putI] = data;
  putI = newPutI;
  return 1; // replace this line with your solution
}

// *********** Fifo1_Get**********
// Gets an element from the FIFO1
// Input: none
// Output: If the FIFO1 is empty return 0
//         If the FIFO1 has data, remove it, and return it
char Fifo1_Get(void){
  //Complete this routine
  char ret;
  if (putI == getI) return 0;
  ret = FIFOarr[getI];
  getI = (getI +1)%FIFO_SIZE;
   return ret; // replace this line with your solution
}



