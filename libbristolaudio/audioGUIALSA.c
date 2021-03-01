
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
 * This file contains the ALSA specific calls.
 */

#ifdef DEBUG
#include <stdio.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#ifdef SUBFRAGMENT
#include <malloc.h>
#endif
#include <stdlib.h>

/*
 * Audio device structure format definitions.
 */
#include "slabaudiodev.h"

#if (BRISTOL_HAS_ALSA  == 1)
#include "slabalsadev.h"

static struct adev alsaDev[MAX_DEVICES + 1];

int
checkAudioALSAcaps(audioDev, devID, fd)
duplexDev *audioDev;
int devID, fd;
{
	return(0);
}

const char *
getAlsaName(duplexDev *audioDev, int cont)
{
	snd_mixer_selem_id_t *sid;

	sid = (snd_mixer_selem_id_t *)(((char *) alsaDev[audioDev->devID].mixer_sid)
		+ snd_mixer_selem_id_sizeof() * cont);

if (audioDev->cflags & SLAB_AUDIODBG)
printf("getAlsaName(%i): \"%s\"\n", cont, snd_mixer_selem_id_get_name(sid));
	return(snd_mixer_selem_id_get_name(sid));
}

int
getAlsaStereoStatus(duplexDev *audioDev, int cont)
{
	/*
	 * Cannot find a stereo status switch, so for now assume that only 
	 * Master Mono and Mic are mono.
	 */
	if (strcmp(getAlsaName(audioDev, cont), "Master Mono") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "Mic") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "Center") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "LFE") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "Wave Center") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "Wave LFE") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "Phone") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "PC Speaker") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "Headphone LFE") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "Headphone Center") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "3D Control - Switch") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "Mic Boost (+20dB)") == 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "External Amplifier Power Down")
		== 0)
		return(1);
	if (strcmp(getAlsaName(audioDev, cont), "3D Control Sigmatel - Depth") == 0)
		return(1);

	return(2);
}

int
setAlsaValue(duplexDev *audioDev, int cont, int side, int value)
{
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	long vmin, vmax, vol;

	if ((--side == 1) && (getAlsaStereoStatus(audioDev, cont) < 2))
		return(0);

	sid = (snd_mixer_selem_id_t *)(((char *) alsaDev[audioDev->devID].mixer_sid)
		+ snd_mixer_selem_id_sizeof() * cont);

    elem = snd_mixer_find_selem(alsaDev[audioDev->devID].mh, sid);

if (audioDev->cflags & SLAB_AUDIODBG)
printf("setAlsaValue(%i, %i, %i)\n", cont, side, value);

	if (snd_mixer_selem_has_playback_volume(elem))
	{
if (audioDev->cflags & SLAB_AUDIODBG)
printf("HAS PLAYBACK FOUND\n");
    	snd_mixer_selem_get_playback_volume_range(elem, &vmin, &vmax);
	} else {
if (audioDev->cflags & SLAB_AUDIODBG)
printf("HAS CAPTURE FOUND\n");
    	snd_mixer_selem_get_capture_volume_range(elem, &vmin, &vmax);
	}

	vol = value * (vmax - vmin) / 100;

	/*
	 * If you get violations in this code, with assert failures in the sound
	 * library then it is likely this controller is either mono, or does not
	 * support continuous control (is a switch). I should work on the correct
	 * way to determine the capabilities of a device
	 */
	if (snd_mixer_selem_has_playback_volume(elem))
	{
if (audioDev->cflags & SLAB_AUDIODBG)
printf("PLAYBACK VOLUME\n");

    	if (snd_mixer_selem_set_playback_volume(elem, side, vol) < -1)
			printf("failed to set value\n");
	} else if (snd_mixer_selem_has_capture_volume(elem)) {
if (audioDev->cflags & SLAB_AUDIODBG)
printf("CAPTURE VOLUME\n");
	   	if (snd_mixer_selem_set_capture_volume(elem, side, vol) < -1)
			printf("failed to set value\n");
	}

	return(0);
}
/*
 * This is for ALSA 0.9, there were such massive alterations to the audio
 * interface definitions that a general rewrite was required.
 * This is always painful stuff, hopefully after this release things will
 * be a bit more stable.
 */

int
validAlsaDev(duplexDev *audioDev, int cont)
{
	if (cont >= alsaDev[audioDev->devID].elem_count)
		return(0);
	return(1);
}

int
getAlsaRecordability(duplexDev *audioDev, int cont)
{
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;

if (audioDev->cflags & SLAB_AUDIODBG)
printf("getRecordability\n");
	sid = (snd_mixer_selem_id_t *)(((char *) alsaDev[audioDev->devID].mixer_sid)
		+ snd_mixer_selem_id_sizeof() * cont);

    elem = snd_mixer_find_selem(alsaDev[audioDev->devID].mh, sid);

	if (snd_mixer_selem_has_capture_switch(elem))
		return(0);

	return(-2);
}

