/*
 * hardware.h
 *
 *  Created on: Jun 3, 2023
 *      Author: Doug
 *
 * Copyright (C) 2011-2023 Doug Brown
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef HAL_AT90USB646_HARDWARE_H_
#define HAL_AT90USB646_HARDWARE_H_

#include <avr/io.h>
#include <util/delay.h>
#include <avr/boot.h>
#include "../../SIMMProgrammer/hal/at90usb646/LUFA/Drivers/USB/USB.h"
#include "../../SIMMProgrammer/hal/at90usb646/cdc_device_definition.h"

/// Bitmask of the LED in its port/pin/DDR registers
#define LED_PORT_MASK			(1 << 7)

/// The number of 1 KB chunks we can use for the main firmware.
/// This is different between the AT90USB128x and AT90USB64x.
#if defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB1287__)
#define FIRMWARE_1KB_CHUNKS		120 // 120 x 1024 byte chunks = 120K
#else
#define FIRMWARE_1KB_CHUNKS		56 // 56 x 1024 byte chunks = 56K
#endif

/** Disables interrupts
 *
 */
static inline void DisableInterrupts(void)
{
	cli();
}

/** Enables interrupts
 *
 */
static inline void EnableInterrupts(void)
{
	sei();
}

/** Does any initial hardware setup necessary on this processor
 *
 */
static inline void InitHardware(void)
{
	// Move interrupt vector table to bootloader section...
	// (By default it's in the application section, and I need
	// the interrupts to use USB correctly)
	uint8_t tmpMCUCR = MCUCR;
	MCUCR = tmpMCUCR | (1 << IVCE);
	MCUCR = tmpMCUCR | (1 << IVSEL);
}

/** Initializes the LED
 *
 */
static inline void LED_Init(void)
{
	DDRD |= LED_PORT_MASK;
}

/** Turns the LED on
 *
 */
static inline __attribute__((unused)) void LED_On(void)
{
	PORTD |= LED_PORT_MASK;
}

/** Turns the LED off
 *
 */
static inline void LED_Off(void)
{
	PORTD &= ~LED_PORT_MASK;
}

/** Toggles the LED
 *
 */
static inline void LED_Toggle(void)
{
	PIND = LED_PORT_MASK;
}

/** Initializes the USB CDC serial port
 *
 */
static inline void USBCDC_Init(void)
{
	// Initialize LUFA
	USB_Init();
}

/** Performs any necessary periodic tasks for the USB CDC serial port
 *
 */
static inline void USBCDC_Check(void)
{
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

/** Sends a byte out the USB serial port
 *
 * @param b The byte
 */
static inline void USBCDC_SendByte(uint8_t b)
{
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, b);
}

/** Reads a byte from the USB serial port, if available
 *
 * @return The byte, or -1 if there is nothing available
 */
static inline int16_t USBCDC_ReadByte(void)
{
	return CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
}

/** Flushes remaining data out to the USB serial port
 *
 */
static inline void USBCDC_Flush(void)
{
	CDC_Device_Flush(&VirtualSerial_CDC_Interface);
}

/** Writes a chunk of data to flash
 *
 * @param buffer The buffer to write to flash (this will contain 1024 bytes to write)
 * @param locationInFlash The location in flash to write it to (0 = start of program space)
 */
static inline void WriteFlash(uint8_t const *buffer, uint32_t locationInFlash)
{
	// Disable interrupts while we're doing this...
	DisableInterrupts();

	// Write this data into the AVR
	// one page at a time (pages are 256 bytes each)
	int x;
	for (x = 0; x < (1024 / SPM_PAGESIZE); x++)
	{
		// Find the start address of this page
		uint32_t thisAddress = locationInFlash + (uint32_t)x * (uint32_t)SPM_PAGESIZE;

		// Erase it (safely)
		boot_page_erase_safe(thisAddress);

		// Load the page write buffer completely with (SPM_PAGESIZE) bytes...
		// (2 at a time)
		int y;
		for (y = 0; y < SPM_PAGESIZE; y += 2)
		{
			uint16_t w = *buffer | (*(buffer + 1) << 8);
			boot_page_fill_safe(thisAddress + (uint32_t)y, w);
			buffer += 2;
		}

		// Write the page write buffer into flash
		boot_page_write_safe(thisAddress);
	}

	// In case this is the last flash, make sure the RWW is enabled
	// so we're safe when we jump into the stored program
	boot_rww_enable_safe();

	// Now it's safe to re-enable interrupts
	EnableInterrupts();
}

/** Jumps to the main firmware
 *
 */
static inline void EnterMainFirmware(void)
{
	// Insert a small delay to ensure that it arrives before rebooting.
	_delay_ms(1000);

	// Done with the USB for now -- the main firmware will re-initialize it.
	USB_Disable();

	// Disable interrupts...
	DisableInterrupts();

	// Change back to the application interrupt vector table
	uint8_t tmpMCUCR = MCUCR;
	MCUCR = tmpMCUCR | (1 << IVCE);
	MCUCR = tmpMCUCR & ~(1 << IVSEL);

	// Wait a little bit to let everything settle and let the program close the port after the USB disconnect
	_delay_ms(2000);

	// Now run the stored program instead
	__asm__ __volatile__ ( "jmp 0x0000" );
}

#endif /* HAL_AT90USB646_HARDWARE_H_ */
