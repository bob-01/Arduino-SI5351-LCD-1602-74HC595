#include "veeprom.h"

void FLASH_EEPROM::read (void * dest_addr, void * eeprom_addr, uint8_t size) {
  //if ((uint32_t)eeprom_addr > (uint32_t)(VEEPROM_PAGE_2_ADDR + 1023)) return;

  switch (size) {
      case 1:
        *((uint8_t *) dest_addr) = *((volatile uint8_t *) eeprom_addr);
        break;
      case 2:
        *((uint16_t *) dest_addr) = *((volatile uint16_t *) eeprom_addr);
        break;
      case 4:
        *((uint32_t *) dest_addr) = *((volatile uint32_t*) eeprom_addr);
        break;
      default:
        *((uint32_t *) dest_addr) = *((volatile uint32_t*) eeprom_addr);
        break;
  }
}
