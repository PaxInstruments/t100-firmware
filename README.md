# Pax Instruments T100 Firmware
## Overview
Firmware for the Pax Instruments thermocouple USB adapter

## Programming
Here are basic instructions.

- Flash bootloader to T100 by `make bootloader`. That will also set the appropriate fuses.
- Type `make iterate` to create a fresh hex file and invoke the micronucleus bootloader.
- Plug the T100 into USB
- Done!

##Bootloader application
You need to install the commandline utility for micronucleus bootloader.

For __OSX__, go to <https://github.com/micronucleus/micronucleus/tree/master/commandline/builds> link and download the binary. Then, from the terminal hit `sudo cp micronucleus /usr/bin` to copy the application to global binary location.

For __Windows__, again download the binary from the link above and put that binary inside any of the global path folders.

For __Linux__, there is no available binary release for micronucleus but you can download the source from <https://github.com/micronucleus/micronucleus/tree/master/commandline> and install the application by `make all` followed by `sudo make install`. You'll need to have `libusb` to compile the source.