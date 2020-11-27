/*
 * main.c
 *
 *  Created on: Dec 20, 2011
 *      Author: Doug
 *
 * Copyright (C) 2011-2020 Doug Brown
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/boot.h>
// Grab files from the SIMMProgrammer project, checked out next to this project
#include "../SIMMProgrammer/hal/at90usb646/LUFA/Drivers/USB/USB.h"
#include "../SIMMProgrammer/hal/at90usb646/cdc_device_definition.h"
#include "../SIMMProgrammer/programmer_protocol.h"

/// Bitmask of the LED in its port/pin/DDR registers
#define LED_PORT_MASK				(1 << 7)
/// Number of bytes sent at a time during firmware programming
#define PROGRAM_CHUNK_SIZE_BYTES	1024

/// Current bootloader state
typedef enum BootloaderCommandState
{
	WaitingForCommand = 0,//!< We're waiting to receive a command
	WritingFirmware       //!< We're flashing the firmware
} BootloaderCommandState;

static void HandleEraseWriteByte(uint8_t byte);
static void HandleWaitingForCommandByte(uint8_t byte);
static inline void LED_Init(void);
static inline void LED_On(void);
static inline void LED_Off(void);
static inline void LED_Toggle(void);
static inline void USBCDC_SendByte(uint8_t b);
static inline int16_t USBCDC_ReadByte(void);
static inline void USBCDC_Flush(void);

/// The current state
static BootloaderCommandState curCommandState = WaitingForCommand;
/// Where we are in the bootloader write process
static int16_t writePosInChunk = -1;
/// The current page index we are writing
static uint16_t curWriteIndex = 0;

/** Main program.
 *
 * @return Never; it has an infinite loop.
 */
int main(void)
{
	cli();

	// Move interrupt vector table to boot loader section...
	// (By default it's in the application section, and I need
	// the interrupts to use USB correctly)
	uint8_t tmpMCUCR = MCUCR;
	MCUCR = tmpMCUCR | (1 << IVCE);
	MCUCR = tmpMCUCR | (1 << IVSEL);

	// Initialize the LED, default it to off
	LED_Init();
	LED_Off();

	// Initialize USB and enable interrupts
	USB_Init();
	sei();

	// Run the USB task, listen for bytes, act in response.
	while (1)
	{
		if (USB_DeviceState == DEVICE_STATE_Configured)
		{
			int16_t recvByte = USBCDC_ReadByte();

			if (recvByte >= 0)
			{
				switch (curCommandState)
				{
				case WaitingForCommand:
					HandleWaitingForCommandByte((uint8_t)recvByte);
					break;
				case WritingFirmware:
					HandleEraseWriteByte((uint8_t)recvByte);
					break;
				}
			}
		}

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
	}
}

/** Handler called when we receive a byte when we're waiting for a command
 *
 * @param byte The byte
 */
static void HandleWaitingForCommandByte(uint8_t byte)
{
	switch (byte)
	{
	case GetBootloaderState:
		USBCDC_SendByte(CommandReplyOK);
		USBCDC_SendByte(BootloaderStateInBootloader);
		curCommandState = WaitingForCommand;
		break;
	case EnterBootloader:
		USBCDC_SendByte(CommandReplyOK);
		curCommandState = WaitingForCommand;
		break;
	case EnterProgrammer:
		// Send a response immediately, and flush the serial port
		USBCDC_SendByte(CommandReplyOK);
		USBCDC_Flush();

		// Insert a small delay to ensure that it arrives before rebooting.
		_delay_ms(1000);

		// Done with the USB for now -- the main firmware will re-initialize it.
		USB_Disable();

		// Disable interrupts...
		cli();

		// Change back to the application interrupt vector table
		uint8_t tmpMCUCR = MCUCR;
		MCUCR = tmpMCUCR | (1 << IVCE);
		MCUCR = tmpMCUCR & ~(1 << IVSEL);

		// Wait a little bit to let everything settle and let the program close the port after the USB disconnect
		_delay_ms(2000);

		// Now run the stored program instead
		__asm__ __volatile__ ( "jmp 0x0000" );
		break;
	case BootloaderEraseAndWriteProgram:
		curCommandState = WritingFirmware;
		curWriteIndex = 0;
		writePosInChunk = -1;
		USBCDC_SendByte(CommandReplyOK);
		break;
	default:
		USBCDC_SendByte(CommandReplyInvalid);
		curCommandState = WaitingForCommand;
		break;
	}
}

/** Handler called when we receive a byte while we're programming firmware
 *
 * @param byte The byte
 */
static void HandleEraseWriteByte(uint8_t byte)
{
	static uint8_t programChunkBytes[PROGRAM_CHUNK_SIZE_BYTES];

	if (writePosInChunk == -1)
	{
		switch (byte)
		{
		case ComputerBootloaderWriteMore:
			writePosInChunk = 0;
			if (curWriteIndex < 56) // 56 x 1024 byte chunks = 56K = application size
			{
				USBCDC_SendByte(BootloaderWriteOK);
			}
			else
			{
				USBCDC_SendByte(BootloaderWriteError);
			}
			break;
		case ComputerBootloaderFinish:
			// Just to confirm that we finished writing...
			LED_Off();
			USBCDC_SendByte(BootloaderWriteOK);
			curCommandState = WaitingForCommand;
			break;
		case ComputerBootloaderCancel:
			LED_Off();
			USBCDC_SendByte(BootloaderWriteConfirmCancel);
			curCommandState = WaitingForCommand;
			break;
		}
	}
	else
	{
		programChunkBytes[writePosInChunk++] = byte;
		if (writePosInChunk >= PROGRAM_CHUNK_SIZE_BYTES)
		{
			// Toggle the LED for some status
			LED_Toggle();

			// Disable interrupts while we're doing this...
			cli();

			// Write this data into the AVR
			// one page at a time (pages are 256 bytes each)
			uint8_t *dataPtr = programChunkBytes;
			int x;
			for (x = 0; x < (PROGRAM_CHUNK_SIZE_BYTES / SPM_PAGESIZE); x++)
			{
				// Find the start address of this page
				uint16_t thisAddress = curWriteIndex * PROGRAM_CHUNK_SIZE_BYTES +
										x * SPM_PAGESIZE;

				// Erase it (safely)
				boot_page_erase_safe(thisAddress);

				// Load the page write buffer completely with (SPM_PAGESIZE) bytes...
				// (2 at a time)
				int y;
				for (y = 0; y < SPM_PAGESIZE; y += 2)
				{
					uint16_t w = *dataPtr | (*(dataPtr + 1) << 8);
					boot_page_fill_safe(thisAddress + y, w);
					dataPtr += 2;
				}

				// Write the page write buffer into flash
				boot_page_write_safe(thisAddress);
			}

			// In case this is the last flash, make sure the RWW is enabled
			// so we're safe when we jump into the stored program
			boot_rww_enable_safe();

			// Now it's safe to re-enable interrupts
			sei();

			USBCDC_SendByte(BootloaderWriteOK);
			curWriteIndex++;
			writePosInChunk = -1;
		}
	}
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

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
