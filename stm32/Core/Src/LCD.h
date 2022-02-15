#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include "stm32f0xx_hal.h"

#define LCD_DISPLAYCONTROL 0x08
#define LCD_SETCGRAMADDR 0x40
#define LCD_CURSORON 0x02

//GPIOB->CRH &=  ~ ( 0b1111 << 16 ); // сбрасываем биты 16-19
//GPIOB->CRH |= 0b1000 << 16;  // устанавливаем биты

#define LCD_DATA_PIN_SET(state) (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, state))
#define LCD_CLK_PIN_SET(state) (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, state))
#define LCD_EN_PIN_SET(state) (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, state))
#define STRUB_EN_PIN LCD_EN_PIN_SET(GPIO_PIN_SET); LCD_EN_PIN_SET(GPIO_PIN_RESET);
#define STRUB_CLK_PIN LCD_CLK_PIN_SET(GPIO_PIN_SET); LCD_CLK_PIN_SET(GPIO_PIN_RESET);

#define PSTR(str) (str)

class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))

class LCD
{
  public:
    void init();
    void createChar(uint8_t location, uint8_t charmap[]);
    void print(const char *str);
    void print(const char str);
    void print (int8_t dec);
    void cursor();
    void noCursor();
    void string();
    void setCursor(uint8_t col, uint8_t row);
    void clear();
    void symbol();
    void blanks();
  private:
    void send (uint8_t value, uint8_t mode);
    void command (uint8_t value);
    void write (uint8_t value);

  uint8_t _displaycontrol;
};

#endif
