
/*
 *  Diverse SLab audio routines.
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


#if (BRISTOL_HAS_OSS == 1)
/*
 * This may want to be /usr/lib/oss/include/sys/soundcard.h if someone has
 * purchased the 4Front drivers?
 */
#include <sys/soundcard.h>
#endif

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

/*
 * Audio device structure format definitions.
 */
#include "slabrevisions.h"
#include "slabaudiodev.h"
#include "bristol.h"

int newInitAudioDevice2(duplexDev *, int, int);
static void checkAudioCaps2(duplexDev *, int, int);

int
ossAudioInit(audioDev, devID, fragSize)
duplexDev *audioDev;
int devID;
{
#if (BRISTOL_HAS_OSS == 1)
	int results, data = 0, mode;

	//audioDev->cflags |= SLAB_AUDIODBG;

	printf("ossAudioInit(%p, %i, %i)\n", audioDev, devID, fragSize);

	/*
	 * This is what we are going to go for.
	 */
	fragSize = 1024;

	/*
	 * Clear out this buffer.
	 */
	if (audioDev->fragBuf != (char *) NULL)
	{
		bristolfree(audioDev->fragBuf);
		audioDev->fragBuf = (char *) NULL;
	}

	checkAudioCaps2(audioDev, devID, audioDev->fd);

	if (fragSize != 0)
	{
		int data;

		switch(fragSize) {
			case 1024:
				data = 0x0040000a;
				break;
			case 2048:
				data = 0x0020000b;
				break;
			case 4096:
				data = 0x0010000c;
				break;
			case 8192:
				data = 0x0008000d;
				break;
			case 16384:
				data = 0x0004000e;
				break;
			/*
			 * This is 8192 blockSampleSize, where internally we deal with
			 * ints or floats, but the IO buffer is in shorts.
			 */
			case 32768:
			default:
				data = 0x0004000f;
				break;
		}
		if ((results = ioctl(audioDev->fd, SNDCTL_DSP_SETFRAGMENT, &data)) == 0)
		{
			if (audioDev->cflags & SLAB_AUDIODBG)
				printf("ioctl(%i, SNDCTL_DSP_SETFRAGMENT, %x)\n",
					audioDev->fd, data);
		} else
			printf("ioctl(%i, SNDCTL_DSP_SETFRAGMENT, %x): failed\n",
				audioDev->fd, data);
	}

	/*
	 * Check capabilities.
	 */
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("ioctl(%i, SNDCTL_DSP_GETCAPS, &0x%x)\n", audioDev->fd, data);

	if ((results = ioctl(audioDev->fd, SNDCTL_DSP_GETCAPS, &data)) == 0)
	{
		audioDev->genCaps = data;

		if (audioDev->cflags & SLAB_AUDIODBG)
		{
			if ((audioDev->genCaps & SNDCTL_DSP_SETTRIGGER) != 0)
				printf("Device %s does support SNDCTL_SET_TRIGGER\n",
					audioDev->devName);
			else
				printf("Device %s does NOT support SNDCTL_SET_TRIGGER\n",
					audioDev->devName);
		}

		/*
		 * Take this out, we only need to check the options here.
		 */
		if (data & DSP_CAP_DUPLEX)
		{

			if (audioDev->cflags & SLAB_AUDIODBG)
				printf("ioctl(%i, SNDCTL_DSP_SETDUPLEX, &0x%x)\n",
					audioDev->fd, data);

			if (ioctl(audioDev->fd, SNDCTL_DSP_SETDUPLEX, &data) < 0)
				printf("Failed to set Duplex\n");
			else
				printf("Set to Duplex\n");
		}
	}

	if ((audioDev->cflags & SLAB_FDUP) == 0)
		audioDev->fd2 = audioDev->fd;
	else
		audioDev->fd2 = fcntl(audioDev->fd, F_DUPFD, audioDev->fd);

	/*
	 * Set to required resolution.
	 */
	if (audioDev->cflags & (SLAB_8_BIT_OUT|SLAB_8_BIT_IN))
	{
		data = 8; /* THIS SHOULD USE THE soundcard.h DEFINITION */
	} else
		data = 16;

	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("ioctl(%i, SNDCTL_DSP_SETFMT, &%i)\n", audioDev->fd, data);

	if ((results = ioctl(audioDev->fd, SNDCTL_DSP_SETFMT, &data)) == 0)
	{
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Set audio format: %i\n", data);
	} else
		printf("Set resolution failed: %i\n", results);

	/*
	 * Set to stereo
	 */
	data = 1;
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("ioctl(%i, SNDCTL_DSP_STEREO, &%i)\n", audioDev->fd, data);
	mode = SNDCTL_DSP_STEREO;
	if ((results = ioctl(audioDev->fd, mode, &data)) == 0)
	{
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Set to stereo: %i\n", data);
	} else
		printf("Set stereo failed: %i\n", results);

	/*
	 * Set to AUDIO_RATE Hz
	 */
	data = audioDev->writeSampleRate;

	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("ioctl(%i, SNDCTL_DSP_SPEED, &%i)\n", audioDev->fd, data);

	if ((results = ioctl(audioDev->fd, SNDCTL_DSP_SPEED, &data)) == 0)
	{
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Set audio readwrite rate to %i\n", data);
	} else
		printf("Set line frequency failed: %i\n", results);

	/*
	 * Save the actual physical line rate for conversions later
	 */
	audioDev->writeSampleRate = data;

	/*
	 * Save the actual physical line rate for conversions later
	 */
	audioDev->readSampleRate = data;

	/*
	 * Finally get the output fragment size. We can have an awkward problem if
	 * this fragment size is not the same as the primary device - with sync IO
	 * where we readBuffer/writeBuffer, having different sizes very soon puts 
	 * the audio daemon in to a lock state. To overcome this, and to support
	 * eventual realtime mixing (where we will specify a small buffersize
	 * anyway) we should add a "desired buffer size" param, or have the adiod
	 * routines cater for differences.
	 */
	data = 0;
	if ((results = ioctl(audioDev->fd, SNDCTL_DSP_GETBLKSIZE, &data)) == 0)
	{
		audioDev->fragSize = data;

		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("ioctl(%i, SNDCTL_DSP_GETBLKSIZE, &0): %i bytes\n",
				audioDev->fd, audioDev->fragSize);
		audioDev->fragBuf = (char *) bristolmalloc(audioDev->fragSize);
	}
	else
		printf("Get frag size failed: %i\n", results);

#ifdef _BRISTOL_DRAIN
	for (data = 0; data < audioDev->preLoad; data++)
		results = write(audioDev->fd, audioDev->fragBuf,
			audioDev->samplecount * 2 * audioDev->channels);
#endif

#endif /* BRISTOL_HAS_OSS */

	return(0);
}

