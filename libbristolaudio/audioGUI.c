
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static char *SLAB_CONVERT_LABELS[32] =	\
	{"Master", "Bass", "Treble", "FM", "PCM", "PC Speaker", "Line", "MIC", \
	"CD", "Mix  ", "Pcm2 ", "Rec  ", "Input Gain", "OGain", "none", "none", \
	"none", "none", "none", "none", "none", "none", (char *) NULL};

extern int setAudioOSSparam(duplexDev *, int, int, int, int);
extern int checkAudioOSScaps(duplexDev *, int, int);
extern char * getOSSName(int);
extern int getOSSRecordability();
extern int getOSSCapByName();
extern int getOSSCapability();

#if (BRISTOL_HAS_ALSA == 1)
extern int setAudioALSAparam(duplexDev *, int, char *, int, int);
extern int checkAudioALSAcaps(duplexDev *, int);
extern void closeALSAmixer(duplexDev *);
extern int openALSAmixer(duplexDev *);
extern char * getAlsaName(duplexDev *, int);
extern int validAlsaDev(duplexDev *, int);

extern int setAlsaRecordSource();
extern int getAlsaRecordability();
extern int getAlsaMutability();
extern int getAlsaStereoStatus();
extern int getAlsaValue();
extern int setAlsaValue();
extern int setAlsaMute();
extern int getAlsaDeviceName();
extern int getAlsaCapByName();
extern int getAlsaCapability();
#endif

/*
 * Setup audio parameters with device support.
 */
void
SL_setAudioDevParam2(audioDev, devID, param, left, right)
duplexDev *audioDev;
int param, devID;
short left, right;
{
	if ((devID < 0) || (devID >= MAX_DEVICES))
		return;
	if (audioDev->cflags & SLAB_NO_CONTROLS)
		return;

#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
	{
		/*
		 * These were OSS calls. We only want to interpret a few of them.
		if (param > 18)
			return;
		 */
		/*
		 * We cannot force this on ALSA, since there are likely issues the
		 * controller types - use an interpreted name.
		 */
		setAudioALSAparam(audioDev, devID, SLAB_CONVERT_LABELS[param],
			left, right);
		return;
	}
#endif

	setAudioOSSparam(audioDev, devID, param, left, right);
}

void
mixerClose(audioDev)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_ALSA == 1)
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("mixerClose()\n");

	if ((audioDev->flags & AUDIO_ALSA) != 0)
	{
		closeALSAmixer(audioDev);
	} else
#endif
	{
		if (audioDev->mixerFD > 0)
			close(audioDev->mixerFD);
	}
	audioDev->mixerFD = -1;
}

int
mixerOpen(audioDev)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_ALSA == 1)
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("mixerOpen()\n");

	if ((audioDev->flags & AUDIO_ALSA) != 0)
	{
		audioDev->monoCaps = 0;
		audioDev->stereoCaps = 0;
		audioDev->recordCaps = 0;
		return(openALSAmixer(audioDev));
	} else
#endif
	{
		if (audioDev->mixerName[0] != '\0')
		{
			audioDev->mixerFD = open(audioDev->mixerName, O_RDWR);
		}
	}
	return(audioDev->mixerFD);
}

char *
getControllerName(audioDev, controller)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return((char *) getAlsaName(audioDev, controller));
#endif

	return (char *) getOSSName(controller);
}

int
setRecordSource(audioDev, controller, position)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return setAlsaRecordSource(audioDev, controller, position);
#endif

	return -1;
}

int
getRecordability(audioDev, controller)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return getAlsaRecordability(audioDev, controller);
#endif

	return getOSSRecordability(audioDev, controller);
}

int
getMutability(audioDev, controller)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return getAlsaMutability(audioDev, controller);
#endif
	/*
	 * -3 indicates no ALSA support
	 */
	return -3;
}

int
getStereoStatus(audioDev, controller)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return getAlsaStereoStatus(audioDev, controller);
#endif
	return -1;
}

int
getValue(audioDev, controller, side)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return getAlsaValue(audioDev, controller, side);
#endif
	return -1;
}

int
setAudioValue(audioDev, controller, side, value)
duplexDev *audioDev;
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("setAudioValue(%p, %i, %i, %i)\n", audioDev, controller, side, value); 
	if (audioDev->cflags & SLAB_NO_CONTROLS)
	{
		return 0;
	}

#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return setAlsaValue(audioDev, controller, side, value);
#endif
	return -1;
}

int
setAudioMute(audioDev, controller, value)
duplexDev *audioDev;
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("setAudioMute()\n");

#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return setAlsaMute(audioDev, controller, value);
#endif
	return -1;
}

int
getAudioCapByName(audioDev, name)
duplexDev *audioDev;
char *name;
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("getAudioCapByName(%s, %s)\n", audioDev->devName, name);

#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return getAlsaCapByName(audioDev, name);
#endif
	return getOSSCapByName(audioDev, name);
}

int
getAudioCapability(audioDev, controller)
duplexDev *audioDev;
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("getAudioCapability(%s, %i)\n", audioDev->devName, controller);

#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
	{
		return getAlsaCapability(audioDev, controller);
	}
#endif
	return getOSSCapability(audioDev, controller);
}

int
validDev(audioDev, index)
duplexDev *audioDev;
{
#if (BRISTOL_HAS_ALSA == 1)
	if ((audioDev->flags & AUDIO_ALSA) != 0)
		return validAlsaDev(audioDev, index);
#endif
	return(0);
}

