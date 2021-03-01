
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

/*
 * This code should open the midi device (working with ALSA raw midi only for
 * the moment (9/11/01)), and read data from it. Not sure how it will be read,
 * either buffers, events, or perhaps just raw data. At some point in the 
 * development this will become a separate thread in the synth code.
 */

/*#define DEBUG */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bristolmidi.h"

extern bristolMidiMain bmidi;

/*
 * This is the OSS code.
 */
int
bristolMidiOSSOpen(char *devname, int flags, int chan, int messages,
int (*callback)(), void *param, int dev, int handle)
{
#ifdef DEBUG
	printf("bristolMidiOSSOpen(%i, %i)\n", dev, handle);
#endif

	if ((bmidi.dev[dev].fd = open(devname, O_RDWR)) < 0)
	{
		printf("Could not open OSS midi interface\n");
		return(BRISTOL_MIDI_DRIVER);
	}

	/*
	 * We use ALSA rawmidi flags to ensure we lock into the read algorithm.
	 * Since these are raw interfaces we use a single receiver and parser.
	 */
	bmidi.dev[dev].flags = BRISTOL_CONN_OSSMIDI;

	return(handle);
}

int
bristolMidiOSSClose(int handle)
{
#ifdef DEBUG
	printf("bristolMidiOSSClose()\n");
#endif

	close(bmidi.dev[bmidi.handle[handle].dev].fd);

	return(BRISTOL_MIDI_OK);
}

int
bristolMidiOSSRead(void *buffer, int size)
{
#ifdef DEBUG
	printf("bristolMidiOSSRead()\n");
#endif

	printf("libbristolmidi - should not be in OSS read routines\n");

	return(-1);
}

