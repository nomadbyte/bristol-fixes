
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

#ifndef AKSREVERB_H
#define AKSREVERB_H

#define AKSREVERB_TBL_SZE 4096

#define TABSIZE 720
#define HISTSIZE 2048

typedef struct BristolAKSREVERB {
	bristolOPSpec spec;
} bristolAKSREVERB;

typedef struct BristolAKSREVERBlocal {
	unsigned int flags;
	int histin1;
	int histin2;
} bristolAKSREVERBlocal;

#endif /* AKSREVERB_H */

