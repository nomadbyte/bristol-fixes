
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
 *	cs80oscinit()
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
#include <math.h>

#include "bristol.h"
#include "bristolblo.h"
#include "cs80osc.h"
#include "bristolcs80.h"

float note_diff;
int samplecount;

static void fillWave(float *, int, int);
static void buildCs80Sound(bristolOP *, bristolOPParams *);

/*
 * Use of these mean that we can only product a single cs80osc oscillator.
 * Even though it is reasonably flexible this does lead to problems, and if
 * we move to a dual manual Cs80 it will need to be resolved.
 *
 * The same is true of using single global result waves, wave[6] and wave[7].
 * These need to be moved to private address space, ie, we need more 
 * instantiation.
 */

/*
 * This can be a single list, it is used to generate the different harmonics.
 *
 * We have 16, 8, 5.3, 4, 2,6, 2
{16', 8', 5-1/3', 4, 2-2/3', 2', 0,0,0,0,0,0,0};
static float sweeps[16] = {1, 2, 3, 4, 6, 8, 8, 8, 8, 8, 4,4,4,4,4,8};
 */
static float sweeps[16] = {2, 4, 6, 8, 12, 16, 8, 8, 8, 8, 4,4,4,4,4,8};

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "CS-80 Osc"
#define OPDESCRIPTION "CS-80 Voltage Controlled Oscillator"
#define PCOUNT 13
#define IOCOUNT 4

#define CS80_IN_IND 0
#define CS80_OUT_IND 1
#define CS80_PWM_IND 2
#define CS80_SINEOUT_IND 3

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("cs80oscdestroy(%x)\n", operator);
#endif

	/*
	 * Unmalloc anything we added to this structure
	 */
	bristolfree(((bristolCS80 *) operator)->wave[0]);
	bristolfree(((bristolCS80 *) operator)->wave[1]);
	bristolfree(((bristolCS80 *) operator)->wave[2]);
	bristolfree(((bristolCS80 *) operator)->wave[3]);
	bristolfree(((bristolCS80 *) operator)->wave[4]);
	bristolfree(((bristolCS80 *) operator)->wave[5]);
	bristolfree(((bristolCS80 *) operator)->wave[6]);
	bristolfree(((bristolCS80 *) operator)->wave[7]);

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
	printf("cs80oscreset(%x, %x)\n", operator, param);
#endif

	/*
	 * For the cs80osc we want to put in a few local memory variables for this
	 * operator specifically.
	 *
	 * These are sineform, waveindex, wavelevel, and percussions, plus two
	 * unique wavetables.
	 */
	if (param->param[0].mem != NULL)
		bristolfree(param->param[0].mem);

	param->param[0].mem = bristolmalloc0(sizeof(float) * CS80_WAVE_SZE);

	param->param[0].float_val = 1.0;
	param->param[1].float_val = 0.2;
	param->param[2].float_val = 0.2;
	param->param[3].float_val = 1.0;
	param->param[4].float_val = 1.0;
	param->param[5].int_val = 512;
	param->param[6].float_val = 1.0;
	param->param[7].float_val = 1.0;

	return(0);
}

/*
 * Alter an internal parameter of an operator.
 * 
 * For the CS80 we need three waveforms - PWM, Ramp and Sine.
 * Sine uses a gain level, the others didn't but we can add them as layer
 * params. They will be rendered twice for brilliance at different frequencies
 * with different dynamic gains.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	int ival = ((int) (value * CONTROLLER_RANGE));

#ifdef BRISTOL_DBG
	printf("cs80oscparam(%x, %x, %i, %f) %i\n",
		operator, param, index, value, ival);
#endif
printf("cs80oscparam(%i, %f) %i\n", index, value, ival);

	switch (index) {
		/*
		 * We have sqr, ramp, sine levels, brill', brill'', pw, transp, tune.
		 *
		 * Sine may become tri?
		 *
		 * The basic waveform is a mixed sine and ramp. Both will be levels
		 * but the ramp is on/off from GUI (with extended option). PWM will be
		 * generated on the fly with a small detune.
		 *
		 * There will be two levels of brilliance for two consequetive octaves
		 * each with configurable level then affected by brilliance which is
		 * inherrited from the emulation code in bristolcs80.c
		 *
		 * Transpose will include sweeps of 16, 8, 5 1/3, 4, 2 2/3 and 2 octave
		 * which are multipliers 1, 2, 3, 4, 6, 8.
		 */
		case 0: /* Sqr, Ramp, Sine, brill1, brill2 */
		case 1:
		case 2:
		case 3: /* Two brilliance parameters. */
		case 4:
			param->param[index].float_val = value;
			break;
		case 5: /* Pulse width */
			param->param[index].int_val = (int) ((1.0 - value) * CS80_H_SZE);
			break;
		case 6: /* Tuning +/- 1/2 tone */
			{
				float tune, notes = 1.0;

				tune = (value - 0.5) * 2;

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
				else
					param->param[index].float_val =
						1.0 - (1 - notes) * -tune;

				break;
			}
			break;
		case 7: /* Transpose */
			param->param[index].float_val = sweeps[ival];
			break;
	}

	return(0);
}

