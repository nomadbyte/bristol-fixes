
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
#include "expdco.h"

float note_diff;

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "DCO"
#define OPDESCRIPTION "Digitally Controlled Oscillator"
#define PCOUNT 5
#define IOCOUNT 4

#define DCO_IN_IND 0
#define DCO_OUT_IND 1
#define DCO_MOD_IND 2
#define DCO_SYNC_IND 3
#define DCO_SYNC_OUT 4

#define DCO_WAVE_COUNT 6

static void fillWave();
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
	bristolfree(((bristolEXPDCO *) operator)->wave[0]);
	bristolfree(((bristolEXPDCO *) operator)->wave[1]);
	bristolfree(((bristolEXPDCO *) operator)->wave[2]);
	bristolfree(((bristolEXPDCO *) operator)->wave[3]);
	bristolfree(((bristolEXPDCO *) operator)->wave[4]);
	bristolfree(((bristolEXPDCO *) operator)->wave[5]);
	bristolfree(((bristolEXPDCO *) operator)->wave[6]);
	bristolfree(((bristolEXPDCO *) operator)->wave[7]);

	bristolfree(((bristolEXPDCO *) operator)->null);

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

	/* The use of 1, 3 and 4 may make things clear later */
	if (param->param[1].mem != NULL)
		bristolfree(param->param[1].mem);
	param->param[1].mem = bristolmalloc0(sizeof(float) * EXPDCO_WAVE_SZE);
	if (param->param[3].mem != NULL)
		bristolfree(param->param[3].mem);
	param->param[3].mem = bristolmalloc0(sizeof(float) * EXPDCO_WAVE_SZE);
	if (param->param[4].mem != NULL)
		bristolfree(param->param[4].mem);
	param->param[4].mem = bristolmalloc0(sizeof(float) * EXPDCO_WAVE_SZE);

	param->param[0].float_val = 0;
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
			/* Wave */
			param->param[index].float_val = value;
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
		case 3: /* gain */
			param->param[index].float_val = value;
			break;
		case 4: /* sync */
			if (value == 0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			break;
	}
/*printf("dco %f, %f, %f, %f\n", param->param[0].float_val, */
/*param->param[1].float_val, param->param[2].float_val, */
/*param->param[3].float_val); */

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
	bristolEXPDCOlocal *local = lcl;
	register int obp, count, cpwm, wdelta;
	register float *ib, *ob, *sb, *mb, *wt1, *wt2, wtp, gdelta, ssg, *sob, *wt3;
	register float gain, gain1, gain2, transp;
	register float wave, wform, lsv;
	bristolEXPDCO *specs;

	specs = (bristolEXPDCO *) operator->specs;

