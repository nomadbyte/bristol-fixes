
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

#include "bristol.h"
#include "bristolactivesense.h"

/*
 * This code is called from midihandlers.c when ACTIVE_SENSE is recieved from
 * the GUI. Once they have been seen then every 'once in a while' this check
 * is done to ensure it is still being received.
 *
 * The check is called from the audioEngine.c preops code section.
 */
int
bristolActiveSense(audioMain *audiomain, bristolMidiMsg *msg)
{
	Baudio *baudio = findBristolAudio(audiomain->audiolist,
		msg->params.bristol.channel, 0);

	if (baudio == NULL)
		return(0);

	if ((baudio->sensecount =
		((msg->params.bristol.valueMSB << 7) + msg->params.bristol.valueLSB)
			* audiomain->samplerate / 1000) == 0)
		baudio->mixflags &= ~BRISTOL_SENSE;
	else
		/*
		 * Note that we are now receiving active sensing
		 */
		baudio->mixflags |= BRISTOL_SENSE;

	if (audiomain->debuglevel > 5)
		printf("ActiveSense %p: %i\n", baudio, baudio->sensecount);

	return(0);
}

int
bristolActiveSenseCheck(audioMain *audiomain, Baudio *baudio)
{
	if (audiomain->debuglevel > 13)
		printf("ActiveSenseCheck %p: %i\n", baudio, baudio->sensecount);

	if ((~baudio->mixflags & BRISTOL_SENSE)
		|| (baudio->sensecount == 0)
		|| (baudio->mixflags & (BRISTOL_HOLDDOWN|BRISTOL_REMOVE)))
		return(1);

	if ((baudio->sensecount -= audiomain->samplecount) > 0)
		return(1);

	if ((audiomain->atReq & BRISTOL_REQSTOP) == 0)
	{
		printf("Active Sensing detected failure of UI\n");
		return(0);
	}

	return(1);
}

