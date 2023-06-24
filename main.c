/*
 * main.c
 *
 *  Created on: Dec 20, 2011
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

// This include will come from whichever hardware is selected
#include "hardware.h"
#include "SIMMProgrammer/programmer_protocol.h"

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
	DisableInterrupts();

	InitHardware();

	// Initialize the LED, default it to off
	LED_Init();
	LED_Off();

	// Initialize USB and enable interrupts
	USBCDC_Init();
	EnableInterrupts();

	// Run the USB task, listen for bytes, act in response.
	while (1)
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

		USBCDC_Check();
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
		// Now enter the main firmware
		EnterMainFirmware();
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
			if (curWriteIndex < FIRMWARE_1KB_CHUNKS)
			{
				USBCDC_SendByte(BootloaderWriteOK);
			}
			else
			{
				USBCDC_SendByte(BootloaderWriteError);
				curCommandState = WaitingForCommand;
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

			// Write the actual flash now
			if (WriteFlash(programChunkBytes, (uint32_t)curWriteIndex * (uint32_t)PROGRAM_CHUNK_SIZE_BYTES))
			{
				USBCDC_SendByte(BootloaderWriteOK);
				curWriteIndex++;
				writePosInChunk = -1;
			}
			else
			{
				USBCDC_SendByte(BootloaderWriteError);
				curCommandState = WaitingForCommand;
			}
		}
	}
}
