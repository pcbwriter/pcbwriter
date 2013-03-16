#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libusb-1.0/libusb.h>

const uint16_t PCBWRITER_VENDOR = 0x1337;
const uint16_t PCBWRITER_PRODUCT = 0xABCD;

const unsigned int PCBWRITER_TIMEOUT = 100; // ms

int main(int argc, char** argv)
{
    if(argc != 2) {
        printf("Usage: %s <value>\n", argv[0]);
        return -1;
    }
    
    uint16_t set_speed = atoi(argv[1]);
    
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
    
    /* bmRequestType is either 0x40 (host to device, vendor request) or
       0xC0 (device to host, vendor request). */
    
    if(libusb_control_transfer(handle, 0x40, 0x80, 0, 0, (unsigned char*) &set_speed, sizeof(uint16_t), PCBWRITER_TIMEOUT) < 0) {
        fprintf(stderr, "Write control request failed.\n");
    }
    
    libusb_close(handle);
    
    libusb_free_device_list(list, 1);
    libusb_exit(ctx);
    /* END USB STUFF */
    
    return 0;
}

