
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
 * This code should open the midi device (working with ALSA raw midi only for
 * the moment (9/11/01)), and read data from it. Not sure how it will be read,
 * either buffers, events, or perhaps just raw data. At some point in the 
 * development this will become a separate thread in the synth code.
 */

/*
 * This should go into a library, is used from various places.
 */
void
bufmerge(register float *src, register float gain1, register float *dst,
	register float gain2, register int size)
{
	float *buf3 = dst;

	for (;size > 0; size-=16)
	{
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
		*dst++ = *buf3++ * gain2 + *src++ * gain1;
	}
	/*
	 * Correctly speaking we should check here to make sure that size is zero
	 * and cater for the last 'n' samples however I think we can just take a
	 * minimum count of 16 and take the rest in powers of two.
	 */
}

void
bufadd(register float *buf1, register float add, register int size)
{
	for (;size > 0; size-=16)
	{
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
		*buf1++ += add;
	}
}

void
bufset(register float *buf1, register float set, register int size)
{
	for (;size > 0; size-=16)
	{
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
		*buf1++ = set;
	}
}

