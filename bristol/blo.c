
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
 * Bandwidth limited oscillator code. This is called to generate the waveforms
 * for bandwidth limited complex waves into the global buffer space that can
 * then be referenced by the diverse oscillators used throughout the engine.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "bristolblo.h"

static int init = 1;

float blosine[BRISTOL_BLO_SIZE];
float blocosine[BRISTOL_BLO_SIZE];
float blosquare[BRISTOL_BLO_SIZE];
float bloramp[BRISTOL_BLO_SIZE];
float blosaw[BRISTOL_BLO_SIZE];
float blotriangle[BRISTOL_BLO_SIZE];
float blopulse[BRISTOL_BLO_SIZE];

/*
 * Generate the waveforms to the given harmonic reference size. The code could
 * be optimised however it is really only likely to ever be called once at
 * startup or 'intermittently' whilst programmming a synth (depending on the
 * oscillator implementation - most use a private cache stuffed at init time).
 */
void
generateBLOwaveforms(int harmonics, float gn, int cutin, float cutoff, int min,
float fraction, int samplerate)
{
	int i, j, k;
	float gain;

	blo.harmonics = harmonics;
	blo.cutin = cutin;
	blo.cutoff = cutoff;
	blo.min = min;
	blo.samplerate = samplerate;
	blo.fraction = fraction * 1024;

	printf("generate bandlimited waveforms(%i, %2.0f, %i, %1.2f, %1.2f, %i)\n",
		harmonics, gn, cutin, cutoff, fraction, samplerate);

	if (harmonics == 0) {
		blo.flags &= ~BRISTOL_BLO;
		return;
	} else
		blo.flags |= BRISTOL_BLO;

	/*
	 * See if we need to stuff our base waves.
	 */
	if (init) {
		init = 0;

		for (i = 0; i < BRISTOL_BLO_SIZE; i++)
		{
			blosine[i] = sinf(2 * M_PI * ((float) i) / BRISTOL_BLO_SIZE) * gn;
			blocosine[i] = cosf(2 * M_PI * ((float) i) / BRISTOL_BLO_SIZE) * gn;
		}
	}

	/*
	memset(blosquare, 0, BRISTOL_BLO_SIZE * sizeof(float));
	memset(bloramp, 0, BRISTOL_BLO_SIZE * sizeof(float));
	memset(blotriangle, 0, BRISTOL_BLO_SIZE * sizeof(float));
	 */
	for (i = 0; i < BRISTOL_BLO_SIZE; i++)
		blosquare[i] = bloramp[i] = blotriangle[i] = 0;

	/*
	 * The square wave:
	 *	sum all odd sine harmonics, gain is reciprocal of harmonic.
	 */
	for (j = 1; j <= harmonics; j++)
	{
		if ((j & 0x01) == 0)
			continue;

		k = 0; gain = 1 / ((float) j);

		for (i = 0; i < BRISTOL_BLO_SIZE; i++)
		{
			blosquare[i] += blosine[k] * gain;
			if ((k += j) >= BRISTOL_BLO_SIZE)
				k -= BRISTOL_BLO_SIZE;
		}
	}
	/* Pulse */
	for (j = 1; j <= harmonics; j++)
	{
		k = 0; gain = 1 / ((float) j);

		if (j & 0x01)
		{
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
			{
				blopulse[i] += (blosine[k] - blosine[(k + 256) & 1023]) * gain;
				if ((k += j) >= BRISTOL_BLO_SIZE)
					k -= BRISTOL_BLO_SIZE;
			}
		} else {
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
			{
				blopulse[i] -= (blosine[k] + blosine[(k + 256) & 1023]) * gain;
				if ((k += j) >= BRISTOL_BLO_SIZE)
					k -= BRISTOL_BLO_SIZE;
			}
		}
	}

	/*
	 * Ramp and saw, they will be inverted but will only be noticeable when
	 * used as LFO. This gives audible crackle at low harmonic counts:
	 *
	 *	sum all odd and subtract all even harmonics, gain is reciprocal of h
	 */
	for (j = 1; j <= harmonics; j++)
	{
		k = 0; gain = 1 / ((float) j);

		if (j & 0x01)
		{
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
			{
				blosaw[i] += blosine[k] * gain;
				if ((k += j) >= BRISTOL_BLO_SIZE)
					k -= BRISTOL_BLO_SIZE;
			}
		} else {
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
			{
				blosaw[i] -= blosine[k] * gain;
				if ((k += j) >= BRISTOL_BLO_SIZE)
					k -= BRISTOL_BLO_SIZE;
			}
		}
	}

	/* Invert this for the ramp */
 	for (i = 0; i < BRISTOL_BLO_SIZE; i++)
		bloramp[i] = -blosaw[i];

	/*
	 * Tri:
	 * Sum odd cosine harmonics, gain is reciprocal of square of harmonic
	 */
	k = 0; gain = 1.0;
	for (i = 0; i < BRISTOL_BLO_SIZE; i++)
		blotriangle[i] = blosine[i];

	for (j = 3; j <= harmonics; j+=2)
	{
		k = 0;
		gain = 1.0 / ((float) (j * j));

		for (i = 0; i < BRISTOL_BLO_SIZE; i++)
		{
			blotriangle[i] += blocosine[k] * gain;
			if ((k += j) >= BRISTOL_BLO_SIZE)
				k -= BRISTOL_BLO_SIZE;
		}
	}

}

