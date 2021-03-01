
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
 *	envelopeinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 *
 *
 *	level1 rate1 level2
 *	level2 rate2 level3
 *	level3 rate3 level4 (Sustain)
 *	level4 rate4 level5
 *	
 *	Level 1 and level 5 are typically going to be zero however not for the
 *	CS-80, they will be equal to sustain via the GUI. All the rates can be
 *	applied in either direction since the envelope may retrigger.
 *
 *	If any two adjacent levels are similar they should be dropped, alternatively
 *	the rate is actually a time for that level?
 *
 *	Needs options for touch sensitivity, rezero, maybe others.
 */

#include "bristol.h"
#include "env5stage.h"

static float samplerate = 0;

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "ENV"
#define OPDESCRIPTION "Digital ADSR 5-stage Envelope Generator"
#define PCOUNT 13
#define IOCOUNT 1

#define ENV_OUT_IND 0

#define LEVEL_0		0
#define RATE_0		1
#define LEVEL_1		2
#define RATE_1		3
#define LEVEL_2		4
#define RATE_2		5
#define LEVEL_3		6
#define RATE_3		7
#define LEVEL_4		8
#define GAIN		9
#define VELOCITY	10
#define REZERO		11
#define RETRIGGER	12

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
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

/*
 * Reset any local memory information.
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif

	param->param[0].float_val = 0;
	param->param[1].float_val = 0.1;
	param->param[2].float_val = 1.0;
	param->param[3].float_val = 0.3;
	param->param[4].float_val = 0.1;
	param->param[5].float_val = 0.3;
	param->param[6].float_val = 0.5;
	param->param[7].float_val = 0.2;
	param->param[8].float_val = 0;
	/* Gain */
	param->param[9].float_val = 1.0;
	/* Velocity and rezero flags */
	param->param[10].int_val = 1;
	param->param[11].int_val = 0;

	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	//int ivalue = value * C_RANGE_MIN_1;

#ifdef BRISTOL_DBG
	printf("env5param(%i, %f)\n", index, value);
#endif

	if (index >= PCOUNT)
		return(0);

	switch (index) {
		case 0:
		case 2:
		case 4:
		case 6:
		case 8:
		case 9:
			/*
			 * Levels and gain
			 */
			param->param[index].float_val = value;
			break;
		case 1:
		case 3:
		case 5:
		case 7:
			/*
			 * Rates: get tweaked into a rate at operational time as
			 * per decay and release.
			 */
			if ((param->param[index].float_val = value * value) == 0)
				param->param[index].float_val
					= 1.0 / (CONTROLLER_RANGE * CONTROLLER_RANGE);

			/*
			 * This is now converted into a number of seconds up to 10
			 */
			if ((param->param[index].float_val = 1.0
				/ (param->param[index].float_val * samplerate * 10)) > 0.5)
				param->param[index].float_val = 0.5;
			break;
		case 10:
		case 11:
		case 12:
			if (value == 0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			break;
	}
#ifdef BRISTOL_DBG
	printf("R: %1.2f %1.4f %1.2f, %1.2f %1.2f, %1.2f %1.2f, %1.2f %1.2f\n",
		param->param[LEVEL_0].float_val, param->param[RATE_0].float_val,
		param->param[LEVEL_1].float_val, param->param[RATE_1].float_val,
		param->param[LEVEL_2].float_val, param->param[RATE_2].float_val,
		param->param[LEVEL_3].float_val, param->param[RATE_3].float_val,
		param->param[LEVEL_4].float_val);
#endif

	return(0);
}