/*
 * This takes a lot of stacked parameters and could be optimised by putting
 * them in registers.
	genPWM(ob, ib, pwb, specs->wave[1], 1.0, 2.0, wtp, count, 512);
 */
static float
genPWM(register float *ob, register float *ib,
register float *pwb, register float *ramp, register float gain,
register float tune, register float wtp,
register int count, register int width)
{
	register int obp, wtpSqr;
	register float gdelta;

	if (gain == 0.0)
		return(wtp);

	while (wtp >= CS80_WAVE_SZE)
		wtp -= CS80_WAVE_SZE;
	while (wtp < 0)
		wtp += CS80_WAVE_SZE;

	for (obp = 0; obp < count;obp++)
	{
		gdelta = wtp - ((float) ((int) wtp));
		/*
		 * The square has to be computed real time as we are going to have
		 * pulse width modulation. Width is a faction of the wavetable size
		 * from zero to 50% (should be 5 to 50%?).
		 *
		 * This is a phased difference between two ramps.
		 *
		 * Fill in the ramp wave, then subtract the same waveform with a
		 * phase offset related to the width.
		 */
		if ((wtpSqr = width + *pwb++) < 30)
			wtpSqr = 30;
		else if (wtpSqr > 1000)
			wtpSqr = 1000;

		if ((wtpSqr = wtp + wtpSqr) >= CS80_WAVE_SZE)
			wtpSqr -= CS80_WAVE_SZE;

		if (wtp == CS80_WAVE_SZE_M)
			ob[obp] += (ramp[(int) wtp] + (ramp[0] - ramp[(int) wtp])
				* gdelta) * gain;
		else
			ob[obp] +=
				(ramp[(int) wtp] + ((ramp[(int) wtp + 1] - ramp[(int) wtp])
				* gdelta)) * gain;

		if (wtpSqr == CS80_WAVE_SZE_M)
			ob[obp] -= (ramp[(int) wtpSqr] + (ramp[0] - ramp[(int) wtpSqr])
				* gdelta) * gain;
		else
			ob[obp] -=
				(ramp[(int) wtpSqr] + ((ramp[(int) wtpSqr + 1]
				- ramp[(int) wtpSqr]) * gdelta)) * gain;

		/*
		 * Roll the buffers forward - need to consider frequency and tuning.
		 */
		if ((wtp += ib[obp] * 0.25 * tune) >= CS80_WAVE_SZE)
			while ((wtp -= CS80_WAVE_SZE) >= CS80_WAVE_SZE)
				;
		while (wtp < 0)
			wtp += CS80_WAVE_SZE;
	}

	return(wtp);
}

/*
 * This takes a lot of stacked parameters and could be optimised by putting
 * them in registers.
	genPWM(ob, ib, pwb, specs->wave[1], 1.0, 2.0, wtp, count, 512);
 */
