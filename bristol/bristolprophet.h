
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

#define P_PM_FREQ_A		1
#define P_PM_PW_A		2
#define P_PM_FILTER		4

#define P_WM_FREQ_A		8
#define P_WM_FREQ_B		0x010
#define P_WM_PW_A		0x020
#define P_WM_PW_B		0x040
#define P_WM_FILTER		0x080

#define P_LFO_B			0x0100
#define P_KBD_B			0x0200

#define P_UNISON		0x1000

#define P_LFO_RAMP		0x2000
#define P_LFO_TRI		0x4000
#define P_LFO_SQR		0x8000

/*
 * These need to go into some local structure for multiple instances
 * of the prophet - malloc()ed into the baudio->mixlocals.
 */
typedef struct pMods {
	float modmix;
	float pm_filtenv;
	float pm_oscb;
	float f_envlevel;
	float mix_a, mix_b, mix_n, pan, gain;
	int voicecount;
} pmods;

