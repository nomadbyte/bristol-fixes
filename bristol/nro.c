
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

/*
 * These are intended to be non-resampling oscillators. They will use an 
 * algorithm to generate the samples and based on the algorithm they will be
 * correct, ie, no resampling will be required. There will be an 'adjustment'
 * at the start/end of each cycle where correction is needed to hit a frequency
 * since no frequency will be an integral fraction of a wave. This same point
 * will be used to seed the next full wave.
 *
 * There are only a few waveforms that are being targetted:
 *
 * 	sine
 * 	tri
 * 	square/pulse/pwm
 * 	saw
 * 	ramp
 *
 * The saw and ramp code is naturally the same however some of the code may 
 * use these as LFO. All of them should support sync, possibly excepting the
 * sine?
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "bristol.h"
#include "nro.h"

#define OPNAME "NRO-OSC"
#define OPDESCRIPTION "Non Resampling Digitally Controlled Oscillator"
#define PCOUNT 16 /* As of 0.30.6 this was the max */
#define IOCOUNT 5

/* IO indeces */
#define NRO_IN_IND 0
#define NRO_OUT_IND 1
#define NRO_PWM_IND 2
#define NRO_SYNC_IND 3
#define NRO_SYNC_OUT 4

/*
 * Param indices
 *
 * Tune fine +/- semitone
 * Tune coarse +/- 7 or 12 semitones
 * Transpose in notes
 * Detune
 *
 * 5 harmonics: 16/8/4/2/sub
 * 4 waves (sqr/saw/tri/sine)
 *
 * Pulse width
 *
 * Reload tendencies
 */
#define FINETUNE 0
#define COARSETUNE 1
#define TRANSPOSE 2
#define DETUNE 3

#define H16 4
#define H8 5
#define H4 6
#define H2 7
#define H32 8

#define SQUARE 9
#define SAW 10
#define TRI 11
#define SINE 12

#define PW 13
#define DRAIN 14

#define EMULATION 15

static float note_diff;

/* Detuning for the different tendencies, controlled by DETUNE */
static float dt[TENDENCY_COUNT] = {
 0.043,
 0.073,
 0.029,
 0.031,
 0.007,
 0.011,
 0.079,
 0.083,
 0.019,
 0.059,
 0.061,
 0.071,
 0.017,
 0.023,
 0.013,
 0.089,
 0.097, 
 0.047,
 0.053,
 0.037,
 0.0,
};

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("nrodestroy(%x)\n", operator);
#endif

	/*
	 * Unmalloc anything we added to this structure
	 */
	bristolfree(((bristolNRO *) operator)->zb);
	bristolfree(((bristolNRO *) operator)->fb);
	bristolfree(((bristolNRO *) operator)->fbm);
	bristolfree(((bristolNRO *) operator)->pwmb);
	bristolfree(((bristolNRO *) operator)->sb);

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
	printf("nroreset(%x, %x)\n", operator, param);
#endif

	param->param[0].float_val = 1.0;
	param->param[1].float_val = 1.0;
	param->param[2].float_val = 0.0;
	param->param[3].float_val = 0.0;
	param->param[4].float_val = 1.0;
	param->param[5].float_val = 0.0;
	param->param[6].float_val = 0.0;
	param->param[7].float_val = 1.0;
	param->param[8].float_val = 1.0;
	param->param[9].float_val = 1.0;
	param->param[11].float_val = 0.0;
	param->param[12].float_val = 0.0;
	param->param[13].float_val = 0.5;
	param->param[14].float_val = 0.98;
	param->param[15].int_val = 0.0;

	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("	nroparam(%x, %x, %i, %x)\n", operator, param, index,
		((int) (value * CONTROLLER_RANGE)));
