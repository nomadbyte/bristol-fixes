
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
 *	arpdcoinit()
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
#include "arpdco.h"

float note_diff, *zbuf;

#define BRISTOL_SQR 4

/*
 * The name of this operator, IO count, and IO names. This is where possible
 * identical to the Prophet DCO, but we need separated outputs. We could use
 * it for the Prophet as well but that would incur the cost of extra mixing
 * code.
 */
#define OPNAME "DCO"
#define OPDESCRIPTION "ARP Digitally Controlled Oscillator"
#define PCOUNT 8
#define IOCOUNT 7

#define DCO_IN_IND 0
#define DCO_MOD_IND 1
#define DCO_OUT_RAMP 2
#define DCO_SYNC_IND 3
#define DCO_OUT_SQUARE 4
#define DCO_OUT_SINE 5
#define DCO_OUT_TRIANGLE 6

#define DCO_WAVE_COUNT 6

static void fillWave();

/*
 * We should not be doing this however I need a quick resolution to some issues
 * with sync of square waves and this is a scratch buffer for the edge detector
 */
static float *sbuf = NULL;

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
	bristolfree(((bristolARPDCO *) operator)->wave[0]);
	bristolfree(((bristolARPDCO *) operator)->wave[1]);
	bristolfree(((bristolARPDCO *) operator)->wave[2]);
	bristolfree(((bristolARPDCO *) operator)->wave[3]);
	bristolfree(((bristolARPDCO *) operator)->wave[4]);
	bristolfree(((bristolARPDCO *) operator)->wave[5]);
	bristolfree(((bristolARPDCO *) operator)->wave[6]);
	bristolfree(((bristolARPDCO *) operator)->wave[7]);

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
	printf("arpreset(%x)\n", operator);
#endif

	param->param[0].int_val = ARPDCO_WAVE_SZE / 2;
	if (param->param[0].mem != NULL) bristolfree(param->param[0].mem);
	param->param[0].mem = bristolmalloc0(sizeof(float) * ARPDCO_WAVE_SZE);
	if (param->param[1].mem != NULL) bristolfree(param->param[1].mem);
	param->param[1].mem = bristolmalloc0(sizeof(float) * ARPDCO_WAVE_SZE);
	param->param[1].float_val = 1.0;
	param->param[2].float_val = 1.0;
	param->param[3].float_val = 1.0;
	param->param[6].int_val = 1;
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
	printf("arpdcoparam(%i, %f/%f)\n",
		index, value, (int) (value*CONTROLLER_RANGE));
