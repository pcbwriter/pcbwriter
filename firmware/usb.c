/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "usb.h"
#include "ctrlreq.h"
#include "usart.h"
#include "motorctrl.h"
#include "dma_spi.h"
#include "flash.h"
#include "stepper.h"
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/scb.h>

#define IS_DEVICE_TO_HOST(x) (x & 0x80)
#define IS_HOST_TO_DEVICE(x) ((x & 0x80) == 0)

#define PCBUSBLOG(...) printf(__VA_ARGS__)

#define DATA_LEN 64

static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_VENDOR,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x1337,
    .idProduct = 0xABCD,
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor data_endp[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x01,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x82,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
}};

static const struct usb_interface_descriptor data_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_VENDOR,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,

    .endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
    .num_altsetting = 1,
    .altsetting = data_iface,
}};

static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0x80,
    .bMaxPower = 0x32,

    .interface = ifaces,
};

static const char *usb_strings[] = {
    "Dingfabrik",
    "PCBWriter",
};

u8 data[DATA_LEN];

static int control_request(usbd_device *usbd_dev, struct usb_setup_data *req, u8 **buf,
        u16 *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
    (void)complete;
    (void)buf;
    (void)usbd_dev;
    (void)len;

    PCBUSBLOG("Got request type: bmRequestType: 0x%x bRequest: 0x%x len: 0x%x\n", req->bmRequestType, req->bRequest, *len);
    
    if((req->bmRequestType & 0x60) == 0x40) { // Vendor request
        if(req->bRequest == REQ_SET_SPEED) {
            if(!IS_HOST_TO_DEVICE(req->bmRequestType) || *len != 2)
                return USBD_REQ_NOTSUPP;
            
            int des_speed = (*buf)[0] + ((*buf)[1] << 8);
            PCBUSBLOG("Set speed: %d\n", des_speed);
            set_speed(des_speed);
            
            return USBD_REQ_HANDLED;
        } else if(req->bRequest == REQ_ENABLE_DEBUG_OUT) {
            if(!IS_HOST_TO_DEVICE(req->bmRequestType) || *len != 1)
                return USBD_REQ_NOTSUPP;
            
            if((*buf)[0])
                enable_debug_out(1);
            else
                enable_debug_out(0);
            
            PCBUSBLOG("Set debug: %d\n", (*buf)[0]);

            return USBD_REQ_HANDLED;
        } else if(req->bRequest == REQ_SET_PERSISTENT_FLASH) {
            if (!IS_HOST_TO_DEVICE(req->bmRequestType) || *len != 0) {
                return USBD_REQ_NOTSUPP;
            }

            if (req->wIndex > 4096) {
                return USBD_REQ_NOTSUPP;
            }

            PCBUSBLOG("Store to flash: 0x%x at index: 0x%x\n", req->wValue, req->wIndex);
            pcb_flash_store(req->wIndex, req->wValue);

            return USBD_REQ_HANDLED;
        } else if(req->bRequest == REQ_GET_PERSISTENT_FLASH) {
            if (!IS_DEVICE_TO_HOST(req->bmRequestType) || *len <= 0) {
                return USBD_REQ_NOTSUPP;
            }

            int index = req->wIndex;
            int length = req->wValue;

            PCBUSBLOG("Get from flash: index: 0x%x len: 0x%x\n", index, length);

            if (length > DATA_LEN) {
                return USBD_REQ_NOTSUPP;
            }

            for (int i = 0; i < length; i++) {
                data[i] = pcb_flash_restore(index + i);
            }

            *buf = data;
            *len = length;

            return USBD_REQ_HANDLED;
        } else if(req->bRequest == REQ_GET_STEPPER_STATUS) {
            if(!IS_DEVICE_TO_HOST(req->bmRequestType)) {
                return USBD_REQ_NOTSUPP;
            }
            
            /* Sanity check */
            if(DATA_LEN < 4)
                return USBD_REQ_NOTSUPP;
            
            data[0] = 0;
            if(!stepper_idle()) data[0] |= 0x01;
            if(stepper_is_homed) data[0] |= 0x02;
            
            data[1] = 0;
            data[2] = (stepper_current_pos & 0x00FF);
            data[3] = (stepper_current_pos & 0xFF00) >> 8;
            
            *buf = data;
            *len = 4;
            
            return USBD_REQ_HANDLED;
        } else if(req->bRequest == REQ_HOME_STEPPER) {
            stepper_home();
            
            return USBD_REQ_HANDLED;
        } else if(req->bRequest == REQ_MOVE_STEPPER) {
            int16_t delta_pos = (int16_t) req->wValue;
            
            if(req->wIndex)
                stepper_move(delta_pos);
            else
                stepper_move_to(delta_pos);
            
            return USBD_REQ_HANDLED;
        } else if(req->bRequest == REQ_STEPPER_OFF) {
            stepper_off();
            
            return USBD_REQ_HANDLED;
        }
    }
    
    PCBUSBLOG("Got unkown request: 0x%x\n", req->bmRequestType);

    return USBD_REQ_NEXT_CALLBACK;
    
    /* if(req->bRequest == 0x80) {
        if(req->bmRequestType & 0x80) {
            * printf("Got read control request! (len=%d)\n", *len);
            data[0] = 42;
        
            *buf = data;
            *len = (*len < 1 ? *len : 1); *
            return USBD_REQ_NOTSUPP;
        } else {
            if(*len != 2)
                return USBD_REQ_NOTSUPP;
            
            int des_speed = (*buf)[0] + ((*buf)[1] << 8);
            // printf("Set speed: %d\n", des_speed);
            set_speed(des_speed);
        }
        return USBD_REQ_HANDLED;
    } */
}

