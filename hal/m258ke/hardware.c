/*
 * hardware.c
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

#include "hardware.h"

/** Goes to the main firmware immediately
 *
 */
void ResetToMainFirmware(void)
{
	// Clear reset status bits so that main firmware knows reset reason
	SYS->RSTSTS = (SYS_RSTSTS_PORF_Msk | SYS_RSTSTS_PINRF_Msk);

	// Boot to APROM next time
	FMC->ISPCTL &= ~FMC_ISPCTL_BS_Msk;

	// Reset!
	NVIC_SystemReset();

	// Should never get here, but just in case...
	while (1);
}
