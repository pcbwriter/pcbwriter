import time
import array
import struct
import usb.core
import usb.util

class StepperStatus:
    def __init__(self, s):
        (self.flags, dummy, self.pos) = struct.unpack("BBh", s)

class PCBWriter:
    PCBWRITER_TIMEOUT = 100 # ms
    
    REQ_SET_SPEED = 0x80
    REQ_ENABLE_DEBUG_OUT = 0x81
    
    REQ_SET_PERSISTENT_FLASH = 0x82
    REQ_GET_PERSISTENT_FLASH = 0x83
    
    REQ_GET_STEPPER_STATUS = 0x90
    REQ_HOME_STEPPER = 0x91
    REQ_MOVE_STEPPER = 0x92
    REQ_STEPPER_OFF = 0x93
    
    REQ_CAN_SEND = 0xC0
    
    def __init__(self):
        self.dev = usb.core.find(idVendor = 0x1337, idProduct = 0xabcd)
        if self.dev is None:
            raise RuntimeError("Device not found")
    
    def __del__(self):
        self.stepper_off()
    
    def put_line(self, data, wait=True, fill=False):
        if wait:
            # Wait for line to become ready
            while not self.dev.ctrl_transfer(bmRequestType=0xC0, bRequest=self.REQ_CAN_SEND, wValue=0, wIndex=0, data_or_wLength=4, timeout=1000)[0]:
                pass
        
        if len(data) > 6000:
            raise ValueError
        
        if self.dev.write(1, data, 0, self.PCBWRITER_TIMEOUT) != len(data):
            raise RuntimeError, "Failed to communicate with endpoint."
        
        if fill:
            if self.dev.write(1, array.array("B", [0]*(6000-len(data))).tostring(), 0, self.PCBWRITER_TIMEOUT) != (6000 - len(data)):
                raise RuntimeError, "Failed to communicate with endpoint."
    
    def get_stepper_status(self):
        return StepperStatus(self.dev.ctrl_transfer(bmRequestType=0xC0, bRequest=self.REQ_GET_STEPPER_STATUS, wValue=0, wIndex=0, data_or_wLength=4, timeout=1000))
    
    def get_stepper_busy(self):
        return bool(self.get_stepper_status().flags & 0x01)
    
    def get_stepper_homed(self):
        return bool(self.get_stepper_status().flags & 0x02)
    
    def home_stepper(self, wait=False):
        self.dev.ctrl_transfer(bmRequestType=0xC0, bRequest=self.REQ_HOME_STEPPER, wValue=0, wIndex=0, data_or_wLength=0, timeout=1000)
        
        if wait:
            while self.get_stepper_busy():
                time.sleep(0.1)

    def move_stepper(self, pos, relative, wait=False):
        self.dev.ctrl_transfer(bmRequestType=0xC0, bRequest=self.REQ_MOVE_STEPPER, wValue=pos, wIndex=relative, data_or_wLength=0, timeout=1000)
        
        if wait:
            while self.get_stepper_busy():
                time.sleep(0.1)

    def stepper_off(self):
        self.dev.ctrl_transfer(bmRequestType=0xC0, bRequest=self.REQ_STEPPER_OFF, wValue=0, wIndex=0, data_or_wLength=0, timeout=1000)