int
bristolBLOcheck(float step)
{
	if ((~blo.flags & BRISTOL_BLO) || (blo.harmonics == 0))
		return(0);
	return((step*blo.harmonics) > blo.fraction);
}

/*
 * This is the runtime equivalent
 */

void
generateBLOwaveformF(float step, float *dst, int wf)
{
	float gain;
	int i, j, k;
	int harmonics = blo.harmonics;

	/*
	 * We need to see if key is above the cutin. If so, rebuild the waveform
	 * with rolling off harmonics.
	 */
	if ((~blo.flags & BRISTOL_BLO) || (blo.harmonics == 0))
		return;

//printf("generateBLOwaveformF(%i, %p, %i)\n", blo.cutin, dst, harmonics);

	switch (wf) {
		case BLO_PULSE:
			// Difference of two ramps.
			for (j = 1; j <= harmonics; j++)
			{
				k = 0; gain = 1 / ((float) j);
				if ((step * j) > blo.fraction) break;

				if (j & 0x01)
				{
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] +=
							(blosine[k] - blosine[(k + 256) & 1023]) * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				} else {
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] -=
							(blosine[k] + blosine[(k + 256) & 1023]) * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				}
			}
			return;
		case BLO_SQUARE:
			/*
			 * The square wave:
			 *	sum all odd sine harmonics, gain is reciprocal of harmonic.
			 */
			for (j = 1; j <= harmonics; j++)
			{
				if ((j & 0x01) == 0)
					continue;

				if ((step * j) > blo.fraction) return;

				k = 0; gain = 1 / ((float) j);

				for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				{
					dst[i] += blosine[k] * gain;
					if ((k += j) >= BRISTOL_BLO_SIZE)
						k -= BRISTOL_BLO_SIZE;
				}
			}
			return;
		case  BLO_RAMP:
			/*
			 * Ramp and saw, they will be inverted but will only be noticeable
			 * when LFO. This gives audible crackle at low harmonic counts:
			 *
			 *	sum all odd and subtract all even harmonics,
			 *	gain is reciprocal of h
			 */
			for (j = 1; j <= harmonics; j++)
			{
				k = 0; gain = 1 / ((float) j);
				if ((step * ((float) j)) > blo.fraction) break;

				if (j & 0x01)
				{
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] += blosine[k] * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				} else {
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] -= blosine[k] * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				}
			}
			return;
		case  BLO_SAW:
			for (j = 1; j <= harmonics; j++)
			{
				k = 0; gain = -1 / ((float) j);
				if ((step * j) > blo.fraction) return;

				if (j & 0x01)
				{
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] += blosine[k] * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				} else {
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] -= blosine[k] * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				}
			}
			return;
		case  BLO_TRI:
			/*
			 * Tri:
			 * Sum odd cosine harmonics, gain is reciprocal of square of harmonic
			 */
			k = 0; gain = 1.0;
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				dst[i] = blosine[i];

			for (j = 3; j <= harmonics; j+=2)
			{
				if ((step * j) > 1024 * blo.fraction) return;
				k = 0;
				gain = 1.0 / ((float) (j * j));

				for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				{
					dst[i] += blocosine[k] * gain;
					if ((k += j) >= BRISTOL_BLO_SIZE)
						k -= BRISTOL_BLO_SIZE;
				}
			}
			return;
		case  BLO_SINE:
			gain = 1.0;
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				dst[i] += blosine[i] * gain;
			return;
		case  BLO_COSINE:
			gain = 1.0;
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				dst[i] += blocosine[i] * gain;
			return;
	}
}

