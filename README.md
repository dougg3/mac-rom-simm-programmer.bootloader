# Mac ROM SIMM Programmer Bootloader
This is the bootloader for the [Mac ROM SIMM Programmer](https://github.com/dougg3/mac-rom-simm-programmer). It is designed to run on the Atmel/Microchip [AT90USB646](https://www.microchip.com/wwwproducts/en/AT90usb646) AVR microcontroller. It presents itself as a USB CDC serial port and provides two functions:

1. Boot the main SIMM programmer firmware when requested
2. Update the main firmware

When the bootloader first boots up, it waits for instructions on what to do next. Thus, there is no need for a bootloader entry button or anything like that. It always enters the bootloader first, and only executes the main firmware when instructed to do so.

## Compiling

The code is designed to build using [avr-gcc](https://gcc.gnu.org/wiki/avr-gcc) and [AVR Libc](https://www.nongnu.org/avr-libc/). It is an [Eclipse CDT](https://www.eclipse.org/cdt/) project that uses the [AVR Eclipse Plugin](http://avr-eclipse.sourceforge.net/wiki/index.php/The_AVR_Eclipse_Plugin). If you download Eclipse (with CDT) and install the AVR Eclipse plugin, it should build. You may need to add avr-gcc to your path if it's not already in the path.

This bootloader uses the [LUFA](http://www.fourwalledcubicle.com/LUFA.php) USB library. The code for LUFA is provided by the main [SIMM programmer firmware project](https://github.com/dougg3/mac-rom-simm-programmer). In order to build this bootloader, make sure the main SIMMProgrammer firmware project is also checked out in your Eclipse workspace.

There are two build configurations: one for the AT90USB646/7 and one for the AT90USB1286/7. The global chip shortage led to the AT90USB128x being easier to procure, even though we don't need its larger flash capacity. Because it has more than 64 KB of flash and the bootloader is stored in the upper 64 KB, it needs a special version of the bootloader that can handle it properly.

## Binaries

Precompiled binaries are available in the [Releases section](https://github.com/dougg3/mac-rom-simm-programmer.bootloader/releases) of this project.

## Fuse bits

Here is the required AVR fuse bit configuration for this bootloader:

- LFUSE = `0xDF`
- HFUSE = `0xD0`
- EFUSE = `0xF8`

## Flashing

The Eclipse project should be able to flash the bootloader to a board using an AVRISP mkII programmer by simply clicking the AVR upload button in the Eclipse toolbar (Ctrl+Alt+U by default). If you have a different programmer, you can customize the configuration of the project.

Sample [avrdude](https://www.nongnu.org/avrdude/) programming command (change the -c parameter if you have a different programmer):

```
avrdude	-pusb646 -cavrisp2 -Pusb -u \
    -Uflash:w:SIMMProgrammerBootloader.hex:a \
    -Ulfuse:w:0xdf:m \
    -Uhfuse:w:0xd0:m \
    -Uefuse:w:0xf8:m
```

If you are flashing a board that has an AT90USB1286 chip, simply replace `usb646` with `usb1286` in the command above.
