
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

#ifndef ENV_H
#define ENV_H

#define STATE_DONE BRISTOL_KEYDONE /* 4 */
#define STATE_RELEASE BRISTOL_KEYOFF /* 2 */
#define STATE_START 10
#define STATE_ATTACK 11
#define STATE_DECAY 12
#define STATE_SUSTAIN 13

#define LINEAR_ATTACK 1

typedef struct BristolENV {
	bristolOPSpec spec;
	float samplerate;
	float duration;
	unsigned int flags;
} bristolENV;

typedef struct BristolENVlocal {
	unsigned int flags;
	float cgain;
	float egain;
	int cstate;
} bristolENVlocal;

#endif /* ENV_H */