unsigned int dma_data_idx = 0;

static void data_rx_cb(usbd_device *usbd_dev, u8 ep)
{
    (void)ep;
    
    int len = usbd_ep_read_packet(usbd_dev, 0x01, &dma_data[dma_data_idx + K_LEFT_OVERSCAN], 64);
    dma_data_idx += len;
    dma_data_idx = (dma_data_idx % K_IMAGE_WIDTH);
    
    // gpio_toggle(GPIOC, GPIO5);
}

struct debug_data_t debug_buf[16];
unsigned int buf_start = 0;
unsigned int buf_end = 0;

static void data_tx_cb(usbd_device *usbd_dev, u8 ep)
{
    (void) usbd_dev;
    (void) ep;
    
    // If the buffer is not empty, transmit the next packet.
    if(buf_start != buf_end) {
        usbd_ep_write_packet(usbd_dev, 0x82, (char*)&debug_buf[buf_start], sizeof(struct debug_data_t));
        buf_start = (buf_start+1)%16;
    }
}

static void set_config(usbd_device *usbd_dev, u16 wValue)
{
    (void)wValue;

    usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, data_rx_cb);
    usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, data_tx_cb);

    usbd_register_control_callback(
                usbd_dev,
                USB_REQ_TYPE_VENDOR,
                USB_REQ_TYPE_TYPE,
                control_request);
}

usbd_device *usbd_dev;

void usb_setup(void)
{
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_AHB2ENR, RCC_AHB2ENR_OTGFSEN);
    
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
            GPIO9 | GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF10, GPIO9 | GPIO11 | GPIO12);
    
    usbd_dev = usbd_init(&otgfs_usb_driver, &dev, &config, usb_strings, 3);
    usbd_register_set_config_callback(usbd_dev, set_config);
}

void usb_put_debug_packet(struct debug_data_t* debug_data)
{
    // If buffer is empty, try to write the packet right away.
    if(buf_start == buf_end &&
       usbd_ep_write_packet(usbd_dev, 0x82, (char*)debug_data, sizeof(struct debug_data_t)) != 0) {
            // success, nothing to do
    // If there is remaining space in the buffer, put the packet there.
    } else if((buf_end+1)%16 != buf_start) {
        debug_buf[buf_end] = *debug_data;
        buf_end = (buf_end+1)%16;
    }
    // Otherwise, discard the packet.
}

void usb_poll(void)
{
    usbd_poll(usbd_dev);
}
