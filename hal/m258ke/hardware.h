/*
 * hardware.h
 *
 *  Created on: Jun 19, 2023
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

#ifndef HAL_M258KE_HARDWARE_H_
#define HAL_M258KE_HARDWARE_H_

#include <stdint.h>
#include <stdbool.h>
#include "../../SIMMProgrammer/hal/m258ke/nuvoton/NuMicro.h"
#include "../../SIMMProgrammer/hal/m258ke/usbcdc_hw.h"

// Borrowed from Nuvoton's sample code
#define GPIO_PIN_DATA(port, pin)	(*((volatile uint32_t *)((GPIO_PIN_DATA_BASE+(0x40*(port))) + ((pin)<<2))))
#define PB14						GPIO_PIN_DATA(1, 14)
#define PD0							GPIO_PIN_DATA(3, 0)

/// Where the LED port and pin are located
#define LED_PORT					PB
#define LED_PIN						14
#define LED_PORTPIN					PB14

/// The number of 1 KB chunks we can use for the main firmware.
#define FIRMWARE_1KB_CHUNKS			128

/// FMC command for programming 32 bits to flash
#define FMC_CMD_32BIT_PROGRAM		0x21
/// FMC command for erasing a 512-byte page of flash
#define FMC_CMD_PAGE_ERASE			0x22

void ResetToMainFirmware(void);

/** Disables interrupts
 *
 */
static inline void DisableInterrupts(void)
{
	__disable_irq();
}

/** Enables interrupts
 *
 */
static inline void EnableInterrupts(void)
{
	__enable_irq();
}

/** Does any initial hardware setup necessary on this processor
 *
 */
static inline void InitHardware(void)
{
	// Unlock protected registers so we can configure clocks, flash, WDT, etc.
	do
	{
		SYS->REGLCTL = 0x59UL;
		SYS->REGLCTL = 0x16UL;
		SYS->REGLCTL = 0x88UL;
	} while (SYS->REGLCTL == 0UL);

	// Enable 48 MHz internal high-speed RC oscillator
	CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;

	// Wait until it's ready
	while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk));

	// Clock HCLK and USB from 48 MHz HIRC
	CLK->CLKSEL0 = (CLK->CLKSEL0 & (~(CLK_CLKSEL0_HCLKSEL_Msk | CLK_CLKSEL0_USBDSEL_Msk))) |
			(7 << CLK_CLKSEL0_HCLKSEL_Pos) | (0 << CLK_CLKSEL0_USBDSEL_Pos);

	// SystemCoreClock, CyclesPerUs, PllClock default to correct values already

	// Enable USB device controller
	CLK->APBCLK0 |= CLK_APBCLK0_USBDCKEN_Msk;

	// Enable GPIOB, GPIOD, and ISP
	CLK->AHBCLK |= CLK_AHBCLK_GPBCKEN_Msk | CLK_AHBCLK_GPDCKEN_Msk | CLK_AHBCLK_ISPCKEN_Msk;

	// Set PD0 as pulled-up input
	PD->PUSEL |= (0x01 << 2*0);

	// Read PD0. If it's shorted to ground, we've been externally asked
	// to enter the bootloader. This is a failsafe to make the
	// board unbrickable if I accidentally mess up a firmware update.
	bool bootPinAskingForBootloader = PD0 == 0;

	// If we didn't find any reason above to stay in the bootloader,
	// go ahead and jump to the main firmware.
	if (!bootPinAskingForBootloader)
	{
		ResetToMainFirmware();
	}
}

/** Initializes the LED
 *
 */
static inline void LED_Init(void)
{
	// Set LED to off
	LED_PORTPIN = 1;

	// Set to push-pull output mode (mode bits = 01)
	LED_PORT->MODE = (LED_PORT->MODE & ~(3UL << 2*LED_PIN)) | (1UL << 2*LED_PIN);
}

/** Turns the LED on
 *
 */
static inline __attribute__((unused)) void LED_On(void)
{
	LED_PORTPIN = 0;
}

/** Turns the LED off
 *
 */
static inline void LED_Off(void)
{
	LED_PORTPIN = 1;
}

/** Toggles the LED
 *
 */
static inline void LED_Toggle(void)
{
	LED_PORTPIN = !LED_PORTPIN;
}

/** Writes a chunk of data to flash
 *
 * @param buffer The buffer to write to flash (this will contain 1024 bytes to write)
 * @param locationInFlash The location in flash to write it to (0 = start of program space)
 * @return True on success, false on failure
 */
static inline bool WriteFlash(uint8_t const *buffer, uint32_t locationInFlash)
{
	// Keep interrupts disabled while we do this.
	DisableInterrupts();

	// Enable ISP and updates to AP memory
	FMC->ISPCTL |= FMC_ISPCTL_ISPEN_Msk | FMC_ISPCTL_APUEN_Msk;

	// Each 1024-byte chunk represents two flash pages. Erase both of them...
	for (uint32_t x = 0; x < 1024; x += 512)
	{
		FMC->ISPCMD = FMC_CMD_PAGE_ERASE;
		FMC->ISPADDR = locationInFlash + x;
		FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;
		__ISB();
		while (FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk);

		// Look for failures and bail
		if (FMC->ISPCTL & FMC_ISPCTL_ISPFF_Msk)
		{
			FMC->ISPCTL |= FMC_ISPCTL_ISPFF_Msk;
			return false;
		}
	}

	// Now program all 1024 bytes, 4 at a time
	for (uint32_t x = 0; x < 1024; x += 4)
	{
		FMC->ISPCMD = FMC_CMD_32BIT_PROGRAM;
		FMC->ISPADDR = locationInFlash + x;
		FMC->ISPDAT = ((uint32_t)buffer[x + 0] << 0) |
					  ((uint32_t)buffer[x + 1] << 8) |
					  ((uint32_t)buffer[x + 2] << 16) |
					  ((uint32_t)buffer[x + 3] << 24);
		FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;
		__ISB();
		while (FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk);

		// Look for failures and bail
		if (FMC->ISPCTL & FMC_ISPCTL_ISPFF_Msk)
		{
			FMC->ISPCTL |= FMC_ISPCTL_ISPFF_Msk;
			return false;
		}
	}

	// Disable ISP and updates to AP memory
	FMC->ISPCTL &= ~(FMC_ISPCTL_ISPEN_Msk | FMC_ISPCTL_APUEN_Msk);

	// And now it's safe to re-enable interrupts
	EnableInterrupts();

	return true;
}

/** Delays for about a second
 *
 * This is not accurate at all. I just came up with a delay loop
 * that is close enough to 1 second. The idea is to use as little
 * flash as possible to implement this delay function. Flash is
 * more important than delay accuracy here since the LDROM is only
 * 4 kilobytes in size.
 */
static void DelayAbout1Sec(void)
{
	volatile uint32_t count;
	for (count = 0; count < 3500000; count++);
}

/** Jumps to the main firmware
 *
 */
static inline void EnterMainFirmware(void)
{
	// Insert a small delay to ensure that it arrives before rebooting.
	DelayAbout1Sec();

	// Keep interrupts disabled now, we want no more USB communication
	DisableInterrupts();

	// Disconnect USB
	USBCDC_Disable();

	// Wait a little bit to let everything settle and let the program close the port after the USB disconnect
	DelayAbout1Sec();
	DelayAbout1Sec();

	// Reset and go to the main firmware
	ResetToMainFirmware();
}

#endif /* HAL_AT90USB646_HARDWARE_H_ */