#ifdef BRISTOL_DBG
	printf("dco(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[DCO_OUT_IND].samplecount;

	ib = specs->spec.io[DCO_IN_IND].buf;
	ob = specs->spec.io[DCO_OUT_IND].buf;
	if ((mb = specs->spec.io[DCO_MOD_IND].buf) == 0)
		return(0);
	lsv = local->lsv;
	if ((sb = specs->spec.io[DCO_SYNC_IND].buf) != 0)
	{
		local->lsv = genSyncPoints(sbuf, sb, lsv, count);
		sb = sbuf;
	}

	wave = param->param[0].float_val;
	gain = param->param[3].float_val;

	transp = param->param[1].float_val * param->param[2].float_val;
	wtp = local->wtp;
	cpwm = local->cpwm;
	ssg = local->ssg >= 0? gain:-gain;

	wt3 = specs->wave[7];
	if ((sob = specs->spec.io[DCO_SYNC_OUT].buf) == NULL)
		sob = specs->null;

 	if (bristolBLOcheck(voice->dFreq*transp)) {
		memset(param->param[1].mem, 0, EXPDCO_WAVE_SZE * sizeof(float));
		memset(param->param[3].mem, 0, EXPDCO_WAVE_SZE * sizeof(float));
		memset(param->param[4].mem, 0, EXPDCO_WAVE_SZE * sizeof(float));

		generateBLOwaveformF(voice->cFreq*transp,
			param->param[1].mem, BLO_SQUARE);
		generateBLOwaveformF(voice->cFreq*transp,
			param->param[3].mem, BLO_RAMP);
		generateBLOwaveformF(voice->cFreq*transp,
			param->param[4].mem, BLO_TRI);
	} else {
		bcopy(specs->wave[1], param->param[1].mem,
			EXPDCO_WAVE_SZE * sizeof(float));
		bcopy(specs->wave[3], param->param[3].mem,
			EXPDCO_WAVE_SZE * sizeof(float));
		bcopy(specs->wave[4], param->param[4].mem,
			EXPDCO_WAVE_SZE * sizeof(float));
	}

	/*
	 * Go jumping through the wavetable, with each jump defined by the value
	 * given on our input line, making sure we fill one output buffer.
	 *
	 * This is quite an intentive oscillator due to the requirements for
	 * waveform morphs ala PWM between different waves.
	 */
	for (obp = 0; obp < count;obp++)
	{
		/*
		 * take a wave based on the value of the waveform:
		 *
		 *	0 0.33 triangle going down.
		 *  0 0.33 ramp going up
		 *  0.33 0.66 ramp going down
		 *  0.33 0.66 square going up PW 50%.
		 *  0.66 1.0 Square going PW 10%
		 *
		 * May extend this to include the jagged edged ramp? Good for stringy
		 * sounds.
		 */
		wdelta = (int) wtp;
		gdelta = wtp - ((float) wdelta);

		/*
		 * If we do not have a modbuf, don't even bother doing this.
		 */
		if ((wform = wave + *mb++) < 0)
			wform = 0;

		/* This generates the sync buffer */
		if (wtp >= EXPDCO_WAVE_SZE_M)
			sob[obp] = (wt3[wdelta] + (wt3[0] - wt3[wdelta]) * gdelta);
		else
			sob[obp] = (wt3[wdelta] + (wt3[wdelta + 1] - wt3[wdelta]) * gdelta);

		if (wform <= 0.33)
		{
			/*
			 * Crossfade tri into ramp
			 */
			wt1 = param->param[4].mem; /* Triangular */
			wt2 = param->param[3].mem; /* Ramp */
			gain2 = wform * 3;
			gain1 = (1.0 - gain2) * gain;
			gain2 *= gain;

			if (wtp >= EXPDCO_WAVE_SZE_M)
				ob[obp] += (wt1[wdelta]
					+ (wt1[0] - wt1[wdelta]) * gdelta)
						* gain1
					+ (wt2[wdelta] + (wt2[0] - wt2[wdelta]) * gdelta)
						* gain2;
			else
				ob[obp] += (wt1[wdelta]
					+ (wt1[wdelta + 1] - wt1[wdelta]) * gdelta)
						* gain1
					+ (wt2[wdelta]
						+ (wt2[wdelta + 1] - wt2[wdelta]) * gdelta)
						* gain2;
		} else if (wform <= 0.66) {
			/*
			 * Crossrade ramp into square, however I want the square to be
			 * a difference of two ramps.....
			 */
			wt1 = param->param[3].mem; /* Triangular */
			wt2 = param->param[1].mem; /* Square */
			gain2 = (wform - 0.33) * 3;
			gain1 = (1.0 - gain2) * gain;
			gain2 *= ssg;

			if (wtp >= EXPDCO_WAVE_SZE_M)
				ob[obp] += (wt1[wdelta]
					+ (wt1[0] - wt1[wdelta]) * gdelta)
						* gain1
					+ (wt2[wdelta] + (wt2[0] - wt2[wdelta]) * gdelta)
						* gain2;
			else
				ob[obp] += (wt1[wdelta]
					+ (wt1[wdelta + 1] - wt1[wdelta]) * gdelta)
						* gain1
					+ (wt2[wdelta]
						+ (wt2[wdelta + 1] - wt2[wdelta]) * gdelta)
						* gain2;
		} else {
			/*
			 * This is for a variable pulse width, reuse gain1 as width.
			 */
			gain1 = EXPDCO_WAVE_SZE_2 - (wform - 0.66) * EXPDCO_WAVE_SZE_3;

			if (wtp >= gain1)
			{
				/*
				 * This creates a pulse width modulated square wave, with a
				 * degrading plateau.
				 */
				if (cpwm > 0)
					cpwm = -BRISTOL_SQR;
				ob[obp] += cpwm * ssg;
			} else {
				if (cpwm <= 0)
					cpwm = BRISTOL_SQR;
				ob[obp] += cpwm * ssg;
			}
		}

		/* Moved sbuf to sb */
		if (sb && (sb[obp] != 0))
		{
			ssg = -ssg;
			wtp = sb[obp];
			continue;
		}

		/*
		 * Move the wavetable pointer forward by amount indicated in input 
		 * buffer for this sample.
		 */
		if ((wtp += ib[obp] * transp) >= EXPDCO_WAVE_SZE)
			wtp -= EXPDCO_WAVE_SZE;
		/*
		 * If we have gone negative, round back up. Allows us to run the 
		 * oscillator backwards.
		 */
		if (wtp < 0)
			wtp += EXPDCO_WAVE_SZE;
	}

/*printf("%f\n", gain1); */
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
expdcoinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolEXPDCO *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("dcoinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolEXPDCO *) bristolmalloc0(sizeof(bristolEXPDCO));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolEXPDCO);

	specs->null = bristolmalloc(sizeof(float) * samplecount);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolEXPDCOlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(EXPDCO_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(EXPDCO_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(EXPDCO_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(EXPDCO_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(EXPDCO_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(EXPDCO_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(EXPDCO_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(EXPDCO_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], EXPDCO_WAVE_SZE, 0);
	fillWave(specs->wave[1], EXPDCO_WAVE_SZE, 1);
	fillWave(specs->wave[2], EXPDCO_WAVE_SZE, 2);
	fillWave(specs->wave[3], EXPDCO_WAVE_SZE, 3);
	fillWave(specs->wave[4], EXPDCO_WAVE_SZE, 4);
	fillWave(specs->wave[5], EXPDCO_WAVE_SZE, 5);
	fillWave(specs->wave[6], EXPDCO_WAVE_SZE, 6);
	fillWave(specs->wave[7], EXPDCO_WAVE_SZE, 7);

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

	specs->spec.param[4].pname = "sync";
	specs->spec.param[4].description = "synchronise to input";
	specs->spec.param[4].type = BRISTOL_INT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_BUTTON;

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

	specs->spec.io[2].ioname = "mod";
	specs->spec.io[2].description = "oscillator wave mod signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[3].ioname = "sync";
	specs->spec.io[3].description = "oscillator sync signal";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[4].ioname = "sync out";
	specs->spec.io[4].description = "oscillator sync output signal";
	specs->spec.io[4].samplerate = samplerate;
	specs->spec.io[4].samplecount = samplecount;
	specs->spec.io[4].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

static void
fillPDsine(float *mem, int count, int compress)
{
	float j = 0, i, inc1, inc2;

	/*
	 * Resample the sine wave as per casio phase distortion algorithms.
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
		*mem++ += sinf(j) * BRISTOL_VPO;

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
			/* 
			 * This is a square wave although it should not be in use (we
			 * generate PW).
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = blosquare[i];
					return;
			}
			for (i = 0;i < count / 2; i++)
				mem[i] = BRISTOL_SQR;
			mem[0] = 0;
			mem[1] = mem[1] / 2;
			mem[i - 1] = mem[1];
			mem[i] = 0;
			mem[i++ + 1] = -mem[1];
			for (;i < count; i++)
				mem[i] = -BRISTOL_SQR;
			mem[i - 1] = -mem[1];
			return;
		case 2:
			/* 
			 * This is a pulse wave although it should not be in use.
			 */
			for (i = 0;i < count / 5; i++)
				mem[i] = BRISTOL_SQR; /*BRISTOL_VPO * 2 / 3; */
			for (;i < count; i++)
				mem[i] = -BRISTOL_SQR; /*BRISTOL_VPO * 2 / 3; */
			return;
		case 3:
			/* 
			 * This is a ramp wave. We scale the index from -.5 to .5, and
			 * multiply by the range. We go from rear to front to table to make
			 * the ramp wave have a positive leading edge.
			for (i = count - 1;i >= 0; i--)
				mem[i] = (((float) i / count) - 0.5) * BRISTOL_VPO * 2.0;
			mem[0] = 0;
			mem[1] = mem[count - 1] = mem[1] / 2;
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = bloramp[i];
					return;
			}
			bristolbzero(mem, count * sizeof(float));
			fillPDsine(mem, count, 5);
			return;
		case 4:
#define BRISTOL_TRI (BRISTOL_VPO * 2.0)
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
				mem[i] = -BRISTOL_TRI
					+ ((float) i * 2 / (count / 2)) * BRISTOL_TRI; 
			for (;i < count; i++)
				mem[i] = BRISTOL_TRI -
					(((float) (i - count / 2) * 2) / (count / 2)) * BRISTOL_TRI;
			return;
		case 5:
			/*
			 * Would like to put in a jagged edged ramp wave. Should make some
			 * interesting strings sounds.
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
			 */
			bristolbzero(mem, count * sizeof(float));
			fillPDsine(mem, count, 5);
			fillPDsine(mem, count/2, 5);
			fillPDsine(&mem[count/2], count/2, 5);
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
			 * Sync waveform.
			 */
			for (i = 0;i < count / 2; i++)
				mem[i] = BRISTOL_SQR;
			for (;i < count; i++)
				mem[i] = -BRISTOL_SQR;
			return;
	}
}

