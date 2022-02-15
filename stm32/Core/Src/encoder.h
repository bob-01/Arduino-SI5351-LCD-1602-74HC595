#ifndef ENCODE_H
#define ENCODE_H

#include <stdio.h>
#include "stm32f030x6.h"

//GPIOB->CRH &=  ~ ( 0b1111 << 16 ); // сбрасываем биты 16-19
//GPIOB->CRH |= 0b1000 << 16;  // устанавливаем биты

void encTick(void);

#endif