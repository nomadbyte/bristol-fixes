
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
#include <stdlib.h>
#include "bristol.h"

extern audioMain audiomain;
extern void midithreadexit();

bristolSound *
dropBristolOp(int op, bristolOP *palette[])
{
	bristolSound *newsound;
	int i = 5;

	if (palette == NULL)
		midithreadexit();

	if (palette[op] == NULL)
	{
		printf("Null palette entry %i\n", op);

		while (palette[op] == NULL) {
			sleep(1);

			if (--i == 0)
			{
				printf("Null pallet entry: unrecovered\n");
				exit(0);
			}
		}

		if (palette[0] == NULL)
			printf("Null zero palette entry\n");
		else
			printf("%s\n", palette[0]->specs->opname);

		i = 5;

		while (palette[0] == NULL) {
			sleep(1);

			if (--i == 0)
			{
				printf("Null zero pallet entry: unrecovered\n");
				exit(0);
			}
		}

		printf("recovered\n");
	}

	i = 5;

	if (palette[op]->specs == NULL)
	{
		printf("Null palette specification\n");

		while (palette[op]->specs == NULL) {
			sleep(1);

			if (--i == 0)
			{
				printf("Null zero pallet entry: unrecovered\n");
				exit(0);
			}
		}

		printf("Null palette specification, recovered\n");
	}

	/*
	 * Allocate the sound structure.
	 */
	newsound = (bristolSound *) bristolmalloc0(sizeof(bristolSound));

	newsound->name = palette[op]->specs->opname;
	newsound->index = op;
	newsound->operate = palette[op]->operate;

	/*
	 * Give it a parameter table.
	 */
	newsound->param =
		(bristolOPParams *) bristolmalloc0(sizeof(bristolOPParams));

	/*
	 * Call a reset to clean/init any params.
	 */
	palette[op]->reset(palette[op], newsound->param);

	return(newsound);
}

void
printSoundList(Baudio *baudio, bristolSound *sound[])
{
	int i;

	if ((audiomain.debuglevel & BRISTOL_DEBUG_MASK) <= BRISTOL_DEBUG3)
		return;

	for (i = 0; i < baudio->soundCount; i++)
	{
		if (sound[i] == NULL)
			return;

		printf("	name	%s\n", sound[i]->name);
		printf("	index	%i\n", sound[i]->index);
		printf("	op	%p\n", sound[i]->operate);
		printf("	param	%p\n", sound[i]->param);
		printf("	next	%i\n", sound[i]->next);
		printf("	flags	%x\n", sound[i]->flags);
	}
}

/*
 * Initialising an algorithm requires minimally that the audio thread is 
 * dormant, and preferably also the midi thread. We build a voice template
 * configuration. It may be the responsiblity of the MIDI handler to init any
 * voices we use.
 */
void
initSoundAlgo(int algo, int index, Baudio *baudio, audioMain *audiomain,
bristolSound *sound[])
{
	if (sound[index] != (bristolSound *) NULL)
	{
		if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG3)
			printf("freeing existing sound structure\n");

		freeSoundAlgo(baudio, index, sound[index]);
	}

	sound[index] = dropBristolOp(algo, audiomain->palette);
	sound[index]->next = -1;
	sound[index]->flags = BRISTOL_SOUND_START|BRISTOL_SOUND_END;

	printSoundList(baudio, sound);
}

void
freeSoundAlgo(Baudio *baudio, int algo, bristolSound *sound[])
{
	if (algo >= baudio->soundCount)
		return;

	if (sound == NULL)
		return;

	if (sound[algo] != (bristolSound *) NULL)
		freeSoundAlgo(baudio, algo + 1, sound);
	else
		return;
/*	if (sound[algo] == NULL) */
/*		return; */

	/*
	 * See if we have assigned parameters, which is normal.
	 */
	if (sound[algo]->param != NULL)
	{
		int j;

		/*
		 * And then for each parameter, see if it has assigmed any local
		 * memory.
		 */
		for (j = 0; j < BRISTOL_PARAM_COUNT; j++)
		{
			if (sound[algo]->param->param[j].mem != NULL)
				bristolfree(sound[algo]->param->param[j].mem);
			sound[algo]->param->param[j].mem = NULL;
		}

		bristolfree(sound[algo]->param);
	}

	bristolfree(sound[algo]);
	sound[algo] = NULL;
}

