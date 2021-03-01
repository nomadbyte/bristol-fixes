
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

#define P1_FE_WHEEL		0x00001
#define P1_OSCB_WHEEL	0x00002
#define P1_LFO_WHEEL	0x00004

#define P1_OSCA_F_WM	0x00008
#define P1_OSCA_F_DM	0x00010
#define P1_OSCA_PW_WM	0x00020
#define P1_OSCA_PW_DM	0x00040
#define P1_OSCB_F_WM	0x00080
#define P1_OSCB_F_DM	0x00100
#define P1_OSCB_PW_WM	0x00200
#define P1_OSCB_PW_DM	0x00400
#define P1_FILT_WM		0x00800
#define P1_FILT_DM		0x01000

#define P1_LFO_B		0x02000
#define P1_KBD_B		0x04000

#define P1_LFO_RAMP		0x08000
#define P1_LFO_TRI		0x10000
#define P1_LFO_SQR		0x20000

#define P1_REPEAT		0x40000
#define P1_DRONE		0x80000

/*
 * These need to go into some local structure for multiple instances
 * of the prophet - malloc()ed into the baudio->mixlocals.
 */
typedef struct pMods {
	float f_envlevel;
	float oscbmod;
	float lfomod;
	float lfohist;
	float mix_a, mix_b, mix_n;
} pmods;

