#ifndef PCBWRITER_DMA_SPI_H
#define PCBWRITER_DMA_SPI_H

#include <inttypes.h>

void dma_setup(void);
void start_dma(void);
void spi_setup(void);

void laser_low_on(void);
void laser_low_off(void);

void dma_loop(void);

uint8_t* get_write_buffer(void);
void write_done(void);

extern unsigned int dma_write_idx;

extern volatile unsigned int max_n_scans;
extern volatile int autostep;

#define K_SCANLINE_LEN 11900
#define K_LEFT_OVERSCAN 1500
#define K_IMAGE_WIDTH 6000

#endif