#endif

	switch (index) {
		case FINETUNE: /* Fine tuning +/- 1/2 tone */
			{
				float tune, notes = 1.0;

				tune = (value - 0.5) * 2;

				/*
				 * Up or down one note
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
			}
			break;
		case COARSETUNE: /* Coarse tuning in "small" amounts */
			{
				float tune, notes = 1.0;
				int i;

				tune = (value - 0.5) * 2;

				/*
				 * Up or down 11 notes.
				 */
				for (i = 0; i < 11;i++)
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

				/* This might be superfluous? */
				((bristolNRO *) operator->specs)->seq++;
			}
			break;
		case TRANSPOSE:
			/* Transpose in semitone steps */
			{
				int semitone = value * CONTROLLER_RANGE;
				float transpose;

				if (semitone == param->param[index].int_val)
					break;

				param->param[index].int_val = semitone;

				transpose = 1.0;

				while (semitone-- > 0)
					transpose *= (note_diff);

				/* This check is now superfluous since we check the semi */
				if (transpose != param->param[index].float_val)
				{
					param->param[index].float_val = transpose;
					((bristolNRO *) operator->specs)->seq++;
				}
			}
			break;
		case DETUNE: /* Detuning */
			param->param[index].float_val = value;
			break;

		case H16: /* Harmonics */
		case H8: /* Harmonics */
		case H4: /* Harmonics */
		case H2: /* Harmonics */
		case H32: /* Harmonics */
			param->param[index].float_val = value * 4;
			break;

		case SQUARE: /* Waveforms */
		case SAW: /* Waveforms */
		case TRI: /* Waveforms */
		case SINE: /* Waveforms */
			param->param[index].float_val = value;
			break;

		case PW: /* Pulse Width */
			if ((param->param[index].float_val = value) < 0.1)
				param->param[index].float_val = 0.1;
			else if ((param->param[index].float_val = value) > 0.9)
				param->param[index].float_val = 0.9;
			break;

		case DRAIN: /* signal draining */
			param->param[index].float_val = (1.0 - value);
			break;

		case EMULATION: /* Some of the wave formats might be a bit specific? */
			param->param[index].int_val = value * CONTROLLER_RANGE;
			break;
	}

	return(0);
}

/*
 * Take the sync buffer and look for zero crossings. When they are found
 * do a resample to find out where the crossing happened as sample level
 * resolution results in distortion and shimmer
 *
 * Resampling, or rather interpolation to zero, means we have to look at 
 * the respective above and below zero values.
 *
 * This is NOT applied to the sync buffer, we apply it to our signal to 
 * generate a sync buffer for returning to the caller.
 */
static float
genSyncPoints(register float *dst, register float *src, register float lsv, register int count)
{
	for (; count > 0; count--, lsv = *src++)
		*dst++ = ((lsv < 0) && (*src >= 0))? lsv / (lsv - *src) : 0;

	return(lsv);
}

/*
static void
addbuf(register float *buf, register float value, register int c)
{
	for (; c > 0; c-=8)
	{
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
	}
}

static void
fillbuf(register float *buf, register float value, register int c)
{
	for (; c > 0; c-=8)
	{
		*buf++ = value;
		*buf++ = value;
		*buf++ = value;
		*buf++ = value;
		*buf++ = value;
		*buf++ = value;
		*buf++ = value;
		*buf++ = value;
	}
}
*/

static void
addbuf2(register float *dst, register float *src, register float value, register int c)
{
	for (; c > 0; c-=8)
	{
		*dst++ = *src++ + value;
		*dst++ = *src++ + value;
		*dst++ = *src++ + value;
		*dst++ = *src++ + value;
		*dst++ = *src++ + value;
		*dst++ = *src++ + value;
		*dst++ = *src++ + value;
		*dst++ = *src++ + value;
	}
}

static void
addpwm(register float *dst, register float *src, register float value, register int c)
{
	for (; c > 0; c-=8)
	{
		if ((*dst = *src++ + value) > 0.9) *dst = 0.9; dst++;
		if ((*dst = *src++ + value) > 0.9) *dst = 0.9; dst++;
		if ((*dst = *src++ + value) > 0.9) *dst = 0.9; dst++;
		if ((*dst = *src++ + value) > 0.9) *dst = 0.9; dst++;
		if ((*dst = *src++ + value) > 0.9) *dst = 0.9; dst++;
		if ((*dst = *src++ + value) > 0.9) *dst = 0.9; dst++;
		if ((*dst = *src++ + value) > 0.9) *dst = 0.9; dst++;
		if ((*dst = *src++ + value) > 0.9) *dst = 0.9; dst++;
	}
}

static void
mult2buf(register float *dst, register float *src, register float value, register int c)
{
	for (; c > 0; c-=8)
	{
		*dst++ = value * *src++;
		*dst++ = value * *src++;
		*dst++ = value * *src++;
		*dst++ = value * *src++;
		*dst++ = value * *src++;
		*dst++ = value * *src++;
		*dst++ = value * *src++;
		*dst++ = value * *src++;
	}
}

static void
multbuf(register float *buf, register float value, register int c)
{
	for (; c > 0; c-=8)
	{
		*buf++ *= value;
		*buf++ *= value;
		*buf++ *= value;
		*buf++ *= value;
		*buf++ *= value;
		*buf++ *= value;
		*buf++ *= value;
		*buf++ *= value;
	}
}

/* freq buffer, mod buffers 1 and 2, sync buffer, output buf and count */
static void
genSineTendency(bristolNRO *gt, bristolNROlocal *t, float *fb, float *mb, float *sb, float *ob, int c)
{
	for (; c > 0; c--, fb++, sb++, mb++, ob++, t->cfc++)
	{
		if ((t->ls += *fb * t->tr) > 2 * M_PI) t->ls -= 2 * M_PI;
		*ob += sinf(t->ls) * gt->vpo * t->gn;
	}
}

