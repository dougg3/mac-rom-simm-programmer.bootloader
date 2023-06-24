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
#define PC9							GPIO_PIN_DATA(2, 9)

/// Where the LED port and pin are located
#define LED_PORT					PC
#define LED_PIN						9
#define LED_PORTPIN					PC9

/// The number of 1 KB chunks we can use for the main firmware.
#define FIRMWARE_1KB_CHUNKS			128

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

	// Enable GPIOC
	CLK->AHBCLK |= CLK_AHBCLK_GPCCKEN_Msk;
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
	return false;
}

/** Jumps to the main firmware
 *
 */
static inline void EnterMainFirmware(void)
{

}

#endif /* HAL_M258KE_HARDWARE_H_ */
