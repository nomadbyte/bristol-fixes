
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


/*
 * These are largely device specific operations for the audio devices. Some of
 * the calls are general, but may eventually be used by several of the SLab
 * applications.
 *
 * The operations for audio control are rather ugly. The application sets up
 * a number of values in the controlBuffer, and the engine intermittanly looks
 * to see if these values are initialised. If so, it acts on the values, and
 * then clears them. The reason we have to use this indirection is that the
 * front end applications do not own the audio device, this is under control
 * of the engine.
 *
 *	Jul 28th 96 - Only LINUX code is active.
 */

/*
 * Audio device structure format definitions.
 */
#include "slabrevisions.h"
#include "slabaudiodev.h"
#include "slabcdefs.h"
#include "bristol.h"

#if (BRISTOL_HAS_ALSA == 1)
#include "slabalsadev.h"
#endif

extern int ossAudioInit(duplexDev *, int, int);
extern int alsaDevOpen(duplexDev *, int, int, int);
extern int alsaDevClose(duplexDev *);
extern int alsaDevAudioStart(duplexDev *);

#ifdef DEBUG
#include <stdio.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#ifdef SUBFRAGMENT
#include <stdlib.h>
#include <malloc.h>
#endif

#include <sys/ioctl.h>

#if (BRISTOL_HAS_OSS == 1)
/*
 * This may want to be /usr/lib/oss/include/sys/soundcard.h if someone has
 * purchased the 4Front drivers?
 */
#include <sys/soundcard.h>
audio_buf_info bufferInfo;
#endif

/*
static char *SLAB_SND_LABELS[SLAB_NRDEVICES] =	SLAB_DEVICE_LABELS;
 * Called once when the device is opened by the engine. Initialises our fd to
 * 44100 16 bits stereo operation. 
 * Changed for 2.30 (or so) to include capability to have no requests to alter
 * the default fragment size. The code will do a simplified audio init, and let\ * the audio IO daemon use the default fragment for IO, decoupled from the block
 * sample size.
 */
int
initAudioDevice2(audioDev, devID, fragSize)
duplexDev *audioDev;
int devID;
{
	/*
	 * The device is basically just opened for the first call of this routine.
	 * This is then called several times afterwards - each time we start the
	 * engine (that is a tape start, not an engine restart). This may be
	 * problematic.
	 *
	 * First set duplex (later check duplex).
	 * Set the fragments.
	 * In this order set bits, channels, speed.
	 */
#ifdef BRISTOL_PA
	if (audioDev->siflags & AUDIO_PULSE)
		return(0);
#endif
#if (BRISTOL_HAS_ALSA == 1)
	/*
	 * ALSA is one of the secondary intrusive flags. If this is set we have
	 * already configured what we want from the open request on the device.
	 */
	if (audioDev->siflags & AUDIO_ALSA)
		return(0);
#endif

	/*
	 * If we get here we need to call some OSS routine.
	 */
	return(ossAudioInit(audioDev, devID, fragSize));
}

int
setAudioStart2(audioDev, devID)
duplexDev *audioDev;
{
	int enable;

#ifdef IOCTL_DBG
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("setAudioStart2(%i)\n", devID);
#endif

	if (audioDev->flags & AUDIO_DUMMY)
		return(0);

#ifdef BRISTOL_PA
	if (audioDev->siflags & AUDIO_PULSE)
		return(0);
#endif
#if (BRISTOL_HAS_ALSA == 1)
	if (audioDev->siflags & AUDIO_ALSA)
	{
		alsaDevAudioStart(audioDev);
		return(0);
	}
#endif
	if (audioDev->fd < 0)
		return(0);

#if (BRISTOL_HAS_OSS == 1)
	if ((audioDev->genCaps & SNDCTL_DSP_SETTRIGGER) &&
		(audioDev->Command == 1))
	{
		enable = (PCM_ENABLE_OUTPUT | PCM_ENABLE_INPUT);
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("ioctl(%i, SNDCTL_DSP_SETTRIGGER, &%08x)\n",
				audioDev->fd, enable);
#endif
		ioctl(audioDev->fd, SNDCTL_DSP_SETTRIGGER, &enable);
	}
#endif
	return(0);
}

