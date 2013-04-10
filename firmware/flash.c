#include "flash.h"
#include <libopencm3/stm32/f4/flash.h>

#define KPCB_FLASH_BASE_ADDR 0x080FFFFF - 0x00002000

void pcb_flash_setup(void)
{
    // set waitstates
    flash_set_ws(FLASH_ACR_LATENCY_5WS);
    flash_unlock();
}

void pcb_flash_store(uint16_t key, uint8_t byte)
{
    if (key > 0xFFF) {
        return;
    }
    
    flash_program_byte(KPCB_FLASH_BASE_ADDR + key, byte, FLASH_CR_PROGRAM_X8);
}

uint8_t pcb_flash_restore(uint16_t key)
{
    if (key > 0xFFF) {
        return 0x0;
    }
    
    return MMIO8(KPCB_FLASH_BASE_ADDR + key);
}
