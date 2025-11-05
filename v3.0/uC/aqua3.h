#include <16F877A.h>
#device ADC=10

#FUSES WDT                    // Watch Dog Timer
#FUSES NOBROWNOUT               //No brownout reset
#FUSES NOLVP                    //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
//#FUSES WRT_1000                 //Program Memory Write Protected from 0 to 0x0FFF

#use delay(crystal=20000000)

#use i2c(master,sda=PIN_B0,scl=PIN_B1)