/*
 * No longer used
 */
void
generateBLOwaveformZ(int key, float *dst, float gn, int wf)
{
	float gain, Key;
	int i, j, k;
	int harmonics;

	/*
	 * We need to see if key is above the cutin. If so, rebuild the waveform
	 * with rolling off harmonics.
	 */
	if ((~blo.flags & BRISTOL_BLO) || (blo.harmonics == 0) || (gn <= 0)
		|| ((key -= blo.cutin) <= 0))
		return;

	Key = key;

	/*
	if ((harmonics = blo.harmonics - (key - blo.cutin) * 3 < 5? 5:
		(int) (blo.harmonics - (key - 95) * 3)) > blo.harmonics)
		harmonics = blo.harmonics;
	 *
	 * We want a roll off above the cutoff.
	 */
	if ((harmonics = (blo.harmonics - Key * blo.cutoff) < blo.min? blo.min:
		(int) (blo.harmonics - (Key * blo.cutoff))) > blo.harmonics)
		harmonics = blo.harmonics;

	/* printf("generateBLOwaveform(%i, %i, %i)\n",key, blo.cutin, harmonics); */

	switch (wf) {
		case BLO_SQUARE:
			/*
			 * The square wave:
			 *	sum all odd sine harmonics, gain is reciprocal of harmonic.
			 */
			for (j = 1; j <= harmonics; j++)
			{
				if ((j & 0x01) == 0)
					continue;

				if ((Key * j) > 512)
					break;

				k = 0; gain = 1 / ((float) j);

				for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				{
					dst[i] += blosine[k] * gain;
					if ((k += j) >= BRISTOL_BLO_SIZE)
						k -= BRISTOL_BLO_SIZE;
				}
			}
			return;
		case  BLO_RAMP:
			/*
			 * Ramp and saw, they will be inverted but will only be noticeable
			 * when LFO. This gives audible crackle at low harmonic counts:
			 *
			 *	sum all odd and subtract all even harmonics,
			 *	gain is reciprocal of h
			 */
			for (j = 1; j <= harmonics; j++)
			{
				k = 0; gain = 1 / ((float) j);

				if (j & 0x01)
				{
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] += blosine[k] * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				} else {
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] -= blosine[k] * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				}
			}
			return;
		case  BLO_SAW:
			for (j = 1; j <= harmonics; j++)
			{
				k = 0; gain = -1 / ((float) j);

				if (j & 0x01)
				{
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] += blosine[k] * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				} else {
					for (i = 0; i < BRISTOL_BLO_SIZE; i++)
					{
						dst[i] -= blosine[k] * gain;
						if ((k += j) >= BRISTOL_BLO_SIZE)
							k -= BRISTOL_BLO_SIZE;
					}
				}
			}
			return;
		case  BLO_TRI:
			/*
			 * Tri:
			 * Sum odd cosine harmonics, gain is reciprocal of square of harmonic
			 */
			k = 0; gain = 1.0;
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				dst[i] = blosine[i];

			for (j = 3; j <= harmonics; j+=2)
			{
				k = 0;
				gain = 1.0 / ((float) (j * j));

				for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				{
					dst[i] += blocosine[k] * gain;
					if ((k += j) >= BRISTOL_BLO_SIZE)
						k -= BRISTOL_BLO_SIZE;
				}
			}
			return;
		case  BLO_SINE:
			gain = 1.0;
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				dst[i] += blosine[i] * gain;
			return;
		case  BLO_COSINE:
			gain = 1.0;
			for (i = 0; i < BRISTOL_BLO_SIZE; i++)
				dst[i] += blocosine[i] * gain;
			return;
	}
}

