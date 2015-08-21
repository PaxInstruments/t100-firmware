#--------------------------------------------------------------------------------------------------
# Name: Makefile
# Project: hid-data example
# Author: Christian Starkjohann
# Creation Date: 2008-04-07
# Tabsize: 4
# Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
# License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
#--------------------------------------------------------------------------------------------------
# Modified for T100 firmware.
#--------------------------------------------------------------------------------------------------
DEVICE     = attiny85
F_CPU      = 16500000   
PROGRAMMER = usbtiny

FLASH        = micronucleus main.hex
BOOTL-JUMPER = avrdude -c $(PROGRAMMER) -p t85 -U flash:w:bin/micronucleus_t100_jumper.hex -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m
BOOTLOADER   = avrdude -c $(PROGRAMMER) -p t85 -U flash:w:bin/micronucleus-1.02.hex -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m
FUSE         = avrdude -c $(PROGRAMMER) -p t85 -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m

CFLAGS  = -Iusbdrv -I. -DDEBUG_LEVEL=0
CFLAGS += -Wno-deprecated-declarations -D__PROG_TYPES_COMPAT__
CFLAGS += -Wl,--gc-sections
CFLAGS += -fdata-sections -ffunction-sections
OBJECTS = usbdrv/usbdrv.o usbdrv/usbdrvasm.o main.o usbRelated.o bitbang_i2c.o

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

# symbolic targets:
help:
	@echo "This Makefile has no default rule. Use one of the following:"
	@echo "make hex ................ to build main.hex"
	@echo "make fuse ............... to flash the fuses"
	@echo "make flash .............. to flash the firmware using micronucleus"
	@echo "make clean .............. to delete objects and hex file"
	@echo "make bootloader ......... to flash the bootloader and fuses"
	@echo "make bootloader-jumper .. to flash the jumper bootloader and fuses"
	@echo "make iterate ............ [ make clean && make hex && make flash ]"

hex: main.hex

# rule for programming fuse bits:
fuse:
	$(FUSE)

# rule for uploading firmware:
flash:
	$(FLASH)

bootloader:
	$(BOOTLOADER)

bootloader-jumper:
	$(BOOTL-JUMPER)

iterate:    
	make clean && make hex && make flash

# rule for deleting dependent files (those which can be built by Make):
clean:
	rm -f main.hex main.lst main.obj main.cof main.list main.map main.eep.hex main.elf *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s

# Generic rule for compiling C files:
.c.o:
	$(COMPILE) -c $< -o $@

# Generic rule for assembling Assembler source files:
.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

# Generic rule for compiling C to assembler, used for debugging only.
.c.s:
	$(COMPILE) -S $< -o $@

# file targets:

main.elf: $(OBJECTS) # usbdrv dependency only needed because we copy it
	$(COMPILE) -o main.elf $(OBJECTS)

main.hex: main.elf
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size main.hex

# debugging targets:

disasm: main.elf
	avr-objdump -d main.elf

cpp:
	$(COMPILE) -E main.c
