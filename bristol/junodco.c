
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
 *	junodcoinit()
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
#include "junodco.h"

float note_diff;

#define BRISTOL_SQR 8

static void buildJunoWave(float *, float, float *, float, float *);
/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "DCO"
#define OPDESCRIPTION "Juno Digitally Controlled Oscillator"
#define PCOUNT 8
#define IOCOUNT 3

#define DCO_IN_IND 0
#define DCO_MOD_IND 1
#define DCO_OUT_IND 2

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
	bristolfree(((bristolJUNODCO *) operator)->wave[0]);
	bristolfree(((bristolJUNODCO *) operator)->wave[1]);
	bristolfree(((bristolJUNODCO *) operator)->wave[2]);
	bristolfree(((bristolJUNODCO *) operator)->wave[3]);
	bristolfree(((bristolJUNODCO *) operator)->wave[4]);
	bristolfree(((bristolJUNODCO *) operator)->wave[5]);
	bristolfree(((bristolJUNODCO *) operator)->wave[6]);
	bristolfree(((bristolJUNODCO *) operator)->wave[7]);

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
	printf("junoreset(%x)\n", operator);
#endif

	if (param->param[0].mem != NULL)
		bristolfree(param->param[0].mem);
	param->param[0].mem = bristolmalloc0(sizeof(float) * JUNODCO_WAVE_SZE);
	if (param->param[1].mem != NULL)
		bristolfree(param->param[1].mem);
	param->param[1].mem = bristolmalloc0(sizeof(float) * JUNODCO_WAVE_SZE);
	if (param->param[2].mem != NULL)
		bristolfree(param->param[2].mem);
	param->param[2].mem = bristolmalloc0(sizeof(float) * JUNODCO_WAVE_SZE);
	if (param->param[3].mem != NULL)
		bristolfree(param->param[3].mem);
	param->param[3].mem = bristolmalloc0(sizeof(float) * JUNODCO_WAVE_SZE);

	param->param[0].float_val = 512;
	param->param[1].float_val = 1.0;
	param->param[2].float_val = 1.0;
	param->param[3].float_val = 1.0;
	param->param[6].int_val = 0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	bristolJUNODCO *specs = (bristolJUNODCO *) operator->specs;

#ifdef BRISTOL_DBG
	printf("junodcoparam(%x, %x, %i, %f)\n", operator, param, index, value);
#endif

	switch (index) {
		case 0:
			/*
			 * Make sure we do not get a too small pulse.
			 */
			if ((param->param[index].float_val = value * JUNODCO_WAVE_SZE / 2)
				< 25)
				param->param[index].float_val = 25;
/*printf("width is now %i\n", param->param[index].float_val); */
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
/*printf("transpose %f\n", param->param[1].float_val); */
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
		case 3: /* gain */
			param->param[index].float_val = value;
			break;
		case 4: /* mix ramp */
		case 5: /* mix tri */
			param->param[index].float_val = value * 2;
			buildJunoWave(param->param[0].mem,
				param->param[4].float_val, specs->wave[3],
				param->param[5].float_val, specs->wave[4]);
			break;
		case 6: /* mix sub square */
			param->param[index].float_val = value * 4;
			break;
		case 7: /* mix PWM square */
printf("junodcoparam(%i, %f)\n", 3, param->param[3].float_val);
			if (value == 0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			break;
	}
	return(0);
}

