
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

#ifndef AKSENV_H
#define AKSENV_H

#define STATE_START	 0
#define STATE_ATTACK 0x020
#define STATE_ON	 0x040
#define STATE_DECAY	 0x080
#define STATE_OFF	 0x100

typedef struct BristolAKSENV {
	bristolOPSpec spec;
} bristolAKSENV;

typedef struct BristolAKSENVlocal {
	unsigned int flags;
	float cgain;
	float egain;
	int cstate;
	int estate;
	int oncount;
} bristolAKSENVlocal;

#endif /* AKSENV_H */

