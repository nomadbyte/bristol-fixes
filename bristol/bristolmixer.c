
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
#include "bristolmm.h"
#include "bristolmixer.h"

/*
 * This will control the global values for the mixing operation on request from
 * the mixer GUI.
 */
int
mixerController(Baudio *baudio, u_char operator,
u_char controller, float value)
{
#ifdef DEBUG
	printf("bristolMixerControl(%i, %i, %f)\n", operator, controller, value);
#endif

	/*
	 * Should have loads of operators here, for the tracks, busses and groups.
	 */

	if (operator != 126)
		return(0);

	switch (controller) {
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 6:
			break;
		case 7:
			break;
		case 8:
			break;
		case 9:
			break;
		case 10:
			break;
		case 11:
			break;
		case 12:
			break;
		case 13:
			break;
		case 14:
			break;
		case 15:
			break;
		case 16:
			break;
		case 17:
			break;
		case 18:
			break;
		case 19:
			break;
		case 20:
			break;
		case 21:
			break;
		case 22:
			break;
		case 23:
			break;
	}

	return(0);
}

int
operateMixerPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
#ifdef DEBUG
	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG5)
		printf("operateMixerPreops(%x, %x, %x) %i\n",
			baudio, voice, startbuf, baudio->cvoices);
#endif
	return(0);
}

int
operateOneMixer(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
/*
printf("operateOneMixer(%x, %x, %x) %i\n",
	baudio, voice, startbuf, baudio->cvoices);
*/
	return(0);
}

static int
bristolMixerDestroy(audioMain *audiomain, Baudio *baudio)
{
printf("removing one mixer\n");
	return(0);
}

/*
 * Mixer initialisation is going to be a special case. This is not a synth
 * as the others, but an integral function of the engine. This link is required
 * to control that mixing function but the actual mixing should be skipped until
 * the mixer GUI has linked up, otherwise it does not make a lot of sense.
 *
 * Should consider the implications of multithreading here such that we can
 * allow for contention for certain tracks? That might be quite hard and as
 * such we may end up with a mixing thread and multiple synth threads.
 */
int
bristolMixerInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one mixer\n");
	/*
	 * Number of operators in this voice. The mixer does not actually need 
	 * these operators, but we may want to have pre and post ops for any given
	 * audio frame, and that fits well here and as such we need a dummy operator.
	 */
	baudio->soundCount = 1;

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * This should be the dummy.
	 */
	initSoundAlgo(8, 0, baudio, audiomain, baudio->sound);

/*
	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG3)
		printf("malloced sound: %x\n", baudio->sound);
*/

	baudio->param = mixerController;
	baudio->destroy = bristolMixerDestroy;
	baudio->operate = operateOneMixer;
	baudio->preops = operateMixerPreops;
	return(0);
}