/* freq buffer, mod buffers 1 and 2, sync buffer, output buf and count */
static void
genSquareTendency(bristolNRO *gt, bristolNROlocal *t, float *fb, float *mb, float *sb, float *ob, int c)
{
	/*
	 * Square will have a fast ramp up/down and then a decaying plateau. We
	 * should accept a bit of overshoot on the fast ramp and consider clamping
	 * it back on the first following sample?
	 *
	 * We should have a single algorithm which is ramp from -VPO to +VPO which 
	 * is then inverted as necessary depending on state:
	 *
	 * 	From current state fast ramp to target, correct overshoot, then drain.
	 *
	 * The period at the +ve height is a function of the PWM buffer content,
	 * the period at the -ve height is then a function of the frequency.
	 *
	 * Both of these are fixed for any given cycle to maintain frequency.
	 */
	for (; c > 0; c--, fb++, sb++, mb++, ob++, t->cfc++)
	{
		if (t->climit > 0) {
			if (*sb != 0) {
				t->cfc = 1.0 - *sb;
				if ((t->ls -= gt->ramp * t->cfc) < gt->llimit)
 					t->flags = TEND_STATE_RAMP_2;
				else
				 	t->flags = TEND_STATE_RAMP_1;
				t->climit = gt->llimit;
			} else if (t->cfc > *fb * t->tr * *mb) {
				if ((t->ls -= gt->ramp * (t->cfc - *fb * t->tr * *mb))
					< gt->llimit)
 					t->flags = TEND_STATE_RAMP_2;
				else
				 	t->flags = TEND_STATE_RAMP_1;
				t->climit = gt->llimit;
			} else {
			 	switch (t->flags) {
					case TEND_STATE_RAMP_1:
						if ((t->ls += gt->ramp) > gt->ulimit)
			 				t->flags = TEND_STATE_RAMP_2;
						break;
					case TEND_STATE_RAMP_2:
						t->ls = gt->ulimit;
		 				t->flags = TEND_STATE_RAMP_3;
						break;
					default:
						t->ls *= gt->drain;
						break;
				}
			}
		} else {
			if (*sb != 0) {
				t->cfc = 1.0 - *sb;
//				t->cfc -= *fb * t->tr;
				if ((t->ls += gt->ramp * t->cfc) > gt->ulimit)
 					t->flags = TEND_STATE_RAMP_2;
				else
				 	t->flags = TEND_STATE_RAMP_1;
				t->climit = gt->ulimit;
			} else if (t->cfc > *fb * t->tr) {
				t->cfc -= *fb * t->tr;
				if ((t->ls += gt->ramp * t->cfc) > gt->ulimit)
 					t->flags = TEND_STATE_RAMP_2;
				else
				 	t->flags = TEND_STATE_RAMP_1;
				t->climit = gt->ulimit;
			} else {
			 	switch (t->flags) {
					case TEND_STATE_RAMP_1:
						if ((t->ls -= gt->ramp) < gt->llimit)
			 				t->flags = TEND_STATE_RAMP_2;
						break;
					case TEND_STATE_RAMP_2:
						t->ls = gt->llimit;
		 				t->flags = TEND_STATE_RAMP_3;
						break;
					default:
						t->ls *= gt->drain;
						break;
				}
			}
		}
		*ob += t->ls * t->gn;
	}
}

