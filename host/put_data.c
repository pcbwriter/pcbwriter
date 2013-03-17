#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <libusb-1.0/libusb.h>

const uint16_t PCBWRITER_VENDOR = 0x1337;
const uint16_t PCBWRITER_PRODUCT = 0xABCD;

const int K_IMAGE_WIDTH = 6000;

const unsigned int PCBWRITER_TIMEOUT = 100; // ms

/* double func(double x)
{
    double y;
    y = (1.-t)*exp(-(x-0.4)*(x-0.4)/(0.05));
    y += t*exp(-(x)*(x)/(0.1));
    y += (1.-t)*exp(-(x+0.4)*(x+0.4)/(0.05));
    return y;
} */

double func(double x)
{
    // return (sin(x/0.001)+1.0)/2.0;
    // return exp(-x*x/1e-7);
    return sin(x/0.001) > 0. ? 1. : 0.;
}

void generate_data(uint8_t* data) {
    for(unsigned int i=0; i<K_IMAGE_WIDTH; i++) {
        data[i] = 0;
    }
    
    double error = 0.0;
    for(unsigned int i=0; i<8*K_IMAGE_WIDTH; i++) {
        double x = ((double)i)/(4*K_IMAGE_WIDTH) - 1.0;
        double y = func(x);
        
        error += y;
        if(error > 1.0) {
            data[i>>3] |= (0x80 >> (i & 7));
            error -= 1.0;
        }
    }
}

int main(int argc, char** argv)
{
    /* BEGIN USB STUFF */
    libusb_context* ctx = 0;
    libusb_init(&ctx);
    libusb_set_debug(ctx, 3);
    
    libusb_device** list;
    ssize_t ndev = libusb_get_device_list(ctx, &list);
    
    if(ndev < 0) {
        fprintf(stderr, "Error: no devices found.\n");
        return 1;
    }
    
    libusb_device* pcbw_dev = 0;
    
    ssize_t i;
    for(i=0; i<ndev; i++) {
        libusb_device* dev = list[i];
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(dev, &desc);
        
        if(desc.idVendor == PCBWRITER_VENDOR && desc.idProduct == PCBWRITER_PRODUCT) {
            pcbw_dev = dev;
            break;
        }
    }
    
    if(!pcbw_dev) {
        fprintf(stderr, "Error: failed to find PCBWriter device.\n");
        return 1;
    }
    
    libusb_device_handle* handle;
    if(libusb_open(pcbw_dev, &handle)) {
        fprintf(stderr, "Failed to open PCBWriter device.\n");
        return 1;
    }
    
    int err;
    err = libusb_claim_interface(handle, 0);
    if(err) {
        fprintf(stderr, "Failed to claim interface: ");
        switch(err) {
            case LIBUSB_ERROR_NOT_FOUND: fprintf(stderr, "not found"); break;
            case LIBUSB_ERROR_BUSY:      fprintf(stderr, "busy");      break;
            case LIBUSB_ERROR_NO_DEVICE: fprintf(stderr, "no device"); break;
            default:                     fprintf(stderr, "unspecified error"); break;
        }
        fprintf(stderr, ".\n");
    }
    
    int transferred;
    unsigned char data[K_IMAGE_WIDTH];
    generate_data(data);
    
    err = libusb_bulk_transfer(handle, 0x01, data, sizeof(data), &transferred, PCBWRITER_TIMEOUT);
    if(err || transferred != sizeof(data)) {
        fprintf(stderr, "Failed to communicate with endpoint.\n");
        fprintf(stderr, "err = %d\n", err);
        fprintf(stderr, "transferred = %d\n", transferred);
    }
    
    libusb_close(handle);
    
    libusb_free_device_list(list, 1);
    libusb_exit(ctx);
    /* END USB STUFF */
    
    return 0;
}

