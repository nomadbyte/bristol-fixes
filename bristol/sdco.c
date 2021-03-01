
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
/* This is sampler oscillator code. It is not used by any synth at the moment. */
/*
 * Need to have basic template for an operator. Will consist of
 *
 *	sdcoinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 *
 * This will be a resampling oscillator.
 */

#include <math.h>

#include "bristol.h"
#include "sdco.h"

float note_diff;

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "SDCO"
#define OPDESCRIPTION "Digitally Controlled Resampling Oscillator"
#define PCOUNT 4
#define IOCOUNT 2

#define SDCO_IN_IND 0
#define SDCO_OUT_IND 1

#define SDCO_WAVE_COUNT 6

static void fillWave();
extern int convertWave();

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("destroy(%x)\n", operator);
#endif

	bristolfree(operator->specs);

	/*
	 * Free any local memory. We should also free ourselves, since we did the
	 * initial allocation.
	 */
	cleanup(operator);
	return(0);
}

/*
 * Give our wavetable structure some default values (ie, empty it).
 */
static void
initWaveTable(sampleData *sd)
{
	int i;

	for (i = 0; i < 128; i++)
	{
		sd[i].layer[0].fade = GAIN_CONSTANT;
		sd[i].layer[1].fade = GAIN_POS;
		sd[i].layer[0].refwave = sd[i].layer[1].refwave = -1;
	}
}

/*
 * Reset any local memory information.
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
#ifdef BRISTOL_DBG
	printf("sdcoreset(%x)\n", operator);
#endif

	/*
	 * Malloc a wave table. We have 128 notes, each needs a sampleData entry.
	 */
	if (param->param[0].mem == 0)
	{
		param->param[0].mem = bristolmalloc0(sizeof(sampleData) * 128);

		initWaveTable(param->param[0].mem);
	}

	param->param[0].int_val = 0;
	param->param[1].float_val = 0.5;
	param->param[2].float_val = 1.0;
	param->param[3].float_val = 1.0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("sdcoparam(%x, %x, %i, %f)\n", operator, param, index, value);
#endif

	if (param->param[0].mem == 0)
	{
		param->param[0].mem = bristolmalloc0(sizeof(sampleData) * 128);

		initWaveTable(param->param[0].mem);
	}

	switch (index) {
		case 0:
			/*
			 * Build a new wave table?
			 */
			fillWave(param->param[0].mem, (int) value);
			break;
		case 1:
			/*
			 * Transpose in octaves. 0 to 5, down 2 up 3.
			 */
			{
				int note = value * CONTROLLER_RANGE;

				switch (note) {
					case 0:
						param->param[1].float_val = 0.25;
						break;
					case 1:
						param->param[1].float_val = 0.50;
						break;
					case 2:
						param->param[1].float_val = 1.0;
						break;
					case 3:
						param->param[1].float_val = 2.0;
						break;
					case 4:
						param->param[1].float_val = 4.0;
						break;
					case 5:
						param->param[1].float_val = 8.0;
						break;
				}
			}
			break;
		case 2: /* Tune in "small" amounts */
			{
				float tune, notes = 1.0;
				int i;

				tune = (value - 0.5) * 2;

				/*
				 * Up or down 7 notes.
				 */
				for (i = 0; i < 7;i++)
				{
					if (tune > 0)
						notes *= note_diff;
					else
						notes /= note_diff;
				}

				if (tune > 0)
					param->param[index].float_val =
						1.0 + (notes - 1) * tune;
				else
					param->param[index].float_val =
						1.0 - (1 - notes) * -tune;

				break;
			}
		case 3:
			param->param[index].float_val = value;
			break;
	}
printf("sdco %i, %f, %f, %f\n", param->param[0].int_val,
param->param[1].float_val, param->param[2].float_val,
param->param[3].float_val);

	return(0);
}

