
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

#ifndef SLAB_MIXED
#define SLAB_MIXED
/*
 * This should be set up according to system type.
 */
#ifndef INC_SOURCE
#define SLAB_NULL			-1
#define SLAB_TREBLE			SOUND_MIXER_TREBLE
#define SLAB_MID			-1
#define SLAB_BASS			SOUND_MIXER_BASS
#define SLAB_INPUT_GAIN		SOUND_MIXER_IGAIN
#define SLAB_OUTPUT_GAIN	SOUND_MIXER_OGAIN
#define SLAB_VOLUME			SOUND_MIXER_VOLUME
#define SLAB_VOL_SYNTH		SOUND_MIXER_SYNTH
#define SLAB_VOL_CD			SOUND_MIXER_CD
#define SLAB_VOL_MIC		SOUND_MIXER_MIC
#define SLAB_VOL_LINE		SOUND_MIXER_LINE
#define SLAB_VOL_LINE1		SOUND_MIXER_LINE1
#define SLAB_VOL_LINE2		SOUND_MIXER_LINE2
#define SLAB_VOL_LINE3		SOUND_MIXER_LINE3
#define SLAB_VOL_PCM		SOUND_MIXER_PCM
#define SLAB_VOL_SPEAKER	SOUND_MIXER_SPEAKER
#define SLAB_VOL_ALT_PCM	SOUND_MIXER_ALTPCM
#define SLAB_VOL_RECLEV		SOUND_MIXER_RECLEV
#define SLAB_VOL_IMIX		SOUND_MIXER_IMIX
#else
#define SLAB_NULL			-1
#define SLAB_TREBLE			2
#define SLAB_MID			-1
#define SLAB_BASS			1
#define SLAB_INPUT_GAIN		12
#define SLAB_OUTPUT_GAIN	13
#define SLAB_VOLUME			0
#define SLAB_VOL_SYNTH		3
#define SLAB_VOL_CD			8
#define SLAB_VOL_MIC		7
#define SLAB_VOL_LINE		6
#define SLAB_VOL_LINE1		14
#define SLAB_VOL_LINE2		15
#define SLAB_VOL_LINE3		16
#define SLAB_VOL_PCM		4
#define SLAB_VOL_SPEAKER	5
#define SLAB_VOL_ALT_PCM	10
#define SLAB_VOL_RECLEV		11
#define SLAB_VOL_IMIX		9
#endif

#define SLAB_CD			SLAB_VOL_CD
#define SLAB_MIC		SLAB_VOL_MIC
#define SLAB_LINE		SLAB_VOL_LINE
#define SLAB_LINE1		SLAB_VOL_LINE1
#define SLAB_LINE2		SLAB_VOL_LINE2
#define SLAB_LINE3		SLAB_VOL_LINE3
#define SLAB_SYNTH		SLAB_VOL_SYNTH
#define SLAB_PCM		SLAB_VOL_PCM

#define SLAB_NULL_MASK			0
#define SLAB_TREBLE_MASK		1<<SLAB_TREBLE
#define SLAB_MID_MASK			SLAB_NULL_MASK
#define SLAB_BASS_MASK			1<<SLAB_BASS
#define SLAB_INPUT_GAIN_MASK	1<<SLAB_INPUT_GAIN
#define SLAB_OUTPUT_GAIN_MASK 	1<<SLAB_OUTPUT_GAIN
#define SLAB_VOLUME_MASK		1<<SLAB_VOLUME
#define SLAB_VOL_SYNTH_MASK		1<<SLAB_VOL_SYNTH
#define SLAB_VOL_CD_MASK		1<<SLAB_VOL_CD
#define SLAB_VOL_MIC_MASK		1<<SLAB_VOL_MIC
#define SLAB_VOL_LINE_MASK		1<<SLAB_VOL_LINE
#define SLAB_VOL_LINE1_MASK		1<<SLAB_VOL_LINE1
#define SLAB_VOL_LINE2_MASK		1<<SLAB_VOL_LINE2
#define SLAB_VOL_LINE3_MASK		1<<SLAB_VOL_LINE3
#define SLAB_VOL_PCM_MASK		1<<SLAB_VOL_PCM

#define SLAB_REC_SRC	0xff

/*
 * These allow the same parameters to be used for either 0-100 ranges (this
 * case, or 0-270 defined as per above. Scaling is done in libmixer. These are
 * the MicroMix definitions.
 */

#define SLAB_MM_BIT			0x100000
#define SLAB_OPCODE_BIT		0x200000

#define SLAB_SPECIAL		0x400001

#define	SLAB_MM_MASK		~SLAB_MM_BIT
#define SLAB_OPCODE_MASK	~SLAB_OPCODE_BIT

#define SLAB_MM_CD			SLAB_VOL_CD | SLAB_MM_BIT
#define SLAB_MM_MIC			SLAB_VOL_MIC | SLAB_MM_BIT
#define SLAB_MM_LINE		SLAB_VOL_LINE | SLAB_MM_BIT
#define SLAB_MM_LINE1		SLAB_VOL_LINE1| SLAB_MM_BIT
#define SLAB_MM_LINE2		SLAB_VOL_LINE2| SLAB_MM_BIT
#define SLAB_MM_LINE3		SLAB_VOL_LINE3| SLAB_MM_BIT
#define SLAB_MM_SYNTH		SLAB_VOL_SYNTH | SLAB_MM_BIT
#define SLAB_MM_PCM			SLAB_VOL_PCM | SLAB_MM_BIT

/*
 * These allow the same parameters to be used for either 0-100 ranges (this
 * case, or 0-270 defined as per above. Scaling is done in libmixer. These are
 * the MicroMix definitions.
 */

#define SLAB_MM_BIT			0x100000
#define SLAB_OPCODE_BIT		0x200000

#define SLAB_SPECIAL		0x400001

#define	SLAB_MM_MASK		~SLAB_MM_BIT
#define SLAB_OPCODE_MASK	~SLAB_OPCODE_BIT

#define SLAB_MM_CD			SLAB_VOL_CD | SLAB_MM_BIT
#define SLAB_MM_MIC			SLAB_VOL_MIC | SLAB_MM_BIT
#define SLAB_MM_LINE		SLAB_VOL_LINE | SLAB_MM_BIT
#define SLAB_MM_LINE1		SLAB_VOL_LINE1| SLAB_MM_BIT
#define SLAB_MM_LINE2		SLAB_VOL_LINE2| SLAB_MM_BIT
#define SLAB_MM_LINE3		SLAB_VOL_LINE3| SLAB_MM_BIT
#define SLAB_MM_SYNTH		SLAB_VOL_SYNTH | SLAB_MM_BIT
#define SLAB_MM_PCM			SLAB_VOL_PCM | SLAB_MM_BIT

#endif /* SLAB_MIXED */
