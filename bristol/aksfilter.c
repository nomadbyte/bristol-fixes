
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
 *	filterinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */
/*
 * This implements three filter algorithms used by the different bristol
 * emulations. These are a butterworth used by the leslie, a chamberlain 
 * used generally, and a rooney used for some of the filter layering and
 * eventually the mixing.
 */
#include <math.h>

#include "bristol.h"
#include "aksfilter.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "DCF"
#define OPDESCRIPTION "Digital Filter Two"
#define PCOUNT 6
#define IOCOUNT 3

#define FILTER_IN_IND 0
#define FILTER_MOD_IND 1
#define FILTER_OUT_IND 2

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("destroy(%x)\n", operator);
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

#define ROOT2 1.4142135623730950488

/*
 * Reset any local memory information.
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif

	param->param[0].float_val = 0.5;
	param->param[1].float_val = 0.5;
	param->param[2].float_val = 0.5;
	param->param[3].int_val = 0; /* Keyboard tracking */
	param->param[4].int_val = 0; /* Filter algorithhm */
	param->param[5].float_val = 1.0;
	return(0);
}

/*
 * Alter an internal parameter of operator.
 */
static int
param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	int intvalue = value * CONTROLLER_RANGE;

#ifdef BRISTOL_DBG
	printf("param(%x, %f)\n", index, value);
#endif

	switch (index) {
		case 0:
			if (intvalue == 0)
				value = gainTable[intvalue].gain;
			else
				value = gainTable[intvalue - 1].gain;
			param->param[0].float_val =
				value + param->param[2].float_val / 512.0f;
			param->param[3].float_val = value;
			break;
		case 1:
			param->param[index].float_val = value;

			param->param[index].int_val = intvalue;
			break;
		case 2: /* Fine tune. */
			param->param[0].float_val =
				param->param[3].float_val + value / 512.0f;
			param->param[2].float_val = value;
			break;
		case 5: /* Gain */
			param->param[index].float_val = value;
			break;
		case 3:
			if (value > 0)
				param->param[index].int_val = 1;
			else
				param->param[index].int_val = 0;
/*			param->param[index].float_val = value; */
			break;
		case 4:
/* printf("Configuring filter %i\n", intvalue); */
			break;
	}

	return(0);
}

#define BRISTOL_SQR 4
#define S_DEC 0.999f

static void
fillWave(float *mem, int count, int type)
{
	int i;

#ifdef BRISTOL_DBG
	printf("fillWave(%x, %i, %i)\n", mem, count, type);
#endif

	/*
	 * This will be a cosine wave. We have count samples, and 2PI radians in
	 * a full wave. Thus we take
	 * 		(2PI * i / count) * 2048.
	 * We fill count + 1 samples to simplify the resampling.
	 */
	for (i = 0;i < count; i++)
		mem[i] = cosf(2 * M_PI * ((float) i) / count);
}

/*
 * filter - takes input signal and filters it according to the mod level.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolAKSFILTERlocal *local = lcl;
	register int count;
	register float *ib, *ob, *mb, cutoff, freq, emphasis, gain, step, sr;
	register float *wave, hp, emp2, Eout, roc;
	register float Bout = local->Bout;
	bristolAKSFILTER *specs;

	/*
	 * Every operator accesses these variables, the count, and a pointer to
	 * each buffer. We should consider passing them as readymade parameters?
	 *
	 * The Filter now takes normalised inputs, in ranges of 12PO.
	 */
	specs = (bristolAKSFILTER *) operator->specs;
	count = specs->spec.io[FILTER_OUT_IND].samplecount;
	ib = specs->spec.io[FILTER_IN_IND].buf;
	mb = specs->spec.io[FILTER_MOD_IND].buf;
	ob = specs->spec.io[FILTER_OUT_IND].buf;

	cutoff = param->param[0].float_val;

#define E_LIM 8.1f

	emphasis = param->param[1].float_val;
	emp2 = (1.0 - emphasis) * 0.05;
	emp2 = 1.0 - emphasis;
	/*emphasis *= E_LIM; */
	gain = param->param[5].float_val;

	sr = specs->spec.io[FILTER_OUT_IND].samplerate;
	wave = specs->wave;

	step = local->step;
	hp = local->hp;
	Eout = local->Eout;
	roc = local->roc;

#ifdef BRISTOL_DBG
	printf("aksfilter(%x, %x, %x)\n", operator, param, local);
