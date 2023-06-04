cmake_minimum_required(VERSION 3.13)
project(SIMMProgrammerBootloader)

# Create a list of all source files common to all architectures
set(SOURCES
	main.c
)

# Get hardware-specific source files
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "avr")
	include(hal/at90usb646/at90usb646_sources.cmake)
else()
	message(FATAL_ERROR "unrecognized architecture for build")
endif()

# The actual bootloader, in ELF format
add_executable(SIMMProgrammerBootloader.elf ${SOURCES} ${HWSOURCES})

# Common compiler options
target_compile_options(SIMMProgrammerBootloader.elf PRIVATE
	-Wall -Os -ffunction-sections -fdata-sections
)
set_property(TARGET SIMMProgrammerBootloader.elf PROPERTY C_STANDARD 99)

# Common linker options
target_link_options(SIMMProgrammerBootloader.elf PRIVATE
	-Wl,-Map,SIMMProgrammerBootloader.map -Wl,--gc-sections
)

# Get hardware-specific options
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "avr")
	include(hal/at90usb646/at90usb646_options.cmake)
endif()
