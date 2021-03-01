
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
/*#define BRISTOL_DBG */

/*
 * Need to have basic template for an operator. Will consist of
 *
 *	eswitchinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */

#include <stdlib.h>

#include "bristol.h"
#include "electroswitch.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "Electromod"
#define OPDESCRIPTION "Electronic Switch"
#define PCOUNT 0
#define IOCOUNT 4

#define ESWITCHMOD_IN_1 0
#define ESWITCHMOD_IN_2 1
#define ESWITCHMOD_CLOCK 2
#define ESWITCHMOD_OUT 3

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif

	/*
	 * Unmalloc anything we added to this structure
	 */
	bristolfree(operator->specs);

	/*
	 * Free any local memory. We should also free ourselves, since we did the
	 * initial allocation.
	 */
	cleanup(operator);
	return(0);
}

/*
 * Reset any local memory information.
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
#ifdef BRISTOL_DBG
	printf("eswitchreset(%x)\n", operator);
#endif

	param->param[0].float_val = 1.0;
	param->param[1].float_val = 1.0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("eswitchparam(%x, %x, %i, %i)\n", operator, param, index, value);
#endif

	param->param[index].float_val = value;

	return(0);
}

/*
 * Amplifier - takes input signal and mod signal, and mults them together.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	register int count;
	register float *ib1, *ib2, *ob, *cbuf;
	bristolESWITCHMOD *specs;

	specs = (bristolESWITCHMOD *) operator->specs;
	count = specs->spec.io[ESWITCHMOD_OUT].samplecount;

#ifdef BRISTOL_DBG
	printf("eswitch(%x, %x, %i)\n", operator, param, count);
#endif

	ib1 = specs->spec.io[ESWITCHMOD_IN_1].buf;
	ib2 = specs->spec.io[ESWITCHMOD_IN_2].buf;
	cbuf = specs->spec.io[ESWITCHMOD_CLOCK].buf;
	ob = specs->spec.io[ESWITCHMOD_OUT].buf;
	
	for (; count > 0;count-=8)
	{
		/*
		 * If the clock signal is greater than zero take the next sample from
		 * input buffer one, other wise input buffer two. When it changes we
		 * should interpret a sample, but that can be done later.
		 */
/*		*ob++ = *ib1++ * *ib2++; */
		if (*cbuf++ >= 0)
		{
			*ob++ = *ib1++; ib2++;
			*ob++ = *ib1++; ib2++;
			*ob++ = *ib1++; ib2++;
			*ob++ = *ib1++; ib2++;
			*ob++ = *ib1++; ib2++;
			*ob++ = *ib1++; ib2++;
			*ob++ = *ib1++; ib2++;
			*ob++ = *ib1++; ib2++;
		} else {
			*ob++ = *ib2++; ib1++;
			*ob++ = *ib2++; ib1++;
			*ob++ = *ib2++; ib1++;
			*ob++ = *ib2++; ib1++;
			*ob++ = *ib2++; ib1++;
			*ob++ = *ib2++; ib1++;
			*ob++ = *ib2++; ib1++;
			*ob++ = *ib2++; ib1++;
		}
	}
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
eswitchinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolESWITCHMOD *specs;

#ifdef BRISTOL_DBG
	printf("eswitchinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	*operator = bristolOPinit(operator, index, samplecount);

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;

	specs = (bristolESWITCHMOD *) bristolmalloc0(sizeof(bristolESWITCHMOD));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolESWITCHMOD);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolESWITCHMODlocal);

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "Input 1";
	specs->spec.io[0].description = "Electromod Input 1 Signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_OUTPUT;
	specs->spec.io[1].ioname = "Input 2";
	specs->spec.io[1].description = "Electromod Input 2 Signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;
	specs->spec.io[2].ioname = "Clock Input";
	specs->spec.io[2].description = "Electromod clock Signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;
	specs->spec.io[3].ioname = "output";
	specs->spec.io[3].description = "Electromod Output Signal";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

