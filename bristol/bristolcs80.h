
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

#ifndef __BRISTOL_CS80_H
#define __BRISTOL_CS80_H

#define CH_I 0
#define CH_II 1

typedef struct CsChMods {
	float *keybrill; /* Calculated from velocity, key, initial brill at keyon */
	float *keylevel; /* Calculated from velocity, key, initial level at keyon */
	float brilliance;
	float level;
	float polybrilliance;
	float polylevel;
	float pwmdepth;
	float noiselevel;
	float filterlevel;
	float filterbrill;
} csChMods;

typedef struct csMods {
	unsigned int flags;
	float *lout; /* Final stage output panning */
	float *rout;
	float lpan[2];
	float rpan[2];
	int channel;
	float channelmix;
	float gain;
	float noisegain;
	float pwm;
	float brilliance;
	float brillianceL;
	float brillianceH;
	float levelL;
	float levelH;
	csChMods chanmods[2];
	float lfo2vco;
	float lfo2vcf;
	float lfo2vca;
} csmods;

#endif /* __BRISTOL_CS80_H */

