#ifndef MAIN_H
#define MAIN_H

//#define SWAP_ROTARY             1 // Swap rotary direction
//#define BUTTON_TONE_ENABLE        // A2 Button tone

#define ROT_A     2       //PD2
#define ROT_B     3       //PD3

#ifdef SWAP_ROTARY
#undef ROT_A
#undef ROT_B
#define ROT_A   3
#define ROT_B   2
#endif

#define BUTTON_STEP    7 
#define BUTTON_VCXO    8
#define BUTTON_TX      9 
#define BUTTON_RIT     10
#define TX_OUT         11
#define tone_pin       12
#define IF_WIDTH_PIN   13

#define Smeter         A0
#define BUTTON_TONE    A1

#include "si5351.h"
#include "Wire.h"
#include "ShiftedLCD.h"
#include <EEPROM.h> // Для работы со встроенной памятью ATmega

Si5351 si5351;

unsigned long currentTime, loopTime;
unsigned long IF, IF2, Fmin, Fmax, Ftx = 3100000, Frit = Ftx, STEP;
unsigned long SI5351_FXTAL;

boolean resetFlag, enc_block=false, enc_flag=false, rit_flag=false, Button_flag=false, tx_flag=false, vcxo_flag=false, step_flag=false, setup_flag=false;
uint8_t IF_WIDTH_FLAG = false;

uint8_t count_avr=0, smeter_count=0, Enc_state, Enc_last, step_count=3, menu_count=0, setup_count=8, xF=1, SI5351_DRIVE_CLK0, SI5351_DRIVE_CLK1, SI5351_DRIVE_CLK2;
uint8_t ftone_flag=false;
int8_t enc_move=0, mode=1;
int8_t ENC_SPIN = 1;

uint16_t Ftone, uSMETER;

boolean BUTTON_TONE_FLAG = false;

// button-left, button-right and button-encoder; single-click, double-click, push-long, push-and-turn
enum event_t { BL=0x10, BR=0x20, BE=0x30, BT= 0x40, SC=0x01, DC=0x02, PL=0x04, PLC=0x05, PT=0x0C };
volatile uint8_t event;

bool curs = false;

#endif