int
setAudioStop2(audioDev, devID)
duplexDev *audioDev;
{
	int enable;

#ifdef IOCTL_DBG
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("setAudioStop2(%i)\n", devID);
#endif

	if (audioDev->siflags & AUDIO_DUMMY)
		return(0);

#ifdef BRISTOL_PA
	if (audioDev->siflags & AUDIO_PULSE)
		return(0);
#endif
#if (BRISTOL_HAS_ALSA == 1)
	if (audioDev->siflags & AUDIO_ALSA)
	{
#ifdef IOCTL_DBG
		/*
		 * This is a superfluous bit of debug, but it will at least show that
		 * we are recognising the ALSA configuration status. There are no audio
		 * stop commands associated with the SLab ALSA drivers, although we
		 * could use the "pause" calls.
		 */
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("ALSA device, returning from setAudioStop2\n");
#endif
		return(0);
	}
#endif

	if (audioDev->fd < 0)
		return(0);

#if (BRISTOL_HAS_OSS == 1)
	if ((audioDev->genCaps & SNDCTL_DSP_SETTRIGGER) &&
		(audioDev->Command == 1))
	{
		enable = ~(PCM_ENABLE_OUTPUT | PCM_ENABLE_INPUT);
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("ioctl(%i, SNDCTL_DSP_SETTRIGGER, &%08x)\n",
				audioDev->fd, enable);
#endif
		ioctl(audioDev->fd, SNDCTL_DSP_SETTRIGGER, &enable);
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
		{
			ioctl(audioDev->fd, SNDCTL_DSP_GETTRIGGER, &enable);
			printf("ioctl(%i, SNDCTL_DSP_GETTRIGGER, &%08x)\n",
				audioDev->fd, enable);
		}
#endif
	}
#endif

	return(0);
}

int
audioClose(duplexDev *audioDev)
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("audioClose(%p, %i, %s)\n", audioDev, audioDev->devID,
			audioDev->devName);

	if (audioDev->flags & AUDIO_DUMMY) {
		printf("closing AUDIO_DUMMY interface\n");
		return(0);
	}

#ifdef BRISTOL_PA
	if (audioDev->siflags & AUDIO_PULSE)
	{
		pulseDevClose(audioDev);
		return(0);
	}
#endif

#if (BRISTOL_HAS_ALSA == 1)
	if (audioDev->siflags & AUDIO_ALSA)
	{
		alsaDevClose(audioDev);
		return(0);
	}
#endif

	if (audioDev->fd != -1)
	{
		close(audioDev->fd);
		audioDev->fd = -1;
	}
	if (audioDev->fd2 != -1)
	{
		close(audioDev->fd2);
		audioDev->fd2 = -1;
	}

	bristolfree(audioDev->fragBuf);
	audioDev->fragBuf = NULL;

	return(0);
}

int
audioOpen(duplexDev *audioDev, int device, int flags)
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("audioOpen(%p, %i, %i): %s\n", audioDev, device, flags,
			audioDev->devName);

	if (audioDev->flags & AUDIO_DUMMY) {
		printf("using AUDIO_DUMMY interface\n");

		if (audioDev->fragBuf)
			bristolfree(audioDev->fragBuf);

		if (audioDev->channels == 0)
			audioDev->channels = 2;

		if (audioDev->fragSize == 0)
			audioDev->fragSize = 1024;

		if (audioDev->writeSampleRate == 0)
			audioDev->writeSampleRate = 44100;
		if (audioDev->readSampleRate == 0)
			audioDev->readSampleRate = 44100;

		audioDev->fragBuf = (char *) bristolmalloc(audioDev->fragSize);

		return(10);
	}

#ifdef BRISTOL_PA
	if (audioDev->siflags & AUDIO_PULSE)
		return(pulseDevOpen(audioDev, device, flags, audioDev->fragSize));
#endif
#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->siflags & AUDIO_ALSA) && (audioDev->devName[0] != '/'))
		return(alsaDevOpen(audioDev, device, flags, audioDev->fragSize));
#endif
	/*
	 * We need to "mung" the flags.
	 */
	if (flags == SLAB_OWRONLY)
		flags = O_WRONLY;
	else if (flags == SLAB_ORDONLY)
		flags = O_RDONLY;
	else if (flags == SLAB_ORDWR)
		flags = O_RDWR;
	else {
		printf("	WHAT WERE THOSE FLAGS: %x\n", flags);
		/*
		 * We may as well take some default value if we are unsure about the
		 * request.
		 */
		flags = O_RDWR;
	}
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("flags are now %i\n", flags);
	/*
	 * Open the device.
	 */
	if ((audioDev->fd = open(audioDev->devName, flags)) < 0)
	{
		/*
		 * Device open failed. Print an error message and exit.
		 */
		printf("Failed to open audio device \"%s\", flags %i\n",
			audioDev->devName, flags);
		return(-10);
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Opened audio device \"%s\"\n", audioDev->devName);
#endif
	}

	/*
	 * We will eventually allow configurable buffer sizes. At the moment we
	 * will take the default value for the main output device, and
	 * configure that on all the other devices.
	 */
	if (audioDev->fragSize == 0)
		audioDev->fragSize = 1024;
	audioDev->flags = flags;

	bristolfree(audioDev->fragBuf);

	audioDev->fragBuf = (char *) bristolmalloc(audioDev->fragSize);

	initAudioDevice2(audioDev, device, audioDev->fragSize);

	/*
	audioDev->flags |= SLAB_AUDIODBG2;
	audioDev->cflags |= SLAB_AUDIODBG;
	*/

	return(audioDev->fd);
}

int
channelStatus(duplexDev *audioDev)
{
	return(0);
}

int
headStatus(duplexDev *audioDev)
{
	return(0);
}

