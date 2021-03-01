
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

#define STAGE_COUNT 6

#define STAGE_0 0
#define STAGE_1 1
#define STAGE_2 2
#define STAGE_3 3
#define STAGE_4 4
#define STAGE_5 5

#define ATTACK_STAGE STAGE_0
#define SUSTAIN_STAGE STAGE_3
#define RELEASE_STAGE STAGE_4
#define END_STAGE STAGE_5

typedef struct BristolENV {
	bristolOPSpec spec;
} bristolENV;

typedef struct BristolENVlocal {
	float current;
	float cgain;
	int state;
} bristolENVlocal;

#endif /* ENV_H */

