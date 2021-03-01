
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
 *	hpfinit()
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
#include "hpf.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "HPF"
#define OPDESCRIPTION "Digital High Pass Filter"
#define PCOUNT 1
#define IOCOUNT 2

#define HPF_IN_IND 0
#define HPF_OUT_IND 1

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
	printf("reset(%x)\n", operator);
#endif
	param->param[2].float_val = 1.0;
	param->param[3].int_val = 0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("param(%x, %f)\n", operator,	value);
#endif

	switch (index) {
		case 0:
			param->param[index].float_val = value / 2;
			break;
		case 1:
			param->param[index].float_val =
				gainTable[CONTROLLER_RANGE - 1
					- (int) (value * CONTROLLER_RANGE)].gain;
			break;
		case 2:
			param->param[index].float_val = value;
			break;
		case 3:
			if (value > 0)
				param->param[index].int_val = 1;
			else
				param->param[index].int_val = 0;
			break;
	}

/*	printf("hpfparam(%x, %x, %i, %f): %f\n", operator, param, index, value, */
/*	param->param[index].float_val); */
	return(0);
}

/*
 * hpf - takes input signal and hpfs it according to the mod level.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolHPFlocal *local = lcl;
	register int count;
	register float *ib, *ob;
	bristolHPF *specs;
	register float BLim;
	register float Bout = local->Bout;
	register float oSource = local->oSource;

	register float cutoff;

	/*
	 * Every operator accesses these variables, the count, and a pointer to
	 * each buffer. We should consider passing them as readymade parameters?
	 *
	 * The Filter now takes normalised inputs, in ranges of 12PO.
	 */
	specs = (bristolHPF *) operator->specs;
	count = specs->spec.io[HPF_OUT_IND].samplecount;
	ib = specs->spec.io[HPF_IN_IND].buf;
	ob = specs->spec.io[HPF_OUT_IND].buf;

	/*
	 * This is the code from one of the SLab floating point hpf routines.
	 * It has been altered here to have a single lowpass hpf, and will be
	 * a starting point for the Bristol hpfs. There will end up being a
	 * number of hpf algorithms.
	if (param->param[3].int_val) {
		if ((BLim = param->param[0].float_val + voice->dFreq / 128) > 1.0)
			BLim = 1.0;
		cutoff = param->param[0].float_val * voice->dFreq / 4;
	} else
	 */
		cutoff = BLim = param->param[0].float_val;
	if (voice->flags & BRISTOL_KEYON)
	{
		/*
		 * Do any relevant note_on initialisation.
		 */
		oSource = 0.0;
		Bout = 0.0;
	}

#ifdef BRISTOL_DBG
	printf("hpf(%x, %x, %x)\n", operator, param, local);
#endif

	/*
	 * subtract a delta from original signal?
	 */
	for (; count > 0; count-=8)
	{
		/*
		 * Take the output value.
		 */
		Bout += (*ib - Bout) * BLim;
		*ob++ = *ib++ - Bout;
		Bout += (*ib - Bout) * BLim;
		*ob++ = *ib++ - Bout;
		Bout += (*ib - Bout) * BLim;
		*ob++ = *ib++ - Bout;
		Bout += (*ib - Bout) * BLim;
		*ob++ = *ib++ - Bout;
		Bout += (*ib - Bout) * BLim;
		*ob++ = *ib++ - Bout;
		Bout += (*ib - Bout) * BLim;
		*ob++ = *ib++ - Bout;
		Bout += (*ib - Bout) * BLim;
		*ob++ = *ib++ - Bout;
		Bout += (*ib - Bout) * BLim;
		*ob++ = *ib++ - Bout;
	}

	/*
	 * Put back the local variables
	 */
	local->Bout = Bout;
	local->oSource = oSource;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
hpfinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolHPF *specs;

#ifdef BRISTOL_DBG
	printf("hpfinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolHPF *) bristolmalloc0(sizeof(bristolHPF));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolHPF);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolHPFlocal);

	/*
	 * Now fill in the control specs for this hpf.
	 */
	specs->spec.param[0].pname = "cutoff";
	specs->spec.param[0].description = "Filter cutoff frequency";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "Filter Input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "output";
	specs->spec.io[1].description = "Filter Output Signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

