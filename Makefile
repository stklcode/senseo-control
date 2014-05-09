#	SenseoControl 2.0
#
#	File:		Makefile
#	Author:		Stefan Kalscheuer
#	Date:		22.04.2013
#
# 	Platform:	ATtiny26
# 				Internal RC-oscillator 8 MHz, CKDIV8 Enabled



# Project specific settings
TARGET = SenseoControl-2.0
MCU = attiny26
SRC = main.c

# You probably want to change this to your own programming device

# AVR ISP mkII
#PGMDEV = avrispmkII
#PGMOPT = -P usb # Try   -B 10   in case of programming errors

# Pony-STK200
#PGMDEV = pony-stk200
#PGMOPT = -E noreset

# STK500
PGMDEV = stk500v2
PGMOPT = -P /dev/ttyS0


# AVR-GCC and AVRDUDE need to be installed
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude
REMOVE = rm -f

# Some C flags
CFLAGS = -Wall -O3

help:
	@echo
	@echo "Availiable targets:"
	@echo "    help - Displays this help"
	@echo
	@echo "    compile - Compiles source code"
	@echo "    info - Outputs device memory information"
	@echo "    program - Programs the device"
	@echo "    clean - Deletes temporary files"
	@echo "    fuses - Writes fuse settings to device (necessary only once per device)"
	@echo
	@echo "    all - Compile, info, program, clean"
	@echo
	@echo "IMPORTANT: Device programming may only be possible as super user"
	@echo
	@echo "See Makefile for contact information."
	@echo

all: compile info program clean

compile:
	@$(CC) $(CFLAGS) -mmcu=$(MCU) $(SRC) -o $(TARGET).elf
	@$(OBJCOPY) -O ihex -j .text -j .data $(TARGET).elf $(TARGET).hex

info:
	avr-size $(TARGET).elf

program:
	@$(AVRDUDE) -p $(MCU) -q -q -u -V -c $(PGMDEV) $(PGMOPT) -U flash:w:$(TARGET).hex:i

fuses:
	@$(AVRDUDE) -p $(MCU) -q -q -u -V -c $(PGMDEV) $(PGMOPT) -U lfuse:w:0xE1:m -U hfuse:w:0x12:m

clean:
	@$(REMOVE) $(TARGET).elf
