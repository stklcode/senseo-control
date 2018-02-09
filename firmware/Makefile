#############################################################################
#  SenseoControl 2.0                                                        #
#  Copyright (C) 2013-2018  Stefan Kalscheuer                               #
#                                                                           #
#  This program is free software: you can redistribute it and/or modify     #
#  it under the terms of the GNU General Public License as published by     #
#  the Free Software Foundation version 3.                                  #
#                                                                           #
#  This program is distributed in the hope that it will be useful,          #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of           #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            #
#  GNU General Public License for more details.                             #
#                                                                           #
#   You should have received a copy of the GNU General Public License       #
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.  #
#############################################################################

# Include environment config
include Makefile.config

# Project specific settings
TARGET = SenseoControl-2.0
MCU = attiny26
SRC = main.c

# Some C flags
CFLAGS = -Wall -Wextra -Os

help:
	@echo
	@echo "Availiable targets:"
	@echo "    help     Displays this help"
	@echo
	@echo "    compile  Compiles source code"
	@echo "    info     Outputs device memory information"
	@echo "    program  Programs the device"
	@echo "    clean    Deletes temporary files"
	@echo "    fuses    Writes fuse settings to device (necessary only once per device)"
	@echo
	@echo "    all      Compile, info, program, clean"
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
