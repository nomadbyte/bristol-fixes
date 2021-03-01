
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
 * FILE:
 *	audioEngineALSA.c
 *
 * NOTES:
 *	This was taken from latency.c, and tweaked to be the device configurator
 *	for the SLab audio interface. This file will only contain the code used by
 *	the SLab audio daemon - configures the desired resolutions, fragments, etc.
 *
 *	The code for the GUI, which is only concerned with the sound source mixing
 *	interface, is separate, in audioGUIALSA.c
 *
 * CALLS:
 *	asoundlib routines.
 *
 * IS CALLED BY:
 *	If it remains as clean as I intend, it should only come from audioEngine.c
 *	if the ALSA flags are configured on this device. Oops, there ended
 *	up with two exceptions, which I could remove. audioWrite()/audioRead() are
 *	called from libslabadiod. Yes, I should change this, but I do not want yet
 *	another level of indirection for the read/write calls.
 *
 * As of 14/12/99 this uses a single duplex channel. Going to change it to use
 * up to two channels: record channel if not WRONLY, playback channel if not
 * RDONLY device. This is a fair bit of work.
 *
 * 1/3/00 - incorporated alterations for ALSA Rev 0.5.X, with lots of #ifdef's.
 * 27/11/00 - ALSA still not operational. Going back to single duplex dev.
 * 1/1/1 - ALSA operational, but issues with IO Errors after recording period.
 */

#include <unistd.h>

#include "slabrevisions.h"
#include "slabcdefs.h"
#include "slabaudiodev.h"
#include "bristol.h"

#if (BRISTOL_HAS_ALSA == 1)

#include "slabalsadev.h"

#include <alsa/pcm.h>

#include <sys/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(linux)
#include <sched.h>
#endif

#ifdef SUBFRAGMENT
#if (MACHINE == LINUX)
#include <malloc.h>
#endif
#endif

#define DEF_FRAGSIZE 1024
#define DEF_FRAGCNT 32

#include <time.h>

static int adInit = 1, dummycapture = 0;
static struct adev alsaDev[MAX_DEVICES + 1];

#define SND_PCM_OPEN_PLAYBACK SND_PCM_STREAM_PLAYBACK
#define SND_PCM_OPEN_CAPTURE SND_PCM_STREAM_CAPTURE

snd_output_t *output = NULL;

/*
 * This assumes the API will manage the memory blocks allocated to the handles,
 * so will only reset them to NULL, not free them.
 */
int
alsaDevClose(duplexDev *audioDev)
{
/*	if (audioDev->cflags & SLAB_AUDIODBG
printf("	alsaDevClose(%08x): %08x, %08x\n",
			audioDev,
			&alsaDev[audioDev->devID].chandle,
			&alsaDev[audioDev->devID].phandle);
*/

	if (dummycapture == 0)
	{
		if (alsaDev[audioDev->devID].chandle != (snd_pcm_t *) NULL)
		{
			if (audioDev->cflags & SLAB_AUDIODBG)
				printf("closing alsa capture channel\n");

			/* snd_pcm_unlink((void *) alsaDev[audioDev->devID].chandle); */
			snd_pcm_drop((void *) alsaDev[audioDev->devID].chandle);
			snd_pcm_hw_free((void *) alsaDev[audioDev->devID].chandle);
			snd_pcm_close((void *) alsaDev[audioDev->devID].chandle);

			alsaDev[audioDev->devID].chandle = (snd_pcm_t *) NULL;
		}
	}

	if (alsaDev[audioDev->devID].phandle != (snd_pcm_t *) NULL)
	{
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("closing alsa playback channel\n");
		snd_pcm_drain((void *) alsaDev[audioDev->devID].phandle);
		snd_pcm_hw_free((void *) alsaDev[audioDev->devID].phandle);
		snd_pcm_close((void *) alsaDev[audioDev->devID].phandle);

		alsaDev[audioDev->devID].phandle = (snd_pcm_t *) NULL;
	}

	audioDev->fd = audioDev->fd2 = -1;

	bristolfree(output);
	output = NULL;

	return(0);
}