#endif

	switch (index) {
		case 0: /* PW */
			param->param[index].float_val = value * ARPDCO_WAVE_SZE * 0.5;
			break;
		case 1:
			/*
			 * Transpose in octaves. 0 to 5, down 2 up 3.
			 */
			{
				int note = value * CONTROLLER_RANGE;

				param->param[1].int_val = 0;

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
					case 10:
						/* This should set values for LFO operation. */
						param->param[1].float_val = 0.125;
						param->param[1].int_val = 1;
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
		case 3: /* gain */
			param->param[index].float_val = value;
			break;
		case 4: /* mix ramp */
		case 5: /* mix tri */
		case 8: /* mix sine */
		case 6: /* mix PWM square */
			/*
			 * These were dropped, we always mix all waveforms. Perhaps there
			 * will be an odyssey/2600 option where we produce either 
			 * ramp/square or ramp/square/sine/triangle, but this can also
			 * be achieved with null pointers.
			 */
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

static float
genSyncPoints(register float *sbuf, float *sb, register float lsv,
register int count)
{
	/*
	 * Take the sync buffer and look for zero crossings. When they are found
	 * do a resample to find out where the crossing happened as sample level
	 * resolution results in distortion and shimmer
	 *
	 * Resampling, or rather interpolation to zero, means we have to look at 
	 * the respective above and below zero values.
	 */
	for (; count > 0; count--, lsv = *sb++)
		*sbuf++ = ((lsv < 0) && (*sb >= 0))? lsv / (lsv - *sb) : 0;

	return(lsv);
}

#define S_DEC 0.999f

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
	bristolARPDCOlocal *local = lcl;
	int obp, count, dosquare;
	float *ramp, wtpSqr, gdelta, width, ssg;
	float *ib, *ob, *ob2, *mb, *sb, *wt, wtp, gain, transp, cpwm, lsv;
	float *obsin, *obtri, *sinwave, *triwave;
	bristolARPDCO *specs;

	specs = (bristolARPDCO *) operator->specs;

#ifdef BRISTOL_DBG
	printf("arpdco(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[DCO_OUT_RAMP].samplecount;
	ib = specs->spec.io[DCO_IN_IND].buf;
	mb = specs->spec.io[DCO_MOD_IND].buf;
	ob = specs->spec.io[DCO_OUT_RAMP].buf;
	ob2 = specs->spec.io[DCO_OUT_SQUARE].buf;
	if ((obsin = specs->spec.io[DCO_OUT_SINE].buf) == 0)
		obsin = zbuf;
	if ((obtri = specs->spec.io[DCO_OUT_TRIANGLE].buf) == 0)
		obtri = zbuf;

	lsv = local->lsv;

	/*
	 * Hm, this was not necessary - could just have taken a zero bufptr.
	 */
	if ((param->param[7].int_val)
		&& ((sb = specs->spec.io[DCO_SYNC_IND].buf) != 0))
			local->lsv = genSyncPoints(sbuf, sb, lsv, count);
	else
		sb = 0;

	width = param->param[0].float_val; /* Pulse width */
	dosquare = param->param[6].int_val;
/*	wtp = local->wtp; */
	gain = param->param[3].float_val;
	ssg = local->ssg >= 0? gain:-gain;

	/*
	 * We are going to add another tune parameter - number of semitones from
	 * zero - an addition to octave transpose and fine tune. We should add
	 * another fine tune for just +/- semitone
	 */
	transp = param->param[1].float_val * param->param[2].float_val
		* param->param[9].float_val * param->param[10].float_val
		* param->param[11].float_val;
	wtp = local->wtp;
	cpwm = local->cpwm;

	sinwave = specs->wave[0];
	if (bristolBLOcheck(voice->dFreq*transp))
	{
		/* Need to restuff tri, ramp */
		memset(param->param[0].mem, 0, ARPDCO_WAVE_SZE * sizeof(float));
		memset(param->param[1].mem, 0, ARPDCO_WAVE_SZE * sizeof(float));
		ramp = param->param[0].mem;
		wt = ramp;
		triwave = param->param[1].mem;
		generateBLOwaveformF(voice->cFreq*transp, ramp, BLO_SAW);
		generateBLOwaveformF(voice->cFreq*transp, triwave, BLO_TRI);
	} else {
		wt = ramp = specs->wave[3];
		triwave = specs->wave[4];
	}

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

		if (wtp >= ARPDCO_WAVE_SZE_M) {
			ob[obp] += (wt[(int) wtp] + (wt[0] - wt[(int) wtp])
				* gdelta) * gain;
			obsin[obp] += (sinwave[(int) wtp]
				+ (sinwave[0] - sinwave[(int) wtp])
				* gdelta) * gain;
			obtri[obp] += (triwave[(int) wtp]
				+ (triwave[0] - triwave[(int) wtp])
				* gdelta) * gain;
		} else {
			ob[obp] += (wt[(int) wtp] + ((wt[(int) wtp + 1] - wt[(int) wtp])
				* gdelta)) * gain;
			obsin[obp] += (sinwave[(int) wtp]
				+ ((sinwave[(int) wtp + 1] - sinwave[(int) wtp])
				* gdelta)) * gain;
			obtri[obp] += (triwave[(int) wtp]
				+ ((triwave[(int) wtp + 1] - triwave[(int) wtp])
				* gdelta)) * gain;
		}

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
		if ((wtpSqr = width + *mb++) < ARPDCO_WAVE_PWMIN)
			wtpSqr = ARPDCO_WAVE_PWMIN;
		else if (wtpSqr > ARPDCO_WAVE_PWMAX)
			wtpSqr = ARPDCO_WAVE_PWMAX;

		if ((wtpSqr = wtp + wtpSqr) > ARPDCO_WAVE_SZE_M)
			wtpSqr -= ARPDCO_WAVE_SZE;

		if (wtp >= ARPDCO_WAVE_SZE_M)
			ob2[obp] += (ramp[(int) wtp] + (ramp[0] - ramp[(int) wtp])
				* gdelta) * ssg;
		else
			ob2[obp] +=
				(ramp[(int) wtp] + ((ramp[(int) wtp + 1] - ramp[(int) wtp])
				* gdelta)) * ssg;

		gdelta = wtpSqr - ((float) ((int) wtpSqr));

		if (wtpSqr == ARPDCO_WAVE_SZE_M)
			ob2[obp] -= (ramp[(int) wtpSqr] + (ramp[0] - ramp[(int) wtpSqr])
				* gdelta) * ssg;
		else
			ob2[obp] -=
				(ramp[(int) wtpSqr] + ((ramp[(int) wtpSqr + 1]
				- ramp[(int) wtpSqr]) * gdelta)) * ssg;

		if (sb && (sbuf[obp] != 0))
		{
			/*
			 * These are positive edged zero crossings on the sync buffer. If
			 * we find them, wtp is moved to zero resampled from buffer.
			 */
			wtp = sbuf[obp];
			ssg = -ssg;

			/* We may need to resample here */

			continue;
		}

		/*
		 * Move the wavetable pointer forward by amount indicated in input 
		 * buffer for this sample.
		 */
		if ((wtp += ib[obp] * transp) >= ARPDCO_WAVE_SZE)
		{
			while (wtp >= ARPDCO_WAVE_SZE)
				wtp -= ARPDCO_WAVE_SZE;
		}

		/*
		 * If we have gone negative, round back up. Allows us to run the 
		 * oscillator backwards.
		 */
		while (wtp < 0)
			wtp += ARPDCO_WAVE_SZE;
	}

	local->wtp = wtp;
	local->cpwm = cpwm;
	local->ssg = ssg;

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
arpdcoinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolARPDCO *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("arpdcoinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	note_diff = pow(2, ((double) 1)/12);

	if (sbuf == NULL)
		sbuf = bristolmalloc(sizeof(float) * samplecount);

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param= param;

	specs = (bristolARPDCO *) bristolmalloc0(sizeof(bristolARPDCO));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolARPDCO);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolARPDCOlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] =
		(float *) bristolmalloc(ARPDCO_WAVE_SZE * sizeof(float));
	specs->wave[1] =
		(float *) bristolmalloc(ARPDCO_WAVE_SZE * sizeof(float));
	specs->wave[2] =
		(float *) bristolmalloc(ARPDCO_WAVE_SZE * sizeof(float));
	specs->wave[3] =
		(float *) bristolmalloc(ARPDCO_WAVE_SZE * sizeof(float));
	specs->wave[4] =
		(float *) bristolmalloc(ARPDCO_WAVE_SZE * sizeof(float));
	specs->wave[5] =
		(float *) bristolmalloc(ARPDCO_WAVE_SZE * sizeof(float));
	specs->wave[6] =
		(float *) bristolmalloc(ARPDCO_WAVE_SZE * sizeof(float));
	specs->wave[7] =
		(float *) bristolmalloc(ARPDCO_WAVE_SZE * sizeof(float));
	zbuf = (float *) bristolmalloc(ARPDCO_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], ARPDCO_WAVE_SZE, 0);
	fillWave(specs->wave[1], ARPDCO_WAVE_SZE, 1);
	fillWave(specs->wave[2], ARPDCO_WAVE_SZE, 2);
	fillWave(specs->wave[3], ARPDCO_WAVE_SZE, 3);
	fillWave(specs->wave[4], ARPDCO_WAVE_SZE, 4);
	fillWave(specs->wave[5], ARPDCO_WAVE_SZE, 5);
	fillWave(specs->wave[6], ARPDCO_WAVE_SZE, 6);
	fillWave(specs->wave[7], ARPDCO_WAVE_SZE, 7);

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

	specs->spec.param[6].pname = "PWM";
	specs->spec.param[6].description = "harmonic inclusion of square/pwm wave";
	specs->spec.param[6].type = BRISTOL_INT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_BUTTON;

	specs->spec.param[7].pname = "Square";
	specs->spec.param[7].description = "harmonic inclusion of square wave";
	specs->spec.param[7].type = BRISTOL_INT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_BUTTON;

	specs->spec.param[8].pname = "Sine";
	specs->spec.param[8].description = "harmonic inclusion of sine wave";
	specs->spec.param[8].type = BRISTOL_INT;
	specs->spec.param[8].low = 0;
	specs->spec.param[8].high = 1;
	specs->spec.param[8].flags = BRISTOL_BUTTON;

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

	specs->spec.io[2].ioname = "ramp output";
	specs->spec.io[2].description = "oscillator ramp output signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[3].ioname = "sync";
	specs->spec.io[3].description = "synchronisation signal";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[4].ioname = "square output";
	specs->spec.io[4].description = "oscillator pulse/square output signal";
	specs->spec.io[4].samplerate = samplerate;
	specs->spec.io[4].samplecount = samplecount;
	specs->spec.io[4].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[5].ioname = "sine output";
	specs->spec.io[5].description = "oscillator sine output signal";
	specs->spec.io[5].samplerate = samplerate;
	specs->spec.io[5].samplecount = samplecount;
	specs->spec.io[5].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[6].ioname = "triangle output";
	specs->spec.io[6].description = "oscillator triangle output signal";
	specs->spec.io[6].samplerate = samplerate;
	specs->spec.io[6].samplecount = samplecount;
	specs->spec.io[6].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
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
		*mem++ = sinf(j) * BRISTOL_VPO;

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
				mem[i] = sin(2 * M_PI * ((double) i) / count) * BRISTOL_VPO;
			return;
		case 1:
		default:
		{
			float value = BRISTOL_SQR;
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
			value = -BRISTOL_SQR;
			for (;i < count; i++)
				mem[i] = (value * S_DEC);
			return;
		}
		case 2:
			/* 
			 * This is a pulse wave - the arp dco does pwm.
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
			for (i = 0; i < count / 2; i++)
				mem[i] = ((float) i / count) * BRISTOL_VPO * 2.0;
			for (; i < count; i++)
				mem[i] = ((float) i / count) * BRISTOL_VPO * 2.0 -
					BRISTOL_VPO * 2;
			for (i = count - 1;i >= 0; i--)
				mem[i] = (((float) i / count) - 0.5) * BRISTOL_VPO * 2.0;
			mem[0] = 0;
			mem[count - 1] = mem[1]
				= BRISTOL_VPO;
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = bloramp[i];
					return;
			}
			fillPDsine(mem, count, 5);
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
					+ ((float) i / (count / 2)) * BRISTOL_VPO * 2;
			for (;i < count; i++)
				mem[i] = BRISTOL_VPO -
					(((float) (i - count / 2) * 2) / (count / 2)) * BRISTOL_VPO;
			return;
		case 5:
			/*
			 * Would like to put in a jagged edged ramp wave. Should make some
			 * interesting strings sounds.
			 */
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
			 * Sine wave - added as a part of the OBX extensions.
			 */
			for (i = 0;i < count; i++)
				mem[i] = sin(2 * M_PI * ((double) i) / count) * BRISTOL_VPO;
			return;
	}
}

