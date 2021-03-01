
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

#define MIXER_VERSION 0

#define MAX_CHAN_COUNT 60
#define MAX_BUS_COUNT 16
#define MAX_VBUS_COUNT 16

#define CHAN_COUNT 16 /* This should be a parameter to the mixer. */

#define DEF_BUS_COUNT 8 /* This should be a parameter to the mixer. */
#define DEF_VBUS_COUNT 8 /* This should be a parameter to the mixer. */

#define FXP_COUNT 30
#define BUS_COUNT (FXP_COUNT * DEF_BUS_COUNT + 12)

#define MASTER_COUNT 17
#define FUNCTION_COUNT 33
#define FIRST_DEV 0
#define PARAM_COUNT 80
#define MEM_COUNT 512 /* This is memory bytes per channel, for expansion */
#define ACTIVE_DEVS \
	(MEM_COUNT * MAX_CHAN_COUNT + BUS_COUNT + FUNCTION_COUNT + FIRST_DEV)

#define DISPLAY_DEV 0
#define DISPLAY_PANEL 7

#define FUNCTION_PANEL 2
#define VBUS_PANEL FUNCTION_PANEL
#define BUS_PANEL (FUNCTION_PANEL + 1)
#define CHAN_PANEL (BUS_PANEL + 1)

#define DEVICE_COUNT (ACTIVE_DEVS)

#define MM_INIT 0
#define MM_SAVE 1
#define MM_LOAD 2
#define MM_GET 3
#define MM_SET 4

#define MM_GETLIST 0

#define MM_S_GLOBAL 0
#define MM_S_BUS 1
#define MM_S_VBUS 2
#define MM_S_CHAN 2