/*
 * Envelope generation:
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param, void *lcl)
{
	register bristolENVlocal *local = lcl;
	register float *ob, current, cgain, gain;
	register int count;
	bristolENV *specs;

	specs = (bristolENV *) operator->specs;

#ifdef BRISTOL_DBG
	printf("env5stage(%x, %x, %x, %x)\n", operator, voice, param, local);
#endif

	count = specs->spec.io[0].samplecount;
	ob = specs->spec.io[0].buf;

	if (voice->flags & BRISTOL_KEYOFF)
	{
		voice->flags |= BRISTOL_KEYOFFING;
		local->state = RELEASE_STAGE;
	}

	cgain = local->cgain;
	current = local->current;

	if (param->param[VELOCITY].int_val)
		gain = param->param[GAIN].float_val
			* (voice->velocity + voice->press + voice->chanpressure);
	else
		gain = param->param[GAIN].float_val;

	if (voice->flags & BRISTOL_KEYREON)
	{
		if (param->param[RETRIGGER].int_val)
		{
			if (param->param[REZERO].int_val != 0)
				current = param->param[LEVEL_0].float_val;
			local->state = ATTACK_STAGE;
		}
	} else if (voice->flags & BRISTOL_KEYON) {
		cgain = gain;
		current = param->param[LEVEL_0].float_val;
		local->state = ATTACK_STAGE;
/*
printf("T: %1.2f %1.4f %1.2f, %1.2f %1.2f, %1.2f %1.2f, %1.2f %1.2f\n",
param->param[LEVEL_0].float_val,
param->param[RATE_0].float_val,
param->param[LEVEL_1].float_val,
param->param[RATE_1].float_val,
param->param[LEVEL_2].float_val,
param->param[RATE_2].float_val,
param->param[LEVEL_3].float_val,
param->param[RATE_3].float_val,
param->param[LEVEL_4].float_val);
*/
		if ((~voice->flags & BRISTOL_KEYREON) &&
			(voice->offset > 0) && (voice->offset < count))
		{
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				printf("envelope trigger offset %i frames\n", voice->offset);

			memset(ob, 0.0f, voice->offset * sizeof(float));
			ob += voice->offset;
			count -= voice->offset;
		}
	}

	while (count-- > 0)
	{
		switch (local->state) {
			case STAGE_0:
				if (current > param->param[LEVEL_1].float_val)
				{
					if ((*ob = (current -= param->param[RATE_0].float_val))
						< param->param[LEVEL_1].float_val)
						local->state++;
				} else {
					if ((*ob = (current += param->param[RATE_0].float_val))
						>= param->param[LEVEL_1].float_val)
						local->state++;
				}
				break;
			case STAGE_1:
				if (current > param->param[LEVEL_2].float_val)
				{
					if ((*ob = (current -= param->param[RATE_1].float_val))
						< param->param[LEVEL_2].float_val)
						local->state++;
				} else {
					if ((*ob = (current += param->param[RATE_1].float_val))
						>= param->param[LEVEL_2].float_val)
						local->state++;
				}
				break;
			case STAGE_2:
				if (current > param->param[LEVEL_3].float_val)
				{
					if ((*ob = (current -= param->param[RATE_2].float_val))
						< param->param[LEVEL_3].float_val)
						local->state++;
				} else {
					if ((*ob = (current += param->param[RATE_2].float_val))
						>= param->param[LEVEL_3].float_val)
						local->state++;
				}
				break;
			case STAGE_3:
				/* Sustain */
				if (current > param->param[LEVEL_3].float_val)
				{
					if ((*ob = (current -= param->param[RATE_2].float_val))
						< param->param[LEVEL_3].float_val)
						current = param->param[LEVEL_3].float_val;
				} else if (current < param->param[LEVEL_3].float_val) {
					if ((*ob = (current += param->param[RATE_2].float_val))
						>= param->param[LEVEL_3].float_val)
						current = param->param[LEVEL_3].float_val;
				} else
					*ob = current;
				break;
			case STAGE_4:
				if (current > param->param[LEVEL_4].float_val)
				{
					if ((*ob = (current -= param->param[RATE_3].float_val))
						< param->param[LEVEL_4].float_val)
						local->state++;
				} else {
					if ((*ob = (current += param->param[RATE_3].float_val))
						>= param->param[LEVEL_4].float_val)
						local->state++;
				}
				break;
			case STAGE_5:
			default:
				*ob = param->param[LEVEL_4].float_val;
				break;
		}

		/*
		 * This smooths over key reassigments, channel or poly pressure changes
		 * where the target gain can jump noticably - our operational gain will
		 * tend towards the target. It may cause a bit of breathing however that
		 * is far preferable to ticking.
		 */
		if (cgain > gain)
		{
			if ((cgain -= 0.067) < gain) cgain = gain;
		} else if (cgain < gain) {
			if ((cgain += 0.067) > gain) cgain = gain;
		}

		*ob++ *= cgain * BRISTOL_VPO;
	}

	if (local->state == END_STAGE)
	{
		voice->flags |= BRISTOL_KEYDONE;
		voice->flags &= ~BRISTOL_KEYOFFING;
	} else
		voice->flags &= ~(BRISTOL_KEYDONE|BRISTOL_KEYOFFING);

	local->current = current;
	local->cgain = cgain;

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
env5stageinit(bristolOP **operator, int index, int sr, int samplecount)
{
	bristolENV *specs;

#ifdef BRISTOL_DBG
	printf("env5stageinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	*operator = bristolOPinit(operator, index, samplecount);
	samplerate = sr;

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;

	specs = (bristolENV *) bristolmalloc0(sizeof(bristolENV));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolENV);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolENVlocal);

	/*
	 * Now fill in the envelope specs for this operator. These are specific to
	 * an ADSR.
	 */
	specs->spec.param[0].pname = "level-3";
	specs->spec.param[0].description = "level 1";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "rate-1";
	specs->spec.param[1].description = "Rate 1";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "level-2";
	specs->spec.param[2].description = "level 2";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "rate-2";
	specs->spec.param[3].description = "Rate 2";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[4].pname = "level-3";
	specs->spec.param[4].description = "level 3";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[5].pname = "rate-3";
	specs->spec.param[5].description = "Rate 3";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[6].pname = "level-4";
	specs->spec.param[6].description = "level 4";
	specs->spec.param[6].type = BRISTOL_FLOAT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[7].pname = "rate-4";
	specs->spec.param[7].description = "Rate 4";
	specs->spec.param[7].type = BRISTOL_FLOAT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[8].pname = "level-5";
	specs->spec.param[8].description = "level 5";
	specs->spec.param[8].type = BRISTOL_FLOAT;
	specs->spec.param[8].low = 0;
	specs->spec.param[8].high = 1;
	specs->spec.param[8].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[9].pname = "gain";
	specs->spec.param[9].description = "Overall signal level";
	specs->spec.param[9].type = BRISTOL_FLOAT;
	specs->spec.param[9].low = 0;
	specs->spec.param[9].high = 1;
	specs->spec.param[9].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[10].pname = "velocity";
	specs->spec.param[10].description = "velocity sensitive gain";
	specs->spec.param[10].type = BRISTOL_INT;
	specs->spec.param[10].low = 0;
	specs->spec.param[10].high = 1;
	specs->spec.param[10].flags = BRISTOL_BUTTON;

	specs->spec.param[11].pname = "rezero";
	specs->spec.param[11].description = "reset gain on retrigger";
	specs->spec.param[11].type = BRISTOL_INT;
	specs->spec.param[11].low = 0;
	specs->spec.param[11].high = 1;
	specs->spec.param[11].flags = BRISTOL_BUTTON;

	specs->spec.param[12].pname = "retrigger";
	specs->spec.param[12].description = "retrigger on reon";
	specs->spec.param[12].type = BRISTOL_INT;
	specs->spec.param[12].low = 0;
	specs->spec.param[12].high = 1;
	specs->spec.param[12].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "output";
	specs->spec.io[0].description = "ADSR Envelope Output Signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_DC|BRISTOL_OUTPUT;

	return(*operator);
}

