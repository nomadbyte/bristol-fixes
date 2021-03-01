
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

#ifndef SLAB_ALSADEV_H
#define SLAB_ALSADEV_H

#include "slabdefinitions.h"

#if (BRISTOL_HAS_ALSA == 1)
#include <alsa/asoundlib.h>
#include "slabalsadev.h"

#ifndef AUDIO_ALSA
#define AUDIO_ALSA 4
#endif
#endif

typedef struct adev {
	/*
	 * This should either be moved into a separate structure, potentially out
	 * of shared memory space since it is specific to the mixing operations, or
	 * it should be put into a union, so that even if the software is recompiled
	 * without ALSA support, the size of this structure does not change.
	 */
	snd_pcm_t *chandle; /* Capture handle */
	snd_pcm_t *phandle; /* Playback handle */
#if (SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 5)
	snd_pcm_channel_status_t cstatus, pstatus;
#elif (SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 4)
	snd_pcm_capture_status_t cstatus;
	snd_pcm_playback_status_t pstatus;
	/*
	 * The rest should be for the mixing device only, the GUI section.
	 */
	snd_mixer_t *mh;
	snd_ctl_t *ch;
    snd_ctl_hw_info_t hwInfo;
	snd_mixer_info_t mixerInfo;
	snd_mixer_groups_t gsInfo;
	snd_mixer_gid_t pgroups[32];
	snd_mixer_eid_t pelements[256];
	snd_mixer_group_t gInfo[32];
	snd_mixer_elements_t esInfo;
	snd_mixer_element_t eInfo[64];
	snd_mixer_element_info_t eData[64];
	snd_mixer_channel_t channelInfo;
#elif (SND_LIB_MAJOR == 1) /* this must be fixed  && SND_LIB_MINOR == 9) */
	snd_pcm_hw_params_t *p_params;
	snd_pcm_sw_params_t *p_swparams;
	snd_pcm_hw_params_t *c_params;
	snd_pcm_sw_params_t *c_swparams;

	snd_mixer_t *mh;
	snd_ctl_t *ch;
    snd_ctl_card_info_t *hwInfo;
	snd_mixer_elem_t *eInfo[64];
	void *mixer_sid;
	int elem_count;
	char *name;
#endif
} aDev;

#endif /* SLAB_ALSADEV_H */