/* freq buffer, mod buffers 1 and 2, sync buffer, output buf and count */
static void
genSquare2Tendency(bristolNRO *gt, bristolNROlocal *t, float *fb, float *mb, float *sb, float *ob, int c)
{
	/*
	 * Square will have a fast ramp up/down and then a decaying plateau. We
	 * should accept a bit of overshoot on the fast ramp and consider clamping
	 * it back on the first following sample?
	 *
	 * We should have a single algorithm which is ramp from -VPO to +VPO which 
	 * is then inverted as necessary depending on state:
	 *
	 * 	From current state fast ramp to target, correct overshoot, then drain.
	 *
	 * The period at the +ve height is a function of the PWM buffer content,
	 * the period at the -ve height is then a function of the frequency.
	 *
	 * Both of these are fixed for any given cycle to maintain frequency.
	 */
	for (; c > 0; c--, fb++, sb++, mb++, ob++, t->cfc++)
	{
		if (t->climit > 0) {
			if (*sb != 0) {
				t->cfc = 1.0 - *sb;
				if ((t->ls -= gt->ramp * t->cfc) < gt->llimit)
 					t->flags = TEND_STATE_RAMP_2;
				else
				 	t->flags = TEND_STATE_RAMP_1;
				t->climit = gt->llimit;
			} else if (t->cfc > *fb * t->tr * *mb) {
				if ((t->ls -= gt->ramp * (t->cfc - *fb * t->tr * *mb))
					< gt->llimit)
 					t->flags = TEND_STATE_RAMP_2;
				else
				 	t->flags = TEND_STATE_RAMP_1;
				t->climit = gt->llimit;
			} else {
			 	switch (t->flags) {
					case TEND_STATE_RAMP_1:
						if ((t->ls += gt->ramp) > gt->ulimit)
			 				t->flags = TEND_STATE_RAMP_2;
						break;
					case TEND_STATE_RAMP_2:
						t->ls = gt->ulimit;
		 				t->flags = TEND_STATE_RAMP_3;
						break;
					default:
						t->ls -= (t->climit + t->ls) * gt->drain;
						break;
				}
			}
		} else {
			if (*sb != 0) {
				t->cfc = 1.0 - *sb;
//				t->cfc -= *fb * t->tr;
				if ((t->ls += gt->ramp * t->cfc) > gt->ulimit)
 					t->flags = TEND_STATE_RAMP_2;
				else
				 	t->flags = TEND_STATE_RAMP_1;
				t->climit = gt->ulimit;
			} else if (t->cfc > *fb * t->tr) {
				t->cfc -= *fb * t->tr;
				if ((t->ls += gt->ramp * t->cfc) > gt->ulimit)
 					t->flags = TEND_STATE_RAMP_2;
				else
				 	t->flags = TEND_STATE_RAMP_1;
				t->climit = gt->ulimit;
			} else {
			 	switch (t->flags) {
					case TEND_STATE_RAMP_1:
						if ((t->ls -= gt->ramp) < gt->llimit)
			 				t->flags = TEND_STATE_RAMP_2;
						break;
					case TEND_STATE_RAMP_2:
						t->ls = gt->llimit;
		 				t->flags = TEND_STATE_RAMP_3;
						break;
					default:
						t->ls -= (t->climit + t->ls) * gt->drain;
						break;
				}
			}
		}
		*ob += t->ls * t->gn;
	}
}

static void
genSawTendency(bristolNRO *gt, bristolNROlocal *t, float *fb, float *mb, float *sb, float *ob, int c)
{
//printf("genSaw %f %f\n", *fb, t->cfc);
	/*
	 * From current ramp down slowly. On full cycle ramp up fast then correct.
	 */
	for (; c > 0; c--, fb++, sb++, mb++, ob++, t->cfc++)
	{
		if (*sb != 0) {
			t->cfc = 1.0 - *sb;
//			t->ls += gt->ramp * t->cfc;
			if ((t->ls += gt->ramp * t->cfc) > gt->ulimit)
 				t->flags = TEND_STATE_RAMP_2;
			else
		 		t->flags = TEND_STATE_RAMP_1;
		} else if (t->cfc > *fb * t->tr) {
			t->cfc -= *fb * t->tr;
			if ((t->ls += gt->ramp * t->cfc) > gt->ulimit)
 				t->flags = TEND_STATE_RAMP_2;
			else
			 	t->flags = TEND_STATE_RAMP_1;
		} else {
		 	switch (t->flags) {
				case TEND_STATE_RAMP_1:
					if ((t->ls += gt->ramp) > gt->ulimit)
		 				t->flags = TEND_STATE_RAMP_2;
					break;
				case TEND_STATE_RAMP_2:
					t->ls = gt->ulimit;
	 				t->flags = TEND_STATE_RAMP_3;
					break;
				default:
					t->ls += (gt->llimit - t->ls) * gt->damp;
					break;
			}
		}
		*ob += t->ls * t->gn;
	}
}

static void
genRampTendency(bristolNRO *gt, bristolNROlocal *t, float *fb, float *mb, float *sb, float *ob, int c)
{
	genSawTendency(gt, t, fb, mb, sb, ob, c);
	multbuf(ob, -1.0, c);
}

/* freq buffer, mod buffers 1 and 2, sync buffer, output buf and count */
static void
genTriTendency(bristolNRO *gt, bristolNROlocal *t, float *fb, float *mb, float *sb, float *ob, int c)
{
//printf("genTri %i\n", t->t);
	for (; c > 0; c--, fb++, sb++, ob++, t->cfc++)
	{
		/*
		 * See if the frequency indicates direction, change.
		 */
		if (*sb != 0) {
			/*
			 * cfc is reset to value unrelated to fb
			 */
			t->cfc = 1.0 - *sb;
			t->climit = t->climit > 0? gt->llimit:gt->ulimit;
			t->ls = t->climit * gt->damp * t->cfc;
		} else if (t->cfc > *fb * t->tr) {
			/*
			 * Calculate where the correction actually hits. This is probably
			 * actually resampling, we evaluate the peak reached at the point
			 * of switching direction then we calculate the tendency towards
			 * the new target for the remainder from that point.
			 */
			t->cfc -= *fb * t->tr;
			t->ls += (t->climit - t->ls) * gt->damp * t->cfc;
			t->climit = t->climit > 0? gt->llimit:gt->ulimit;
			t->ls += (t->climit - t->ls) * gt->damp * (1.0 - t->cfc);
		} else 
			*ob += (t->ls += (t->climit - t->ls) * gt->damp);
		*ob += t->ls * t->gn;
	}
}