static void buildJunoWave(register float *wave,
	register float ramp, register float *rampw,
	register float tri, register float *triw)
{
	register int i;

/*	printf("buildJunoWave(%f, %f)\n", ramp, tri); */

	bristolbzero(wave, sizeof(float) * JUNODCO_WAVE_SZE);

	/*
	 * Put the ramp and tri in twice - they should be an octave above the
	 * square wave.
	 */
	for (i = 0; i < JUNODCO_WAVE_SZE; i+=1)
	{
		*wave = *rampw++ * ramp;
		*wave++ += *triw++ * tri;
	}
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
	bristolJUNODCOlocal *local = lcl;
	register int obp, count, dosquare, cpwm;
	register float *ib, *ob, *mb, *wt, wtp, gain, transp, subgain, subw;
	register float width;
	bristolJUNODCO *specs;

	specs = (bristolJUNODCO *) operator->specs;

#ifdef BRISTOL_DBG
	printf("junodco(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[DCO_OUT_IND].samplecount;
	ib = specs->spec.io[DCO_IN_IND].buf;
	mb = specs->spec.io[DCO_MOD_IND].buf;
	ob = specs->spec.io[DCO_OUT_IND].buf;
	wt = (float *) param->param[0].mem;
	width = param->param[0].float_val;
	dosquare = param->param[7].int_val;
	wtp = local->wtp;
	gain = param->param[3].float_val * 2;
	subgain = param->param[6].float_val;
	transp = param->param[1].float_val * param->param[2].float_val;
	wtp = local->wtp;
	subw = local->subw;
	cpwm = local->cpwm;

	if (bristolBLOcheck(voice->dFreq*transp))
	{
		wt = (float *) param->param[1].mem;

		memset(param->param[2].mem, 0, JUNODCO_WAVE_SZE * sizeof(float));
		memset(param->param[3].mem, 0, JUNODCO_WAVE_SZE * sizeof(float));

		generateBLOwaveformF(voice->cFreq*transp, param->param[2].mem,BLO_RAMP);
		generateBLOwaveformF(voice->cFreq*transp, param->param[3].mem, BLO_TRI);

		buildJunoWave(param->param[1].mem,
			param->param[4].float_val, param->param[2].mem,
			param->param[5].float_val, param->param[3].mem);
	} else
		wt = (float *) param->param[0].mem;

	if (voice->flags & BRISTOL_KEYON)
		wtp = 0;

/*printf("%f, %f, %f\n", mb[0], mb[128], mb[200]); */
	/*
	 * Go jumping through the wavetable, with each jump defined by the value
	 * given on our input line, making sure we fill one output buffer.
	 */
	for (obp = 0; obp < count;)
	{
		/*
		 * Take a sample from the wavetable into the output buffer. This 
		 * should also be scaled by gain parameter.
		 */
		ob[obp] += (wt[(int) wtp] + subw * subgain) * gain;
		/*
		 * The PWM wave should actually be twice the running frequency. It may
		 * be easier to make the subwave a calculated operation, and make that
		 * run at half the frequency?
		 * This should be redone to be the difference of two phased ramps.
		 */
		if (dosquare)
		{
			if (wtp > width - *mb++)
			{
				if (cpwm <= 0)
					cpwm = BRISTOL_SQR;
				ob[obp] += (cpwm * 0.98) * gain * 4;
			} else {
				if (cpwm > 0)
					cpwm = -BRISTOL_SQR;
				ob[obp] += (cpwm * 0.98) * gain * 4;
			}
		}
		subw *= 0.98;
		/*
		 * Move the wavetable pointer forward by amount indicated in input 
		 * buffer for this sample. Toggle the subwave index with each cycle.
		 */
		if ((wtp += ib[obp++] * transp) >= JUNODCO_WAVE_SZE)
		{
			wtp -= JUNODCO_WAVE_SZE;
			if (subw == 0)
				subw = BRISTOL_SQR;
			else {
				if (subw > 0)
					subw = -BRISTOL_SQR;
				else
					subw = BRISTOL_SQR;
			}
		}
		/*
		 * If we have gone negative, round back up. Allows us to run the 
		 * oscillator backwards.
		 */
		if (wtp < 0)
			wtp += JUNODCO_WAVE_SZE;
	}

	local->wtp = wtp;
	local->subw = subw;
	local->cpwm = cpwm;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
junodcoinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolJUNODCO *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("junodcoinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolJUNODCO *) bristolmalloc0(sizeof(bristolJUNODCO));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolJUNODCO);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolJUNODCOlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(JUNODCO_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(JUNODCO_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(JUNODCO_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(JUNODCO_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(JUNODCO_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(JUNODCO_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(JUNODCO_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(JUNODCO_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], JUNODCO_WAVE_SZE, 0);
	fillWave(specs->wave[1], JUNODCO_WAVE_SZE, 1);
	fillWave(specs->wave[2], JUNODCO_WAVE_SZE, 2);
	fillWave(specs->wave[3], JUNODCO_WAVE_SZE, 3);
	fillWave(specs->wave[4], JUNODCO_WAVE_SZE, 4);
	fillWave(specs->wave[5], JUNODCO_WAVE_SZE, 5);
	fillWave(specs->wave[6], JUNODCO_WAVE_SZE, 6);
	fillWave(specs->wave[7], JUNODCO_WAVE_SZE, 7);

	/*
	 * Now fill in the dco specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "Pulse";
	specs->spec.param[0].description = "Pulse width";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "Transpose";
	specs->spec.param[1].description = "Octave transposition";
	specs->spec.param[1].type = BRISTOL_INT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 12;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "Tune";
	specs->spec.param[2].description = "fine tuning of frequency";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "gain";
	specs->spec.param[3].description = "output gain on signal";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 2;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[4].pname = "Ramp";
	specs->spec.param[4].description = "harmonic inclusion of ramp wave";
	specs->spec.param[4].type = BRISTOL_INT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_BUTTON;

	specs->spec.param[5].pname = "Triangle";
	specs->spec.param[5].description = "harmonic inclusion of triagular wave";
	specs->spec.param[5].type = BRISTOL_INT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_BUTTON;

	specs->spec.param[6].pname = "Square";
	specs->spec.param[6].description = "harmonic inclusion of sub square wave";
	specs->spec.param[6].type = BRISTOL_INT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_BUTTON;

	specs->spec.param[7].pname = "PWM";
	specs->spec.param[7].description = "harmonic inclusion of pwm square wave";
	specs->spec.param[7].type = BRISTOL_INT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "input rate modulation signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "mod";
	specs->spec.io[1].description = "pulse width modulation";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[2].ioname = "output";
	specs->spec.io[2].description = "oscillator output signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

/*
 * Waves have a range of 24, which is basically two octaves. For larger 
 * differences will have to apply apms.
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
		{
			float value = BRISTOL_SQR;
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
				mem[i] = (value *= 0.98);
			value = -BRISTOL_SQR;
			for (;i < count; i++)
				mem[i] = (value *= 0.98);
			return;
		}
		case 2:
			/* 
			 * This is a pulse wave - the juno dco does pwm separately.
			 */
			for (i = 0;i < count / 5; i++)
				mem[i] = BRISTOL_VPO * 2 / 3;
			for (;i < count; i++)
				mem[i] = -BRISTOL_VPO * 2 / 3;
			return;
		case 3:
			/* 
			 * This is a ramp wave. We scale the index from -.5 to .5, and
			 * multiply by the range. We go from rear to front to table to make
			 * the ramp wave have a positive leading edge.
			for (i = count - 1;i >= 0; i--)
				mem[i] = (((float) i / count) - 0.5) * BRISTOL_VPO * 2.0;
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = bloramp[i];
				return;
			}
			for (i = 0; i < count / 2; i++)
				mem[i] = ((float) i / count) * BRISTOL_VPO * 2.0;
			for (; i < count; i++)
				mem[i] = ((float) i / count) * BRISTOL_VPO * 2.0 -
					BRISTOL_VPO * 2;
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
			 * interesting strings sounds.
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				int k = 0;

				for (i = 0;i < count; i++)
					mem[i] = blotriangle[i];

				for (i = 0;i < count; i++)
				{
					mem[i] += blotriangle[k];
					if ((k += 2) >= BRISTOL_BLO_SIZE)
						k -= BRISTOL_BLO_SIZE;
				}
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
			return;
		case 7:
			return;
	}
}

