
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

/*#define DEBUG */

#include "bristol.h"
#include "dxop.h"

extern int bristolGlobalController(struct BAudio *, u_char, u_char, float);

float *lfobuf, *op0buf, *op1buf, *op2buf, *op3buf, *op4buf, *modbuf;

int
DXGlobalController(Baudio *baudio, u_char controller,
u_char operator, float value)
{
	int op, index;
	dxmix *mix = (dxmix *) baudio->mixlocals;

	/*
	 * Rhodes Chorus
	 */
	if (controller == 123)
	{
		if ((baudio->effect[0] == NULL) || (operator > 3))
		{
			printf("no effect to process\n");
			return(0);
		}

		baudio->effect[0]->param->param[operator].float_val = value;

		return(0);
	}

	/*
	 * DX global controller code.
	 */
/*printf("DXGlobalController(%i, %i, %f)\n", controller, operator, value); */

	if (operator == 99)
	{
/*
		if (operator == 0) {
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
		} else {
			baudio->midi_pitch = value * 24;
		}
*/
		baudio->midi_pitch = value * 24;
		return(0);
	}

	if (operator == 100)
	{
		baudio->glide = value * value * baudio->glidemax;
		return(0);
	}

	if (operator == 101)
	{
		baudio->mixflags &= ~DX_ALGO_M;
		baudio->mixflags |= (int) (value * CONTROLLER_RANGE);
/*printf("flags are now %x from %i\n", baudio->mixflags, */
/*(int) (value * CONTROLLER_RANGE)); */
		return(0);
	}

	/*
	 * Main volume
	 */
	if (operator == 102)
	{
		mix[0].vol = value;
		return(0);
	}

	/*
	 * We have an algorithm parameter which configures the order of IO for the
	 * fm operators, and a set of flags for key tracking per operator.
	 */
	op = operator / 10;
	index = operator - (op * 10);

	switch (index) {
		case 0:
			mix[op].igain = value * 4;
			break;
		case 1:
			/*
			 * Key tracking (ie, may function as LFO).
			 */
			if (value != 0)
				mix[op].flags &= ~DX_KEY;
			else
				mix[op].flags |= DX_KEY;
			break;
		case 2:
			mix[op].pan = value;
			break;
		case 3:
			/*
			 * Configure cont controller 1 to adust IGAIN.
			 */
			if (value == 0)
				mix[op].flags &= ~DX_IGC;
			else
				mix[op].flags |= DX_IGC;
			break;
	}
	return(0);
}

void
dxOpOne(int ind, float *kbuf, float *opbuf, float *obuf, int count,
dxmix *mix, audioMain *am, Baudio *ba, bristolVoice *v)
{
	float igain = mix[ind].igain;
	int flags = v->flags & BRISTOL_KEYDONE;

	bufmerge(opbuf, 1.0, modbuf, 0.0, count);
/*printf("dxOpOne(%i)\n", ind); */

	if (mix[ind].flags & DX_IGC)
		igain *= ba->contcontroller[1];
		
	/*
	 * Check on our mods,
	 */
	if (mix[ind].flags & DX_KEY)
		bufmerge(kbuf, 1.0, modbuf, igain, count);
	else
		bufmerge(lfobuf, 1.0, modbuf, igain, count);

	/*
	 * Push in our pointers. Note that if we move the pointers into the
	 * params structure we could avoid doing this for each op.
	 */
	am->palette[(*ba->sound[0]).index]->specs->io[0].buf = modbuf;
	am->palette[(*ba->sound[0]).index]->specs->io[1].buf = obuf;

	/*
	 * Call the operator.
	 */
	(*ba->sound[ind]).operate(
		(am->palette)[9],
		v,
		(*ba->sound[ind]).param,
		v->locals[v->index][ind]);

	v->flags = v->flags & flags? v->flags: v->flags & ~BRISTOL_KEYDONE;
}