/*
 * Convert to samples per cycle for whole waveforms, ramp and saw for example,
 * but also square since it take the frequency and appliesa PWM modulation.
 */
static void
convertF2S(float *src, float *dst, float trans, float sr, int c)
{
	if (trans <= 0)
		trans = 1;

	for (; c > 0; c-=8)
	{
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
	}
}

/*
 * Number of samples per half cycle for the waves that have two identical 
 * half cycles (phase inverted). Tri uses this.
 */
static void
convertF2HS(float *src, float *dst, float trans, float sr, int c)
{
	if ((trans *= 2) <= 0)
		trans = 2;

	for (; c > 0; c-=8)
	{
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
		*dst++ = sr / (*src++ * trans);
	}
}

static void
convertF2Pi(float *src, float *dst, float trans, float sr, int c)
{
	if (trans >= 0)
		trans *= 2 * M_PI / sr;
	else
		trans = 2 * M_PI / sr;

	/*
	 * Convert to a number of PI. We have the frequency and sample rate from 
	 * which we can calculate which part of M_PI represents this sample.
	 */
	for (; c > 0; c-=8)
	{
		*dst++ = trans * *src++;
		*dst++ = trans * *src++;
		*dst++ = trans * *src++;
		*dst++ = trans * *src++;
		*dst++ = trans * *src++;
		*dst++ = trans * *src++;
		*dst++ = trans * *src++;
		*dst++ = trans * *src++;
	}
}

/* This should perhaps eventually come from a file? */
void
initNROglobals(bristolNRO *gt, int sr, int sc)
{
	float t;

/*	printf("	init global tendencies (%i/%i)\n", sr, sc);*/

	gt->conv[BRISTOL_TEND_TRI]		= convertF2HS; /* Test sync */
	gt->gen[BRISTOL_TEND_TRI]		= genTriTendency;

	gt->conv[BRISTOL_TEND_SINE]		= convertF2Pi; /* OK */
	gt->gen[BRISTOL_TEND_SINE]		= genSineTendency;

	gt->conv[BRISTOL_TEND_SQUARE]	= convertF2S; /* Test sync */
	gt->gen[BRISTOL_TEND_SQUARE]	= genSquareTendency;

	gt->conv[BRISTOL_TEND_SAW]		= convertF2S; /* Test sync */
	gt->gen[BRISTOL_TEND_SAW]		= genSawTendency;

	gt->conv[BRISTOL_TEND_RAMP]		= convertF2S; /* Test */
	gt->gen[BRISTOL_TEND_RAMP]		= genRampTendency;

	gt->conv[BRISTOL_TEND_SQR2]		= convertF2S; /* Test */
	gt->gen[BRISTOL_TEND_SQR2]		= genSquare2Tendency;

	gt->ulimit = BRISTOL_VPO;
	gt->llimit = -BRISTOL_VPO;
	gt->vpo = BRISTOL_VPO;

	/*
	 * We want a rise time in here in us to be reformed into a sample delta.
	 *
	 * If we took 10us (already very long) at 44100 kHz we would get just a 
	 * half a sample. At 2x oversampling we get roughly 10us per sample (its
	 * actually just over 11) so then we could look at some reasonable values
	 * here. For the time being, with a native sampling rate, we will have to 
	 * look at above 50us rise times, otherwise we will introduce undesirable
	 * artifacts into the sound.
	 *
	 * 2 * VPO / (us * sr)
	 * 24 / (0.000070 * 44100) = 8
	 *
	 * We should check that the configured time here is not subsample at what-
	 * ever rate we are working.
	 */
	t = 0.000070;
	gt->ramp = BRISTOL_VPO * 2 / (t * sr);
	gt->ramp = 8;

	/*
	 * Damp and drain are voltage leakage functions basically. The signal will
	 * tend to zero or the desired target. We took some empirical values to 
	 * start with so for now just make sure these empiricals do not change
	 * with sample rate. Now this is a power function so VPO * pow(t, 12) = x
	 * where x is fixed at 8.
	 * 12 * pow(t, sr / 3600) = 8
	 * pow(t, sr/3600) = 2/3
		 t^sr/2600 = 2/3
		 sr/2600* log(t) = log (2/3)
		 log(t) = log(2/3) * 2600 / 23
	 */
	gt->damp = 0.06353;
	gt->drain = 0.98;

	gt->sr = sr;
	gt->sc = sc;
}

