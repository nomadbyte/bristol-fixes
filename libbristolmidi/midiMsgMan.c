
/*
 *  Diverse Bristol midi routines.
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

/*#define DEBUG */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/poll.h>

#include "bristol.h"
#include "bristolmidi.h"

extern int acceptConnection();
extern int bristolMidiDevRead();
extern int checkcallbacks();

int midiMode;

/*bristolMidiMsg msg; */

static fd_set Input_fds;
static struct timeval waittime;

extern bristolMidiMain bmidi;

/*
 * This needs to go into a separate midi management interface.
 */
int
midiCheck()
{
	int i, max = 0, opens = 0, count = 0;

#ifdef DEBUG
	printf("midiCheck()\n");
#endif

	/*
	 * This should go into select() over any opened devices.
	 */
	while (~bmidi.flags & BRISTOL_MIDI_TERMINATE)
	{
		FD_ZERO(&Input_fds);
		count = 0, max = 0;

		for (i = 0; i < BRISTOL_MIDI_DEVCOUNT; i++)
			if (bmidi.dev[i].fd > 0)
			{
				/*
				 * We will poll for the control socket, and select for the
				 * rest.
				 */
				FD_SET(bmidi.dev[i].fd, &Input_fds);
				if (bmidi.dev[i].fd > max)
					max = bmidi.dev[i].fd;
				count++;
			}

		if (count == 0) {
			usleep(100000);
			continue;
		}

		/*
		 * We could wait forever, but use 1 second for now.
		 */
		waittime.tv_sec = 1;
		waittime.tv_usec = 0;

		if (select(max + 1, &Input_fds, NULL, NULL, &waittime) > 0)
		{
			for (i = 0; i < BRISTOL_MIDI_DEVCOUNT; i++)
			{
				if ((bmidi.dev[i].fd > 0) &&
					(FD_ISSET(bmidi.dev[i].fd, &Input_fds)))
				{
					if (bmidi.dev[i].flags & BRISTOL_ACCEPT_SOCKET)
					{
						if (acceptConnection(i) >= 0)
							opens++;
					} else {
						if (bristolMidiDevRead(i, &bmidi.dev[i].msg) < 0)
						{
							if (--opens == 0)
							{
								/*
								 * This should be an optional flag.
								 */
								if (bmidi.flags & BRISTOL_MIDI_WAIT)
								{
									printf("Last open conn, exiting\n");
									_exit(0);
								}
							}

							/*
							 * The next issue we should take care of is the
							 * fact that this link has closed. If we had a
							 * synth active on this link it will hang around.
							 * Unfortunately we can't really do this since
							 * there could be multiple registrations for the
							 * given channel. Definately for future study. If
							 * in doubt, use the '-T' flag to bristol to have
							 * it terminate on last close.
							bmidi.dev[i].msg.command = MIDI_SYSTEM;
							bmidi.dev[i].msg.channel = bmidi.dev[i].lastchan;

							bmidi.dev[i].msg.params.bristol.SysID = 83;
							bmidi.dev[i].msg.params.bristol.L = 76;
							bmidi.dev[i].msg.params.bristol.a = 97;
							bmidi.dev[i].msg.params.bristol.b = 98;
							bmidi.dev[i].msg.params.bristol.msgLen
								= sizeof(bristolMsg);

							bmidi.dev[i].msg.params.bristol.msgType =
								MSG_TYPE_PARAM;
							bmidi.dev[i].msg.params.bristol.operator = 127;
							bmidi.dev[i].msg.params.bristol.controller = 0;
							bmidi.dev[i].msg.params.bristol.valueLSB =
								BRISTOL_EXIT_ALGO & 0x7f;
							bmidi.dev[i].msg.params.bristol.valueMSB =
								(BRISTOL_EXIT_ALGO >> 7) & 0x7f;
							bmidi.dev[i].msg.params.bristol.from = i;
							 */

							FD_CLR(bmidi.dev[i].fd, &Input_fds);
							close(bmidi.dev[i].fd);
							bmidi.dev[i].fd = -1;
							bmidi.dev[i].state = -1;

							//checkcallbacks(&bmidi.dev[i].msg);
						}
					}
				}
			}
		}
	}

	return(0);
}

