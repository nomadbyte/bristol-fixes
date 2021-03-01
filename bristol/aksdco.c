
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
 *	aksdcoinit()
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
#include "aksdco.h"

static float note_diff, *zbuf;

#define BRISTOL_SQR 4
#define SINE_LIM 0.05f

/*
 * The name of this operator, IO count, and IO names. This is where possible
 * identical to the Prophet DCO, but we need separated outputs. We could use
 * it for the Prophet as well but that would incur the cost of extra mixing
 * code.
 */
#define OPNAME "DCO"
#define OPDESCRIPTION "AKS Digitally Controlled Oscillator"
#define PCOUNT 6
#define IOCOUNT 4

#define DCO_IN_IND 0
#define DCO_MOD_IND 1
#define DCO_OUT_1 2
#define DCO_OUT_2 3

static void fillWave(float *, int, int);
static void fillSineWave(float *, float, float, float);
static void fillTriWave(float *, float, float, float);
static void fillPulseWave(float *, float, float, float);

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
	bristolfree(((bristolAKSDCO *) operator)->wave[0]);
	bristolfree(((bristolAKSDCO *) operator)->wave[1]);
	bristolfree(((bristolAKSDCO *) operator)->wave[2]);
	bristolfree(((bristolAKSDCO *) operator)->wave[3]);
	bristolfree(((bristolAKSDCO *) operator)->wave[4]);
	bristolfree(((bristolAKSDCO *) operator)->wave[5]);
	bristolfree(((bristolAKSDCO *) operator)->wave[6]);
	bristolfree(((bristolAKSDCO *) operator)->wave[7]);

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
	printf("aksreset(%x)\n", operator);
#endif

	if (param->param[0].mem != 0)
		bristolfree(param->param[0].mem);
	if (param->param[1].mem != 0)
		bristolfree(param->param[1].mem);

	param->param[0].mem = bristolmalloc0(sizeof(float) * AKSDCO_WAVE_SZE);
	param->param[1].mem = bristolmalloc0(sizeof(float) * AKSDCO_WAVE_SZE);

	param->param[0].float_val = 0.5;
	param->param[1].float_val = 0.5;
	param->param[2].float_val = 1.0;
	param->param[3].float_val = 1.0;
	param->param[4].float_val = 1;
	param->param[5].float_val = 1.0;
	param->param[7].int_val = 0;
	param->param[9].float_val = 1.0;
	param->param[10].float_val = 1.0;
	param->param[11].float_val = 1.0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("aksdcoparam(%x, %x, %i, %f)\n", operator, param, index, value);
