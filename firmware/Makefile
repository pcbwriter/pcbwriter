##
## Derived from examples in the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##
## Modified by Norbert Braun for the PCBWriter project.

BINARY = pcbwriter
OBJS += dac.o usart.o timer.o dma_spi.o motorctrl.o usb.o statusled.o flash.o stepper.o acc_profile.o

LDSCRIPT = stm32f4-discovery.ld

include Makefile.include

acc_profile.c: acc_profile.py
	./acc_profile.py > acc_profile.c
