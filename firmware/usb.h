#ifndef PCBWRITER_USB_H
#define PCBWRITER_USB_H

#include "debugdata.h"
#include <inttypes.h>

void usb_setup(void);
void usb_poll(void);
void usb_put_debug_packet(struct debug_data_t* debug_data);

#endif