int
getAlsaMutability(duplexDev *audioDev, int cont)
{
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;

if (audioDev->cflags & SLAB_AUDIODBG)
printf("getMutability\n");
	sid = (snd_mixer_selem_id_t *)(((char *) alsaDev[audioDev->devID].mixer_sid)
		+ snd_mixer_selem_id_sizeof() * cont);

    elem = snd_mixer_find_selem(alsaDev[audioDev->devID].mh, sid);

	/*
	 * If we have playback capability then we also have mutability.
	 */
	if (snd_mixer_selem_has_playback_switch(elem))
		return(1);

	return(0);
}

int
getAlsaCapability(duplexDev *audioDev, int cont)
{
if (audioDev->cflags & SLAB_AUDIODBG)
printf("getAlsaCapability(%i)\n", cont);

	if (cont >= alsaDev[audioDev->devID].elem_count)
		return(-1);
	return(cont);
}

int
getAlsaCapByName(duplexDev *audioDev, char *name)
{
	snd_mixer_selem_id_t *sid;
	int cont;

	if (name[strlen(name) - 1] == ' ')
		name[strlen(name) - 1] = '\0';

if (audioDev->cflags & SLAB_AUDIODBG)
printf("getAlsaCapByName(%s)\n", name);
	for (cont = 0; cont < alsaDev[audioDev->devID].elem_count; cont++)
	{
		sid = (snd_mixer_selem_id_t *)(((char *)
			alsaDev[audioDev->devID].mixer_sid)
			+ snd_mixer_selem_id_sizeof() * cont);

		if (strcmp(snd_mixer_selem_id_get_name(sid), name) == 0)
			return(cont);
	}
	return(-1);
}

int
getAlsaValue(duplexDev *audioDev, int cont, int side)
{
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	long vmin, vmax, vol;

	sid = (snd_mixer_selem_id_t *)(((char *) alsaDev[audioDev->devID].mixer_sid)
		+ snd_mixer_selem_id_sizeof() * cont);

    elem = snd_mixer_find_selem(alsaDev[audioDev->devID].mh, sid);

if (audioDev->cflags & SLAB_AUDIODBG)
printf("getAlsaValue\n");

	if (snd_mixer_selem_has_playback_volume(elem))
	{
    	snd_mixer_selem_get_playback_volume_range(elem, &vmin, &vmax);
	    snd_mixer_selem_get_playback_volume(elem, side, &vol);
	} else {
    	snd_mixer_selem_get_capture_volume_range(elem, &vmin, &vmax);
	    snd_mixer_selem_get_capture_volume(elem, side, &vol);
	}

	return(vol * 100 / (vmax - vmin));
}

int
setAudioALSAparam(duplexDev *audioDev, int devID, char *param,
	short left, short right)
{
	int cont;

if (audioDev->cflags & SLAB_AUDIODBG)
printf("setAudioALSAparam(%i)\n", devID);

	if ((cont = getAlsaCapability(audioDev, devID)) == -1)
	{
if (audioDev->cflags & SLAB_AUDIODBG)
printf("could not find capability \"%s\"\n", param);
		return(0);
	}

	setAlsaValue(audioDev, cont, 1, left);

	if (getAlsaStereoStatus(audioDev, cont) > 1)
		setAlsaValue(audioDev, cont, 2, right);

	return(0);
}

int
setAlsaRecordSource(duplexDev *audioDev, int cont, int position)
{
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;

if (audioDev->cflags & SLAB_AUDIODBG)
printf("setAlsaRecordSource\n");
	sid = (snd_mixer_selem_id_t *)(((char *) alsaDev[audioDev->devID].mixer_sid)
		+ snd_mixer_selem_id_sizeof() * cont);

    elem = snd_mixer_find_selem(alsaDev[audioDev->devID].mh, sid);

	if (snd_mixer_selem_has_capture_switch(elem))
	{
		snd_mixer_selem_set_capture_switch(elem, 0, position);
		snd_mixer_selem_set_capture_switch(elem, 1, position);
	}

	return(0);
}

const char *
getAlsaDeviceName(duplexDev *audioDev)
{
if (audioDev->cflags & SLAB_AUDIODBG)
printf("setDeviceName(%s)\n", alsaDev[audioDev->devID].name);
	return(alsaDev[audioDev->devID].name);
}

int
closeALSAmixer(audioDev)
duplexDev *audioDev;
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("closeALSAmixer(): %p\n", alsaDev[audioDev->devID].mh);

	if (alsaDev[audioDev->devID].mh != (snd_mixer_t *) NULL)
	{
		int err;

		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("real closeALSAmixer(): %p\n", alsaDev[audioDev->devID].mh);

		if ((err = snd_mixer_close((void *) alsaDev[audioDev->devID].mh)) < 0)
		{
			if (audioDev->cflags & SLAB_AUDIODBG)
				printf("SND Mixer Close error: %s\n", snd_strerror(err));
		}
		if ((err = snd_ctl_close(alsaDev[audioDev->devID].ch)) < 0) {
			if (audioDev->cflags & SLAB_AUDIODBG)
				printf("SND CTL Close error: %s\n", snd_strerror(err));
		}
	}

	alsaDev[audioDev->devID].mh = NULL;
	alsaDev[audioDev->devID].ch = NULL;

	return(0);
}

