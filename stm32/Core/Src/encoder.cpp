#include "encoder.h"

extern volatile bool change;

int8_t encoder_val = 0;
bool resetFlag;
uint8_t Enc_state, Enc_last;
int8_t ENC_SPINE = -1;

// алгоритм со сбросом от Ярослава Куруса
void encTick () {
  Enc_state = (GPIOA->IDR & 0xC0) >> 6;

  if (resetFlag && Enc_state == 0b11) {
    if (Enc_last == 0b10) encoder_val += ENC_SPINE;
    if (Enc_last == 0b01) encoder_val -= ENC_SPINE;
    resetFlag = 0;
    change = true;
  }
  if (Enc_state == 0b00) resetFlag = 1;
  Enc_last = Enc_state;
}