#endif

	/*
	 * Cutoff is now a parameter between zero and one and should represent
	 * a frequency from 0 to 16kHz. It can be modified so must be calculated
	 * on the fly from the mod buffer.
	 */
	for (; count > 0;count--)
	{
		if ((freq = cutoff + *mb * 0.1) < 0)
			freq = 0.00001f;
		else if (freq > 1.0)
			freq = 1.0f;

		*ob = (Bout += ((*ib - Bout) * freq));

		/*
		inc = 1024.0f / (sr / (freq * 16000.0f)); 

		if ((step += inc) >= AKS_WAVE_SZE)
			while (step >= AKS_WAVE_SZE)
				step -= AKS_WAVE_SZE;

		gdelta = step - ((float) ((int) step));

		out = wave[(int) step]
			+ (wave[(int) step + 1] - wave[(int) step]) * gdelta;

		 * This does not work to bad, but the emphasis should control the gain
		 * of the harmonic and the rate at which it tracks the input signal.
		 * the latter should be a inversely proportional. It is a HPF.
		 *
		if (step < AKS_WAVE_SZE_H)
		{
			if (out > *ib) {
				if ((hp += emp2) > E_LIM)
					hp = E_LIM;
			} else if (out < *ib) {
				if ((hp -= emp2) < 0)
					hp = 0.0;
			}
		} else {
			if (out < *ib) {
				if ((hp += emp2) > E_LIM)
					hp = E_LIM;
			} else if (out > *ib) {
				if ((hp -= emp2) < 0)
					hp = 0.0;
			}
		}
		 *
		 * Due to that, what we are going to do is use another filter, a low
		 * pass filter with a different frequency to the primary filter. It
		 * will give us a high pass envelope that will control the gain of
		 * the resonace. As resonance goes up its gain does, but also lowers
		 * the frequency of the HPF used so that it tends to flat.
		 *
		 * At full on then the HPF will tend to be a flat signal, the gain will
		 * max out and give us pure self oscillation.
		hp = *ib - (Eout += ((*ib - Eout) * emp2));

		*ob = out * hp * emphasis;
		 *
		 * That is not great either. Going to implement a filter worked on a
		 * while ago that uses a 'dot' that tracks the input signal. The dot
		 * is connected by attraction and gains a velocity that is affected by
		 * the attraction.
		 *
		 * ROC is affected by the current output adjusted by the delta signal:
		 * This oscillates nicely but is not too warm.
		 */
		/* The attraction g */
		if ((roc += (*ob - Eout) * freq) > E_LIM)
			roc = E_LIM;
		Eout += roc;
/*		*ob += Eout * emphasis * 10.25; */
		*ob += Eout * emphasis * 10.25;

		ib++;
		mb++;
		*ob++ *= gain;
	}

	local->roc = roc;
	local->hp = hp;
	local->step = step;
	local->Bout = Bout;
	local->Eout = Eout;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
aksfilterinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolAKSFILTER *specs;

#ifdef BRISTOL_DBG
	printf("filterinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolAKSFILTER *) bristolmalloc0(sizeof(bristolAKSFILTER));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolAKSFILTER);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolAKSFILTERlocal);

	specs->wave = (float *) bristolmalloc(AKS_WAVE_SZE * (sizeof(float) + 1));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave, AKS_WAVE_SZE + 1, 0);

	/*
	 * Now fill in the control specs for this filter.
	 */
	specs->spec.param[0].pname = "cutoff";
	specs->spec.param[0].description = "Filter cutoff frequency";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "resonance";
	specs->spec.param[1].description = "Filter emphasis";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "modulation";
	specs->spec.param[2].description = "Depth of modulation control";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "keyboard tracking";
	specs->spec.param[3].description = "Cutoff tracks keyboard";
	specs->spec.param[3].type = BRISTOL_TOGGLE;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_BUTTON;

	specs->spec.param[4].pname = "filter type";
	specs->spec.param[4].description = "Rooney, Butterworth, Chamberlain";
	specs->spec.param[4].type = BRISTOL_TOGGLE;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_BUTTON;

	specs->spec.param[5].pname = "gain";
	specs->spec.param[5].description = "Filter gain";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "Filter Input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "mod";
	specs->spec.io[1].description = "Filter Control Signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_DC|BRISTOL_INPUT|BRISTOL_HIDE;

	specs->spec.io[2].ioname = "output";
	specs->spec.io[2].description = "Filter Output Signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

