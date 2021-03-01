
/*
 *  Diverse Bristol audio routines.
 *  Copyright (c) by Nick Copeland <nickycopeland@hotmail.com> 1996,2012
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef BRIGHTONLEDSTATES_H
#define BRIGHTONLEDSTATES_H

#define BRIGHTON_LED_OFF			0x0000
#define BRIGHTON_LED_RED			0x0001
#define BRIGHTON_LED_GREEN			0x0002
#define BRIGHTON_LED_YELLOW			0x0003
#define BRIGHTON_LED_BLUE			0x0004
#define BRIGHTON_LED_MASK			0x000F
#define BRIGHTON_LED_FLASH			0x0010
#define BRIGHTON_LED_FLASHING		0x0020
#define BRIGHTON_LED_FLASH_ON		0x0040
#define BRIGHTON_LED_FLASH_FAST		0x0080

#define BRIGHTON_LED_RED_FLASHING		(BRIGHTON_LED_RED|BRIGHTON_LED_FLASH)
#define BRIGHTON_LED_GREEN_FLASHING		(BRIGHTON_LED_GREEN|BRIGHTON_LED_FLASH)
#define BRIGHTON_LED_YELLOW_FLASHING	(BRIGHTON_LED_YELLOW|BRIGHTON_LED_FLASH)

#endif /* BRIGHTONLEDSTATES_H */

