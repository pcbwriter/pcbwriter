#ifndef PCBWRITER_FLASH_H
#define PCBWRITER_FLASH_H

#include <inttypes.h>

void pcb_flash_setup(void);
void pcb_flash_store(uint16_t key, uint8_t byte);
uint8_t pcb_flash_restore(uint16_t key);

#endif
