/**
 ** Supermodel
 ** A Sega Model 3 Arcade Emulator.
 ** Copyright 2011 Bart Trzynadlowski, Nik Henson 
 **
 ** This file is part of Supermodel.
 **
 ** Supermodel is free software: you can redistribute it and/or modify it under
 ** the terms of the GNU General Public License as published by the Free 
 ** Software Foundation, either version 3 of the License, or (at your option)
 ** any later version.
 **
 ** Supermodel is distributed in the hope that it will be useful, but WITHOUT
 ** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 ** FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 ** more details.
 **
 ** You should have received a copy of the GNU General Public License along
 ** with Supermodel.  If not, see <http://www.gnu.org/licenses/>.
 **/
 
/*
 * RTC72421.cpp
 * 
 * Epson RTC-72421 implementation. 
 *
 * TO-DO List:
 * -----------
 * - Writes do nothing yet.
 */

#include "RTC72421.h"

#include "Supermodel.h"


/******************************************************************************
 Emulation Functions
******************************************************************************/

/*
 * Reads are frozen to a fixed timestamp: 1998-01-01 00:00:00 (Thursday).
 * Host time is nondeterministic: it breaks netplay and replay determinism.
 * Writes stay unimplemented; games may write, reads stay frozen.
 */
UINT8 CRTC72421::ReadRegister(unsigned reg)
{
	switch (reg&0xF)
	{
		case 0:		// 1-second digit
			return 0;
		case 1:		// 10-seconds digit
			return 0;
		case 2:		// 1-minute digit
			return 0;
		case 3:		// 10-minute digit
			return 0;
		case 4:		// 1-hour digit
			return 0;
		case 5:		// 10-hours digit
			return 0;
		case 6:		// 1-day digit (days in month)
			return 1;
		case 7:		// 10-days digit
			return 0;
		case 8:		// 1-month digit
			return 1;
		case 9:		// 10-months digit
			return 0;
		case 10:	// 1-year digit
			return 8;
		case 11:	// 10-years digit
			return 9;
		case 12:	// day of the week
			return 4;	// Thursday
		case 13:
			break;
		case 14:
			break;
		case 15:
			break;
	}
	
	return 0;
}

void CRTC72421::WriteRegister(unsigned reg, UINT8 data)
{
	// TO-DO: emulate me!
}

void CRTC72421::Reset(void)
{
	// nothing to do
}


/******************************************************************************
 Initialization and Shutdown
******************************************************************************/

void CRTC72421::Init(void)
{
	// this function really only exists for consistency with other device classes
}

CRTC72421::CRTC72421(void)
{	
	DebugLog("Built RTC-72421\n");
}

CRTC72421::~CRTC72421(void)
{	
	DebugLog("Destroyed RTC-72421\n");
}
