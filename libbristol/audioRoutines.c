
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
#define DUPLICATE

/*
 * We need to declare a duplexDev from the SLab routines, initialise this for
 * a set of audio parameters (sampleRate, driverType, resolution, etc), and
 * then call our audio open library, also from SLab. The benefits of this are
 * the need to only maintain one audio library for any new driver release 
 * across all the audio applications. It requires a bit of work here, but the
 * benefits far outway the extra work. It does require that I import a lot of
 * structures (ie, headers) from SLab.
 */

#include "bristol.h"
#include "bristolmidi.h"
#include "engine.h"

#ifdef TEST
#include <unistd.h>
#endif

#ifdef DUPLICATE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int dupfd = -1;
#endif

static duplexDev audioDev;

extern int audioClose(duplexDev *);
extern int audioOpen(duplexDev *, int, int);
extern int audioWrite(duplexDev *, char *, int);
extern int audioRead(duplexDev *, char *, int);
extern int setAudioStart2(duplexDev *, int);

int
bristolAudioClose()
{
	return(audioClose(&audioDev));
}

int
bristolAudioOpen(char *device, int rate, int count, int flags)
{
	printf("bristolAudioOpen(%s, %i, %i, %x)\n", device, rate, count, flags);

	/*
	 * Take the audioDev, configure a full set of reasonable parameters as
	 * required by the libslabaudio, and call audioOpen with this device.
	 */
	audioDev.channels = 2;
	audioDev.samplecount = count;

	audioDev.writeSampleRate = rate;
	audioDev.devID = 0;
	if ((audioDev.preLoad = flags & 0x00ff) == 0)
		audioDev.preLoad = 4;
	audioDev.fd2 = -1;
	/*
	 * need to keep the as well, and need to reference the format to make sure
	 * we get the correct buffer size. For now only shorts on output.
	 *
	 * Can this warning, multichannel will be handled with Jack
#warning Fix buf calc for formats and channels
	 */
	audioDev.fragSize = count * sizeof(short) * audioDev.channels;
	audioDev.fragBuf = 0;
	audioDev.OSegmentSize = audioDev.fragSize;
#if (BRISTOL_HAS_ALSA == 1)
	if (flags & BRISTOL_ALSA)
		audioDev.siflags = AUDIO_ALSA;
#endif
#ifdef BRISTOL_PA
	if (flags & BRISTOL_PULSE_S)
		audioDev.siflags = AUDIO_PULSE;
#endif
	audioDev.cflags |= SLAB_SUBFRAGMENT;
	audioDev.cflags |= SLAB_AUDIODBG;

	sprintf(audioDev.devName, "%s", device);
	audioDev.mixerName[0] = '\0';

#ifdef TEST
	audioDev.fragBuf = (char *) bristolmalloc(audioDev.fragSize);
	return(audioDev.fragSize);
#endif

	audioDev.flags = SLAB_FULL_DUPLEX;

	if (flags & BRISTOL_DUMMY)
		audioDev.flags |= AUDIO_DUMMY;

	if (audioOpen(&audioDev, 0, SLAB_ORDWR) < 0)
		return(-1);

	printf(
		"opened audio device with a fragment size of %i, buffer %p, fd %i/%i\n",
		audioDev.fragSize, audioDev.fragBuf, audioDev.fd, audioDev.fd2);

	return(audioDev.fragSize);
}

int
bristolAudioStart()
{
	return(setAudioStart2(&audioDev, 0));
}

static short accum = 0;

