
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
 *	noiseinit()
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
#include "noise.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "Noise"
#define OPDESCRIPTION "Noise Generator"
#define PCOUNT 3
#define IOCOUNT 1

/* Improves with buffer size since smaller sizes are less random */
#define NOISESIZE 65536

#define NOISE_OUT_IND 0

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
	printf("noisereset(%x)\n", operator);
#endif
	param->param[0].float_val = 0.000001;
	param->param[1].int_val = 0;
	param->param[2].int_val = 0.25;
	param->param[3].int_val = 1.0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("noiseparam(%i, %f)\n", index, value);
#endif

	if (index == 0)
		param->param[index].float_val = value * 0.00025;

	if (index == 1)
	{
		if (value == 0)
			param->param[index].int_val = 0;
		else
			param->param[index].int_val = 1;
	}

	if (index == 2)
		param->param[index].float_val = value;

	return(0);
}

/*
static static float scale = 2.0f / 0xffffffff;
static float scale = 24.0f / 0xffffffff;
 */
static float scale = 0.0001;
int x1 = 0x67452301;
int x2 = 0xefcdab89;

static int
gennoise(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolNOISE *specs = (bristolNOISE *) operator->specs;
	register int i = specs->spec.io[NOISE_OUT_IND].samplecount;
	register float level, *ob;
	bristolNOISElocal * local = lcl;
	register float lp, lsv;

	ob = specs->spec.io[NOISE_OUT_IND].buf;
	level = param->param[0].float_val * scale;

	while (i-=4)
	{
		x1 ^= x2; *ob++ += x2 * level; x2 += x1;
		x1 ^= x2; *ob++ += x2 * level; x2 += x1;
		x1 ^= x2; *ob++ += x2 * level; x2 += x1;
		x1 ^= x2; *ob++ += x2 * level; x2 += x1;
	}

	if (param->param[1].int_val == 0)
		return(0);

	lp = param->param[2].float_val;
	lsv = local->coff;

	for (i = specs->spec.io[NOISE_OUT_IND].samplecount,
			ob = specs->spec.io[NOISE_OUT_IND].buf;
			i > 0;
			i--, ob++)
		*ob = (lsv += ((*ob - lsv) * lp));

	local->coff = lsv;

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
noiseinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolNOISE *specs;

#ifdef BRISTOL_DBG
	printf("noiseinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	*operator = bristolOPinit(operator, index, samplecount);

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = gennoise;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;

	specs = (bristolNOISE *) bristolmalloc0(sizeof(bristolNOISE));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolNOISE);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolNOISElocal);

	specs->spec.param[0].pname = "gain";
	specs->spec.param[0].description = "output gain on signal";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "White/Pink";
	specs->spec.param[1].description = "white pink";
	specs->spec.param[1].type = BRISTOL_ENUM;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_BUTTON;

	specs->spec.param[2].pname = "White/Pink";
	specs->spec.param[2].description = "white pink filtering";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "output";
	specs->spec.io[0].description = "Noise Output Signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

