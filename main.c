/*
 * main.c
 *
 *  Created on: Dec 20, 2011
 *      Author: Doug
 */

#include <avr/io.h>
#include <util/delay.h>

typedef enum ProgrammerCommand
{
	EnterWaitingMode = 0,
	DoElectricalTest,
	IdentifyChips,
	ReadByte,
	ReadChips,
	EraseChips,
	WriteChips,
	GetBootloaderState,
	EnterBootloader,
	EnterProgrammer
} ProgrammerCommand;

typedef enum ProgrammerReply
{
	CommandReplyOK,
	CommandReplyError,
	CommandReplyInvalid
} ProgrammerReply;

typedef enum BootloaderStateReply
{
	BootloaderStateInBootloader,
	BootloaderStateInProgrammer
} BootloaderStateReply;

int main(void)
{
	DDRD |= (1 << 7);
	PORTD &= ~(1 << 7);

	int x;
	for (x = 0; x < 20; x++)
	{
		_delay_ms(100);
		PIND = (1 << 7);
	}

	_delay_ms(500);

	// Launch the program...
	__asm__ __volatile__ ( "jmp 0x0000" );
}