int
DXalgoN(audioMain *am, register Baudio *ba, bristolVoice *voice, float *kb)
{
	register int i, sc = am->samplecount;
	register float *bufptr = lfobuf;
	dxmix *mix = (dxmix *) ba->mixlocals;

	/*
	 * Fill the wavetable with the correct note value
	 */
	fillFreqTable(ba, voice, kb, sc, 1);
	/*
	 * Build a LFO frequency table
	 */
	for (i = 0; i < sc ; i+=8)
	{
		*bufptr++ = 0.001 + ba->contcontroller[1] / 2;
		*bufptr++ = 0.001 + ba->contcontroller[1] / 2;
		*bufptr++ = 0.001 + ba->contcontroller[1] / 2;
		*bufptr++ = 0.001 + ba->contcontroller[1] / 2;
		*bufptr++ = 0.001 + ba->contcontroller[1] / 2;
		*bufptr++ = 0.001 + ba->contcontroller[1] / 2;
		*bufptr++ = 0.001 + ba->contcontroller[1] / 2;
		*bufptr++ = 0.001 + ba->contcontroller[1] / 2;
	}

	bzero(op0buf, am->segmentsize);
	bzero(op1buf, am->segmentsize);
	bzero(op2buf, am->segmentsize);

	switch (ba->mixflags & DX_ALGO_M) {
		case 0:
			/*
			 * Op0 feeds op1, op2 feeds op3, op4 feeds op5.
			 */
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(3, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[3].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[3].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op1buf, am->segmentsize);
			bzero(op2buf, am->segmentsize);
			dxOpOne(1, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op1buf, am->segmentsize);
			bzero(op2buf, am->segmentsize);
			dxOpOne(2, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(5, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 1:
			/*
			 * Op0 feeds op1 into op2, then op3 feeds op4 into op5.
			 */
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(2, kb, op2buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op0buf, am->segmentsize);
			bzero(op1buf, am->segmentsize);
			bzero(op2buf, am->segmentsize);
			dxOpOne(3, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(5, kb, op2buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 2:
		case 19:
		case 20:
			/*
			 * Op0 feeds op1 and op2, then op3 feeds op4 and op5.
			 */
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(2, kb, op1buf, op0buf, sc, mix, am, ba, voice);

			bufmerge(op2buf, mix[0].vol * mix[1].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[1].pan), ba->leftbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op0buf, am->segmentsize);
			bzero(op1buf, am->segmentsize);
			bzero(op2buf, am->segmentsize);
			dxOpOne(3, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(5, kb, op1buf, op0buf, sc, mix, am, ba, voice);

			bufmerge(op2buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 3:
			/*
			 * Op0 and op1 feed op2, then op3 and op4 feed op5.
			 * These then need a crossover between the L and R channels.
			 */
			bzero(op3buf, am->segmentsize);
			bzero(op4buf, am->segmentsize);
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(2, kb, op1buf, op3buf, sc, mix, am, ba, voice);

			bzero(op0buf, am->segmentsize);
			dxOpOne(3, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(5, kb, op2buf, op4buf, sc, mix, am, ba, voice);

			bufmerge(op3buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);
			bufmerge(op4buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op4buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);
			break;
		case 4:
		case 11: /* Same algo, just 4 and 5 take no input from 1. */
			/*
			 * Op0 feeds op1, op2 into op3. It also feeds op4 and op5 into op6.
			 */
			bzero(op3buf, am->segmentsize);
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(2, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			bzero(op3buf, am->segmentsize);
			dxOpOne(3, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(5, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);
			break;
		case 5:
			/*
			 * op1 feeds op2 into op3, and op4 into op5 and op6.
			 */
			bzero(op3buf, am->segmentsize);
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(2, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			bzero(op3buf, am->segmentsize);
			dxOpOne(3, kb, op1buf, op0buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(5, kb, op0buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);
			break;
		case 6:
			/*
			 * Op0 feeds op4. Op4 feeds op2, op3, op5 and op6.
			 */
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(3, kb, op1buf, op2buf, sc, mix, am, ba, voice);

			bzero(op3buf, am->segmentsize);
			dxOpOne(1, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[1].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[1].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			dxOpOne(2, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			dxOpOne(4, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			dxOpOne(5, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 7:
		case 18:
			/*
			 * Op0 and op1 feed op2, then op3 feeds op4 and op5.
			 */
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(2, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			bzero(op3buf, am->segmentsize);
			dxOpOne(3, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			dxOpOne(5, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 8:
			/*
			 * Op0 and op1 feed op2, and ops 3 to 6 feed eachother.
			 */
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[1].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * (1.0 - mix[1].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op0buf, am->segmentsize);
			bzero(op1buf, am->segmentsize);
			bzero(op3buf, am->segmentsize);
			bzero(op4buf, am->segmentsize);
			dxOpOne(2, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(3, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			dxOpOne(5, kb, op3buf, op4buf, sc, mix, am, ba, voice);
			bufmerge(op4buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op4buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 9:
			/*
			 * Op2 feeds op3 into op4. Then 0, 1 and  op4 feed 5.
			 */
			bzero(op3buf, am->segmentsize);
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op0buf, op1buf, sc, mix, am, ba, voice);

			dxOpOne(2, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(3, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op3buf, op1buf, sc, mix, am, ba, voice);

			dxOpOne(5, kb, op1buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 10:
			/*
			 * Op0 feeds op1, op 3 feed op4, then 4 and 2 feed 5.
			 */
			dxOpOne(0, kb, op1buf, op0buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			bufmerge(op1buf, mix[0].vol * mix[1].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op1buf, mix[0].vol * (1.0 - mix[1].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			bzero(op4buf, am->segmentsize);
			dxOpOne(2, kb, op3buf, op4buf, sc, mix, am, ba, voice);

			dxOpOne(3, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op3buf, op4buf, sc, mix, am, ba, voice);

			dxOpOne(5, kb, op4buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		/*case 11: // offloaded into algo 4 - good as the same. */
		case 12:
			/*
			 * Op0 feeds op1, and 2, 3 and 4 feed op 5.
			 */
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[1].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * (1.0 - mix[1].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			bzero(op4buf, am->segmentsize);
			dxOpOne(2, kb, op3buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(3, kb, op3buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			dxOpOne(5, kb, op3buf, op4buf, sc, mix, am, ba, voice);
			bufmerge(op4buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op4buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 13:
			bzero(op3buf, am->segmentsize);
			dxOpOne(0, kb, op3buf, op0buf, sc, mix, am, ba, voice);

			dxOpOne(1, kb, op3buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(2, kb, op1buf, op0buf, sc, mix, am, ba, voice);

			dxOpOne(3, kb, op3buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op2buf, op0buf, sc, mix, am, ba, voice);

			dxOpOne(5, kb, op0buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op4buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op4buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 14:
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(2, kb, op2buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op4buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op0buf, am->segmentsize);
			bzero(op1buf, am->segmentsize);
			dxOpOne(3, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op1buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op4buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op0buf, am->segmentsize);
			bzero(op1buf, am->segmentsize);
			dxOpOne(5, kb, op1buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op4buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;

		case 15:
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op0buf, sc, mix, am, ba, voice);

			bzero(op2buf, am->segmentsize);
			dxOpOne(2, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(3, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[3].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[3].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(4, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(5, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 16:
			/*
			 * Op0 feeds op1, and 2, 3 and 4 feed op 5.
			 */
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[1].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * (1.0 - mix[1].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			dxOpOne(2, kb, op3buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(3, kb, op3buf, op2buf, sc, mix, am, ba, voice);
			dxOpOne(4, kb, op3buf, op2buf, sc, mix, am, ba, voice);

			dxOpOne(5, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 17:
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			dxOpOne(1, kb, op1buf, op0buf, sc, mix, am, ba, voice);
			bufmerge(op0buf, mix[0].vol * mix[1].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op0buf, mix[0].vol * (1.0 - mix[1].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			dxOpOne(2, kb, op3buf, op2buf, sc, mix, am, ba, voice);

			dxOpOne(3, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[3].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[3].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			dxOpOne(4, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op3buf, am->segmentsize);
			dxOpOne(5, kb, op2buf, op3buf, sc, mix, am, ba, voice);
			bufmerge(op3buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op3buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 21:
		case 22:
			dxOpOne(0, kb, op1buf, op0buf, sc, mix, am, ba, voice);

			dxOpOne(1, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			bufmerge(op1buf, mix[0].vol * mix[1].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op1buf, mix[0].vol * (1.0 - mix[1].pan), ba->leftbuf, 1.0,
				am->samplecount);

			dxOpOne(2, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(3, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[3].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[3].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(4, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(5, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
		case 23:
			dxOpOne(0, kb, op0buf, op1buf, sc, mix, am, ba, voice);
			bufmerge(op1buf, mix[0].vol * mix[0].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op1buf, mix[0].vol * (1.0 - mix[0].pan), ba->leftbuf, 1.0,
				am->samplecount);

			dxOpOne(1, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[1].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[1].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(2, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[2].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[2].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(3, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[3].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[3].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(4, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[4].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[4].pan), ba->leftbuf, 1.0,
				am->samplecount);

			bzero(op2buf, am->segmentsize);
			dxOpOne(5, kb, op0buf, op2buf, sc, mix, am, ba, voice);
			bufmerge(op2buf, mix[0].vol * mix[5].pan, ba->rightbuf, 1.0,
				am->samplecount);
			bufmerge(op2buf, mix[0].vol * (1.0 - mix[5].pan), ba->leftbuf, 1.0,
				am->samplecount);

			break;
	}

	return(0);
}

int
DXalgoNpostops(audioMain *am, Baudio *ba, bristolVoice *v, register float *s)
{
	bufmerge(ba->leftbuf, 0.0, ba->leftbuf, 8.0, am->samplecount);
	bufmerge(ba->rightbuf, 0.0, ba->rightbuf, 8.0, am->samplecount);

	return(0);
}

int
destroyOneDXVoice(audioMain *audiomain, Baudio *baudio)
{
	printf("destroy DX sound\n");
	return(0);
	bristolfree(lfobuf);
	bristolfree(op0buf);
	bristolfree(op1buf);
	bristolfree(op2buf);
}

int
bristolDXInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one DX sound\n");

	baudio->soundCount = 6; /* Number of operators in this DX voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * DX oscillators.
	 */
	initSoundAlgo(9, 0, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 1, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 2, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 3, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 4, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 5, baudio, audiomain, baudio->sound);

	baudio->param = DXGlobalController;
	baudio->destroy = destroyOneDXVoice;
	baudio->operate = DXalgoN;
	baudio->postops = DXalgoNpostops;

	if (lfobuf == 0)
		lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op0buf == 0)
		op0buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op1buf == 0)
		op1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op2buf == 0)
		op2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op3buf == 0)
		op3buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op4buf == 0)
		op4buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (modbuf == 0)
		modbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(12 * sizeof(dxmix));
	baudio->mixflags |= BRISTOL_STEREO;
	return(0);
}

int
bristolRhodesInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one Rhodes sound\n");

	baudio->soundCount = 6; /* Number of operators in this DX voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * DX oscillators.
	 */
	initSoundAlgo(9, 0, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 1, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 2, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 3, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 4, baudio, audiomain, baudio->sound);
	initSoundAlgo(9, 5, baudio, audiomain, baudio->sound);

	/*
	 * Put in a vibrachorus on our effects list.
	 */
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);

	baudio->param = DXGlobalController;
	baudio->destroy = destroyOneDXVoice;
	baudio->operate = DXalgoN;
	baudio->postops = DXalgoNpostops;

	if (lfobuf == 0)
		lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op0buf == 0)
		op0buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op1buf == 0)
		op1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op2buf == 0)
		op2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op3buf == 0)
		op3buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (op4buf == 0)
		op4buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (modbuf == 0)
		modbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(12 * sizeof(dxmix));
	baudio->mixflags |= BRISTOL_STEREO;
	return(0);
}

