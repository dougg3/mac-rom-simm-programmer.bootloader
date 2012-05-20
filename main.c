/*
 * main.c
 *
 *  Created on: Dec 20, 2011
 *      Author: Doug
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/boot.h>
#include "LUFA/Drivers/USB/USB.h"
#include "../SIMMProgrammer/cdc_device_definition.h"
#include "../SIMMProgrammer/programmer_protocol.h"

typedef enum BootloaderCommandState
{
	WaitingForCommand = 0,
	WritingFirmware
} BootloaderCommandState;

static BootloaderCommandState curCommandState = WaitingForCommand;
static int16_t writePosInChunk = -1;
static uint16_t curWriteIndex = 0;

#define PROGRAM_CHUNK_SIZE_BYTES	1024

void HandleEraseWriteByte(uint8_t byte);
void HandleWaitingForCommandByte(uint8_t byte);

#define SendByte(b) CDC_Device_SendByte(&VirtualSerial_CDC_Interface, b)
#define ReadByte() CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface)
#define SendData(d, l) CDC_Device_SendData(&VirtualSerial_CDC_Interface, d, l)

int main(void)
{
	cli();

	// Move interrupt vector table to boot loader section...
	// (By default it's in the application section, and I need
	// the interrupts to use USB correctly)
	uint8_t tmpMCUCR = MCUCR;
	MCUCR = tmpMCUCR | (1 << IVCE);
	MCUCR = tmpMCUCR | (1 << IVSEL);

	DDRD |= (1 << 7);
	PORTD &= ~(1 << 7);

	USB_Init();
	sei();

	while (1)
	{
		if (USB_DeviceState == DEVICE_STATE_Configured)
		{
			int16_t recvByte = ReadByte();

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

void HandleWaitingForCommandByte(uint8_t byte)
{
	switch (byte)
	{
	case GetBootloaderState:
		SendByte(CommandReplyOK);
		SendByte(BootloaderStateInBootloader);
		curCommandState = WaitingForCommand;
		break;
	case EnterBootloader:
		SendByte(CommandReplyOK);
		curCommandState = WaitingForCommand;
		break;
	case EnterProgrammer:
		SendByte(CommandReplyOK);
		CDC_Device_Flush(&VirtualSerial_CDC_Interface);

		// Insert a small delay to ensure that it arrives before rebooting.
		_delay_ms(1000);

		// Done with the USB interface -- the programmer will re-initialize it.
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
		SendByte(CommandReplyOK);
		break;
	default:
		SendByte(CommandReplyInvalid);
		curCommandState = WaitingForCommand;
		break;
	}
}

void HandleEraseWriteByte(uint8_t byte)
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
				SendByte(BootloaderWriteOK);
			}
			else
			{
				SendByte(BootloaderWriteError);
			}
			break;
		case ComputerBootloaderFinish:
			// Just to confirm that we finished writing...
			SendByte(BootloaderWriteOK);
			curCommandState = WaitingForCommand;
			break;
		case ComputerBootloaderCancel:
			SendByte(BootloaderWriteConfirmCancel);
			curCommandState = WaitingForCommand;
			break;
		}
	}
	else
	{
		programChunkBytes[writePosInChunk++] = byte;
		if (writePosInChunk >= PROGRAM_CHUNK_SIZE_BYTES)
		{
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
				for (y = 0; y < SPM_PAGESIZE; y+=2)
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

			SendByte(BootloaderWriteOK);
			curWriteIndex++;
			writePosInChunk = -1;
		}
	}
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
