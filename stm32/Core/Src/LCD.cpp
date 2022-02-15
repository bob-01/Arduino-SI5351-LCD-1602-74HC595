#include "LCD.h"

void LCD::init()
{
  HAL_Delay(50);
  LCD::command(0x30);
  HAL_Delay(5);
  LCD::command(0x30);
  HAL_Delay(100);
  LCD::command(0x30);
  HAL_Delay(5);
  LCD::command(0x3C);
  LCD::command(0x0C);
  LCD::command(0x01);
  LCD::command(0x02);
};

void LCD::command (uint8_t value) {
  send(value, 0x00);
  HAL_Delay(1);
}

// mode: 0 - command, 1 - data
void LCD::send (uint8_t value, uint8_t mode)
{
  uint8_t i;
  uint8_t MSB = 1; //MSB

  for(i = 0; i < 8; i++) {
    if (MSB == 0) 
        LCD_DATA_PIN_SET((GPIO_PinState) (value & (0x01 << i))); // LSB
    else 
        LCD_DATA_PIN_SET((GPIO_PinState) (value & (0x80 >> i)));  //MSB
    STRUB_CLK_PIN;
  }

  LCD_DATA_PIN_SET((GPIO_PinState) mode);
  STRUB_EN_PIN;
}

void LCD::setCursor(uint8_t col, uint8_t row) {
  switch (row) {
    case 0:
      command(col | 0x80);
      break;
    case 1:
      command((0x40 + col) | 0x80);
      break;
    case 2:
      command((0x14 + col) | 0x80);
      break;
    case 3:
      command((0x54 + col) | 0x80);
      break;
  }
}

void LCD::write(uint8_t value) {
  send(value, 0x01);
  HAL_Delay(1);
}

void LCD::print(const char *str) {
  uint8_t i=0;
  while(str[i] != 0)
  {
    write(str[i]);
    i++;
  }
}

void LCD::print (const char str) {
  write(str);
}

void LCD::print (int8_t dec) {
  write(dec + '0');
}

void LCD::blanks () {
  print("        ");
}

void LCD::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    write(charmap[i]);
  }
}

void LCD::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LCD::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
