#include "encoder.h"

// алгоритм со сбросом от Ярослава Куруса
void encTick () {
  /*
  Enc_state = digitalRead(ROT_A) | digitalRead(ROT_B) << 1;  // digitalRead хорошо бы заменить чем-нибудь более быстрым
  if (resetFlag && Enc_state == 0b11) {
    if (Enc_last == 0b10) enc_move=1*ENC_SPIN;
    if (Enc_last == 0b01) enc_move=-1*ENC_SPIN;
    resetFlag = 0;
    enc_flag = true;
  }
  if (Enc_state == 0b00) resetFlag = 1;
  Enc_last = Enc_state;
  */
}
