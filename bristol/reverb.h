
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

#ifndef VREVERB_H
#define VREVERB_H

#define DMAX 32768
#define DLINES 6

static int primes[DLINES] = {
	2999, 2331, 1893, 1097, 1051, 337
};

/*
static int primes2[DLINES] = {
	11, 13, 7, 19, 23,
};
*/

typedef struct BristolVREVERB {
	bristolOPSpec spec;
} bristolVREVERB;

typedef struct BristolVREVERBlocal {
	unsigned int flags;
	int start[DLINES];
	int end[DLINES];
	int lptr[DLINES];
} bristolVREVERBlocal;

#endif /* VREVERB_H */

