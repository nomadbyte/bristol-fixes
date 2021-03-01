
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
 *	dcainit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */

#include "bristol.h"
#include "dca.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "DCA"
#define OPDESCRIPTION "Digital Amplifier"
#define PCOUNT 0
#define IOCOUNT 3

#define DCA_IN_IND 0
#define DCA_MOD_IND 1
#define DCA_OUT_IND 2

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
static int reset(bristolOP *operator, bristolDCA *local)
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif
	param->param[0].float_val = 1.0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("param(%x)\n", operator);
#endif

	/*
	 * Gain parameter, not used at the moment.....
	 */
	if (index == 0)
		param->param[0].float_val = value;

	return(0);
}

/*
 * Amplifier - takes input signal and mod signal, and mults them together.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param, void *lcl)
{
	register int count;
	register float *ib, *ob, *mb, gain;
	bristolDCA *specs;

	specs = (bristolDCA *) operator->specs;
	count = specs->spec.io[DCA_OUT_IND].samplecount;

#ifdef BRISTOL_DBG
	printf("dca(%x, %x, %x)\n", operator, param, lcl);
#endif

	ib = specs->spec.io[DCA_IN_IND].buf;
	mb = specs->spec.io[DCA_MOD_IND].buf;
	ob = specs->spec.io[DCA_OUT_IND].buf;
	gain = param->param[0].float_val;

	/*
printf("%x, %x, %x\n", ib, mb, ob);
	 * Go through each sample and amplify it, correcting gain back to zero.
	 */
	for (;count > 0; count-=16)
	{
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
		*ob++ += *ib++ * *mb++;
	}
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
dcainit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolDCA *specs;

#ifdef BRISTOL_DBG
	printf("dcainit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolDCA *) bristolmalloc0(sizeof(bristolDCA));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolDCA);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolDCAlocal);

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "Amplifier Input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "mod";
	specs->spec.io[1].description = "Amplifier Control Signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_DC|BRISTOL_INPUT|BRISTOL_HIDE;

	specs->spec.io[2].ioname = "output";
	specs->spec.io[2].description = "Amplifier Output Signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