void
initNROlocal(bristolNRO *gt, bristolNROlocal *t, int form, float pw, int flags)
{
	int i;

//	t[0].pw = t[1].pw = t[2].pw = t[3].pw = t[4].pw = pw;

	if ((gt->seq == t->seq)
		&& ((flags & (BRISTOL_KEYON|BRISTOL_KEYREON)) == 0))
		return;

	for (i = 0; i < TENDENCY_COUNT; i++)
	{
		t[i].climit = gt->ulimit;
		t[i].cfc = 0;
		t[i].ls = 0;
		t[i].lsv = gt->ulimit;
	}

	switch (form) {
		case TEND_FORMAT_P8:
		default:
			if (t->seq != 0) {
				t->seq = gt->seq;
				return;
			}

			t[0].form = t[1].form = t[2].form = t[3].form = t[4].form
				= t[TENDENCY_MAX].form = BRISTOL_TEND_SQUARE;
			t[5].form = t[6].form = t[7].form = t[8].form = t[9].form
				= BRISTOL_TEND_SAW;
			t[10].form = t[11].form = t[12].form = t[13].form = t[14].form
				= BRISTOL_TEND_TRI;
			t[15].form = t[16].form = t[17].form = t[18].form = t[19].form
				= BRISTOL_TEND_SINE;

			t[0].tr = t[5].tr = t[10].tr = t[15].tr = 1;
			t[1].tr = t[6].tr = t[11].tr = t[16].tr = t[TENDENCY_MAX].tr = 2;
			t[2].tr = t[7].tr = t[12].tr = t[17].tr = 4;
			t[3].tr = t[8].tr = t[13].tr = t[18].tr = 8;
			t[4].tr = t[9].tr = t[14].tr = t[19].tr = 0.5;

			t[0].gn = t[5].gn = t[10].gn = t[15].gn = t[TENDENCY_MAX].gn = 1.0;
			t[1].gn = t[6].gn = t[11].gn = t[16].gn = 1.0;
			t[2].gn = t[7].gn = t[12].gn = t[17].gn = 1.0;
			t[3].gn = t[8].gn = t[13].gn = t[18].gn = 1.0;
			t[4].gn = t[9].gn = t[14].gn = t[19].gn = 1.0;

			t[TENDENCY_MAX].pw = 0.5;

			break;
		case TEND_FORMAT_BME:
			/*
			 * Here we only want one (or a small few?) strands but the algorithm
			 * is SQR2, sharp leading edge that fades not back to zero but back
			 * to the other extreme. It then starts moving from square to tri.
			 */
			t[0].form = BRISTOL_TEND_SQR2;
			t[0].gn = 1.0;
			break;
	}

	t->seq = gt->seq;
}

