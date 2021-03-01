
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

/*
 * Library routines for operator management. Initialisation, creation and
 * destruction of operator.s
 */

#include "bristol.h"

extern void bristolfree(char *);

void
bristolOPfree(bristolOP *operator)
{
#ifdef DEBUG
	printf("bristolOPfree(%x)\n", operator);
#endif

	bristolfree((char *) operator);
}

/*
 * Free up any memory from our op and io structures. Should be in library.
 */
int
cleanup(bristolOP *operator)
{
#ifdef DEBUG
	printf("cleanup(%x)\n", operator);
#endif

	bristolOPfree(operator);

	return(0);
}

bristolIO *
bristolIOinit(bristolIO **io, int index, char *name, int rate, int samplecount)
{
#ifdef DEBUG
	printf("bristolIOinit(%x, %i, %s, %i, %i)\n",
		io, index, name, rate, samplecount);
#endif

	*io = (bristolIO *) bristolmalloc(sizeof(bristolIO));

	/*
	 * Much of this will probably go into a library, for the basic init work
	 * of any operator. The local stuff will remain in the operator code.
	 */
	(*io)->IOname = name;
	(*io)->index = index;
	(*io)->flags = 0;
	(*io)->last = (struct BristolIO *) NULL;
	(*io)->next = (struct BristolIO *) NULL;
	(*io)->samplecnt = samplecount;
	(*io)->samplerate = rate;

	/*
	 * And get an IO buffer.
	 */
	(*io)->bufmem = (float *) bristolmalloc((*io)->samplecnt * sizeof(float));

	return(*io);
}

/*
 * Create the operator structure, and do some basic init work.
 */
bristolOP *
bristolOPinit(bristolOP **operator, int index, int samplecount)
{
#ifdef DEBUG
	printf("bristolOPinit(%x, %i, %i)\n", operator, index, samplecount);
#endif

	*operator = (bristolOP *) bristolmalloc(sizeof(bristolOP));

	/*
	 * Much of this will probably go into a library, for the basic init work
	 * of any operator. The local stuff will remain in the operator code.
	 */
	(*operator)->index = index;
	(*operator)->flags = 0;
	(*operator)->last = (struct BristolOP *) NULL; /* filled in by parent */
	(*operator)->next = (struct BristolOP *) NULL; /* filled in by parent */

	return(*operator);
}