int
setAlsaMute(duplexDev *audioDev, int cont, int onoff)
{
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	int join;

if (audioDev->cflags & SLAB_AUDIODBG)
printf("setAlsaMute(%i, %i)\n", cont, onoff);
	sid = (snd_mixer_selem_id_t *)(((char *) alsaDev[audioDev->devID].mixer_sid)
		+ snd_mixer_selem_id_sizeof() * cont);

    elem = snd_mixer_find_selem(alsaDev[audioDev->devID].mh, sid);

join = snd_mixer_selem_has_playback_volume_joined(elem);
if (audioDev->cflags & SLAB_AUDIODBG)
printf("joined on device %i is %i\n", cont, join);

	if (snd_mixer_selem_has_playback_switch(elem))
	{
		snd_mixer_selem_set_playback_switch(elem, 0, 1 - onoff);
		if (getAlsaStereoStatus(audioDev, cont) > 1)
			snd_mixer_selem_set_playback_switch(elem, 1, 1 - onoff);
	}

	return(1);
}

int
openALSAmixer(duplexDev *audioDev)
{
	int err, selem_count, elem_index = 0, mixer_n_selems = 0;
	snd_mixer_selem_id_t *sid;

	snd_ctl_card_info_alloca(&alsaDev[audioDev->devID].hwInfo);

	if (alsaDev[audioDev->devID].ch != NULL)
		return(0);

	if ((err = snd_ctl_open(&alsaDev[audioDev->devID].ch,
		&audioDev->mixerName[0], 0)) < 0)
	{
		printf("Could not open control interface\n");
		return(-1);
	}
	if ((err = snd_ctl_card_info(alsaDev[audioDev->devID].ch,
		alsaDev[audioDev->devID].hwInfo)) < 0)
	{
		printf("Could not get hardware info\n");
		return(-1);
	}

	alsaDev[audioDev->devID].name =
		strdup(snd_ctl_card_info_get_name(alsaDev[audioDev->devID].hwInfo));

	if (audioDev->cflags & SLAB_AUDIODBG)
	{
		printf("Found: %s\n", alsaDev[audioDev->devID].name);
		printf("Hardware: %s\n",
			snd_ctl_card_info_get_mixername(alsaDev[audioDev->devID].hwInfo));
	}

	if ((err = snd_mixer_open(&alsaDev[audioDev->devID].mh, 0)) < 0)
	{
		printf("Could not get mixer\n");
		return(-1);
	}

	if ((err = snd_mixer_attach(alsaDev[audioDev->devID].mh,
		&audioDev->mixerName[0])) < 0)
	{
		printf("Could not attach to mixer %s\n", audioDev->mixerName);
		return(-1);
	}
	if ((err = snd_mixer_selem_register(alsaDev[audioDev->devID].mh,
		NULL, NULL)) < 0)
	{
		printf("Could not get mixer\n");
		return(-1);
	}
/*
	snd_mixer_set_callback(alsaDev[audioDev->devID].mh, mixer_event);
*/
	if ((err = snd_mixer_load (alsaDev[audioDev->devID].mh)) < 0)
	{
		printf("Could not get mixer\n");
		return(-1);
	}
	selem_count = snd_mixer_get_count(alsaDev[audioDev->devID].mh);

	alsaDev[audioDev->devID].mixer_sid
		= malloc(snd_mixer_selem_id_sizeof() * selem_count);

	for (alsaDev[audioDev->devID].eInfo[mixer_n_selems]
			= snd_mixer_first_elem(alsaDev[audioDev->devID].mh);
		alsaDev[audioDev->devID].eInfo[mixer_n_selems];
		alsaDev[audioDev->devID].eInfo[mixer_n_selems]
			= snd_mixer_elem_next(
				alsaDev[audioDev->devID].eInfo[mixer_n_selems - 1]))
	{
		sid = (snd_mixer_selem_id_t *)(((char *)
			alsaDev[audioDev->devID].mixer_sid)
			+ snd_mixer_selem_id_sizeof() * mixer_n_selems);
		snd_mixer_selem_get_id(alsaDev[audioDev->devID].eInfo[mixer_n_selems],
			sid);
		if (!snd_mixer_selem_is_active(alsaDev[audioDev->devID].eInfo[
			mixer_n_selems]))
			break;
		mixer_n_selems++;
	}

	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("found %i elements\n", mixer_n_selems);

	alsaDev[audioDev->devID].elem_count = mixer_n_selems;

    for (elem_index = 0; elem_index < mixer_n_selems; elem_index++)
	{
		sid = (snd_mixer_selem_id_t *)(((char *)
			alsaDev[audioDev->devID].mixer_sid)
			+ snd_mixer_selem_id_sizeof() * elem_index);
		if (audioDev->cflags & SLAB_AUDIODBG)
			printf("	%s\n", snd_mixer_selem_id_get_name(sid));
	}

	return(0);
}
#endif /* ALSA VERSION */

