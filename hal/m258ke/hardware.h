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
#include "../../SIMMProgrammer/hal/m258ke/usbcdc_hw.h"

/// The number of 1 KB chunks we can use for the main firmware.
#define FIRMWARE_1KB_CHUNKS			128

/** Disables interrupts
 *
 */
static inline void DisableInterrupts(void)
{
}

/** Enables interrupts
 *
 */
static inline void EnableInterrupts(void)
{
}

/** Does any initial hardware setup necessary on this processor
 *
 */
static inline void InitHardware(void)
{
}

/** Initializes the LED
 *
 */
static inline void LED_Init(void)
{
}

/** Turns the LED on
 *
 */
static inline __attribute__((unused)) void LED_On(void)
{
}

/** Turns the LED off
 *
 */
static inline void LED_Off(void)
{
}

/** Toggles the LED
 *
 */
static inline void LED_Toggle(void)
{
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

#endif /* HAL_AT90USB646_HARDWARE_H_ */
