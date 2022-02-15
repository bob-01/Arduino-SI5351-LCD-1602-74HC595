//GPIOB->CRH &=  ~ ( 0b1111 << 16 ); // сбрасываем биты 16-19
//GPIOB->CRH |= 0b1000 << 16;  // устанавливаем биты
#include <stdint.h>

int8_t encoder_val = 0;

void encTick();