static float
genWave(register float *ob, register float *ib, register float *wt1,
register float gain, register float tune, register float wtp,
register int count)
{
	register int obp;

	if (gain == 0.0)
		return(wtp);

	while (wtp >= CS80_WAVE_SZE)
		wtp -= CS80_WAVE_SZE;
	while (wtp < 0)
		wtp += CS80_WAVE_SZE;

	for (obp = 0; obp < count;obp++)
	{
		if (((int) wtp) == CS80_WAVE_SZE_M) {
			ob[obp] += (wt1[0] * (wtp - ((float) ((int) wtp)))
				+ wt1[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))*gain;
		} else {
			ob[obp] += (wt1[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
				+ wt1[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))*gain;
		}

		/*
		 * Roll the buffers forward - need to consider frequency and tuning.
		 */
		if ((wtp += ib[obp] * 0.25 * tune) >= CS80_WAVE_SZE)
			while ((wtp -= CS80_WAVE_SZE) >= CS80_WAVE_SZE)
				;
		while (wtp < 0)
			wtp += CS80_WAVE_SZE;
	}

	return(wtp);
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
 * This oscillator is going to have 4 continous controllers for the signal
 * strengths of the different harmonics, 32/16/8/4. It will have different
 * gain levels for the tri and ramp waves. Finally the phase shifting pulse
 * wave will function probably on the lowest selected harmonic at first since
 * we are not going to recalculate it unless the results of two phase shifting
 * pulse harmonics turns out to be seriously attractive.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	register bristolCS80local *local = lcl;
	register int count, pwm, chan;
	register float *ib, *sb, *ob, *pwb, transp;
	bristolCS80 *specs;

	specs = (bristolCS80 *) operator->specs;

#ifdef BRISTOL_DBG
	printf("cs80osc(%x, %x, %x)\n", operator, param, local);
#endif

	chan = ((csmods *) voice->baudio->mixlocals)->channel;

/*
printf("Voice %i %i: %1.2f %1.2f\n", voice->key.key,
((csmods *) voice->baudio->mixlocals)->channel,
((csmods *) voice->baudio->mixlocals)->chanmods[chan].keybrill[voice->index],
((csmods *) voice->baudio->mixlocals)->chanmods[chan].keylevel[voice->index]);
*/

	count = specs->spec.io[CS80_OUT_IND].samplecount;
	ib = specs->spec.io[CS80_IN_IND].buf;
	ob = specs->spec.io[CS80_OUT_IND].buf;
	sb = specs->spec.io[CS80_SINEOUT_IND].buf;
	pwb = specs->spec.io[CS80_PWM_IND].buf;

	transp = param->param[6].float_val * param->param[7].float_val;

	if (voice->flags & BRISTOL_KEYON)
		local->wtp0 = local->wtp1 = local->wtp2 =
		local->wtp3 = local->wtp4 = local->wtp5 = 0;

	/*
	 * This is the starting pulse width 50% to about 97% duty cycle. The PWM
	 * will go either side of this point, ie, the PWM range is actually more
	 * like 3% to 97%.
	 */
	pwm = param->param[5].int_val;

	/* Generate the base ramp wave. */
	local->wtp0 = genWave(ob, ib, specs->wave[1], param->param[1].float_val,
		sweeps[0] * transp, local->wtp0, count);
	/* First brilliance base ramp wave. */
	local->wtp1 = genWave(ob, ib, specs->wave[1],
		((csmods *) voice->baudio->mixlocals)
			->chanmods[chan].keybrill[voice->index]
			* param->param[1].float_val
			* param->param[3].float_val,
		sweeps[1] * transp + 0.011 * voice->detune, local->wtp1, count);
	/* Second brilliance base ramp wave. */
	local->wtp2 = genWave(ob, ib, specs->wave[1],
		((csmods *) voice->baudio->mixlocals)
			->chanmods[chan].keybrill[voice->index]
			* param->param[1].float_val
			* param->param[4].float_val,
		sweeps[3] * transp + 0.023 * voice->detune, local->wtp2, count);

	/* Generate the base sine wave. */
	local->wtp6 = genWave(sb, ib, specs->wave[0], param->param[2].float_val,
		sweeps[0] * transp + 0.005 * voice->detune, local->wtp6, count);
	/* First brilliance base sine wave. */
	local->wtp7 = genWave(sb, ib, specs->wave[0],
		((csmods *) voice->baudio->mixlocals)
			->chanmods[chan].keybrill[voice->index]
			* param->param[2].float_val
			* param->param[3].float_val,
		sweeps[1] * transp + 0.013 * voice->detune, local->wtp7, count);
	/* Second brilliance base sine wave. */
	local->wtp8 = genWave(sb, ib, specs->wave[0],
		((csmods *) voice->baudio->mixlocals)
			->chanmods[chan].keybrill[voice->index]
			* param->param[2].float_val
			* param->param[4].float_val,
		sweeps[3] * transp + 0.027 * voice->detune, local->wtp8, count);

	/* Square wave */
	local->wtp3 = genPWM(ob, ib, pwb, specs->wave[1], param->param[0].float_val,
		sweeps[0] * transp + 0.007 * voice->detune, local->wtp3, count, pwm);
	/* First brilliance square wave */
	local->wtp4 = genPWM(ob, ib, pwb, specs->wave[1],
		((csmods *) voice->baudio->mixlocals)
			->chanmods[chan].keybrill[voice->index]
			* param->param[0].float_val
			* param->param[3].float_val,
		sweeps[1] * transp + 0.013 * voice->detune, local->wtp4, count, pwm);
	/* Second brilliance square wave */
	local->wtp5 = genPWM(ob, ib, pwb, specs->wave[1],
		((csmods *) voice->baudio->mixlocals)
			->chanmods[chan].keybrill[voice->index]
			* param->param[0].float_val
			* param->param[4].float_val,
		sweeps[3] * transp + 0.017 * voice->detune, local->wtp5, count, pwm);

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
cs80oscinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolCS80 *specs;

#ifdef BRISTOL_DBG
	printf("cs80oscinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	*operator = bristolOPinit(operator, index, samplecount);
	note_diff = pow(2, ((float) 1)/12);;

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;

	specs = (bristolCS80 *) bristolmalloc0(sizeof(bristolCS80));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolCS80);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolCS80local);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(CS80_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(CS80_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(CS80_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(CS80_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(CS80_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(CS80_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(CS80_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(CS80_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], CS80_WAVE_SZE, 0);
	fillWave(specs->wave[1], CS80_WAVE_SZE, 1);
	fillWave(specs->wave[2], CS80_WAVE_SZE, 2);
	fillWave(specs->wave[3], CS80_WAVE_SZE, 3);
	fillWave(specs->wave[4], CS80_WAVE_SZE, 4);
	fillWave(specs->wave[5], CS80_WAVE_SZE, 5);
	fillWave(specs->wave[6], CS80_WAVE_SZE, 6);
	fillWave(specs->wave[7], CS80_WAVE_SZE, 7);

	/*
	 * Now fill in the cs80osc specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "Square strength";
	specs->spec.param[0].description = "Square harmonic";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "Ramp strength";
	specs->spec.param[1].description = "gain of Ramp harmonic";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "Sine strength";
	specs->spec.param[2].description = "gain of Sine harmonic";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "Brilliance' strength";
	specs->spec.param[3].description = "gain of Brilliance' harmonic";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[4].pname = "Brilliance'' strength";
	specs->spec.param[4].description = "gain of Brilliance'' harmonic";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[5].pname = "Pulse Width";
	specs->spec.param[5].description = "Pulse width of square wave";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[6].pname = "Tune";
	specs->spec.param[6].description = "1/2 semitone up or down";
	specs->spec.param[6].type = BRISTOL_FLOAT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[7].pname = "Transpose";
	specs->spec.param[7].description = "Transpose";
	specs->spec.param[7].type = BRISTOL_FLOAT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the cs80osc IO specs.
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

	specs->spec.io[2].ioname = "PW Modulating Input";
	specs->spec.io[2].description = "input of signal for PWM process";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[3].ioname = "sine output";
	specs->spec.io[3].description = "sine oscillator output signal";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

/*
 * Hm, just in case you could not tell, this is a phase distortion oscillator.
 * It is somewhat warmer than the original cs80osc.
 */
static void fillPDwave(register float *mem, register int count,
register double reach)
{
	register int i, recalc1 = 1, recalc2 = 1;
	register double j = 0, Count = (double) count, inc = reach;

	for (i = 0;i < count; i++)
	{
		mem[i] = sin(((double) (2 * M_PI * j)) / Count) * CS80_WAVE_GAIN;

		if ((j > (count * 3 / 4)) && recalc2)
		{
			recalc2 = 0;
			inc = ((float) (count / 4)) / (((float) count) - i);
		} else if ((j > (count / 4)) && recalc1) {
			recalc1 = 0;
			/* 
			 * J has the sine on a peak, we need to calculate the number
			 * of steps required to get the sine back to zero crossing by
			 * the time i = count / 2, ie, count /4 steps in total.
			 */
			inc = ((float) (count / 4)) / (((float) (count / 2)) - i);
		}

		j += inc;
	}

/*
	printf("\n%f, %f\n", j, inc);
	for (i = 0; i < count; i+=8)
	{
		printf("\n%i: ", i + k);
		for (k = 0; k < 8; k++)
			printf("%03.1f, ", mem[i + k]);
	}
	printf("\n");
*/
}

#define S_DEC 0.999f

static void
fillWave(register float *mem, register int count, int type)
{
	register double Count;

#ifdef BRISTOL_DBG
	printf("fillWave(%x, %i, %i)\n", mem, count, type);
#endif

	Count = (double) count;

	switch (type) {
		case 0:
		default:
			/*
			 * This will be a sine wave. We have count samples, and
			 * 2PI radians in a full sine wave. Thus we take
			 * 		(2PI * i / count) * 2048.
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				int i;

				for (i = 0;i < count; i++)
					mem[i] = blosine[i];
				return;
			}
			fillPDwave(mem, count, 1.0);
			return;
		case 1:
			/*
			 * Ramp wave
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				int i;

				for (i = 0;i < count; i++)
					mem[i] = bloramp[i];
				return;
			}
			fillPDwave(mem, count, 20.0);
			return;
		case 4:
		{
			float value = 8.0;
			int i;

			/* 
			 * This is a square wave, with decaying plateaus.
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = blosquare[i];
				return;
			}
			for (i = 0;i < count / 2; i++)
				mem[i] = (value * S_DEC);
			value = -8.0;
			for (;i < count; i++)
				mem[i] = (value * S_DEC);
			return;
		}
		case 2:
			{
				int i;

				if (blo.flags & BRISTOL_BLO)
				{
					for (i = 0;i < count; i++)
						mem[i] = blotriangle[i];
					return;
				}

				for (i = 0;i < count / 2; i++)
					mem[i] = -CS80_WAVE_GAIN
						+ ((float) i / (count / 2)) * CS80_WAVE_GAIN * 2;
				for (;i < count; i++)
					mem[i] = CS80_WAVE_GAIN -
						(((float) (i - count / 2) * 2) / (count / 2))
							* CS80_WAVE_GAIN;
		}
			break;
	}
}

static void
buildCs80Sound(bristolOP *operator, bristolOPParams *param)
{
	register float gain, *source, *dest, sweep, oscindex;
	register int j;
	bristolCS80 *specs;

	specs = (bristolCS80 *) operator->specs;

#ifdef BRISTOL_DBG
	printf("fillCs80Wave(%x)\n", operator);
#endif
printf("fillCs80Wave() %f %f\n", param->param[1].float_val,
param->param[2].float_val);

	/*
	 * Dest is actually the local wave for this operator.
	 */

	/*
	 * Take the ramp to the destination wave.
	 */
	source = specs->wave[1];

	oscindex = 0;

	dest = param->param[0].mem;

	gain = param->param[1].float_val;

	sweep = sweeps[0];

	if (gain != 0)
	{
		for (j = 0; j < CS80_WAVE_SZE; j++)
		{
			*dest++ = source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= CS80_WAVE_SZE)
				oscindex -= CS80_WAVE_SZE;
		}
	} else
		bristolbzero(param->param[0].mem, CS80_WAVE_SZE * sizeof(float));

	gain = param->param[2].float_val;

	if (gain != 0)
	{
		source = specs->wave[0];
		oscindex = 0;
		dest = param->param[0].mem;

		for (j = 0; j < CS80_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= CS80_WAVE_SZE)
				oscindex -= CS80_WAVE_SZE;
		}
	}
}

