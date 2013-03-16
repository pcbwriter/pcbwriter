#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

const uint16_t PCBWRITER_VENDOR = 0x1337;
const uint16_t PCBWRITER_PRODUCT = 0xABCD;

const unsigned int PCBWRITER_TIMEOUT = 100; // ms

struct debug_data_t {
    uint32_t seq_num;
    uint16_t speed;
    uint16_t des_speed;
    uint16_t control;
    uint32_t delta;
} __attribute__((packed));

volatile int got_sigint = 0;

void sigint_handler(int sig)
{
    got_sigint = 1;
}

int main(int argc, char** argv)
{
    /* Install SIGINT handler */
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigint_handler;
    
    sigaction(SIGINT, &sa, NULL);
    
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
    
    /* int transferred;
    unsigned char data[] = "USB TEST";
    err = libusb_bulk_transfer(handle, 0x01, data, sizeof(data), &transferred, PCBWRITER_TIMEOUT);
    if(err || transferred != sizeof(data)) {
        fprintf(stderr, "Failed to communicate with endpoint.\n");
    } */
    
    int ss_fd = open("softscope.fifo", O_WRONLY);
    if(ss_fd < 0) {
        fprintf(stderr, "Failed to open softscope.fifo.\n");
        return 1;
    }
    
    uint8_t enable = 1;
    if(libusb_control_transfer(handle, 0x40, 0x81, 0, 0, (unsigned char*) &enable, sizeof(uint8_t), PCBWRITER_TIMEOUT) < 0) {
        fprintf(stderr, "Control request failed.\n");
    }
    
    int exp_seq_num = 0;
    
    while(1) {
        int transferred;
        struct debug_data_t debug_data;
        err = libusb_bulk_transfer(handle, 0x82, (unsigned char*)&debug_data, sizeof(debug_data), &transferred, PCBWRITER_TIMEOUT);
        if(err) {
            fprintf(stderr, "Failed to communicate with endpoint.\n");
            break;
        } else if(transferred != sizeof(debug_data)) {
            fprintf(stderr, "Endpoint returned wrong amount of data.\n");
            break;
        } else {
            if(debug_data.seq_num != exp_seq_num) {
                printf("Missed packet (seq=%d, expected=%d)\n", debug_data.seq_num, exp_seq_num);
            }
            exp_seq_num = debug_data.seq_num+1;
            
            float ss_data[4];
            ss_data[0] = debug_data.speed;
            ss_data[1] = debug_data.des_speed;
            ss_data[2] = debug_data.control;
            ss_data[3] = debug_data.delta;
            
            if(write(ss_fd, ss_data, sizeof(ss_data)) != sizeof(ss_data)) {
                fprintf(stderr, "Write error.\n");
                break;
            }
        }
        
        if(got_sigint) {
            printf("SIGINT caught, exiting.\n");
            break;
        }
    }
    
    enable = 0;
    if(libusb_control_transfer(handle, 0x40, 0x81, 0, 0, (unsigned char*) &enable, sizeof(uint8_t), PCBWRITER_TIMEOUT) < 0) {
        fprintf(stderr, "Control request failed.\n");
    }
    
    close(ss_fd);
    
    libusb_close(handle);
    
    libusb_free_device_list(list, 1);
    libusb_exit(ctx);
    /* END USB STUFF */
    
    return 0;
}

