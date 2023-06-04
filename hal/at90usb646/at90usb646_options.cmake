# AVR-specific include paths
target_include_directories(SIMMProgrammerBootloader.elf PRIVATE
	hal/at90usb646
)

# AVR-specific compiler definitions
target_compile_definitions(SIMMProgrammerBootloader.elf PRIVATE
	F_CPU=16000000UL
	F_USB=16000000UL
	USE_LUFA_CONFIG_HEADER
)

# AVR-specific compiler options
# "No jump tables option" is needed in order to work around bug in older AVR GCC
# where the bootloader relocation causes problems with jump tables
target_compile_options(SIMMProgrammerBootloader.elf PRIVATE
	-fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields -fno-jump-tables
)

if(NOT DEFINED AVR_TARGET_MCU)
	message(FATAL_ERROR "unspecified AVR_TARGET_MCU. Valid options: at90usb646, at90usb1286")
elseif(${AVR_TARGET_MCU} STREQUAL "at90usb646")
	target_compile_options(SIMMProgrammerBootloader.elf PRIVATE
		-mmcu=at90usb646
	)
	target_link_options(SIMMProgrammerBootloader.elf PRIVATE
		-mmcu=at90usb646 -Wl,--section-start=.text=0xE000
	)
elseif(${AVR_TARGET_MCU} STREQUAL "at90usb1286")
	target_compile_options(SIMMProgrammerBootloader.elf PRIVATE
		-mmcu=at90usb1286
	)
	target_link_options(SIMMProgrammerBootloader.elf PRIVATE
		-mmcu=at90usb1286 -Wl,--section-start=.text=0x1E000
	)
else()
	message(FATAL_ERROR "invalid AVR_TARGET_MCU. Valid options: at90usb646, at90usb1286")
endif()

# AVR-specific command/target to generate .hex file from the ELF file
add_custom_command(OUTPUT SIMMProgrammerBootloader.hex
	COMMAND ${CMAKE_OBJCOPY} -R .eeprom -O ihex SIMMProgrammerBootloader.elf SIMMProgrammerBootloader.hex
	DEPENDS SIMMProgrammerBootloader.elf
)
add_custom_target(SIMMProgrammerBootloader_hex ALL DEPENDS SIMMProgrammerBootloader.hex)
