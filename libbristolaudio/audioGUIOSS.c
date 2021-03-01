
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

#include <sys/ioctl.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <malloc.h>

static int setAudioOSS(int, duplexDev *, int, int, int);

#if (BRISTOL_HAS_OSS == 1)
/*
 * This may want to be /usr/lib/oss/include/sys/soundcard.h if someone has
 * purchased the 4Front drivers?
 */
#include <sys/soundcard.h>
audio_buf_info bufferInfo;
mixer_info mInfo;

static char *SLAB_SND_LABELS[SLAB_NRDEVICES] =	SOUND_DEVICE_LABELS;
#endif

/*
 * Setup audio parameters with device support.
 */
void
setAudioOSSparam(audioDev, devID, param, left, right)
duplexDev *audioDev;
int param, devID;
short left, right;
{
	if (audioDev->mixerFD > 0)
		setAudioOSS(audioDev->mixerFD, audioDev, param, left, right);
}

/*
 * Binned since alsa support was implemented.
 *
 * This is a device specific routine. We take the fd, parameter, and left
 * right values, and configure the device. These routines should be used by the
 * engine to do the actual audio horsework.
 *
 * This is actually an internal routine, it should not be called from outside
 * of this file.
 */
static int
setAudioOSS(fd, audioDev, param, valueL, valueR)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_OSS == 1)
	int value, command;

	switch(param) {
		case SLAB_REC_SRC:
		case SLAB_MM_CD:
		case SLAB_MM_MIC:
		case SLAB_MM_LINE:
		case SLAB_MM_LINE1:
		case SLAB_MM_LINE2:
		case SLAB_MM_LINE3:
		case SLAB_MM_SYNTH:
		case SLAB_MM_PCM:
		case SLAB_TREBLE:
		case SLAB_BASS:
		case SLAB_MID:
		case SLAB_INPUT_GAIN:
		case SLAB_VOL_CD:
		case SLAB_VOL_MIC:
		case SLAB_VOL_LINE:
		case SLAB_VOL_LINE1:
		case SLAB_VOL_LINE2:
		case SLAB_VOL_LINE3:
		case SLAB_VOL_SYNTH:
		case SLAB_VOL_PCM:
		default:
		/*
		 * These are all the rest, which should be from the microMixer, 
		 * scaled to 100 already.
		 */
			command = param & SLAB_MM_MASK;
			value = (valueR << 8) +
				valueL;
	}
#endif /* linux */

#ifdef DEBUG
	printf("Command: %x, value: %x\n", command, value);
#endif

#if (BRISTOL_HAS_OSS == 1)
#ifdef IOCTL_DBG
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("ioctl(%i, MIXER_WRITE(%i), %i)\n", fd, command, value);
#endif
	if (ioctl(fd, MIXER_WRITE(command), &value) == -1) {
/*
			printf("Error setting audio parameter %s\n",
				SLAB_SND_LABELS[command]);
*/
		return(0);
	}
#endif

	return(0);
}

void
checkAudioOSScaps(audioDev, devID, fd)
duplexDev *audioDev;
int devID, fd;
{
#if (BRISTOL_HAS_OSS == 1)
	int stereodevs = 0;

#ifdef DEBUG
	printf("checkAudioCaps2(%i, %i)\n", devID, fd);
#endif

    if (ioctl(fd, SOUND_MIXER_READ_STEREODEVS, &stereodevs) == -1)
	{
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
#endif
			printf("Failed to get stereo capabilities: %08x\n", stereodevs);
	} else {
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Capabilities: %08x\n", stereodevs);
#endif
		audioDev->stereoCaps = stereodevs;
	}


#ifdef DEBUG
	for (i = 1; i < 131072; i = i << 1) {
		if (i & stereodevs)
			printf("Found stereo dev %08x\n", i);
	}
#endif

	stereodevs = 0;
    if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &stereodevs) == -1)
	{
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
#endif
			printf("Failed to get audio capabilities: %08x\n", stereodevs);
	} else {
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Mono Capabilities: %08x\n", stereodevs);
#endif
		audioDev->monoCaps = stereodevs;
	}


	stereodevs = 0;
    if (ioctl(fd, SOUND_MIXER_READ_RECMASK, &stereodevs) == -1)
	{
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
#endif
			printf("Failed to get record capabilities: %08x\n", stereodevs);
	} else {
#ifdef IOCTL_DBG
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("Record Caps: %08x\n", stereodevs);
#endif
		audioDev->recordCaps = stereodevs;
	}

#ifdef DEBUG
	for (i = 1; i < 131072; i = i << 1) {
		if (i & stereodevs)
			printf("Found mono dev %08x\n", i);
	}
#endif /* DEBUG */
#endif /* linux */
}

char *
getOSSName(controller)
int controller;
{
	char *name = NULL;

#if (BRISTOL_HAS_OSS == 1)
	if ((controller < 0) || (controller > SOUND_MIXER_NRDEVICES))
		return("none");

	name = SLAB_SND_LABELS[controller];
#endif
	return(name);
}

int
getOSSCapByName(audioDev, name)
duplexDev *audioDev;
char *name;
{
#if (BRISTOL_HAS_OSS == 1)
	int i;

	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
	{
		if (strcmp(SLAB_SND_LABELS[i], name) == 0)
			return i;
	}
#endif
	return(-1);
}

int
getOSSCapability(audioDev, controller)
duplexDev *audioDev;
{
	if ((audioDev->stereoCaps | audioDev->monoCaps) & (1 << controller))
		return controller;
	return(-1);
}

int
getOSSRecordability(audioDev, cont)
duplexDev *audioDev;
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("getOSSRecordability(%i, %i)\n", audioDev->devID, cont);

	if (audioDev->recordCaps & (1<<cont))
		return(1);
	/*
	 * -2 indicates no support
	 */
	return(-2);
}

