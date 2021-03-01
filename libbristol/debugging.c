
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

int bristolParamPrint(bristolOPParam *param)
{
#ifdef DEBUG
	printf("bristolParamPrint(0x%08x)\n", param);
#endif

	printf("		name	%s\n", param->pname);
	printf("		desc	%s\n", param->description);
	printf("		type	%i\n", param->type);
	printf("		low	%i\n", param->low);
	printf("		high	%i\n", param->high);
	printf("		flags	%i\n", param->flags);

	return(0);
}

int bristolIOprint(bristolOPIO *io)
{
#ifdef DEBUG
	printf("bristolIOprint(0x%08x)\n", io);
#endif

	printf("		name	%s\n", io->ioname);
	printf("		desc	%s\n", io->description);
	printf("		rate	%i\n", io->samplerate);
	printf("		count	%i\n", io->samplecount);
	printf("		flags	%x\n", io->flags);
	printf("		buf	%p\n", io->buf);

	return(0);
}

int bristolSpecPrint(bristolOPSpec *specs)
{
	int i;

#ifdef DEBUG
	printf("bristolSpecPrint(0x%08x)\n", specs);
#endif

	printf("		name	%s\n", specs->opname);
	printf("		desc	%s\n", specs->description);
	printf("		pcount	%i\n", specs->pcount);
	printf("		param	%p\n", specs->param);
	printf("		iocount	%i\n", specs->iocount);
	printf("		io	%p\n", specs->io);
	printf("		lclsize	%i\n", specs->localsize);

	for (i = 0; i < specs->iocount; i++)
		bristolIOprint(&specs->io[i]);

	for (i = 0; i < specs->pcount; i++)
		bristolParamPrint(&specs->param[i]);

	return(0);
}

int
bristolOPprint(bristolOP *operator)
{
#ifdef DEBUG
	printf("bristolOPprint(0x%08x)\n", operator);
#endif

	printf("	index	%i\n", operator->index);
	printf("	flags	%i\n", operator->flags);
	printf("	next	%p\n", operator->next);
	printf("	last	%p\n", operator->last);
	printf("	spec	%p\n", operator->specs);
	printf("	size	%i\n", operator->size);
	printf("	init	%p\n", operator->init);
	printf("	dest	%p\n", operator->destroy);
	printf("	reset	%p\n", operator->reset);
	printf("	param	%p\n", operator->param);
	printf("	operate	%p\n", operator->operate);

	bristolSpecPrint(operator->specs);

	return(0);
}

