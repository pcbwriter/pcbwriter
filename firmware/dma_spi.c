#include "dma_spi.h"
#include "stepper.h"
#include "statusled.h"

#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/f4/dma.h>
#include <libopencm3/stm32/f4/spi.h>
#include <libopencm3/cm3/nvic.h>

uint8_t dma_data_0[K_SCANLINE_LEN] __attribute__((aligned(4)));
uint8_t dma_data_1[K_SCANLINE_LEN] __attribute__((aligned(4)));
uint8_t dma_data_2[K_SCANLINE_LEN] __attribute__((aligned(4)));

unsigned int dma_write_idx = 0;

/* Buffer that is currently being scanned out. */
unsigned int cur_scan_buf = 0;
/* Buffer that is currently being written. */
unsigned int cur_write_buf = 1;

int buf_ready = 0;

/* Functions that are internal to this module. */
uint8_t* get_scan_buffer(void);
void swap_buffers(void);

/* Return pointer to beginning of buffer that is currently being scanned out. */
uint8_t* get_scan_buffer(void)
{
    if(cur_scan_buf == 0) return dma_data_0;
    else if(cur_scan_buf == 1) return dma_data_1;
    else return dma_data_2;
}

/* Return pointer to beginning of buffer that USB data is to be written to. */
uint8_t* get_write_buffer(void)
{
    if(cur_write_buf == 1) return dma_data_1;
    else return dma_data_2;
}

/* Called by USB code to indicate that all data for the next line has been received. */
void write_done(void)
{
    buf_ready = 1;
}

void dma_setup(void)
{
    /* Setup data */
    unsigned int i;
    /* Debug only (adjust overscan and image width) */
    /* for(i=0; i < K_LEFT_OVERSCAN; i++) {
        dma_data[i] = 0x00;
    }
    for(; i < (K_LEFT_OVERSCAN + K_IMAGE_WIDTH); i++) {
        dma_data[i] = 0xFF;
    } */
    for(i=0; i<K_SCANLINE_LEN; i++) {
        dma_data_0[i] = 0x00;
    }
    
    for(i=0; i<K_SCANLINE_LEN; i++) {
        dma_data_1[i] = 0x00;
    }
    
    for(i=0; i<K_SCANLINE_LEN; i++) {
        dma_data_2[i] = 0x00;
    }
    
    /* Enable peripheral clocks. */
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_DMA1EN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM8EN | RCC_APB2ENR_SYSCFGEN);
    
    /* Enable GPIO clocks. */
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPBEN);
    
    /* SCK (Not needed for our application.) */
    /* gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO13);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO13); */
    
    /* Non-DMA output pins (idle ON, low power on) */
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13 | GPIO14);
    gpio_set(GPIOB, GPIO13);
    
    /* MOSI */
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO15);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO15);
    
    dma_disable_stream(DMA1, DMA_STREAM4);
    
    /* Without this the interrupt routine will never be called. */
    nvic_enable_irq(NVIC_DMA1_STREAM4_IRQ);
    nvic_set_priority(NVIC_DMA1_STREAM4_IRQ, 1);
}

/* void laser_on(void)
{
    // FIXME: make sure DMA is off
    // Note that the SPI hardware keeps the data line on its last level after
    // the last data byte has been sent.
    spi_write(SPI2, 0xFF);
}

void laser_off(void)
{
    // FIXME: make sure DMA is off
    spi_write(SPI2, 0x00);
} */

/* Switch laser ON in low-power mode (to find index diode) */
void laser_low_on(void)
{
    gpio_set(GPIOB, GPIO14);
}

/* Switch off low power mode */
void laser_low_off(void)
{
    gpio_clear(GPIOB, GPIO14);
}

void start_dma(void)
{
    laser_low_off();
    
    dma_channel_select(DMA1, DMA_STREAM4, DMA_SxCR_CHSEL_0);
    dma_set_priority(DMA1, DMA_STREAM4, DMA_SxCR_PL_VERY_HIGH);
    dma_set_peripheral_size(DMA1, DMA_STREAM4, DMA_SxCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_STREAM4, DMA_SxCR_MSIZE_8BIT);
    dma_enable_memory_increment_mode(DMA1, DMA_STREAM4);
    dma_set_transfer_mode(DMA1, DMA_STREAM4, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM4);
    
    dma_set_number_of_data(DMA1, DMA_STREAM4, K_SCANLINE_LEN);
    dma_set_peripheral_address(DMA1, DMA_STREAM4, (uint32_t)&SPI2_DR);
    dma_set_memory_address(DMA1, DMA_STREAM4, (uint32_t)get_scan_buffer());
    
    dma_enable_stream(DMA1, DMA_STREAM4);
    
    // Start SPI DMA
    spi_enable_tx_dma(SPI2);
}

void spi_setup(void)
{
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_SPI2EN);
    
    spi_reset(SPI2);
    spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_2, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI2);
    spi_set_nss_high(SPI2);
    
    spi_enable(SPI2);
}

volatile int dma_done = 0;

unsigned int n_scans;
volatile unsigned int max_n_scans = 1;
volatile int autostep = 0;

void swap_buffers(void)
{
    cur_scan_buf = cur_write_buf;
    cur_write_buf = (cur_scan_buf == 1 ? 2 : 1);
    dma_write_idx = 0;
    buf_ready = 0;
}

void dma1_stream4_isr(void)
{
    dma_clear_interrupt_flags(DMA1, DMA_STREAM4, DMA_ISR_TCIF);
    dma_disable_stream(DMA1, DMA_STREAM4);
    
    dma_done = 1;
    
    /* Make sure laser high-power mode is OFF */
    spi_write(SPI2, 0x00);
    
    laser_low_on();
    
    /* Prepare next line to be scanned out. */
    if(cur_scan_buf == 0 || n_scans == max_n_scans) {
        if(buf_ready) {
            swap_buffers();
            n_scans = 0;
            if(autostep)
                stepper_step(0);
        } else {
            cur_scan_buf = 0;
        }
    }
    
    set_status(LED_BLUE, cur_scan_buf == 0);
    
    n_scans++;
}
