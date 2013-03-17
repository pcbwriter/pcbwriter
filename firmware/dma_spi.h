#ifndef PCBWRITER_DMA_SPI_H
#define PCBWRITER_DMA_SPI_H

#include <inttypes.h>

void dma_setup(void);
void start_dma(void);
void spi_setup(void);

void laser_on(void);
void laser_off(void);

void dma_loop(void);

#define K_SCANLINE_LEN 11900
#define K_LEFT_OVERSCAN 1500
#define K_IMAGE_WIDTH 6000

extern uint8_t dma_data[K_SCANLINE_LEN];

#endif
