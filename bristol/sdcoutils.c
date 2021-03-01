
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
/* This is sampler oscillator code. It is not used by any synth at the moment. */

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "bristol.h"
#include "sdco.h"

int
getHint(char *file)
{
	char *index;

	if ((index = rindex(file, '.')) != NULL)
	{
		index++;

		if (strlen(index) <= 0)
			return(HINT_RAW);

		if (strcmp(index, "raw") == 0)
			return(HINT_RAW);
	}
	return(HINT_UNKNOWN);
}

int
findKey(sampleData *sd, int key, int layer, int dir)
{
	int ckey;

	if (dir == RANGE_UP)
	{
		/*
		 * If we are outside of our range, return.
		 */
		if (key == 127)
			return(-1);

		for (ckey = key + 1; ckey < 128; ckey++)
		{
			if (sd[ckey].layer[layer].wave == 0)
				continue;

			return(ckey);
		}
	} else {
		/*
		 * If we are outside of our range, return.
		 */
		if (key == 0)
			return(-1);

		for (ckey = key - 1; ckey < 128; ckey--)
		{
			if (sd[ckey].layer[layer].wave == 0)
				continue;

			return(ckey);
		}
	}

	return(-1);
}

/*
 * This is called after the sampleData structure has been filled with as many
 * samples as as specified for the voice, and will fill in any pointers for
 * data that needs to be interpolated between keys.
 *
 * Two potential methods for filling the structures. Firstly, take nearest
 * available wave, secondly take a mixture of the nearest below and nearest
 * above. The former will be better for the Rhodes since it only has a single
 * tine, and needs a purer sound. The latter would be useful for a piano, 
 * as it has multiple strings per note, and we can do multiple crosspoints
 * where the nearest piano and nearest forte are not always the same sample.
 */
int
fixWavepointers(sampleData *sd, int layer)
{
	int ckey;

	for (ckey = 0; ckey < 128; ckey++)
	{
		/*
		 * See if we have a piano wave, if not, find the upper and lower
		 * pairs.
		 */
		if (sd[ckey].layer[layer].wave == 0)
		{
			int lower, upper, target;
			double freqstep;
			/*
			 * We do not have a wave on this note. Go and find some upper and
			 * lower reference pairs, and use the nearest of them.
			 */
			lower = findKey(sd, ckey, layer, RANGE_DOWN);
			upper = findKey(sd, ckey, layer, RANGE_UP);

			/*
			 * If we have no samples on this layer, return.
			 */
			if ((lower == -1) && (upper == -1))
				return(-1);

			if (lower == -1)
			{
				target = upper;
			} else {
				if (upper == -1)
					target = lower;
				else {
					/*
					 * If we have a contention between samples, find nearest.
					 */
					if ((ckey - lower) < (upper - ckey))
						target = lower;
					else
						target = upper;
				}
			}
			sd[ckey].layer[layer].refwave = target;

			/*
			 * Since we do not have a wave loaded on this note we need to
			 * evaluate the rate at which the related sample should be
			 * played. This is 2s power twelth of the distance between the
			 * notes.
			 */
			freqstep = ckey - target;
			if (freqstep > 0)
				sd[ckey].layer[layer].sr = pow(2, freqstep / 12);
			else
				sd[ckey].layer[layer].sr = pow(2, freqstep / 12);
		} else {
			/*
			 * We have a wave on this note. Take this index, and unity gain.
			 */
			sd[ckey].layer[layer].refwave = ckey;
			/*
			 * If we have the wave then the step rate is 1.0
			 */
			sd[ckey].layer[layer].sr = 1.0;
		}
/*
printf("k %i-%i: w %08x %i %i %i %i %i %i %f %i\n",
layer, ckey, 
sd[ckey].layer[layer].wave,
sd[ckey].layer[layer].nsr,
sd[ckey].layer[layer].count,
sd[ckey].layer[layer].lnr,
sd[ckey].layer[layer].lfr,
sd[ckey].layer[layer].lt,
sd[ckey].layer[layer].refwave,
sd[ckey].layer[layer].sr,
sd[ckey].layer[layer].fade);
*/
	}
	return(0);
}

int
convertRhodes(sampleData *sd, char *file, int loc, int layer)
{
	int fd;
	short *src;
	float *dst;
	struct stat sbuf;

	if (sd[loc].layer[layer].wave != 0)
		bristolfree(sd[loc].layer[layer].wave);

	if ((fd = open(file, O_RDONLY)) < 0)
		return(-1);

	if (fstat(fd, &sbuf) < 0)
	{
		printf("Could not stat file\n");
		close(fd);
	} else
		printf("filesize is %i bytes\n", (int) sbuf.st_size);

	sd[loc].layer[layer].wave
		= (float *) bristolmalloc((sbuf.st_size / 2) * sizeof(float));

	sd[loc].layer[layer].count = sbuf.st_size / 2;
	sd[loc].layer[layer].lnr = 0;
	sd[loc].layer[layer].lfr = sbuf.st_size / 2;
	sd[loc].layer[layer].lt = LOOP_NONE;

	if (read(fd, sd[loc].layer[layer].wave, sbuf.st_size) != sbuf.st_size)
	{
		bristolfree(sd[loc].layer[layer].wave);
		close(fd);
		return(-1);
	}

	/*
	 * Get our pointers to refer to end of samples.
	 */
	src = ((short *) sd[loc].layer[layer].wave) + (sbuf.st_size / 2);
	dst = sd[loc].layer[layer].wave + (sbuf.st_size / 2);

	/*
printf("buffer %x, src %x, dst %x\n", sd[loc].layer[layer].wave, src, dst);
	 * Then converge the samples into float from back to front.
	 */
	for (; src != (short *) sd[loc].layer[layer].wave;)
		*--dst = ((float) *--src) * 0.003;

	sd[loc].layer[layer].nsr = 44100; /* must go when sampler is released */

	close(fd);

	/*
	 * Fix some table pointers for layer 0 and 1
	 */
	fixWavepointers(sd, layer);

	return(0);
}

int
freeWavemem(sampleData *sd)
{
	printf("Need to clean up the free() wave memory code\n");
	return(0);
}

int
convertWave(sampleData *sd, char *file, int type, int loc, int layer)
{
	int spec;

	/*
	 * We have been passed a sampledata array, a filename, and at least some
	 * conversion and index hints. We may not use the latter two.
	 * Minimally we open the file and load its data into the hinted location.
	 */
	/*
	 * See if the filename gives any indication of type, or if the first few
	 * bytes do.
	 */
	if ((spec = getHint(file)) != type)
		printf("file %s does not indicate specified type: %i, %i\n",
			file, spec, type);

	if (spec == HINT_UNKNOWN)
		spec = type;

	if (type == HINT_RHODES)
	{
		/*
		 * This is a predefined set of samples
		 */
		return(convertRhodes(sd, file, loc, layer));
	}

	switch(spec) {
		case HINT_RAW:
			/*
			 * Raw file. Load its contents into the specified location and
			 * configure a set of defaults for the rest of the table.
			 */
			break;
		default:
			break;
	}

	return(-1);
}

