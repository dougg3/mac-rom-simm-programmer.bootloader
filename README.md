# Mac ROM SIMM Programmer Bootloader
This is the bootloader for the [Mac ROM SIMM Programmer](https://github.com/dougg3/mac-rom-simm-programmer). It is designed to run on the Atmel/Microchip [AT90USB646](https://www.microchip.com/wwwproducts/en/AT90usb646) AVR microcontroller and the Nuvoton [M258KE3AE](https://www.nuvoton.com/products/microcontrollers/arm-cortex-m23-mcus/m254-m256-m258-low-power-lcd-series/m258ke3ae/) ARM Cortex-M23 microcontroller. It presents itself as a USB CDC serial port and provides two functions:

1. Boot the main SIMM programmer firmware when requested
2. Update the main firmware

When the AVR version of the bootloader first boots up, it waits for instructions on what to do next. Thus, there is no need for a bootloader entry button or anything like that. It always enters the bootloader first, and only executes the main firmware when instructed to do so. The ARM version automatically jumps to the main firmware when it powers on, but this can be interrupted by shorting J2 to ground before powering it on.

## AT90USB646/AT90USB1286 (AVR) Version

### Compiling

The code is designed to build using [avr-gcc](https://gcc.gnu.org/wiki/avr-gcc) and [AVR Libc](https://www.nongnu.org/avr-libc/). It can be built using either CMake or Eclipse with the [AVR Eclipse plugin](https://avr-eclipse.sourceforge.net/wiki/index.php/The_AVR_Eclipse_Plugin).

This bootloader uses the [LUFA](http://www.fourwalledcubicle.com/LUFA.php) USB library. The code for LUFA is provided by the main [SIMM programmer firmware project](https://github.com/dougg3/mac-rom-simm-programmer) as a Git submodule.

To check out the project with the submodule, type the following command:

`git clone --recursive https://github.com/dougg3/mac-rom-simm-programmer.bootloader.git`

To build with CMake:

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../SIMMProgrammer/toolchain-avr.cmake -DAVR_TARGET_MCU=at90usb646 ..
make
```

There are two build configurations: one for the AT90USB646/7 and one for the AT90USB1286/7. The global chip shortage led to the AT90USB128x being easier to procure, even though we don't need its larger flash capacity. Because it has more than 64 KB of flash and the bootloader is stored in the upper 64 KB, it needs a special version of the bootloader that can handle it properly. To build the AT90USB1286/7 variant instead, replace `at90usb646` in the cmake command above with `at90usb1286`.

### Binaries

Precompiled binaries are available in the [Releases section](https://github.com/dougg3/mac-rom-simm-programmer.bootloader/releases) of this project.

### Fuse bits

Here is the required AVR fuse bit configuration for this bootloader:

- LFUSE = `0xDF`
- HFUSE = `0xD0`
- EFUSE = `0xF8`

### Flashing

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

## M258KE3AE (ARM) version

### Compiling

These instructions are tested with [ARM GCC 6-2017-q1-update](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/6-2017-q1-update). To build with CMake:

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../SIMMProgrammer/toolchain-m258ke.cmake ..
make
```

### Binaries

Precompiled binaries are available in the [Releases section](https://github.com/dougg3/mac-rom-simm-programmer.bootloader/releases) of this project.

### Flashing

You can use Nuvoton's Windows-based [NuMicro ICP Programming Tool](https://www.nuvoton.com/tool-and-software/software-tool/programmer-tool/) to flash the bootloader. You will need a Nuvoton programmer such as the [Nu-Link](https://www.nuvoton.com/tool-and-software/debugger-and-programmer/1-to-1-debugger-and-programmer/nu-link/) or [Nu-Link-Pro](https://www.nuvoton.com/tool-and-software/debugger-and-programmer/1-to-1-debugger-and-programmer/nu-link-pro/).

In the Load File section, choose SIMMProgrammerBootloader.hex to be loaded to LDROM. You can leave APROM alone. Make sure Config 0 is set to 0xFFFFFF7F (boot options = LDROM). At the bottom of the window, choose to program LDROM and Chip Setting. Then click Start to flash the chip.