int
alsaChannelConfigure(duplexDev *audioDev, snd_pcm_t **handle,
	snd_pcm_hw_params_t **h_params,
	snd_pcm_sw_params_t **s_params,
	char *dir)
{
	char *devicename;
	int err, stream;
	snd_pcm_uframes_t count;
	unsigned long fragsize;
	int nfds;
	struct pollfd *pfds;

	snd_pcm_hw_params_alloca(h_params);
	memset(*h_params, 0, sizeof(snd_pcm_hw_params));
	snd_pcm_sw_params_alloca(s_params);
	memset(*s_params, 0, sizeof(snd_pcm_hw_params));

	if (audioDev->fragSize == 0)
		audioDev->fragSize = DEF_FRAGSIZE;

	devicename = strdup(audioDev->devName);

	if (strcmp(dir, "capture") == 0)
		stream = SND_PCM_STREAM_CAPTURE;
	else
		stream = SND_PCM_STREAM_PLAYBACK;

	/*
	 * Open the device. This use of handle may cause a small memory leak when
	 * the audio interface is restarted, I don't know if I own the memory or
	 * if the library does.
	 */
	bristolfree(*handle);
	if (snd_pcm_open(handle, devicename, stream, 0) < 0)
	{
		fprintf(stderr, "Error opening PCM device %s\n", devicename);
		bristolfree(devicename);
		return(-1);
	}

	bristolfree(devicename);

	if (snd_pcm_hw_params_any(*handle, *h_params) < 0)
	{
		printf("Cound not get %s any params\n", dir);
		return(-1);
	}
	if (snd_pcm_hw_params_set_access(*handle,
		*h_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
	{
		printf("Could not set %s access methods\n", dir);
		return(-1);
	}
	if (snd_pcm_hw_params_set_format(*handle,
		*h_params, SND_PCM_FORMAT_S16_LE) < 0)
	{
		printf("Could not set %s format\n", dir);
		return(-1);
	}
	if (snd_pcm_hw_params_set_channels(*handle,
		*h_params, audioDev->channels) < 0)
	{
		printf("Could not set channels\n");
		return(-1);
	}
	if (snd_pcm_hw_params_set_rate_near(*handle, *h_params,
		(unsigned int *) &audioDev->writeSampleRate, 0) <0)
	{
		printf("Could not set %s rate to %i\n", dir,
			audioDev->writeSampleRate);
		return(-1);
	}

	/*
	 * For bristol only supporting stereo synths this interface is fine. More
	 * generally with 'n' channels this will fail however that is for future
	 * study anyway
	 *
	 * The following only works for 16bit stereo samples.
	 *
	 * Can this warning, we are going to use Jack for multichannel stuff
#warning Not compatible with alternative sample sizes or channel counts
	 */
	count = audioDev->fragSize >> 2;
	fragsize = count * audioDev->preLoad;
	if (snd_pcm_hw_params_set_period_size(*handle, *h_params, count, 0) < 0)
	{
		snd_pcm_uframes_t frames;

		printf("Could not configure %s period size\n", dir);

		snd_pcm_hw_params_get_period_size(*h_params, &frames, &stream);

		printf("period size is %i\n", (int) frames);

		return(-1);
	}
	if (snd_pcm_hw_params_set_periods( *handle, *h_params, audioDev->preLoad, 0) < 0)
	{
		printf("Could not configure %s periods\n", dir);
		return(-1);
	}

	if ((err = snd_pcm_hw_params(*handle, *h_params)) < 0)
	{
		printf("Could not set %s hardware parameters: %s\n",
			dir, snd_strerror(err));
		return(-1);
	}
	if (snd_pcm_hw_params_set_buffer_size( *handle, *h_params,
		count * audioDev->preLoad) < 0)
	{
		printf("Could not configure %s buffer size\n", dir);
		return(-1);
	}

	if (snd_pcm_sw_params_current(*handle, *s_params) < 0)
	{
		printf("Could not get %s current configuration\n", dir);
		return(-1);
	}
	if (snd_pcm_sw_params_set_start_threshold(*handle,
/*		*s_params, 0U) < 0) */
		*s_params, 0x7fffffff) < 0)
/*		*s_params, audioDev->fragSize * 1024) < 0) */
	{
		printf("Could not set %s start threshold\n", dir);
		return(-1);
	}

	if (snd_pcm_sw_params_set_stop_threshold(*handle, *s_params,
		count * audioDev->preLoad) < 0)
	{
		printf("Could not set %s stop threshold\n", dir);
		return(-1);
	}

	if (snd_pcm_sw_params_set_silence_threshold(*handle, *s_params, 0) < 0)
	{
		printf("Could not set %s stop threshold\n", dir);
		return(-1);
	}

	if (snd_pcm_sw_params_set_avail_min(*handle, *s_params, count) < 0)
	{
		printf("Could not set %s avail min\n", dir);
		return(-1);
	}
	if (snd_pcm_sw_params(*handle, *s_params) < 0)
	{
		printf("Could not configure %s software parameters\n", dir);
		return(-1);
	}

	snd_pcm_dump(*handle, output);

	nfds = snd_pcm_poll_descriptors_count(*handle);
	pfds = (struct pollfd *) bristolmalloc(sizeof (struct pollfd) * nfds);
	snd_pcm_poll_descriptors(*handle, pfds, nfds);

	if (strcmp(dir, "capture") == 0)
	{
		audioDev->fd2 = pfds[0].fd;
		if (snd_pcm_prepare(*handle) < 0)
			printf("prepare failure on capture channel\n");
	} else {
		audioDev->fd = pfds[0].fd;
		if (snd_pcm_prepare(*handle) < 0)
			printf("prepare failure on playback channel\n");
	}

	bristolfree(pfds);

	return(0);
}

/*
 * The rest of this code is for ALSA 0.9, and hopefully greater. Each new
 * minor revision introduces some serious redefinition of the interfaces.
 */
int
alsaDevOpen(duplexDev *audioDev, int devID, int flags, int fragSize)
{
	int err;

	/*
	 * This is now a function of the engine, since it is using thread priority
	 * rather than process priority scheduling.
	 */

	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("opening device %s, flags %08x\n",
			audioDev->devName, audioDev->flags);

	if (adInit)
	{
		printf("init %i bytes of device structure\n", (int) sizeof(alsaDev));
		bzero(alsaDev, sizeof(alsaDev));
		adInit = 0;
	}

	if (audioDev->channels == 0)
		audioDev->channels = 2;

	if (audioDev->writeSampleRate == 0)
		audioDev->writeSampleRate = 44100;
	if (audioDev->readSampleRate == 0)
		audioDev->readSampleRate = 44100;

	if (flags == SLAB_OWRONLY)
		flags = SLAB_OWRONLY;
	else if (flags == SLAB_ORDONLY)
		flags = SLAB_RDONLY;
	else if (flags == SLAB_ORDWR)
		flags = SLAB_OWRONLY|SLAB_RDONLY;

	bristolfree(output);
	output = NULL;
	err = snd_output_stdio_attach(&output, stdout, 0);

	/*
	 * Open and configure the playback channel.
	 */
	if (flags & SLAB_OWRONLY)
	{
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("open playback on %s, pre %i\n",
				audioDev->devName, audioDev->preLoad);

		if (alsaChannelConfigure(audioDev,
				&alsaDev[audioDev->devID].phandle,
				&alsaDev[audioDev->devID].p_params,
				&alsaDev[audioDev->devID].p_swparams,
				"playback") < 0)
			return(-1);
	}

	/*
	 * And do the same for the capture channel
	 */
	if ((flags & SLAB_RDONLY) && (strcmp(audioDev->devName, "plug:dmix") != 0))
	{
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("open capture on %s: pre %i\n",
				audioDev->devName, audioDev->preLoad);

		if (alsaChannelConfigure(audioDev,
				&alsaDev[audioDev->devID].chandle,
				&alsaDev[audioDev->devID].c_params,
				&alsaDev[audioDev->devID].c_swparams,
				"capture") < 0)
			return(-1);
	} else
		dummycapture = 1;

	bristolfree(audioDev->fragBuf);

	audioDev->fragBuf = (char *) bristolmalloc(audioDev->fragSize);

/*printf("	alsaDevOpen(%08x): %08x, %08x\n", */
/*			audioDev, */
/*			&alsaDev[audioDev->devID].chandle, */
/*			&alsaDev[audioDev->devID].phandle); */

//	snd_pcm_link(alsaDev[audioDev->devID].chandle,
//		alsaDev[audioDev->devID].phandle);

	return(0);
}

int
alsaDevAudioStart(duplexDev *audioDev)
{
#ifdef _BRISTOL_DRAIN
	int err, i;

	printf("restart audio interface %i %i\n",
		audioDev->samplecount, audioDev->preLoad);

	snd_pcm_drop(alsaDev[audioDev->devID].phandle);
	snd_pcm_prepare(alsaDev[audioDev->devID].phandle);

	for (i = 0; i < audioDev->preLoad; i++)
		snd_pcm_writei(alsaDev[audioDev->devID].phandle, audioDev->fragBuf,
			audioDev->samplecount);

	if (dummycapture == 0) {
		snd_pcm_drop(alsaDev[audioDev->devID].chandle);
		snd_pcm_prepare(alsaDev[audioDev->devID].chandle);
	}

	if ((err = snd_pcm_start(alsaDev[audioDev->devID].phandle)) < 0) {
		printf("Playback start error: %s\n", snd_strerror(err));
		return(-1);
	}

	if (dummycapture == 0)
	{
		if ((err = snd_pcm_start(alsaDev[audioDev->devID].chandle)) < 0) {
			printf("Record start error: %s\n", snd_strerror(err));
			return(-1);
		}
	}
#else
	if ((audioDev->flags == ADIOD_OUTPUT) || (audioDev->flags == ADIOD_DUPLEX))
	{
		printf("Start playback %i\n", audioDev->preLoad);
		if ((err = snd_pcm_start(alsaDev[audioDev->devID].phandle)) < 0) {
			printf("Playback start error: %s\n", snd_strerror(err));
			return(-1);
		}
	}

	if ((audioDev->flags == ADIOD_INPUT) || (audioDev->flags == ADIOD_DUPLEX))
	{
		if (dummycapture == 0)
		{
			printf("Start capture\n");
			if ((err = snd_pcm_start(alsaDev[audioDev->devID].chandle)) < 0) {
				printf("Record start error: %s\n", snd_strerror(err));
				return(-1);
			}
		}
	}
#endif

	return(0);
}

void showstat(snd_pcm_t *handle, size_t frames)
{
	int err;
	snd_pcm_status_t *status;

	snd_pcm_status_alloca(&status);
	if ((err = snd_pcm_status(handle, status)) < 0) {
		printf("Stream status error: %s\n", snd_strerror(err));
		exit(0);
	}
	printf("*** frames = %li ***\n", (long)frames);
	snd_pcm_status_dump(status, output);
}
#endif /* BRISTOL_HAS_ALSA */

int
audioWrite(audioDev, buffer, count)
duplexDev *audioDev;
char *buffer;
int count;
{
	if (audioDev->flags & AUDIO_DUMMY)
		return(count);

	if (audioDev->flags & SLAB_AUDIODBG2)
		printf("audioWrite(%i)\n", count);

#if (BRISTOL_HAS_ALSA == 1)
	/*
	 * This if for ALSA 0.9 and 1.0
	 */
	if (audioDev->siflags & AUDIO_ALSA)
	{
		long r;

		/* 
		 * Slab historically used byte buffer sizes. ALSA 0.9 uses frames, so
		 * we now work in samples.
		 */
		while((r = 
			snd_pcm_writei(alsaDev[audioDev->devID].phandle, buffer, count))
			== EAGAIN)
			printf("Do again\n");

		if (r < 0)
		{
			printf("	Write Error: %s %i\n", snd_strerror(r), (int) r);
			return(r);
		}
		return(count);
	}
#endif /* BRISTOL_HAS_ALSA */

#ifdef BRISTOL_PA
	if (audioDev->siflags & AUDIO_PULSE)
		return(pulseDevWrite(audioDev->fd, buffer, count));
#endif

	/*
	 * Otherwise just to a normal OSS fd write operation. Since bristol release
	 * went to 1.0 started using sample counts rather than bytes, so OSS has to
	 * calculate the actual buffer size. Going to be lazy for now, and assume
	 * short samples..... Hm.
	 */
	return(write(audioDev->fd, buffer, count * 2 * audioDev->channels));
}

int
audioRead(audioDev, buffer, count)
duplexDev *audioDev;
char *buffer;
int count;
{
	int result;

	if (audioDev->flags & SLAB_AUDIODBG2)
		printf("audioRead(%i)\n", count);

	if (audioDev->flags & AUDIO_DUMMY)
	{
		usleep(100000);
		return(count * 2 * audioDev->channels);
	}

#if (BRISTOL_HAS_ALSA == 1)
	/*
	 * This if for ALSA 0.9 and 1.0
 	printf("audioRead(%x, %i)\n", buffer, count);
	 */
	if (dummycapture)
	{
		memset(buffer, 0, audioDev->fragSize);
		return(count);
	}
	if (audioDev->siflags & AUDIO_ALSA)
		return(snd_pcm_readi(alsaDev[audioDev->devID].chandle, buffer, count));
#endif

#ifdef BRISTOL_PA
	if (audioDev->siflags & AUDIO_PULSE)
		return(pulseDevRead(audioDev->fd2, buffer, count));
#endif

	/*
	 * If this is not an ALSA dev, do a normal OSS fd read operation.
	 */
	result = read(audioDev->fd2, buffer, count * 2 * audioDev->channels);

	return(result / 2 / audioDev->channels);
}

