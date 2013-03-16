#ifndef PCBWRITER_DMA_SPI_H
#define PCBWRITER_DMA_SPI_H

#include <inttypes.h>

void dma_setup(void);
void start_dma(void);
void spi_setup(void);

void dma_loop(void);

extern uint8_t dma_data[10000] __attribute__((aligned(4)));

#endif
