
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
 *	ringmodinit()
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
#include "ringmod.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "Ringmod"
#define OPDESCRIPTION "Ringmod Generator"
#define PCOUNT 3
#define IOCOUNT 3

#define RINGMOD_IN_1 0
#define RINGMOD_IN_2 1
#define RINGMOD_OUT 2

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
	printf("ringmodreset(%x)\n", operator);
#endif

	param->param[0].float_val = 1.0;
	param->param[1].float_val = 1.0;
	param->param[2].float_val = 1.0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("ringmodparam(%x, %x, %i, %i)\n", operator, param, index, value);
#endif

	if (index == 2)
	{
		param->param[index].float_val = value * 4;
		return(0);
	}
	param->param[index].float_val = value * 2;

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
	register float *ib1, *ib2, *ob, gain;
	bristolRINGMOD *specs;

	specs = (bristolRINGMOD *) operator->specs;
	count = specs->spec.io[RINGMOD_OUT].samplecount;

#ifdef BRISTOL_DBG
	printf("ringmod(%x, %x, %i)\n", operator, param, count);
#endif

	ib1 = specs->spec.io[RINGMOD_IN_1].buf;
	ib2 = specs->spec.io[RINGMOD_IN_2].buf;
	ob = specs->spec.io[RINGMOD_OUT].buf;

	/*
	 * See if we have a gain configured on the second input. If we do then
	 * take that as the ringmod input. If not then take that as the input
	 * selector - ib1 if +ve, otherwise zero. This emulates the DC capabilities
	 * of the original rinngmod, although it could better be another option.
	 */
	if (param->param[1].float_val > 0)
	{
		gain = 0.0833 * param->param[0].float_val * param->param[1].float_val
			* param->param[2].float_val;

		for (; count > 0;count-=8)
		{
			*ob++ = *ib1++ * *ib2++ * gain;
			*ob++ = *ib1++ * *ib2++ * gain;
			*ob++ = *ib1++ * *ib2++ * gain;
			*ob++ = *ib1++ * *ib2++ * gain;
			*ob++ = *ib1++ * *ib2++ * gain;
			*ob++ = *ib1++ * *ib2++ * gain;
			*ob++ = *ib1++ * *ib2++ * gain;
			*ob++ = *ib1++ * *ib2++ * gain;
		}
	} else {
		gain = param->param[0].float_val;

		for (; count > 0;count-=8)
		{
			if (*ib2++ >= 0)
				*ob++ = *ib1++ * gain;
			else
				*ob++ = 0;
		}
	}
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
ringmodinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolRINGMOD *specs;

#ifdef BRISTOL_DBG
	printf("ringmodinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolRINGMOD *) bristolmalloc0(sizeof(bristolRINGMOD));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolRINGMOD);

	specs->spec.param[0].pname = "gain 1";
	specs->spec.param[0].description= "Input signal gain 1";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "gain 2";
	specs->spec.param[1].description = "Input signal gain 2";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "gain";
	specs->spec.param[2].description = "Output signal gain";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolRINGMODlocal);

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "Input 1";
	specs->spec.io[0].description = "Ringmod Input 1 Signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_OUTPUT;
	specs->spec.io[1].ioname = "Input 2";
	specs->spec.io[1].description = "Ringmod Input 2 Signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;
	specs->spec.io[2].ioname = "output";
	specs->spec.io[2].description = "Ringmod Output Signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