int
bristolAudioWrite(register float *buf, register int count)
{
	register short *audioBuf;
	register int clipped = 0, result, Count = count;

	if (audioDev.flags & SLAB_AUDIODBG2)
		printf("bristolAudioWrite(%p, %i), %i\n",
			buf, count, audioDev.samplecount);

	/*
	 * Based on the assumption that we are always going to be working with
	 * floats, and the output device has a given format (assume 16 bit for now)
	 * then convert the buffer of data.
	 */
	audioBuf = (short *) audioDev.fragBuf;

	/*
	 * sample count is actually frame count, ie, this is actually a sample from
	 * each channel. As such, it looks like count is wrong. It is correct.
	 */
	for (; Count > 0; Count-=4)
	{
		/*
		 * This is a conversion routine from the floats used by bristol, and 
		 * most of the code is for clipchecking.
		 */
		*(audioBuf++) = (short) (*buf > 32767?32767:*buf < -32767?-32767:*buf);
		if ((*buf > 32767) || (*buf < -32768)) clipped = 1;
		buf++;
		*(audioBuf++) = (short) (*buf > 32767?32767:*buf < -32767?-32767:*buf);
		buf++;
		*(audioBuf++) = (short) (*buf > 32767?32767:*buf < -32767?-32767:*buf);
		buf++;
		*(audioBuf++) = (short) (*buf > 32767?32767:*buf < -32767?-32767:*buf);
		if ((*buf > 32767) || (*buf < -32768)) clipped = 1;
		buf++;
		*(audioBuf++) = (short) (*buf > 32767?32767:*buf < -32767?-32767:*buf);
		buf++;
		*(audioBuf++) = (short) (*buf > 32767?32767:*buf < -32767?-32767:*buf);
		if ((*buf > 32767) || (*buf < -32768)) clipped = 1;
		buf++;
		*(audioBuf++) = (short) (*buf > 32767?32767:*buf < -32767?-32767:*buf);
		if ((*buf > 32767) || (*buf < -32768)) clipped = 1;
		buf++;
		*(audioBuf++) = (short) (*buf > 32767?32767:*buf < -32767?-32767:*buf);
		buf++;
	}

#ifdef TEST
	return(0);
#endif

	if ((result = audioWrite(&audioDev, audioDev.fragBuf, audioDev.samplecount))
		< 0)
	{
		printf("Write Failed: %i\n", result);
		/*
		 * We could get into a panic here. The device originally opened 
		 * correctly (otherwise we would assumably not be writing here. Lets
		 * just close and reopen the device, and see how things go.
		 *
		 * We should actually return a bad value, and continue, letting the
		 * calling party clear things up. This has been done.
		 */
		return(result);
	}
#ifdef DUPLICATE
	if (dupfd > 0)
	{
		register short *sbuf = (short *) audioDev.fragBuf, d;

		Count = count;

		for (; Count > 0; Count--)
			accum += *sbuf++ / 2;

		if (accum != 0)
			d = write(dupfd, audioDev.fragBuf, audioDev.fragSize);
	}
#endif
	if (clipped)
		printf("Clipping output\n");

	return(0);
}

int
bristolAudioRead(register float *buf, register int count)
{
	register short *audioBuf;
	register int count2 = count;
	register float norm = 12.0f/32768.0f;

	if (audioDev.flags & SLAB_AUDIODBG2)
		printf("bristolAudioRead(%i), %i\n", count, audioDev.samplecount);

	/*
	 * Based on the assumption that we are always going to be working with
	 * floats, and the output device has a given format (assume 16 bit for now)
	 * then convert the buffer of data.
	 */
	audioBuf = ((short *) audioDev.fragBuf) - 2;

#ifdef TEST
	{
		usleep(6000);
		return(0);
	}
#endif

	if (audioRead(&audioDev, audioDev.fragBuf, audioDev.samplecount) < 0)
	{
		printf("Read Failed: fs %i, %p\n",
			audioDev.fragSize, audioDev.fragBuf);
		return(-6);
	}

	/*
	 * Demultiplex channels.
	 *
	 * The count is samples, but data is in frames, ie, two samples for the
	 * time being. I really want this data to be in a format that is +/-12,
	 * that means we have to normalise it and we should do it here.
	 */
	for (; count > 0; count-=8)
	{
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
	}

	audioBuf = ((short *) audioDev.fragBuf) - 1;

	for (; count2 > 0; count2-=8)
	{
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
		*buf++ = ((float) *(audioBuf+=2)) * norm;
	}

	return(0);
}

/*
 * This was added for enabling audio debug
 */
int
bristolAudioOption(int option, int value)
{
	switch (option) {
		case BRISTOL_NRP_REQ_DEBUG:
			if (value == 0)
				audioDev.cflags &= ~SLAB_AUDIODBG;
			else if (value == 1)
				audioDev.cflags |= SLAB_AUDIODBG;
			else
				audioDev.cflags |= SLAB_AUDIODBG|SLAB_AUDIODBG2;
			break;
	}

	return(0);
}

