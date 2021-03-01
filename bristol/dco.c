
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
 *	dcoinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */

#include <math.h>

#include "bristol.h"
#include "bristolblo.h"
#include "dco.h"

float note_diff;

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "DCO"
#define OPDESCRIPTION "Digitally Controlled Oscillator"
#define PCOUNT 2
#define IOCOUNT 2
/* static char *IO[] = {"FreqMod", "Output"}; */

#define DCO_IN_IND 0
#define DCO_OUT_IND 1

#define DCO_WAVE_COUNT 6

static void fillWave();

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
	bristolfree(((bristolDCO *) operator)->wave[0]);
	bristolfree(((bristolDCO *) operator)->wave[1]);
	bristolfree(((bristolDCO *) operator)->wave[2]);
	bristolfree(((bristolDCO *) operator)->wave[3]);
	bristolfree(((bristolDCO *) operator)->wave[4]);
	bristolfree(((bristolDCO *) operator)->wave[5]);
	bristolfree(((bristolDCO *) operator)->wave[6]);
	bristolfree(((bristolDCO *) operator)->wave[7]);

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
	printf("dcoreset(%x)\n", operator);
#endif

	if (param->param[0].mem != NULL)
		bristolfree(param->param[0].mem);
	param->param[0].mem = bristolmalloc0(sizeof(float) * DCO_WAVE_SZE);
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
	printf("dcoparam(%x, %x, %i, %f)\n", operator, param, index, value);
#endif

	switch (index) {
		case 0:
			if ((param->param[index].int_val = value * CONTROLLER_RANGE)
				> DCO_WAVE_COUNT)
				param->param[index].int_val = DCO_WAVE_COUNT;
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

	return(0);
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
	bristolDCOlocal *local = lcl;
	register int obp, count;
	register float *ib, *ob, *wt, wtp, gain, transp;
	bristolDCO *specs;

	specs = (bristolDCO *) operator->specs;

#ifdef BRISTOL_DBG
	printf("dco(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[DCO_OUT_IND].samplecount;
	ib = specs->spec.io[DCO_IN_IND].buf;
	ob = specs->spec.io[DCO_OUT_IND].buf;
	wtp = local->wtp;
	gain = param->param[3].float_val;
	transp = param->param[1].float_val * param->param[2].float_val;
	wtp = local->wtp;

 	if (bristolBLOcheck(voice->cFreq*transp)) {
		wt = param->param[0].mem;
		memset(wt, 0, DCO_WAVE_SZE * sizeof(float));
		switch (param->param[0].int_val) {
			case 0:
				/* Sine, cannot BWL this */
				wt = specs->wave[param->param[0].int_val];
				break;
			case 1:
				generateBLOwaveformF(voice->cFreq*transp, wt, BLO_SQUARE);
				break;
			case 2:
				/* This will be pulse */
				generateBLOwaveformF(voice->cFreq*transp, wt, BLO_PULSE);
				break;
			case 3:
				generateBLOwaveformF(voice->cFreq*transp, wt, BLO_SAW);
				break;
			case 4:
				generateBLOwaveformF(voice->cFreq*transp, wt, BLO_TRI);
				break;
			case 5:
			{
				float ws[1024];
				memset(ws, 0, DCO_WAVE_SZE * sizeof(float));
				generateBLOwaveformF(voice->cFreq*transp, &ws[0], BLO_SAW);

				for (obp = 0; obp < 1024; obp++)
					wt[obp] = (ws[obp] + ws[(obp * 2) & 1023]) * 0.5;

				break;
			}
			case 6:
				generateBLOwaveformF(voice->cFreq*transp, wt, BLO_RAMP);
				break;
			case 7:
				generateBLOwaveformF(voice->cFreq*transp, wt, BLO_RAMP);
				break;
		}
	} else
		wt = specs->wave[param->param[0].int_val];

/*printf("%i, %f, %i: %x %x %x\n", count, gain, param->param[0].int_val, wt, ib, ob); */
	/*
	 * Go jumping through the wavetable, with each jump defined by the value
	 * given on our input line, making sure we fill one output buffer.
	 */
	for (obp = 0; obp < count; obp++)
	{
		if (wtp >= DCO_WAVE_SZE_M)
			ob[obp] += (wt[0] * (wtp - ((float) ((int) wtp)))
				+ wt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp))))) * gain;
		else
			ob[obp] += (wt[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
				+ wt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp))))) * gain;

		/*
		 * Move the wavetable pointer forward by amount indicated in input 
		 * buffer for this sample.
		 */
		if ((wtp += ib[obp] * transp) >= DCO_WAVE_SZE)
		{
			while (wtp >= DCO_WAVE_SZE)
				wtp -= DCO_WAVE_SZE;
		}
		/*
		 * If we have gone negative, round back up. Allows us to run the 
		 * oscillator backwards.
		 */
		while (wtp < 0)
			wtp += DCO_WAVE_SZE;
	}

	local->wtp = wtp;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
dcoinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolDCO *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("dcoinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolDCO *) bristolmalloc0(sizeof(bristolDCO));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolDCO);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolDCOlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(DCO_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(DCO_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(DCO_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(DCO_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(DCO_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(DCO_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(DCO_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(DCO_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], DCO_WAVE_SZE, 0);
	fillWave(specs->wave[1], DCO_WAVE_SZE, 1);
	fillWave(specs->wave[2], DCO_WAVE_SZE, 2);
	fillWave(specs->wave[3], DCO_WAVE_SZE, 3);
	fillWave(specs->wave[4], DCO_WAVE_SZE, 4);
	fillWave(specs->wave[5], DCO_WAVE_SZE, 5);
	fillWave(specs->wave[6], DCO_WAVE_SZE, 6);
	fillWave(specs->wave[7], DCO_WAVE_SZE, 7);

	/*
	 * Now fill in the dco specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "waveform";
	specs->spec.param[0].description= "sine square pulse ramp triangle tan saw";
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
	 * Now fill in the dco IO specs.
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
 * differences will have to apply apms. The diverse leading edge wave have a
 * rather raw feeling and should be smoothed out a bit.
 */
static void
fillWave(float *mem, int count, int type)
{
	int i;

#ifdef BRISTOL_DBG
	printf("fillWave(%x, %i, %i)\n", mem, count, type);
#endif

	switch (type) {
		case 0:
			/*
			 * This will be a sine wave. We have count samples, and
			 * 2PI radians in a full sine wave. Thus we take
			 * 		(2PI * i / count) * 2048.
			 */
			for (i = 0;i < count; i++)
				mem[i] = sin(2 * M_PI * ((double) i) / count) * BRISTOL_VPO;
			return;
		case 1:
		default:
			/* 
			 * This is a square wave.
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = blosquare[i];
				return;
			}
			for (i = 0;i < count / 2; i++)
				mem[i] = BRISTOL_VPO * 2 / 3;
			for (;i < count; i++)
				mem[i] = -BRISTOL_VPO * 2 / 3;
			mem[0] = mem[count / 2] = 0;
			mem[count - 1] = mem[1]
				= mem[count / 2 - 1] = mem[count / 2 + 1]
				= mem[2] / 2;
			return;
		case 2:
			/* 
			 * This is a pulse wave (we do not have PWM yet). We should go and
			 * build this out of a subtracted offset ramp wave if BLO is 
			 * enabled.
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = blopulse[i];
				return;
			}
			for (i = 0;i < count / 5; i++)
				mem[i] = BRISTOL_VPO * 2 / 3;
			for (;i < count; i++)
				mem[i] = -BRISTOL_VPO * 2 / 3;
			mem[0] = mem[count / 2] = 0;
			mem[count - 1] = mem[1]
				= mem[count / 2 - 1] = mem[count / 2 + 1]
				= mem[2] / 2;
			return;
		case 3:
			/* 
			 * This is a ramp wave. We scale the index from -.5 to .5, and
			 * multiply by the range. We go from rear to front to table to make
			 * the ramp wave have a positive leading edge.
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = bloramp[i];
				return;
			}
			for (i = count - 1;i >= 0; i--)
				mem[i] = (((float) i / count) - 0.5) * BRISTOL_VPO * 2.0;
			mem[0] = 0;
			mem[count - 1] = mem[1]
				= BRISTOL_VPO;
			return;
		case 4:
			/*
			 * Triangular wave. From MIN point, ramp up at twice the rate of
			 * the ramp wave, then ramp down at same rate.
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = blotriangle[i];
				return;
			}
			for (i = 0;i < count / 2; i++)
				mem[i] = -BRISTOL_VPO
					+ ((float) i * 2 / (count / 2)) * BRISTOL_VPO; 
			for (;i < count; i++)
				mem[i] = BRISTOL_VPO -
					(((float) (i - count / 2) * 2) / (count / 2)) * BRISTOL_VPO;
			return;
		case 5:
			/*
			 * Would like to put in a jagged edged ramp wave. Should make some
			 * interesting strings sounds. If we have BLO enabled then we are
			 * just going to layer the wave with the second harmonic of itself.
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0; i < count; i++)
					mem[i] = (blosaw[i] + blosaw[(i * 2) & 1023]) * 0.5;

				return;
			}
			{
				float accum = -BRISTOL_VPO;

				for (i = 0; i < count / 2; i++)
				{
					mem[i] = accum +
						(0.5 - (((float) i) / count)) * BRISTOL_VPO * 4;
					if (i == count / 8)
						accum = -BRISTOL_VPO * 3 / 4;
					if (i == count / 4)
						accum = -BRISTOL_VPO / 2;
					if (i == count * 3 / 8)
						accum = -BRISTOL_VPO / 4;
				}
				for (; i < count; i++)
					mem[i] = -mem[count - i];
			}
			return;
		case 6:
			/*
			 * Tangiential wave. We limit some of the values, since they do get
			 * excessive. This is only half a tan as well, to maintain the
			 * base frequency.
			 *
			 * This should be dropped.
			 */
			for (i = 0;i < count; i++)
			{
				if ((mem[i] =
					tan(M_PI * ((double) i) / count) * BRISTOL_VPO / 16)
					> BRISTOL_VPO * 8)
					mem[i] = BRISTOL_VPO * 6;
				if (mem[i] < -(BRISTOL_VPO * 6))
					mem[i] = -BRISTOL_VPO * 6;
			}
			return;
		case 7:
			/*
			 * This will be to load a (fixed?) filename. Should probably make
			 * another operator for sample management though, since it would
			 * be better to have pitch shifting incorporated.
			 */
			return;
	}
}

