
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

#define O_UNISON		0x00000001ULL

/*
 * Have to watch out here, mixflags are also used globally in the range
 * 0xffff0000.00000000 for midi ops, we can use the rest in the algo though.
 */
#define O_S_H			0x00000002ULL
#define O_FREQ_1		0x00000004ULL
#define O_FREQ_2		0x00000008ULL
#define O_FILT			0x00000010ULL

#define O_PWM_1			0x00000020ULL
#define O_PWM_2			0x00000040ULL
#define O_TREM			0x00000080ULL

#define O_XMOD			0x00000100ULL

#define O_F_OSC1		0x00000200ULL
#define O_F_KBD			0x00000400ULL
#define O_F_OSC2_1		0x00000800ULL
#define O_F_OSC2_2		0x00001000ULL
#define O_F_NOISE1		0x00002000ULL
#define O_F_NOISE2		0x00004000ULL

#define O_MULTI_LFO		0x00008000ULL

#define O_LFO_SINE		0x00010000ULL
#define O_LFO_SQUARE	0x00020000ULL
#define O_LFO_SH		0x00040000ULL

#define O_ENV_MOD		0x00080000ULL /* OBXA */
#define O_F_4POLE		0x00100000ULL /* OBXA */

#define O_MOD1_ENABLE	0x00200000ULL /* OBXA MODS */
#define O_MOD2_ENABLE	0x00400000ULL /* OBXA MODS */

#define O_MOD_PWM		0x01000000ULL

/*
 * These need to go into some local structure for multiple instances
 * of the prophet - malloc()ed into the baudio->mixlocals.
 */
typedef struct pMods {
	float d_mod1, d_mod2, pan, wheelmod, wmdepth;
	int voicecount;
} pmods;