static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *p,
	void *lcl)
{
	bristolNROlocal *t = lcl;
	bristolNRO *gt = (bristolNRO *) operator->specs;

	gt->drain = p->param[DRAIN].float_val;

	addpwm(gt->pwmb, gt->spec.io[NRO_PWM_IND].buf,
		p->param[PW].float_val, gt->sc);

	/* Convert the sync buffer into zero crossings */
	t[TENDENCY_MAX].lsv
		= genSyncPoints(gt->sb, gt->spec.io[NRO_SYNC_IND].buf,
			t[TENDENCY_MAX].lsv, gt->sc);

	/* Put in a default frequency buffer */
	mult2buf(gt->fb, gt->spec.io[NRO_IN_IND].buf, voice->baudio->gtune *
		p->param[FINETUNE].float_val * p->param[COARSETUNE].float_val, gt->sc);

	initNROlocal(gt, t,  p->param[EMULATION].int_val, p->param[PW].float_val,
		voice->flags);

	if (p->param[EMULATION].int_val == TEND_FORMAT_BME)
	{
		gt->conv[t[0].form](gt->fb, gt->fbm, 2.0, gt->sr, gt->sc);

		if ((t[0].gn = p->param[H16].float_val * p->param[SQUARE].float_val)
			!= 0)
		{
			t[0].tr = 8 / p->param[TRANSPOSE].float_val
				+ dt[0] * p->param[DETUNE].float_val;

			gt->gen[t[0].form](gt, &t[0],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}

		if ((t[1].gn = p->param[H8].float_val * p->param[SQUARE].float_val)
			!= 0)
		{
			t[1].tr = 4 / p->param[TRANSPOSE].float_val
				+ dt[1] * p->param[DETUNE].float_val;

			gt->gen[t[1].form](gt, &t[1],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}

		if ((t[2].gn = p->param[H4].float_val * p->param[SQUARE].float_val)
			!= 0)
		{
			t[2].tr = 2 / p->param[TRANSPOSE].float_val
				+ dt[2] * p->param[DETUNE].float_val;

			gt->gen[t[2].form](gt, &t[2],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}

		return(0);
	}

	/*
	 * Calculate the gain and transpositions then generate waves.
	 */
	if (p->param[SQUARE].float_val != 0)
	{
		gt->conv[t[0].form](gt->fb, gt->fbm, 2.0, gt->sr, gt->sc);

		if ((t[0].gn = p->param[H16].float_val * p->param[SQUARE].float_val)
			!= 0)
		{
			t[0].tr = 8 / p->param[TRANSPOSE].float_val
				+ dt[0] * p->param[DETUNE].float_val;

			gt->gen[t[0].form](gt, &t[0],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}

		if ((t[1].gn = p->param[H8].float_val * p->param[SQUARE].float_val)
			!= 0)
		{
			t[1].tr = 4 / p->param[TRANSPOSE].float_val
				+ dt[1] * p->param[DETUNE].float_val;

			gt->gen[t[1].form](gt, &t[1],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}

		if ((t[2].gn = p->param[H4].float_val * p->param[SQUARE].float_val)
			!= 0)
		{
			t[2].tr = 2 / p->param[TRANSPOSE].float_val
				+ dt[2] * p->param[DETUNE].float_val;

			gt->gen[t[2].form](gt, &t[2],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}

		if ((t[3].gn = p->param[H2].float_val * p->param[SQUARE].float_val)
			!= 0)
		{
			t[3].tr = 1 / p->param[TRANSPOSE].float_val
				+ dt[3] * p->param[DETUNE].float_val;

			gt->gen[t[3].form](gt, &t[3],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}

		if ((t[4].gn = p->param[H32].float_val * p->param[SQUARE].float_val)
			!= 0)
		{
			t[4].tr = 16 / p->param[TRANSPOSE].float_val
				+ dt[4] * p->param[DETUNE].float_val;

			gt->gen[t[4].form](gt, &t[4],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}
	}

//	mult2buf(gt->fb, gt->fbm, 1.001, gt->sc); /* transpositions */
	if (p->param[SAW].float_val != 0)
	{
		gt->conv[t[5].form](gt->fb, gt->fbm, 2.0, gt->sr, gt->sc);

		if ((t[5].gn = p->param[H16].float_val * p->param[SAW].float_val) != 0)
		{
			t[5].tr = 8 / p->param[TRANSPOSE].float_val
				+ dt[5] * p->param[DETUNE].float_val;

			gt->gen[t[5].form](gt, &t[5],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}
		if ((t[6].gn = p->param[H8].float_val * p->param[SAW].float_val) != 0)
		{
			t[6].tr = 4 / p->param[TRANSPOSE].float_val
				+ dt[6] * p->param[DETUNE].float_val;

			gt->gen[t[6].form](gt, &t[6],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}
		if ((t[7].gn = p->param[H4].float_val * p->param[SAW].float_val) != 0)
		{
			t[7].tr = 2 / p->param[TRANSPOSE].float_val
				+ dt[7] * p->param[DETUNE].float_val;

			gt->gen[t[7].form](gt, &t[7],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}
		if ((t[8].gn = p->param[H2].float_val * p->param[SAW].float_val) != 0)
		{
			t[8].tr = 1 / p->param[TRANSPOSE].float_val
				+ dt[8] * p->param[DETUNE].float_val;

			gt->gen[t[8].form](gt, &t[8],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}
		if ((t[9].gn = p->param[H32].float_val * p->param[SAW].float_val) != 0)
		{
			t[9].tr = 16 / p->param[TRANSPOSE].float_val
				+ dt[9] * p->param[DETUNE].float_val;

			gt->gen[t[9].form](gt, &t[9],
				gt->fbm,
				gt->pwmb,
				gt->sb,
				gt->spec.io[NRO_OUT_IND].buf,
				gt->sc);
		}

	}

	t[TENDENCY_MAX].tr = 4 / p->param[TRANSPOSE].float_val;

	addbuf2(gt->pwmb, gt->zb, 0.5, gt->sc);

	/* finally generate sync out signal */
	gt->conv[0](gt->fb, gt->fbm, 2.0, gt->sr, gt->sc);
	if (gt->spec.io[NRO_SYNC_OUT].buf)
		gt->gen[0](gt, &t[TENDENCY_MAX],
			gt->fbm,
			gt->pwmb, /* No PWM thanks to the addbuf2() */
			gt->zb, /* no sync */
			gt->spec.io[NRO_SYNC_OUT].buf,
			gt->sc);

	return(0);
}

bristolOP *
nroinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolNRO *specs;

#ifdef BRISTOL_DBG
	printf("bit1oscinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolNRO *) bristolmalloc0(sizeof(bristolNRO));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolNRO);

	specs->sb = bristolmalloc0(sizeof(float) * samplecount);
	specs->fb = bristolmalloc0(sizeof(float) * samplecount);
	specs->fbm = bristolmalloc0(sizeof(float) * samplecount);
	specs->pwmb = bristolmalloc0(sizeof(float) * samplecount);
	specs->zb = bristolmalloc0(sizeof(float) * samplecount);

	initNROglobals(specs, samplerate, samplecount);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	/* Up to 20 independent oscillations - 5 harmonics, 4 waves? */
	specs->spec.localsize = sizeof(bristolNROlocal) * TENDENCY_COUNT;

	/*
	 * Now fill in the bit1osc specs for this operator. These are specific to an
	 * oscillator.
	 *
	 * Tune fine +/- semitone
	 * Tune coarse +/- 7 or 12 semitones
	 * Transpose in notes
	 * Detune
	 *
	 * 5 harmonics: 16/8/4/2/sub
	 * 4 waves (sqr/saw/tri/sine)
	 *
	 * Pulse width
	 *
	 * Reload tendencies
	 */
	specs->spec.param[0].pname = "tune";
	specs->spec.param[0].description = "fine semitone tuning";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "tune";
	specs->spec.param[1].description = "coarse 12 semitone tuning";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "tune";
	specs->spec.param[2].description = "transpose in semitones";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "detune";
	specs->spec.param[3].description = "harmonic disonance";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[4].pname = "16' harmonic";
	specs->spec.param[4].description = "16' harmonic volume";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[5].pname = "8' harmonic";
	specs->spec.param[5].description = "8' harmonic volume";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[6].pname = "4' harmonic";
	specs->spec.param[6].description = "4' harmonic volume";
	specs->spec.param[6].type = BRISTOL_FLOAT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[7].pname = "2' harmonic";
	specs->spec.param[7].description = "2' harmonic volume";
	specs->spec.param[7].type = BRISTOL_FLOAT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[8].pname = "sub-harmonic";
	specs->spec.param[8].description = "sub-harmonic volume";
	specs->spec.param[8].type = BRISTOL_FLOAT;
	specs->spec.param[8].low = 0;
	specs->spec.param[8].high = 1;
	specs->spec.param[8].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[9].pname = "square";
	specs->spec.param[9].description = "square signal level";
	specs->spec.param[9].type = BRISTOL_FLOAT;
	specs->spec.param[9].low = 0;
	specs->spec.param[9].high = 1;
	specs->spec.param[9].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[10].pname = "ramp";
	specs->spec.param[10].description = "ramp signal level";
	specs->spec.param[10].type = BRISTOL_FLOAT;
	specs->spec.param[10].low = 0;
	specs->spec.param[10].high = 1;
	specs->spec.param[10].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[11].pname = "tri";
	specs->spec.param[11].description = "tri signal level";
	specs->spec.param[11].type = BRISTOL_FLOAT;
	specs->spec.param[11].low = 0;
	specs->spec.param[11].high = 1;
	specs->spec.param[11].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[12].pname = "sine";
	specs->spec.param[12].description = "sine signal level";
	specs->spec.param[12].type = BRISTOL_FLOAT;
	specs->spec.param[12].low = 0;
	specs->spec.param[12].high = 1;
	specs->spec.param[12].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[13].pname = "pulse width";
	specs->spec.param[13].description = "square wave duty cycle";
	specs->spec.param[13].type = BRISTOL_FLOAT;
	specs->spec.param[13].low = 0;
	specs->spec.param[13].high = 1;
	specs->spec.param[13].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[14].pname = "drain";
	specs->spec.param[14].description = "signal draining";
	specs->spec.param[14].type = BRISTOL_FLOAT;
	specs->spec.param[14].low = 0;
	specs->spec.param[14].high = 1;
	specs->spec.param[14].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[15].pname = "Type";
	specs->spec.param[15].description = "Oscillator type";
	specs->spec.param[15].type = BRISTOL_INT;
	specs->spec.param[15].low = 0;
	specs->spec.param[15].high = TENDENCY_CONV;
	specs->spec.param[15].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the bit1osc IO specs.
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

	specs->spec.io[3].ioname = "Sync Input";
	specs->spec.io[3].description = "input to phase sync process";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[4].ioname = "Sync Output";
	specs->spec.io[4].description = "simple output signal for driving sync";
	specs->spec.io[4].samplerate = samplerate;
	specs->spec.io[4].samplecount = samplecount;
	specs->spec.io[4].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

