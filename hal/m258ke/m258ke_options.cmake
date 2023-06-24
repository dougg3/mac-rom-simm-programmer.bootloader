# M258KE-specific include paths
target_include_directories(SIMMProgrammerBootloader.elf PRIVATE
	hal/m258ke
)

# M258KE-specific compiler definitions
target_compile_definitions(SIMMProgrammerBootloader.elf PRIVATE
)

# M258KE-specific compiler options
target_compile_options(SIMMProgrammerBootloader.elf PRIVATE
	-mcpu=cortex-m23 -march=armv8-m.base -mthumb
)

target_link_options(SIMMProgrammerBootloader.elf PRIVATE
	-mcpu=cortex-m23 -march=armv8-m.base -mthumb
	-T ${CMAKE_SOURCE_DIR}/SIMMProgrammer/hal/m258ke/nuvoton/LDROM.ld
	--specs=nano.specs
)

# M258KE-specific command/target to generate .hex file from the ELF file
add_custom_command(OUTPUT SIMMProgrammerBootloader.hex
	COMMAND ${CMAKE_OBJCOPY} -O ihex SIMMProgrammerBootloader.elf SIMMProgrammerBootloader.hex
	DEPENDS SIMMProgrammerBootloader.elf
)
add_custom_target(SIMMProgrammerBootloader_hex ALL DEPENDS SIMMProgrammerBootloader.hex)