/*
 * Needs to be ALSAfied - ie, binned if we have alsa - this is GUI stuff.
 */
static void
checkAudioCaps2(audioDev, devID, fd)
duplexDev *audioDev;
int devID, fd;
{
	int i, stereodevs = 0;

#if (BRISTOL_HAS_OSS == 1)
	printf("checkAudioCaps2(%i, %i)\n", devID, fd);

    if (ioctl(fd, SOUND_MIXER_READ_STEREODEVS, &stereodevs) == -1)
	{
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Failed to get stereo capabilities: %08x\n", stereodevs);
	} else {
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Capabilities: %08x\n", stereodevs);
		audioDev->stereoCaps = stereodevs;
	}


	for (i = 1; i < 131072; i = i << 1) {
		if (i & stereodevs)
			printf("Found stereo dev %08x\n", i);
	}

	stereodevs = 0;
    if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &stereodevs) == -1)
	{
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Failed to get audio capabilities: %08x\n", stereodevs);
	} else {
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Mono Capabilities: %08x\n", stereodevs);
		audioDev->monoCaps = stereodevs;
	}


	stereodevs = 0;
    if (ioctl(fd, SOUND_MIXER_READ_RECMASK, &stereodevs) == -1)
	{
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Failed to get record capabilities: %08x\n", stereodevs);
	} else {
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Record Caps: %08x\n", stereodevs);
		audioDev->recordCaps = stereodevs;
	}

#endif /* BRISTOL_HAS_OSS */
}