static void
runlayer(bristolVoice *voice, bristolOPParams *param, bristolSDCOlocal *local,
register int count, register float *ib, register float *ob, sampleData *sd,
register int layer)
{
	register int obp;
	register float *wt, wtp, gain, transp, sr;
	register sampleData *tsd, *rsd;

	if (((wt = sd[sd[voice->key.key].layer[layer].refwave].layer[layer].wave)
			== 0) || ((sd[voice->key.key].layer[layer].refwave < 0)))
		return;

	transp = param->param[1].float_val * param->param[2].float_val;

	wtp = local->wtptr[layer];
	tsd = &sd[voice->key.key];
	rsd = &sd[sd[voice->key.key].layer[layer].refwave];

	switch (tsd->layer[layer].fade) {
		case GAIN_POS:
			gain = param->param[3].float_val * voice->velocity;
			break;
		case GAIN_NEG:
			gain = 1.0 - param->param[3].float_val * voice->velocity;
			break;
		default:
			gain = 1.0;
			break;
	}

	if (wtp >= rsd->layer[layer].count)
	{
		local->wtptr[layer] = wtp;
		return;
	}

	sr = tsd->layer[layer].sr;
	if (layer == 1)
		sr *=1.01;

	/*
	 * Incorporate layer
	 */
	for (obp = 0; obp < count;obp++)
	{
		/*
		 * This is for the sampler and needs to be resample.
		 */
		ob[obp] += (wt[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
			+ wt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp))))) * gain;

		/*
		 * Arguably we should have the following to allow for mods.
		if ((wtp += tsd->layer[layer].sr * transp) >= rsd->layer[layer].count)
		 */
		if ((wtp += (sr + ib[obp]) * transp) >= rsd->layer[layer].count)
			break;
	}

	local->wtptr[layer] = wtp;
}

/*
 * As of the first write, 9/11/01, this takes nearly 20mips to operate a single
 * oscillator. Will work on optimisation of the code, using non-referenced 
 * variables in register workspace.
 *
 *	output buffer pointer.
 *	output buffer index.
 *	input buffer index.
 *	wavetable.
 *	wavetable index.
 *	count.
 *	gain.
 *
 * With optimisations this is reduced to a nominal amount. Put most parameters
 * in registers, and then stretched the for loop to multiples of 16 samples.
 *
 * I would prefer to have this turned into a linear input, rated at 1float per
 * octave. This would require some considerable alterations to the oscillator
 * sample step code, though. The basic operation is reasonably simple - we take
 * a (midikey/12) and it gives us a step rate. We need to work in portamento and
 * some more subtle oscillator driver changes.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolSDCOlocal *local = lcl;
	int count;
	float *ib, *ob;
	bristolSDCO *specs;
	sampleData *sd = (sampleData *) param->param[0].mem;

	specs = (bristolSDCO *) operator->specs;

#ifdef BRISTOL_DBG
	printf("sdco(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[SDCO_OUT_IND].samplecount;
	ib = specs->spec.io[SDCO_IN_IND].buf;
	ob = specs->spec.io[SDCO_OUT_IND].buf;

	if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
		local->wtptr[0] = local->wtptr[1] = 0;

	runlayer(voice, param, local, count, ib, ob, sd, 0);
	runlayer(voice, param, local, count, ib, ob, sd, 1);

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
sdcoinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolSDCO *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("sdcoinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	note_diff = pow(2, ((double) 1)/12);

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param= param;

	specs = (bristolSDCO *) bristolmalloc0(sizeof(bristolSDCO));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolSDCO);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolSDCOlocal);

	/*
	 * Now fill in the sdco specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "waveform";
	specs->spec.param[0].description= "sample selection";
	specs->spec.param[0].type = BRISTOL_ENUM;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 5;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "Transpose";
	specs->spec.param[1].description = "Octave transposition";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 12;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "Tune";
	specs->spec.param[2].description = "fine tuning of frequency";
	specs->spec.param[2].type = BRISTOL_INT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 127;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "gain";
	specs->spec.param[3].description = "output gain on signal";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 2;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	/*
	 * Now fill in the sdco IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "input rate modulation signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "output";
	specs->spec.io[1].description = "oscillator output signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

/*
 * Waves have a range of 24, which is basically two octaves. For larger 
 * differences will have to apply apms.
 */
static void
fillWave(sampleData mem[], int type)
{
#ifdef BRISTOL_DBG
	printf("fillWave(%x, %i)\n", mem, type);
#endif

	switch (type) {
		case 0:
			/*
			 * Load up the samples for the rhodes. Open the rhodes sample(s),
			 * convert the data into floats, and put in any necessary note
			 * references for internote interpolations.
			 *
			 * At the moment (12/6/02) we only load a single sample.
			 * We need to put a shim in here to link through to libsndutil or
			 * similar library, and allow that to do our convertion work for us.
			 * Preferably the library should recognise some of the common 
			 * sampler formats such as AKAI, Ensoniq, etc, where a soundfile
			 * has a compound of multiple notes and layers.
			 */
			if (convertWave(mem, "/tmp/rhodespiano.raw", HINT_RHODES, 38, 1)
				< 0)
				printf("issue loading rhodes piano sample\n");
/*			if (convertWave(mem, "/tmp/rhodes.raw", HINT_RHODES, 39, 0) < 0) */
/*				printf("issue loading rhodes forte samples\n"); */

			break;
		case 1:
			/*
			 * Load up the samples for the grand
			 */
			break;
	}
}