#endif

	switch (index) {
		case 1: /* Tune in small amounts */
			/*
			 * This should be over a few hertz? That will have to be studied
			 * when the coarse tuning has been implemented since the range of
			 * this parameter should span any step in that control, it may be
			 * in cents or larger steps.
			 */
			param->param[index].float_val = 1.0 + note_diff * value / 8;
			break;
		case 0: /* Tune in large amounts */
			/*
			 * This should be exponential 15 to 15 KHz for an audible oscillator
			 * and something like 0.02 to 500 Hz for an 'LFO'.
			 */
			{
				float tune, notes = 1.0;
				int i;

/*				value = gainTable[(int) (value * C_RANGE_MIN_1)].gain; */
/*				if (param->param[5].int_val == 2) */

				tune = (value - 0.5) * 2;

				/*
				 * Up or down wide note range
				 */
				for (i = 0; i < 80;i++)
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
		case 2: /* distortion of waveform. */
		case 3: /* gain */
		case 4: /* gain */
			param->param[index].float_val = value;
			/*
			 * This will result in the active waves being built. The distorts
			 * will cover pulse width from 10 to 90 (or probably more), a ramp
			 * that goes from leading edge to trailing edge including a tri
			 * output, and a sign going from M to W.
			 * Hm, the oscillator has to be minimally aware of its function as
			 * all three are different. This can be taken from the LFO value?
			 * We want to have our two desired waveforms put into the first
			 * two locations, but the source may be different depending on 
			 * which oscillator this is.
			 */
			if (param->param[5].int_val == 0)
			{
				fillSineWave(param->param[0].mem, AKSDCO_WAVE_SZE,
					param->param[2].float_val,
					param->param[3].float_val);
				fillTriWave(param->param[1].mem, AKSDCO_WAVE_SZE,
					SINE_LIM,
					param->param[4].float_val);
			} else {
				fillPulseWave(param->param[0].mem, AKSDCO_WAVE_SZE,
					param->param[2].float_val,
					param->param[3].float_val);
				fillTriWave(param->param[1].mem, AKSDCO_WAVE_SZE,
					param->param[2].float_val,
					param->param[4].float_val);
			}
			break;
		case 5: /* LFO.... Encodes the type of oscilllator. */
			param->param[index].int_val = value * CONTROLLER_RANGE;
			break;
		case 7: /* sync flag */
			if (value == 0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			break;
		/* case 8: used for waveform mix. */
		case 9: /* semitone tuning. */
			/*
			 * Transpose in semitones upwards.
			 */
			{
				int semitone = value * CONTROLLER_RANGE;

				param->param[index].float_val = 1.0;

				while (semitone-- > 0) {
					param->param[index].float_val *= (note_diff);
				}
			}
			break;
		case 10: /* semitone fine tuning. */
			/*
			 * finetune by a semitones up/down.
			 */
			{
				float tune, notes = 1.0;

				tune = (value - 0.5);

				/*
				 * Up or down 7 notes.
				 */
				if (tune > 0)
					notes *= note_diff;
				else
					notes /= note_diff;

				if (tune > 0)
					param->param[index].float_val =
						1.0 + (notes - 1) * tune;
				else param->param[index].float_val =
						1.0 - (1 - notes) * -tune;

				break;
			}
		case 11: /* This will be used by the OBX pitch bend */
			{
				float tune, notes = 1.0;
				int i;

				tune = (value - 0.5) * 2;

				/*
				 * Up or down 12 notes.
				 */
				for (i = 0; i < 12;i++)
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
	}

	return(0);
}

#define S_DEC 0.999f

/*
 * This oscillator takes an input frequency buffer that is related to the 
 * midi key, glide and pitch bend, all frequency matched so that glide is 
 * at a constant rate irrespective of pitch and pitch bend is +/- a given 
 * range. It then takes a mod buf which contains anything we want to put into
 * it - for the AKS it is the result of all the patch pins in the matrix. It
 * is going to be linear information so may need to be scaled suitably.
 * [Notes 3 Nov 06: Perhaps the freq buf should not be used. Pitch bend will
 * be mapped to the X axis of the joystick in the modbuffer, and glide was not
 * implemented by this synth and will have the value 0 (no glide)].
 * [Alternatively allow the midi library to map pitch bend (coarse and fine) to 
 * another set of controllers that can then be used here for X axis modulation.]
 *
 * There are then two output buffers, one for each of the generated waveforms.
 *
 * Target tuning is 0.02 Hz to 16KHz, and we are probably going to have two
 * coarrse/fine controllers to cover it, 28bits.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolAKSDCOlocal *local = lcl;
	int obp, count;
	float *wt1, *wt2;
	float *ib, *ob1, *ob2, *mb, wtp, gdelta, transp;
	bristolAKSDCO *specs;

	specs = (bristolAKSDCO *) operator->specs;

#ifdef BRISTOL_DBG
	printf("aksdco(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[DCO_IN_IND].samplecount;
	ib = specs->spec.io[DCO_IN_IND].buf;
	mb = specs->spec.io[DCO_MOD_IND].buf;
	ob1 = specs->spec.io[DCO_OUT_1].buf;
	ob2 = specs->spec.io[DCO_OUT_2].buf;

	wt1 = (float *) param->param[0].mem;
	wt2 = (float *) param->param[1].mem;

	/*
	 * This needs to be changed, a lot.
	transp = param->param[1].float_val * param->param[2].float_val
		* param->param[9].float_val * param->param[10].float_val
		* param->param[11].float_val;
	 */
	transp = param->param[0].float_val * param->param[1].float_val;

	wtp = local->wtp;

	/*
	 * Go jumping through the wavetable, with each jump defined by the value
	 * given on our input line, making sure we fill one output buffer.
	 */
	for (obp = 0; obp < count;obp++)
	{
		/*
		 * Take a sample from the wavetable into the output buffer. This 
		 * should also be scaled by gain parameter.
		 *
		 * We can seperate this into subroutine calls, or we can take our
		 * values and take each wave?
		 */
		gdelta = wtp - ((float) ((int) wtp));

		if (wtp >= AKSDCO_WAVE_SZE_M) {
			ob1[obp] = wt1[(int) wtp] + (wt1[0] - wt1[(int) wtp]) * gdelta;
			ob2[obp] = wt2[(int) wtp] + (wt2[0] - wt2[(int) wtp]) * gdelta;
		} else {
			ob1[obp] = wt1[(int) wtp]
				+ (wt1[(int) wtp + 1] - wt1[(int) wtp]) * gdelta;
			ob2[obp] = wt2[(int) wtp]
				+ (wt2[(int) wtp + 1] - wt2[(int) wtp]) * gdelta;
		}

		/*
		 * Move the wavetable pointer forward by amount indicated in input 
		 * buffer for this sample.
		 */
		if ((wtp += ib[obp] * transp) >= AKSDCO_WAVE_SZE)
		{
			while (wtp >= AKSDCO_WAVE_SZE)
				wtp -= AKSDCO_WAVE_SZE;
		}
		/*
		 * If we have gone negative, round back up. Allows us to run the 
		 * oscillator backwards.
		 */
		while (wtp < 0)
			wtp += AKSDCO_WAVE_SZE;
	}

	local->wtp = wtp;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
aksdcoinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolAKSDCO *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("aksdcoinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolAKSDCO *) bristolmalloc0(sizeof(bristolAKSDCO));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolAKSDCO);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolAKSDCOlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] =
		(float *) bristolmalloc(AKSDCO_WAVE_SZE * sizeof(float));
	specs->wave[1] =
		(float *) bristolmalloc(AKSDCO_WAVE_SZE * sizeof(float));
	specs->wave[2] =
		(float *) bristolmalloc(AKSDCO_WAVE_SZE * sizeof(float));
	specs->wave[3] =
		(float *) bristolmalloc(AKSDCO_WAVE_SZE * sizeof(float));
	specs->wave[4] =
		(float *) bristolmalloc(AKSDCO_WAVE_SZE * sizeof(float));
	specs->wave[5] =
		(float *) bristolmalloc(AKSDCO_WAVE_SZE * sizeof(float));
	specs->wave[6] =
		(float *) bristolmalloc(AKSDCO_WAVE_SZE * sizeof(float));
	specs->wave[7] =
		(float *) bristolmalloc(AKSDCO_WAVE_SZE * sizeof(float));
	zbuf = (float *) bristolmalloc(AKSDCO_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], AKSDCO_WAVE_SZE, 0);
	fillWave(specs->wave[1], AKSDCO_WAVE_SZE, 1);
	fillWave(specs->wave[2], AKSDCO_WAVE_SZE, 2);
	fillWave(specs->wave[3], AKSDCO_WAVE_SZE, 3);
	fillWave(specs->wave[4], AKSDCO_WAVE_SZE, 4);
	fillWave(specs->wave[5], AKSDCO_WAVE_SZE, 5);
	fillWave(specs->wave[6], AKSDCO_WAVE_SZE, 6);
	fillWave(specs->wave[7], AKSDCO_WAVE_SZE, 7);

	/*
	 * Now fill in the dco specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "Coarse";
	specs->spec.param[0].description = "Coarse tuning";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "Fine";
	specs->spec.param[1].description = "Fine tuning";
	specs->spec.param[1].type = BRISTOL_INT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 12;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "Waveform";
	specs->spec.param[2].description = "waveform distortion";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "gain1";
	specs->spec.param[3].description = "output gain on first signal";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 2;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[4].pname = "gain2";
	specs->spec.param[4].description = "output gain on second signal";
	specs->spec.param[4].type = BRISTOL_INT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_BUTTON;

	specs->spec.param[5].pname = "Frequency";
	specs->spec.param[5].description = "audible or LFO oscillation";
	specs->spec.param[5].type = BRISTOL_INT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "input rate modulation signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "sine output";
	specs->spec.io[1].description = "oscillator sine output signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[2].ioname = "square output";
	specs->spec.io[2].description = "oscillator pulse/square output signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[3].ioname = "ramp output";
	specs->spec.io[3].description = "oscillator sine output signal";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

static void
fillPulseWave(float *mem, float count, float distort, float gain)
{
	float j = 1.0, i;

	if (distort <= SINE_LIM)
		distort = SINE_LIM;
	else if (distort >= 1.0 - SINE_LIM)
		distort = 1.0 - SINE_LIM;

	/*
	 * For now this is going to be a quite mathematical pulse wave, when it
	 * works we can change it to use some more interesting phase distorted
	 * waveforms.
	 */
	for (i = 0;i < count; i++)
	{
		*mem++ = j * gain;

		if (i > distort * count)
			j = -1.0;
	}
}

static void
fillTriWave(float *mem, float count, float distort, float gain)
{
	float j = -1.0, i, inc1, inc2, offset;

	if (distort <= SINE_LIM)
		distort = SINE_LIM;
	else if (distort >= 1.0 - SINE_LIM)
		distort = 1.0 - SINE_LIM;

	/*
	 * We start from a trough and ramp up to the designated midpoint, then back
	 * down
	 *
	 * For now this is going to be a quite mathematical ramp wave, when it
	 * works we can change it to use some more interesting phase distorted
	 * waveforms, however that might result in it going from ramp to sine back
	 * to ramp.
	 */
	inc1 = 2.0 / (offset = distort * count);
	inc2 = -2.0 / (count - offset);
/* printf("%f %f %f %f\n", distort, offset, inc1, inc2); */

	for (i = 0; i < count; i++)
	{
		*mem++ = j * gain;

		if (i < offset)
			j += inc1;
		else
			j += inc2;
	}
}

static void
fillSineWave(float *mem, float count, float distort, float gain)
{
	float j = 0, i, inc1, inc2;

	if (distort <= SINE_LIM)
		distort = SINE_LIM;
	else if (distort >= 1.0 - SINE_LIM)
		distort = 1.0 - SINE_LIM;

	/*
	 * We are going to distort the sine wave by altering the rate at which it
	 * scans through each of its two halves. The value can go from 0 to 1.0
	 * and that should be controlled.
	 * It defines the poing at which we reach  PI, or half of a sine.
	 */
	inc1 = ((float) M_PI) / (distort * count);
	inc2 = ((float) M_PI) / (count - distort * count);

	for (i = 0;i < count; i++)
	{
		*mem++ = sinf(j) * gain;

		if (j < M_PI)
			j += inc1;
		else
			j += inc2;
	}
}

static void
fillPDsine(float *mem, int count, int compress)
{
	float j = 0, i, inc1, inc2;

	/*
	 * Resample the sine wave with a phase distortion algorithm.
	 *
	 * We have to get to M_PI/2 in compress steps, hence
	 *
	 *	 inc1 = M_PI/(2 * compress)
	 *
	 * Then we have to scan M_PI in (count - 2 * compress), hence:
	 *
	 *	 inc2 = M_PI/(count - 2 * compress)
	 */
	inc1 = ((float) M_PI) / (((float) 2) * ((float) compress));
	inc2 = ((float) M_PI) / ((float) (count - 2 * compress));

	for (i = 0;i < count; i++)
	{
		*mem++ = sinf(j);

		if (i < compress)
			j += inc1;
		else if (i > (count - 2 * compress))
			j += inc1;
		else
			j += inc2;
	}
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
				mem[i] = sin(2 * M_PI * ((double) i) / count);
			return;
		case 1:
		default:
		{
			float value = BRISTOL_SQR;
			/* 
			 * This is a square wave, with decaying plateaus.
			 */
			for (i = 0;i < count / 2; i++)
				mem[i] = (value * S_DEC);
			value = -BRISTOL_SQR;
			for (;i < count; i++)
				mem[i] = (value * S_DEC);
			return;
		}
		case 2:
			/* 
			 * This is a pulse wave - the aks dco does pwm.
			 */
			for (i = 0;i < count / 5; i++)
				mem[i] = 1.0;
			for (;i < count; i++)
				mem[i] = -1.0;
			return;
		case 3:
			/* 
			 * This is a ramp wave. We scale the index from -.5 to .5, and
			 * multiply by the range. We go from rear to front to table to make
			 * the ramp wave have a positive leading edge.
			for (i = 0; i < count / 2; i++)
				mem[i] = ((float) i / count) * 2.0;
			for (; i < count; i++)
				mem[i] = ((float) i / count) * 2.0 - 2;
			for (i = count - 1;i >= 0; i--)
				mem[i] = (((float) i / count) - 0.5) * 2.0;
			mem[0] = 0;
			mem[count - 1] = mem[1]
				= 1.0;
			 */
			fillPDsine(mem, count, 5);
			return;
		case 4:
			/*
			 * Triangular wave. From MIN point, ramp up at twice the rate of
			 * the ramp wave, then ramp down at same rate.
			 */
			for (i = 0;i < count / 2; i++)
				mem[i] = (-1.0 + ((float) i * 2 / (count / 2))) * 2;
			for (;i < count; i++)
				mem[i] = (1.0 - (((float) (i - count / 2) * 2) / (count / 2)))
						* 2;
			return;
		case 5:
			/*
			 * Would like to put in a jagged edged ramp wave. Should make some
			 * interesting strings sounds.
			 */
			{
				float accum = -1.0;

				for (i = 0; i < count / 2; i++)
				{
					mem[i] = accum +
						(0.5 - (((float) i) / count)) * 4;
					if (i == count / 8)
						accum = -0.75;
					if (i == count / 4)
						accum = -0.5;
					if (i == count * 3 / 8)
						accum = -0.25;
				}
				for (; i < count; i++)
					mem[i] = -mem[count - i];
			}
			return;
		case 6:
			return;
		case 7:
			/*
			 * Sine wave - added as a part of the OBX extensions.
			 */
			for (i = 0;i < count; i++)
				mem[i] = sin(2 * M_PI * ((double) i) / count);
			return;
	}
}

