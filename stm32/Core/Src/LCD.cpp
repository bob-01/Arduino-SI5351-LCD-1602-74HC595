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

void LCD::command (uint8_t value) {
  send(value, 0x00);
  HAL_Delay(1);
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
  uint8_t i = 0;
  while(str[i] != 0)
  {
    write(str[i]);
    i++;
  }
}

void LCD::print (char c) {
  write(c);
}

void LCD::print(int n) {
  print((long) n, DEC);
}

void LCD::print(unsigned char b, int base = DEC) {
  print((unsigned long) b);
}

void LCD::print(long n, int base = DEC) {
  if (base == 0) {
    write(n);
  } else {
    if (base == 10) {
      if (n < 0) {
        print('-');
        n = -n;
        printNumber(n, 10);
      } else printNumber(n, 10);
    } else {
        printNumber(n, base);
    }
  }
}

void LCD::print(unsigned long n) {
  printNumber(n, DEC);
}

void LCD::printNumber(unsigned long n, int base)
{
  char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) base = 10;

  do {
    char c = n % base;
    n /= base;

    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while(n);

  print(str);
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
