
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

/*#define DEBUG */

#include <math.h>

#include "bristol.h"
#include "vibrachorus.h"

#define SPEED		0
#define DEPTH		1
#define FLAGS		2

#define OPNAME "VibraChorus"
#define OPDESCRIPTION "Vibrato Chorus"
#define PCOUNT 3
#define IOCOUNT 2

#define VCHORUS_IN_IND 0
#define VCHORUS_OUT_IND 1

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("destroy(%x)\n", operator);
#endif

	bristolfree(operator->specs);

	cleanup(operator);
	return(0);
}

/*
 * This is called by the frontend when a parameter is changed.
static int param(param, value)
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef DEBUG
	printf("checkParams(%f)\n", value);
#endif

	switch (index) {
		case 0:
		case 1:
			if ((param->param[index].int_val = value * 256) <= 0)
				param->param[index].int_val = 1;
			break;
		case 2:
			if (value == 0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			break;
	}
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
	if (param->param[0].mem)
		bristolfree(param->param[0].mem);

	param->param[0].mem = bristolmalloc0(sizeof(float) * HISTSIZE);

	param->param[0].int_val = 10;
	param->param[1].int_val = 10;
	param->param[2].int_val = 0;
	return(0);
}

static float table[TABSIZE];

/*
 * Vibrachorus will take a signal and give it vibra. Depending on the gain the
 * vibra can be remixed in with the original signal to create a chorus effect.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolVCHORUS *specs;
	bristolVCHORUSlocal *local = (bristolVCHORUSlocal *) lcl;
	register float *source, *dest;
	register int count, *history, depth, speed;
	register float freq1, histout, value, scanp, scanr, delay;
	register int histin, i, chorus;

	specs = (bristolVCHORUS *) operator->specs;

	count = specs->spec.io[VCHORUS_OUT_IND].samplecount;
	source = specs->spec.io[VCHORUS_IN_IND].buf;
	dest = specs->spec.io[VCHORUS_OUT_IND].buf;

	/*
	 * operational parameters.
	 */
	speed = param->param[SPEED].int_val;
	depth = param->param[DEPTH].int_val;
	chorus = param->param[FLAGS].int_val;
	history = param->param[0].mem;

#ifdef DEBUG
	printf("vchorus()\n");
#endif

	if (local == NULL)
		return(0);

	freq1 = local->Freq1;
	histin = local->Histin;
	histout = local->Histout;
	scanp = local->scanp;
	scanr = local->scanr;

	for (i = 0; i < count; i++) {
		history[histin] = *source;

		if ((histout + 1) >= HISTSIZE)
		{
			value = history[0] * (histout - ((float) ((int) histout)))
				+ history[(int) histout]
					* (1.0 - (histout - ((float) ((int) histout))));
		} else {
			value = history[(int) histout + 1]
					* (histout - ((float) ((int) histout)))
				+ history[(int) histout]
					* (1.0 - (histout - ((float) ((int) histout))));
		}

		if (chorus)
			*dest++ = *source + value;
		else
			*dest++ = value;

		source++;

		if (++histin >= HISTSIZE) histin = 0;
		if ((histout += freq1) >= HISTSIZE) histout -= HISTSIZE;
		if (histout < 0) histout = 0;

		/*
		 * The output buffers get a filtered volume fraction, plus a phase 
		 * component.
		 */
		if ((scanr -= 1) <= 0)
		{
			scanr = speed;

			/*
			 * Freq1 will initially define a length of the delay chain. Then
			 * we interpolate the changes we need to make to histout to move
			 * to this depth.
			 */
			freq1 = depth - table[(int) ((scanp + 100) >= TABSIZE?
				scanp + 100 - TABSIZE:scanp + 100)] * depth;

#ifdef DEBUG2
			printf("scan %3.0f: ddepth %f, tbl %f", scanp, freq1,
				table[(int) ((scanp + 100) >= TABSIZE?
					scanp + 100 - TABSIZE
					:scanp + 100)]);
#endif

			/*
			 * Get the current depth
			 */
			delay = histin + speed - histout < 0?
				((float) histin) + speed - histout + HISTSIZE
				:((float) histin) + speed - histout;

			freq1 = (delay - freq1) / speed;
#ifdef DEBUG2
			printf(" adepth %f, step %f\n", delay, freq1);
#endif
			if (++scanp >= TABSIZE)
				scanp = 0;
		}
	}

	local->Freq1 = freq1;
	local->Histin = histin;
	local->Histout = histout;
	local->scanr = scanr;
	local->scanp = scanp;
	return(0);
}

static void
buildSineTable(float *table)
{
	int i;

	for (i = 260; i < (TABSIZE + 260); i++)
		table[i - 260] = (float) sin(2 * M_PI * ((double) i) / TABSIZE);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
vchorusinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolVCHORUS *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("vchorusinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param= param;

	specs = (bristolVCHORUS *) bristolmalloc0(sizeof(bristolVCHORUS));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolVCHORUS);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolVCHORUSlocal);

	/*
	 * Now fill in the specs for this operator.
	 */
	specs->spec.param[0].pname = "speed";
	specs->spec.param[0].description= "rotation speed";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "depth";
	specs->spec.param[1].description = "depth of rotation";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "flags";
	specs->spec.param[2].description = "delayed signal";
	specs->spec.param[2].type = BRISTOL_INT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "Input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "output";
	specs->spec.io[1].description = "output signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	buildSineTable(table);

	return(*operator);
}

