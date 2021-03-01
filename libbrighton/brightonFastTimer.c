
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

/*
 * Fast timer events.
 *
 * Devices are registered for fast timers, the callback should occur on the
 * clock cycle.
 */
#include "brightoninternals.h"

#define _BRIGHTON_FT_GO 0x0001
#define _BRIGHTON_FT_ON 0x0002

#define _BRIGHTON_FT_COUNT 128

static struct {
	struct {
		brightonWindow *win;
		int panel;
		int index;
	} ftl[_BRIGHTON_FT_COUNT];
	int tick, duty;
	int current;
	int in, out, total;
	int flags;
} brightonFTL;

static void
brightonScanTimerList(int ms)
{
	brightonEvent event;

//printf("ScanFastTimerList(%i): %i/%i\n", ms, brightonFTL.tick, brightonFTL.duty);
	if (brightonFTL.total == 0)
		return;

	if ((brightonFTL.current += ms) >= brightonFTL.tick)
	{
		event.command = BRIGHTON_FAST_TIMER;

		if (brightonFTL.flags & _BRIGHTON_FT_ON)
		{
			/* Previous led is still on */
			event.value = 0;

			brightonParamChange(
				brightonFTL.ftl[brightonFTL.out].win,
				brightonFTL.ftl[brightonFTL.out].panel,
				brightonFTL.ftl[brightonFTL.out].index,
				&event);
		}

		if (++brightonFTL.out >= brightonFTL.total)
			brightonFTL.out = 0;

		brightonFTL.current = 0;
		brightonFTL.flags |= _BRIGHTON_FT_ON;

		/*
		 * Send ON event
		 */
		event.value = 1.0;

		brightonParamChange(
			brightonFTL.ftl[brightonFTL.out].win,
			brightonFTL.ftl[brightonFTL.out].panel,
			brightonFTL.ftl[brightonFTL.out].index,
			&event);
	} else {
		if ((brightonFTL.current >= brightonFTL.duty)
			&& (brightonFTL.flags & _BRIGHTON_FT_ON))
		{
			brightonFTL.flags &= ~_BRIGHTON_FT_ON;
			/*
			 * Send OFF event
			 */
			event.command = BRIGHTON_FAST_TIMER;
			event.value = 0;

			brightonParamChange(
				brightonFTL.ftl[brightonFTL.out].win,
				brightonFTL.ftl[brightonFTL.out].panel,
				brightonFTL.ftl[brightonFTL.out].index,
				&event);
		}
	}
}

static int
brightonFTRegister(brightonWindow *win, int panel, int index)
{
//printf("Req: %i %i/%i\n", brightonFTL.in, panel, index);

	if (brightonFTL.in >= _BRIGHTON_FT_COUNT)
		return(-1);

	brightonFTL.ftl[brightonFTL.in].win = win;
	brightonFTL.ftl[brightonFTL.in].panel = panel;
	brightonFTL.ftl[brightonFTL.in++].index = index;

	return(++brightonFTL.total);
}

static int
brightonFTDegister(brightonWindow *win, int panel, int index, int id)
{
	brightonFTL.flags = 0;
	brightonFTL.in = 0;
	brightonFTL.out = 0;
	brightonFTL.total = 0;
	brightonFTL.current = 0;

	return(-1);
}

int
brightonFastTimer(brightonWindow *win, int panel, int index, int command, int ms)
{
//printf("brightonFastTimer(%i, %i)\n", command, ms);
	switch (command) {
		case BRIGHTON_FT_CLOCK:
			if (brightonFTL.flags & _BRIGHTON_FT_GO)
				brightonScanTimerList(ms);
			break;
		case BRIGHTON_FT_TICKTIME:
			brightonFTL.tick = ms;
			break;
		case BRIGHTON_FT_DUTYCYCLE:
			brightonFTL.duty = ms;
			break;
		case BRIGHTON_FT_START:
			if (brightonFTL.flags & _BRIGHTON_FT_GO)
				brightonFTL.flags &= ~_BRIGHTON_FT_GO;
			else
				brightonFTL.flags |= _BRIGHTON_FT_GO;
			break;
		case BRIGHTON_FT_STOP:
		default:
			brightonFTL.flags &= ~_BRIGHTON_FT_GO;
			brightonFTL.out = 0;
			brightonFTL.current = 0;
			break;
		case BRIGHTON_FT_REQ:
			return(brightonFTRegister(win, panel, index));
		case BRIGHTON_FT_CANCEL:
			return(brightonFTDegister(win, panel, index, command));
		case BRIGHTON_FT_STEP:
			break;
	}

	return(0);
}

