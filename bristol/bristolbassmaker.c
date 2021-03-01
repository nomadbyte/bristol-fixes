
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

/*#define DEBUG */

#include "bristol.h"

int
bassmakerGlobalController(Baudio *baudio, u_char controller,
u_char operator, float value)
{
	return(0);
}

int
operateOneBassMakerVoice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	/*
	 * The bass maker is a GUI space sequencer, the engine component really 
	 * does nothing and may not even get requested to start with. This is 
	 * held as a stub though since within a single engine there are some
	 * potentially interesting stuff that could be done here taking lead from
	 * the GUI.
	 */
	voice->flags |= BRISTOL_KEYDONE;
	return(0);
}

int
destroyOneBassMaker(audioMain *audiomain, Baudio *baudio)
{
	printf("removing one bassmaker\n");
	return(0);
}

int
bristolBassMakerInit(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising (empty) bassmaker\n");

	baudio->soundCount = 1; /* Number of operators in this bassmaker voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * BassMaker oscillator
	 */
	initSoundAlgo(17, 0, baudio, audiomain, baudio->sound);

	baudio->param = bassmakerGlobalController;
	baudio->destroy = destroyOneBassMaker;
	baudio->operate = operateOneBassMakerVoice;

	return(0);
}

