#ifndef __VEEPROM_H
#define __VEEPROM_H

#include "stdint.h"

#define VEEPROM_PAGE_1_ADDR                 (0x08003800)
#define VEEPROM_PAGE_2_ADDR                 (0x08003C00)

class FLASH_EEPROM {
  public:
      void read(void *, void * , uint8_t);
  private:
      static bool lock();
      static bool unlock();
};
#endif