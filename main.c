/*
 * main.c
 *
 *  Created on: Dec 20, 2011
 *      Author: Doug
 */

#include <avr/io.h>
#include <util/delay.h>
#include "LUFA/Drivers/USB/USB.h"
#include "../SIMMProgrammer/cdc_device_definition.h"
#include "../SIMMProgrammer/programmer_protocol.h"

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
				switch (recvByte)
				{
				case GetBootloaderState:
					SendByte(CommandReplyOK);
					SendByte(BootloaderStateInBootloader);
					break;
				case EnterBootloader:
					SendByte(CommandReplyOK);
					break;
				case EnterProgrammer:
					SendByte(CommandReplyOK);
					USB_Disable();

					// Disable interrupts...
					cli();

					// Change back to the application interrupt vector table
					tmpMCUCR = MCUCR;
					MCUCR = tmpMCUCR | (1 << IVCE);
					MCUCR = tmpMCUCR & ~(1 << IVSEL);

					// Wait a little bit to let everything settle and let the program close the port after the USB disconnect
					_delay_ms(2000);

					// Now run the stored program instead
					__asm__ __volatile__ ( "jmp 0x0000" );
					break;
				default:
					SendByte(CommandReplyInvalid);
					break;
				}
			}
		}

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
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
