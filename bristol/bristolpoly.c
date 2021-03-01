
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
/* Korg mono poly agorithm */
/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolpoly.h"

#define CONTROL_MG1 1
/*
 * This should be zero, we might change it.
 */
#define CONTROL_BEND 2

#define DEST_FLT 0
#define DEST_PITCH 1
#define DEST_VCO 2

/*
 * Buffer requirements.
 */
static float *freqbuf = (float *) NULL;
static float *scratchbuf = (float *) NULL;
static float *osc0buf = (float *) NULL;
static float *osc1buf = (float *) NULL;
static float *osc2buf = (float *) NULL;
static float *osc3buf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *mg1buf = (float *) NULL;
static float *mg2buf = (float *) NULL;

int
polyController(Baudio *baudio, u_char operator, u_char controller, float value)
{
	int ivalue = value * CONTROLLER_RANGE;
	pmods *mods = (pmods *) baudio->mixlocals;

#ifdef DEBUG
	printf("bristolPolyControl(%i, %i, %f)\n", operator, controller, value);
#endif

	if (operator != 126)
		return(0);

	switch (controller) {
		case 0:
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 1:
			break;
		case 2: /* Global tune */
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			/*
			 * We need to do this as a part of the synth, the usual call to
			 * alterAllNotes(baudio) would not work here.
			 */
			mods->keydata[0].oFreq
				= mods->keydata[0].cFreq
				= mods->keydata[0].dFreq
				= baudio->ctab[mods->keydata[0].key].step - mods->detune;
			mods->keydata[1].oFreq
				= mods->keydata[1].cFreq
				= mods->keydata[1].dFreq
				= baudio->ctab[mods->keydata[1].key].step + mods->detune;
			mods->keydata[2].oFreq
				= mods->keydata[2].cFreq
				= mods->keydata[2].dFreq
				= baudio->ctab[mods->keydata[2].key].step - mods->detune * 2;
			mods->keydata[3].oFreq
				= mods->keydata[3].cFreq
				= mods->keydata[3].dFreq
				= baudio->ctab[mods->keydata[3].key].step + mods->detune * 2;
			break;
		case 3:
			baudio->mixflags &= ~0x03;
			baudio->mixflags |= ivalue;
			break;
		case 4: /* PWM source */
			baudio->mixflags &= ~PWM_MASK;
			switch (ivalue)
			{
				case 0:
					baudio->mixflags |= PWM_MG2;
					break;
				case 1:
					baudio->mixflags |= PWM_MG1;
					break;
				case 2:
					baudio->mixflags |= PWM_ENV;
					break;
			}
			break;
		case 5: /* PWM depth */
			((pmods *) baudio->mixlocals)->pwmdepth = value;
			break;
		case 6: /* General mods */
			mods->xmod = value;
			break;
		case 7: /* General mods */
			mods->mfreq = value;
			break;
		case 8: /* General mods */
			if (ivalue == 0)
				baudio->mixflags |= P_MOD_ENV;
			else
				baudio->mixflags &= ~P_MOD_ENV;
			break;
		case 9: /* General mods */
			baudio->mixflags &= ~PWM_MASK;
			switch (ivalue)
			{
				case 0:
					baudio->mixflags |= P_MOD_XMOD;
					baudio->mixflags &= ~P_MOD_SYNC;
					break;
				case 1:
					baudio->mixflags |= P_MOD_XMOD|P_MOD_SYNC;
					break;
				case 2:
					baudio->mixflags &= ~P_MOD_XMOD;
					baudio->mixflags |= P_MOD_SYNC;
					break;
			}
			break;
		case 10: /* General mods */
			if (ivalue == 0)
				baudio->mixflags |= P_MOD_SING;
			else
				baudio->mixflags &= ~P_MOD_SING;
			break;
		case 11: /* Detune */
			mods->detune = value / 4;

			mods->keydata[0].dFreq = mods->keydata[0].oFreq - mods->detune;
			mods->keydata[1].dFreq = mods->keydata[1].oFreq + mods->detune;
			mods->keydata[2].dFreq = mods->keydata[2].oFreq - mods->detune * 2;
			mods->keydata[3].dFreq = mods->keydata[3].oFreq + mods->detune * 2;
			break;
		case 12: /* Trigger */
			if (ivalue == 0)
				baudio->mixflags &= ~P_MULTI;
			else
				baudio->mixflags |= P_MULTI;
			break;
		case 13: /* Damp */
			if (ivalue == 0)
				baudio->mixflags |= P_DAMP;
			else
				baudio->mixflags &= ~P_DAMP;
			break;
		case 14: /* Effects */
			if (ivalue == 0)
				baudio->mixflags &= ~P_EFFECTS;
			else
				baudio->mixflags |= P_EFFECTS;
			break;
		case 15: /* MG1/2 */
			((pmods *) baudio->mixlocals)->bendintensity = value;
			break;
		case 16: /* MG1/2 */
			((pmods *) baudio->mixlocals)->benddst = ivalue;
			break;
		case 17: /* MG1/2 */
			((pmods *) baudio->mixlocals)->mg1intensity = value;
			break;
		case 18: /* MG1/2 */
			((pmods *) baudio->mixlocals)->mg1dst = ivalue;
			break;
		case 19:
			/*
			 * MONO = Unison request
			 */
			if (ivalue == 0)
				baudio->mixflags &= ~P_UNISON;
			else
				baudio->mixflags |= P_UNISON;
			baudio->mixflags &= ~P_SHARE;
			break;
		case 20:
			/*
			 * Poly
			 */
			baudio->mixflags &= ~P_UNISON;
			baudio->mixflags &= ~P_SHARE;
			break;
		case 21:
			/*
			 * Share mode
			 */
			baudio->mixflags &= ~P_UNISON;
			baudio->mixflags |= P_SHARE;
			break;
		case 22:
		case 23:
		case 24:
		case 25:
			/*
			 * PWM application
			 */
			if (ivalue == 0)
				baudio->mixflags &= ~(P_PWM_1 << (controller - 22));
			else
				baudio->mixflags |= (P_PWM_1 << (controller - 22));
			break;
		case 26:
			((pmods *) baudio->mixlocals)->arpegtotal =
				10 + (1.0 - value) * 300;
			break;
		case 27:
			/*
			 * Chord will operate in different modes. There will be MONO chord
			 * which will accept 4 notes and hold them, and transpose them
			 * for each keypress. In Poly mode will arpeggiate between the
			 * notes for cheesey 80's style arpeg and transpose the arpeggiation
			 * with new notes. In share mode will just rotate through the notes
			 * accepting each onto the next in the sequence.
			 */
			if (ivalue == 0) {
				baudio->mixflags &= ~P_CHORD;
				baudio->mixflags |= P_CHORD_OFF;
			} else {
				baudio->mixflags |= P_CHORD;
			}
			((pmods *) baudio->mixlocals)->firstchord = -1;
			break;
		case 28:
			/*
			 * 2 = up
			 * 1 = updown
			 * 0 = down
			 */
			mods->arpegdir = ivalue;
			break;
		case 30:
			mods->osc0gain = value;
			break;
		case 31:
			mods->osc1gain = value;
			break;
		case 32:
			mods->osc2gain = value;
			break;
		case 33:
			mods->osc3gain = value;
			break;
	}
	return(0);
}

/*
 * Diverse cases to cater for. We nee to watch out to clear noteoff flags
 * for restrikes.
 */
static void
polyNoteOn(audioMain *audiomain, Baudio *baudio, bristolVoice *voice)
{
	pmods *mods = ((pmods *) baudio->mixlocals);
	int i = 0;
/*printf("key %i on, %x\n", voice->key.key, baudio->mixflags); */

	if (baudio->mixflags & P_CHORD)
	{
		/*
		 * Chord will fill the 4 Oscillators, and then transpose all of
		 * them according to the keys when further notes are pressed,
		 * arpeggiate between them all with/without transpose.
		 *
		 * Find out how many keys are on
		 */
		if (mods->keydata[0].key != -1) i++;
		if (mods->keydata[1].key != -1) i++;
		if (mods->keydata[2].key != -1) i++;
		if (mods->keydata[3].key != -1) i++;

		/*
		 * If this is the first note, keep it for eventual transpose.
		 */
		if (i == 0) {
			mods->firstchord = voice->key.key;
		} else if (i == 4) {
			int delta;

			mods->lastchord = voice->key.key;
/*printf("all on %i %i\n", mods->lastchord, voice->key.key); */
			/*
			 * Do transpose stuff, etc. Three modes of operation:
			 *
			 * MONO: accept 4 notes and hold them, and transpose them.
			 * SHARE: arpeggiate between the notes for cheesey 80's style arpeg.
			 * POLY: rotate through the notes accepting each.
			 */
			if (baudio->mixflags & (P_UNISON|P_SHARE)) {
				/*
				 * MONO:
				 */
				delta = voice->key.key - mods->keydata[0].key;
				mods->firstchord = voice->key.key;

				/*
				 * Apply this delta to each osc, this is easy for the first
				 * key:
				 */
				mods->keydata[0].key = voice->key.key;
				mods->keydata[0].oFreq = voice->dFreq;
				mods->keydata[0].dFreq = voice->dFreq - mods->detune;
				mods->keydata[0].cFreq = voice->cFreq;
				mods->keydata[0].cFreqstep = voice->cFreqstep;
				mods->keydata[0].cFreqmult = voice->cFreqmult;
				mods->keydata[0].lastkey = voice->lastkey;

				mods->keydata[1].key += delta;
				mods->keydata[1].oFreq = voice->dFreq;
				mods->keydata[1].dFreq = mods->keydata[1].cFreq
					= voice->baudio->ctab[mods->keydata[1].key].step + mods->detune;
				mods->keydata[2].key += delta;
				mods->keydata[2].oFreq = voice->dFreq;
				mods->keydata[2].dFreq = mods->keydata[2].cFreq
					= voice->baudio->ctab[mods->keydata[2].key].step
						- mods->detune * 2;
				mods->keydata[3].key += delta;
				mods->keydata[3].oFreq = voice->dFreq;
				mods->keydata[3].dFreq = mods->keydata[3].cFreq
					= voice->baudio->ctab[mods->keydata[3].key].step
						+ mods->detune * 2;

				/*
				 * KeyOn is managed now by the arpeggiator
				 */
				if (baudio->mixflags & P_SHARE)
				{
					if (baudio->mixflags & P_REON)
					{
						baudio->mixflags &= ~P_REON;
						voice->flags &= ~BRISTOL_KEYREON;
						voice->flags |= BRISTOL_KEYON;
						mods->arpegcount = 0;
						mods->arpegstep = 0;
					} else
						voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);
				}

				return;
			} else {
				/*
				 * POLY:
				 */
				if ((delta = ++mods->nextosc) > 3)
					delta = mods->nextosc = 0;

				mods->keydata[delta].oFreq = voice->cFreq;
				mods->keydata[delta].cFreq = voice->cFreq;
				mods->keydata[delta].dFreq = voice->dFreq
				 + (delta - 2) * mods->detune;
				mods->keydata[delta].cFreqstep = voice->cFreqstep;
				mods->keydata[delta].cFreqmult = voice->cFreqmult;
				mods->keydata[delta].key = voice->key.key;
				mods->keydata[delta].lastkey = voice->lastkey;

				/*
				 * KeyOn is managed now by the arpeggiator
				 */
				if (baudio->mixflags & P_REON)
				{
					baudio->mixflags &= ~P_REON;
					voice->flags &= ~BRISTOL_KEYREON;
					voice->flags |= BRISTOL_KEYON;
					mods->arpegcount = 0;
					mods->arpegstep = 0;
				} else
					voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);

				return;
			}

			/*
			 * If this is after the first voice then clear the trigger if nec.
			 */
			if (((baudio->mixflags & P_MULTI) == 0) && (i > 0))
				voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);

			return;
		}

		mods->keydata[i].oFreq = voice->cFreq;
		mods->keydata[i].cFreq = voice->cFreq;
		mods->keydata[i].dFreq = voice->dFreq
			+ (i - 2) * mods->detune;
		mods->keydata[i].cFreqstep = voice->cFreqstep;
		mods->keydata[i].cFreqmult = voice->cFreqmult;
		mods->keydata[i].key = voice->key.key;
		mods->keydata[i].lastkey = voice->lastkey;

		/*
		 * If this is after the first voice then clear the trigger if nec.
		 */
		if (((baudio->mixflags & P_MULTI) == 0) && (i > 1))
			voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);

		return;
	}

	if (baudio->mixflags & P_UNISON)
	{
		/*
		 * All osc to the desired note.
		 */
		mods->keydata[0].cFreq = mods->keydata[1].cFreq
			= mods->keydata[2].cFreq = mods->keydata[3].cFreq
			= voice->cFreq;
		mods->keydata[0].oFreq = mods->keydata[1].oFreq =
		mods->keydata[2].oFreq = mods->keydata[3].oFreq =
		mods->keydata[0].dFreq = mods->keydata[1].dFreq
			= mods->keydata[2].dFreq = mods->keydata[3].dFreq
			= voice->dFreq;
		mods->keydata[0].cFreqstep = mods->keydata[1].cFreqstep
			= mods->keydata[2].cFreqstep = mods->keydata[3].cFreqstep
			= voice->cFreqstep;
		mods->keydata[0].cFreqmult = mods->keydata[1].cFreqmult
			= mods->keydata[2].cFreqmult = mods->keydata[3].cFreqmult
			= voice->cFreqmult;
		mods->keydata[0].key = mods->keydata[1].key
			= mods->keydata[2].key = mods->keydata[3].key
			= voice->key.key;
		mods->keydata[0].lastkey = mods->keydata[1].lastkey
			= mods->keydata[2].lastkey = mods->keydata[3].lastkey
			= voice->lastkey;

		mods->keydata[0].dFreq += mods->detune;
		mods->keydata[1].dFreq -= mods->detune;
		mods->keydata[2].dFreq += mods->detune * 2;
		mods->keydata[3].dFreq -= mods->detune * 2;
	} else if (baudio->mixflags & P_SHARE) {
		/*
		 * Find out how many keys are on
		 */
		if (mods->keydata[0].key != -1) i++;
		if (mods->keydata[1].key != -1) i++;
		if (mods->keydata[2].key != -1) i++;
		if (mods->keydata[3].key != -1) i++;
/*printf("keys: %i\n", i); */

		/*
		 * Sharing will monitor the number of notes:
		 *	1 = UNISON
		 * 	2 = OSC1/2 and OSC3/4
		 *	else poly.
		 */
		if (i == 0)
		{
/*printf("unison\n"); */
			/*
			 * Do unison mode
			 */
			mods->keydata[0].cFreq = mods->keydata[1].cFreq
				= mods->keydata[2].cFreq = mods->keydata[3].cFreq
				= voice->cFreq;
			mods->keydata[0].oFreq = mods->keydata[1].oFreq =
			mods->keydata[2].oFreq = mods->keydata[3].oFreq =
			mods->keydata[0].dFreq = mods->keydata[1].dFreq
				= mods->keydata[2].dFreq = mods->keydata[3].dFreq
				= voice->dFreq;
			mods->keydata[0].cFreqstep = mods->keydata[1].cFreqstep
				= mods->keydata[2].cFreqstep = mods->keydata[3].cFreqstep
				= voice->cFreqstep;
			mods->keydata[0].cFreqmult = mods->keydata[1].cFreqmult
				= mods->keydata[2].cFreqmult = mods->keydata[3].cFreqmult
				= voice->cFreqmult;
			mods->keydata[0].key = mods->keydata[1].key
				= mods->keydata[2].key = mods->keydata[3].key
				= voice->key.key;
			mods->keydata[0].lastkey = mods->keydata[1].lastkey
				= mods->keydata[2].lastkey = mods->keydata[3].lastkey
				= voice->lastkey;
			voice->flags |= BRISTOL_VCO_MASK;

			mods->keydata[0].dFreq += mods->detune;
			mods->keydata[1].dFreq -= mods->detune;
			mods->keydata[2].dFreq += mods->detune * 2;
			mods->keydata[3].dFreq -= mods->detune * 2;

			return;
		} else if (i == 4) {
			/*
			 * There are 3 cases where we have four notes.
			 *
			 *	They are all the same - this is second key
			 *		assign this note to 1 and 3.
			 *	0 = 1 and 2 = 3, we have two notes keyed.
			 *		assign this to 1, clear 3.
			 *  all are different.
			 *		New key, rotate through them.
			 */
			if (mods->keydata[0].key == mods->keydata[2].key)
			{
/*printf("was unison, now dual\n"); */
				/*
				 * Was unison, steal 2 and 3
				 */
				mods->keydata[2].cFreq = mods->keydata[3].cFreq
					= voice->cFreq;
				mods->keydata[2].oFreq =
				mods->keydata[2].dFreq = mods->keydata[3].dFreq
					= voice->dFreq;
				mods->keydata[2].dFreq -= mods->detune * 2;
				mods->keydata[3].dFreq += mods->detune * 2;
					
				mods->keydata[2].cFreqstep = mods->keydata[3].cFreqstep
					= voice->cFreqstep;
				mods->keydata[2].cFreqmult = mods->keydata[3].cFreqmult
					= voice->cFreqmult;
				mods->keydata[2].key = mods->keydata[3].key
					= voice->key.key;
				mods->keydata[2].lastkey = mods->keydata[3].lastkey
					= voice->lastkey;
			} else if (mods->keydata[0].key == mods->keydata[1].key) {
/*printf("was dual, now three\n"); */
				/*
				 * Was dual, steal 1 free 3
				 */
				mods->keydata[1].cFreq = voice->cFreq;
				mods->keydata[1].oFreq = voice->dFreq;
				mods->keydata[1].dFreq = voice->dFreq + mods->detune;
				mods->keydata[1].cFreqstep = voice->cFreqstep;
				mods->keydata[1].cFreqmult = voice->cFreqmult;
				mods->keydata[1].key = voice->key.key;
				mods->keydata[1].lastkey = voice->lastkey;

				mods->keydata[3].key = -1;
			} else {
				/*
				 * Was all, rotate the steal.
				 */
				i = mods->nextosc;
/*printf("was loaded, now %i\n", i); */

				mods->keydata[i].cFreq = voice->cFreq;
				mods->keydata[i].oFreq = voice->dFreq;
				mods->keydata[i].dFreq = voice->dFreq
				 	+ (i - 2) * mods->detune;
				mods->keydata[i].cFreqstep = voice->cFreqstep;
				mods->keydata[i].cFreqmult = voice->cFreqmult;
				mods->keydata[i].key = voice->key.key;
				mods->keydata[i].lastkey = voice->lastkey;

				if (++mods->nextosc > 3)
					mods->nextosc = 0;
			}
		} else {
			i = 3;
			/*
			 * In this case we had one free key. Use it.
			 */
			if (mods->keydata[0].key == -1)
				i = 0;
			else if (mods->keydata[1].key == -1)
				i = 1;
			else if (mods->keydata[2].key == -1)
				i = 2;

			mods->keydata[i].cFreq = voice->cFreq;
			mods->keydata[i].oFreq = voice->dFreq;
			mods->keydata[i].dFreq = voice->dFreq
				 	+ (i - 2) * mods->detune;
			mods->keydata[i].cFreqstep = voice->cFreqstep;
			mods->keydata[i].cFreqmult = voice->cFreqmult;
			mods->keydata[i].key = voice->key.key;
			mods->keydata[i].lastkey = voice->lastkey;
		}
	} else {
		int osc = 0;
		/*
		 * Poly mode. Find a free osc and assign it. Else take the next
		 * in rotation.
		 */
		if ((mods->keydata[0].key != -1)
			&& (mods->keydata[0].key != voice->key.key))
		{
			if ((mods->keydata[1].key != -1)
				&& (mods->keydata[0].key != voice->key.key))
			{
				if ((mods->keydata[2].key != -1)
					&& (mods->keydata[0].key != voice->key.key))
				{
					if ((mods->keydata[3].key != -1)
						&& (mods->keydata[0].key != voice->key.key))
					{
						/*
						 * Bummer, all taken. Go for the next free.
						 */
						if ((osc = mods->nextosc++) >= 3)
							mods->nextosc = 0;
					} else osc = 3;
				} else osc = 2;
			} else osc = 1;
		}
		mods->keydata[osc].cFreq = voice->cFreq;
		mods->keydata[osc].oFreq = voice->dFreq;
		mods->keydata[osc].dFreq = voice->dFreq
		 	+ (osc - 2) * mods->detune;
		mods->keydata[osc].cFreqstep = voice->cFreqstep;
		mods->keydata[osc].cFreqmult = voice->cFreqmult;
		mods->keydata[osc].key = voice->key.key;
		mods->keydata[osc].lastkey = voice->lastkey;

		voice->flags |= (BRISTOL_K_VCO1 >> osc);
	}

	/*
	 * If we are not multi trigger then clear this flag
	 */
	if (((baudio->mixflags & P_MULTI) == 0)
		&& (baudio->lvoices > 0))
		voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);
}

static void
polyNoteOff(audioMain *audiomain, Baudio *baudio, bristolVoice *voice)
{
	pmods *mods = ((pmods *) baudio->mixlocals);
	int i = 0;

#warning issues here if we have two midi events between frames
	/*
	 * Find out how many keys are on
	 */
	if (mods->keydata[0].key != -1) i++;
	if (mods->keydata[1].key != -1) i++;
	if (mods->keydata[2].key != -1) i++;
	if (mods->keydata[3].key != -1) i++;

	/*
	 * In chord we never turn off if we were in share mode. We only turn off
	 * in other modes after we have loaded all the oscillators.
	 */
	if (baudio->mixflags & P_CHORD)
	{
		if ((baudio->mixflags & (P_SHARE|P_MULTI)) == 0)
		{
			voice->flags &= ~BRISTOL_KEYOFF;
			return;
		}

		/*
		 * So now we have 4 keys loaded. We should clear the keyoff if this
		 * was not the most recent key.
		 */
		if (i < 4)
			voice->flags &= ~BRISTOL_KEYOFF;
		else if (mods->lastchord != voice->key.key)
		{
			voice->flags &= ~BRISTOL_KEYOFF;
/*printf("note off, key: %i, last %i, chord %i, %i\n", */
/*voice->key.key, voice->lastkey, mods->lastchord, i); */
		}

		return;
	}

	if (baudio->mixflags & P_SHARE)
	{
		int a = 2, b = 3;
		/*
		 * We need to free this key if we find it, see how many are left,
		 * redistributing notes if required.
		 */
		if (i == 4) {
			/*
			 * There are 3 cases of having 4 notes on
			 *	They are the same - all off.
			 *	1 = 2 then go unison.
			 *	else turn this note off, make it the next available
			 */
			if (mods->keydata[0].key == mods->keydata[2].key) {
/*printf("was unison, all off\n"); */
				/*
				 * All off, do nothing.
				 */
				return;
			}

			if (mods->keydata[0].key == mods->keydata[1].key) {
/*printf("was dual, go unison\n"); */
				/*
				 * Dual, go unison. Find out which key went off, it will not
				 * be the unison note.
				 */
				if (mods->keydata[0].key == voice->key.key)
					i = 2;
				else if (mods->keydata[2].key == voice->key.key)
					i = 0;
				else
					/*
					 * Was an already dropped note
					 */
					return;

				mods->keydata[0].cFreq = mods->keydata[1].cFreq
					= mods->keydata[2].cFreq = mods->keydata[3].cFreq
						= mods->keydata[i].cFreq;
				mods->keydata[0].dFreq = mods->keydata[1].dFreq
					= mods->keydata[2].dFreq = mods->keydata[3].dFreq
						= mods->keydata[i].dFreq;
				mods->keydata[0].cFreqstep = mods->keydata[1].cFreqstep
					= mods->keydata[2].cFreqstep = mods->keydata[3].cFreqstep
						= mods->keydata[i].cFreqstep;
				mods->keydata[0].cFreqmult = mods->keydata[1].cFreqmult
					= mods->keydata[2].cFreqmult = mods->keydata[3].cFreqmult
						= mods->keydata[i].cFreqmult;
				mods->keydata[0].key = mods->keydata[1].key
					= mods->keydata[2].key = mods->keydata[3].key
						= mods->keydata[i].key;
				mods->keydata[0].lastkey = mods->keydata[1].lastkey
					= mods->keydata[2].lastkey = mods->keydata[3].lastkey
						= mods->keydata[i].lastkey;
				voice->flags &= ~BRISTOL_KEYOFF;
				return;
			}

/*printf("free key\n"); */
			/*
			 * Find the key, turn it off, becomes the next.
			 */
			if (mods->keydata[0].key == voice->key.key) {
				mods->keydata[0].key = -1;
				mods->nextosc = 0;
			} else if (mods->keydata[1].key == voice->key.key) {
				mods->keydata[1].key = -1;
				mods->nextosc = 1;
			} else if (mods->keydata[2].key == voice->key.key) {
				mods->keydata[2].key = -1;
				mods->nextosc = 2;
			} else if (mods->keydata[3].key == voice->key.key) {
				mods->keydata[3].key = -1;
				mods->nextosc = 3;
			}

			voice->flags &= ~BRISTOL_KEYOFF;
			return;
		}

/*printf("three keys\n"); */
		/*
		 * We have three keys on, need to find out which one went off and
		 * assign the dual to the others.
		 */
		if (mods->keydata[0].key == voice->key.key) {

			if (mods->keydata[1].key == -1) {
				a = 2;
				b = 3;
			} else if (mods->keydata[2].key == -1) {
				a = 1;
				b = 3;
			} else {
				a = 1;
				b = 2;
			}

		} else if (mods->keydata[1].key == voice->key.key) {

			if (mods->keydata[0].key == -1) {
				a = 2;
				b = 3;
			} else if (mods->keydata[2].key == -1) {
				a = 0;
				b = 3;
			} else {
				a = 0;
				b = 2;
			}

		} else if (mods->keydata[2].key == voice->key.key) {

			if (mods->keydata[0].key == -1) {
				a = 1;
				b = 3;
			} else if (mods->keydata[1].key == -1) {
				a = 0;
				b = 3;
			} else {
				a = 0;
				b = 1;
			}

		} else if (mods->keydata[3].key == voice->key.key) {

			if (mods->keydata[0].key == -1) {
				a = 1;
				b = 2;
			} else if (mods->keydata[1].key == -1) {
				a = 0;
				b = 2;
			} else {
				a = 0;
				b = 1;
			}

		}

		mods->keydata[0].cFreq = mods->keydata[1].cFreq
			= mods->keydata[a].cFreq;
		mods->keydata[0].dFreq = mods->keydata[1].dFreq
			= mods->keydata[a].dFreq;
		mods->keydata[0].cFreqstep = mods->keydata[1].cFreqstep
			= mods->keydata[a].cFreqstep;
		mods->keydata[0].cFreqmult = mods->keydata[1].cFreqmult
			= mods->keydata[a].cFreqmult;
		mods->keydata[0].key = mods->keydata[1].key
			= mods->keydata[a].key;
		mods->keydata[0].lastkey = mods->keydata[0].lastkey
			= mods->keydata[1].lastkey;

		mods->keydata[2].cFreq = mods->keydata[3].cFreq
			= mods->keydata[b].cFreq;
		mods->keydata[2].dFreq = mods->keydata[3].dFreq
			= mods->keydata[b].dFreq;
		mods->keydata[2].cFreqstep = mods->keydata[3].cFreqstep
			= mods->keydata[b].cFreqstep;
		mods->keydata[2].cFreqmult = mods->keydata[3].cFreqmult
			= mods->keydata[b].cFreqmult;
		mods->keydata[2].key = mods->keydata[3].key
			= mods->keydata[b].key;
		mods->keydata[2].lastkey = mods->keydata[3].lastkey
			= mods->keydata[b].lastkey;

		voice->flags &= ~BRISTOL_KEYOFF;
		return;
	} else if (baudio->mixflags & P_UNISON) {
		/*
		 * This is the Unison mode. If this is not the current key then
		 * clear the keyoff flag.
		 */
		if (mods->keydata[0].key != voice->key.key)
			voice->flags &= ~BRISTOL_KEYOFF;
	} else {
		/*
		 * If this is the last note let is continue with the key off active.
		 */
		if (i != 1) {
			/*
			 * This is the poly mode. The operation does not work well with 
			 * damping off, the last note should hold.....
			 *
			 * If we are DAMPED then skip the note off:
			 */
			if (mods->keydata[0].key == voice->key.key) {
				if ((baudio->mixflags & P_DAMP) == 0)
					mods->keydata[0].key = -1;
				voice->flags &= ~BRISTOL_K_VCO1;
			}
			if (mods->keydata[1].key == voice->key.key) {
				if ((baudio->mixflags & P_DAMP) == 0)
					mods->keydata[1].key = -1;
				voice->flags &= ~BRISTOL_K_VCO2;
			}
			if (mods->keydata[2].key == voice->key.key) {
				if ((baudio->mixflags & P_DAMP) == 0)
					mods->keydata[2].key = -1;
				voice->flags &= ~BRISTOL_K_VCO3;
			}
			if (mods->keydata[3].key == voice->key.key) {
				if ((baudio->mixflags & P_DAMP) == 0)
					mods->keydata[3].key = -1;
				voice->flags &= ~BRISTOL_K_VCO4;
			}

			/*
			 * If that was not the last key then clear the KEYOFF event
			 */
			if (voice->flags & BRISTOL_VCO_MASK)
				voice->flags &= ~BRISTOL_KEYOFF;
		}
	}
}

/*
 * Preops will do noise, and one oscillator - the LFO.
 */
int
operatePolyPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
#ifdef DEBUG
	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG5)
		printf("operatePolyPreops(%x, %x, %x) %i\n",
			baudio, voice, startbuf, baudio->cvoices);
#endif

	if (((pmods *) baudio->mixlocals)->mg1locals == 0)
	{
		/*
		 * We don't really need to do this, the synth is mono
		 */
		((pmods *) baudio->mixlocals)->mg1locals =
			baudio->locals[voice->index][9];
		((pmods *) baudio->mixlocals)->mg2locals =
			baudio->locals[voice->index][10];
	}

	if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
		polyNoteOn(audiomain, baudio, voice);

	if (voice->flags & BRISTOL_KEYOFF)
		polyNoteOff(audiomain, baudio, voice);

	bristolbzero(osc0buf, audiomain->segmentsize);
	bristolbzero(osc1buf, audiomain->segmentsize);
	bristolbzero(osc2buf, audiomain->segmentsize);
	bristolbzero(osc3buf, audiomain->segmentsize);
	bristolbzero(scratchbuf, audiomain->segmentsize);

/* LFO MG1 */
	/*
	 * We have to watch for the waveform selector:
	 */
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[0].buf
		= baudio->leftbuf;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[1].buf
		= 0;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[2].buf
		= 0;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[3].buf
		= 0;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[4].buf
		= 0;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[5].buf
		= 0;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[6].buf
		= 0;

	switch (baudio->mixflags & 0x03)
	{
		case 0:
			audiomain->palette[(*baudio->sound[9]).index]->specs->io[1].buf
				= mg1buf;
			break;
		case 1:
			audiomain->palette[(*baudio->sound[9]).index]->specs->io[5].buf
				= mg1buf;
			break;
		case 2:
			audiomain->palette[(*baudio->sound[9]).index]->specs->io[6].buf
				= mg1buf;
			break;
		case 3:
			audiomain->palette[(*baudio->sound[9]).index]->specs->io[2].buf
				= mg1buf;
			break;
	}

	(*baudio->sound[9]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[9]).param,
		/*baudio->locals[voice->index][9]); */
		((pmods *) baudio->mixlocals)->mg1locals);
/* MG1 OVER */

/* LFO MG2 */
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[0].buf
		= baudio->leftbuf;
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[1].buf
		= mg2buf;
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[2].buf = 0;
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[3].buf = 0;
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[4].buf = 0;
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[5].buf = 0;
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[6].buf = 0;

	(*baudio->sound[10]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[10]).param,
		/*baudio->locals[voice->index][10]); */
		((pmods *) baudio->mixlocals)->mg2locals);
/* MG2 OVER */
	return(0);
}

int
operateOnePoly(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register int sc = audiomain->samplecount, o_act = 0, oper, kh;
	pmods *mods = ((pmods *) baudio->mixlocals);

/* Arpeggiate */
	/*
	 * We do arpeggiator operations here as they may affect the envelope
	 * trigger. The mixing of the selected oscillators that depend on the
	 * arpeggiator is done after the oscillator generation has completed.
	 */
	if (baudio->mixflags & P_CHORD)
	{
		if (baudio->mixflags & P_UNISON) {
		} else {
			if (mods->arpegcount++ > mods->arpegtotal)
			{
				mods->arpegcount = 0;

				switch (mods->arpegdir) {
					case P_ARPEG_UP:
						if (++mods->arpegstep > 3)
							mods->arpegstep = 0;
						break;
					case P_ARPEG_DOWN:
						if (--mods->arpegstep < 0)
							mods->arpegstep = 3;
						break;
					case P_ARPEG_UPDOWN:
						if (++mods->arpegstep > 3)
						{
							mods->arpegstep = 3;
							mods->arpegdir = P_ARPEG_DOWNUP;
						}
						break;
					case P_ARPEG_DOWNUP:
						if (--mods->arpegstep < 0)
						{
							mods->arpegstep = 0;
							mods->arpegdir = P_ARPEG_UPDOWN;
						}
						break;
				}
				if ((baudio->mixflags & P_MULTI)
					&& ((voice->flags & BRISTOL_KEYOFF) == 0))
					voice->flags |= BRISTOL_KEYREON;
			}
		}
	}
/* */

/* ADSR - FILTER MOD */
	/*
	 * Run the ADSR for the filter.
	 */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
/* ADSR - FILTER - OVER */

/* PWM Scratchbuf */
	/*
	 * If we have any mods on the oscillators, we need to put them in here.
	 * This should be under the control of polypressure and/or
	 * channelpressure?
	 */
	bzero(scratchbuf, audiomain->segmentsize);

	/*
	 * We know the intensity and mod source. Build the buffer now and apply
	 * it depending on the waveform. PWM can have 3 sources, mg1, mg2 or
	 * filter env.
	 */
	if (baudio->mixflags & PWM_MG1)
		bufmerge(mg1buf, mods->pwmdepth * 128,
			scratchbuf, 0.0, sc);
	else if (baudio->mixflags & PWM_MG2)
		bufmerge(mg2buf, mods->pwmdepth * 128,
			scratchbuf, 0.0, sc);
	else
		bufmerge(adsrbuf, mods->pwmdepth * 128,
			scratchbuf, 0.0, sc);
/* PWM Scratchbuf - DONE */

	kh = voice->key.key;
/* OSCILLATORS */
	/*
	 * Due to the mod routings we have to organise how the oscillators are 
	 * generated. They are going to have independent bufferings to allow the
	 * mod routings depending on switches, and have a separate final mix
	 * section. The 'cheaper' option of having the osc gain parameter do the
	 * mixing does not work well with mod routing. Since this is a mono synth
	 * the overhead is not excessive.
	 */
	for (o_act = 0; o_act < 4; o_act++)
	{
		if (mods->keydata[o_act].key == -1)
			continue;

		oper = o_act;

		/*
		 * Last oscillator is actually the 8th sound.
		 * Depending on our oscillator need to select the output buffer.
		 */
		switch (o_act)
		{
			case 0:
				audiomain->palette[
					(*baudio->sound[oper]).index]->specs->io[2].buf = osc0buf;
				break;
			case 1:
				audiomain->palette[
					(*baudio->sound[oper]).index]->specs->io[2].buf = osc1buf;
				break;
			case 2:
				audiomain->palette[
					(*baudio->sound[oper]).index]->specs->io[2].buf = osc2buf;
				break;
			case 3:
				/*
				 * Looks odd, but the 4th osc is operator 9
				 */
				oper = 8;
				audiomain->palette[
					(*baudio->sound[oper]).index]->specs->io[2].buf = osc3buf;
				break;
		}

		/*
		 * Insert the rest of the desired buffers.
		 */
		audiomain->palette[(*baudio->sound[oper]).index]->specs->io[0].buf
			= freqbuf;
		if (baudio->mixflags & (P_PWM_1 << o_act))
			audiomain->palette[(*baudio->sound[oper]).index]->specs->io[1].buf
				= scratchbuf;
		else
			audiomain->palette[(*baudio->sound[oper]).index]->specs->io[1].buf
				= baudio->rightbuf;
		/*
		 * Default is no sync, may be changed by mod routing later.
		 */
		audiomain->palette[(*baudio->sound[oper]).index]->specs->io[3].buf = 0;

		/*
		 * Stuff the voice parameters with our key information. This is called
		 * cheating, but as this is a synth within a synth.....
		 */
		voice->cFreq = mods->keydata[o_act].cFreq;
		voice->dFreq = mods->keydata[o_act].dFreq;
		voice->cFreqstep = mods->keydata[o_act].cFreqstep;
		voice->cFreqmult = mods->keydata[o_act].cFreqmult;
		voice->key.key = mods->keydata[o_act].key;
		voice->lastkey = mods->keydata[o_act].lastkey;

		/*
		 * Fill our frequency information
		 */
		fillFreqTable(baudio, voice, freqbuf, sc, 1);

		mods->keydata[o_act].cFreq = voice->cFreq;

		/* 
		 * If wheels are set for VCO then apply it to all of them.
		 */
		if (mods->mg1dst == DEST_PITCH)
		{
			bufmerge(mg1buf,
				mods->mg1intensity * baudio->contcontroller[CONTROL_MG1],
					freqbuf, 1.0, sc);
		} else if (mods->mg1dst == DEST_VCO) {
			if (baudio->mixflags & P_EFFECTS)
			{
				if (baudio->mixflags & P_MOD_SING)
				{
					if (o_act != 0)
						bufmerge(mg1buf, mods->mg1intensity * mods->mfreq
							* baudio->contcontroller[CONTROL_MG1],
							freqbuf, 1.0, sc);
				} else {
					/*
					 * Dual, apply to 1 and 3.
					 */
					if ((o_act == 1) || (o_act == 3))
						bufmerge(mg1buf, mods->mg1intensity * mods->mfreq
							* baudio->contcontroller[CONTROL_MG1],
							freqbuf, 1.0, sc);
				}
			} else {
				/*
				 * Only apply to VCO-1
				 */
				if (o_act == 0)
					bufmerge(mg1buf, mods->mg1intensity
						* baudio->contcontroller[CONTROL_MG1],
						freqbuf, 1.0, sc);
			}
		}

		/*
		 * If DEST_PITCH apply mods. This is rather an ugly bit of logic.
		 */
		if ((mods->benddst == DEST_PITCH)
		|| ((mods->benddst == DEST_VCO)
			&& (
				/* VCO and no effects and osc-0 */
				(((baudio->mixflags & P_EFFECTS) == 0) && (o_act == 0))
				||
					((baudio->mixflags & P_EFFECTS)
						/* VCO and effects and SINGLE and not osc-0 */
						&& (((baudio->mixflags & P_MOD_SING) && (o_act != 0))
							||
							/* VCO and effects and not SINGLE and osc-1/3 */
							(((baudio->mixflags & P_MOD_SING) == 0)
								&& ((o_act == 1) && (o_act == 3)))
						)
					)
				)
			)
		)
		{
			register int c = sc;
			/*
			 * value of 0.5 is no change.
			 * value of 0.0 is one octave down
			 * value of 1.0 is one octave up
			 */
			register float *f = freqbuf,
				a = baudio->contcontroller[CONTROL_BEND] - 0.5;

			a = a * baudio->note_diff * 12 *
				mods->bendintensity;

			for (; c > 0; c-=16)
			{
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
				*f++ += a;
			}
		}

/* MODS */
		/*
		 * Some of the FX appear to conflict with the wheel mods, but it is
		 * just a case of applying them except as per VCO selection of wheel.
		 */
		if (baudio->mixflags & P_EFFECTS)
		{
			/*
			 * We have diverse mod routings to apply, they are not really
			 * effects in a true sense, but the MonoPoly "Effects" enabled or
			 * disabled the routing.
			 *
			 * There are two main modes, single and double
			 *	Single takes Osc0 to mod 1/2/3.
			 *	Double takes Osc 0->1 and 2->3 (original had 0->2/1->3)
			 */
			if (baudio->mixflags & P_MOD_SING)
			{
				/*
				 * In single, apply mods to other oscillators
				 */
				if (o_act != 0)
				{
					/*
					 * Apply mods. We can have sync, an amount of XMOD and
					 * a modulated frequency.
					 */
					if (baudio->mixflags & P_MOD_SYNC)
						audiomain->palette[
							(*baudio->sound[oper]).index]->specs->io[3].buf
								= osc0buf;

					/*
					 * Put the fxbuf into the frequency buf for this osc.
					 */
					if (baudio->mixflags & P_MOD_XMOD)
						bufmerge(osc0buf, mods->xmod, freqbuf, 1.0, sc);

					if (baudio->mixflags & P_MOD_ENV)
						bufmerge(adsrbuf, mods->mfreq * 2, freqbuf, 1.0, sc);
				}
			} else {
				if (o_act == 1)
				{
					/*
					 * Apply mods. We can have sync, an amount of XMOD and
					 * a modulated frequency.
					 */
					if (baudio->mixflags & P_MOD_SYNC)
						audiomain->palette[
							(*baudio->sound[oper]).index]->specs->io[3].buf
								= osc0buf;

					/*
					 * Put the fxbuf into the frequency buf for this osc.
					 */
					if (baudio->mixflags & P_MOD_XMOD)
						bufmerge(osc0buf, mods->xmod, freqbuf, 1.0, sc);
					if (baudio->mixflags & P_MOD_ENV)
						bufmerge(adsrbuf, mods->mfreq * 2, freqbuf, 1.0, sc);
				} else if (o_act == 3) {
					/*
					 * Apply mods. We can have sync, an amount of XMOD and
					 * a modulated frequency.
					 */
					if (baudio->mixflags & P_MOD_SYNC)
						audiomain->palette[
							(*baudio->sound[oper]).index]->specs->io[3].buf
								= osc2buf;

					/*
					 * Put the fxbuf into the frequency buf for this osc.
					 */
					if (baudio->mixflags & P_MOD_XMOD)
						bufmerge(osc2buf, mods->xmod, freqbuf, 1.0, sc);
					if (baudio->mixflags & P_MOD_ENV)
						bufmerge(adsrbuf, mods->mfreq * 2, freqbuf, 1.0, sc);
				}
			}
		}
/* Mods done */

		(*baudio->sound[oper]).operate(
			(audiomain->palette)[8],
			voice,
			(*baudio->sound[oper]).param,
			voice->locals[voice->index][oper]);
	}
/* OSCILLATORS - DONE */
	 voice->key.key = kh;

/* Arpeggiated mixer. */
	/*
	 * Take each osc buf and mix into leftbuf ready for filter. We need to 
	 * consider what do to with argep at this point, since if we are chorded
	 * the only selected oscillators may actually voice.
	 */
	if (baudio->mixflags & P_CHORD)
	{
		if (baudio->mixflags & P_UNISON) {
			bufmerge(osc0buf, mods->osc0gain, baudio->leftbuf, 0.0, sc);
			bufmerge(osc1buf, mods->osc1gain, baudio->leftbuf, 1.0, sc);
			bufmerge(osc2buf, mods->osc2gain, baudio->leftbuf, 1.0, sc);
			bufmerge(osc3buf, mods->osc3gain, baudio->leftbuf, 1.0, sc);
		} else {
			if (mods->arpegstep == 0)
				bufmerge(osc0buf, mods->osc0gain, baudio->leftbuf, 0.0, sc);
			else if (mods->arpegstep == 1)
				bufmerge(osc1buf, mods->osc1gain, baudio->leftbuf, 0.0, sc);
			else if (mods->arpegstep == 2)
				bufmerge(osc2buf, mods->osc2gain, baudio->leftbuf, 0.0, sc);
			else if (mods->arpegstep == 3)
				bufmerge(osc3buf, mods->osc3gain, baudio->leftbuf, 0.0, sc);
		}
	} else {
		/*
		 * No chording, mix them all.
		 */
		bufmerge(osc0buf, mods->osc0gain, baudio->leftbuf, 0.0, sc);
		bufmerge(osc1buf, mods->osc1gain, baudio->leftbuf, 1.0, sc);
		bufmerge(osc2buf, mods->osc2gain, baudio->leftbuf, 1.0, sc);
		bufmerge(osc3buf, mods->osc3gain, baudio->leftbuf, 1.0, sc);
	}
/* Arpeggiated mixer - done */

/* NOISE */
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf =
		baudio->leftbuf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);
/* NOISE OVER */

	/* Filter input driving */
	bufmerge(baudio->leftbuf, 192.0, baudio->leftbuf, 0.0, sc);

/* FILTER */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf
		= baudio->leftbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf
		= adsrbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf
		= baudio->rightbuf;

	if (mods->mg1dst == DEST_FLT)
		bufmerge(mg1buf, mods->mg1intensity
			* baudio->contcontroller[CONTROL_MG1] * 2,
			adsrbuf, 1.0, sc);

	if (mods->benddst == DEST_FLT)
	{
		register int c = sc;
		register float *f = adsrbuf, g = baudio->contcontroller[CONTROL_BEND];

		g *= mods->bendintensity * 8;

		for (; c > 0; c-=16)
		{
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
			*f++ += g;
		}
	}

	(*baudio->sound[4]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);
/* FILTER OVER */

/* ADSR - ENV MOD */
	/*
	 * Run the ADSR for the amp.
	 */
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[5]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);
/* ADSR - ENV - OVER */

/* AMP */
	bristolbzero(baudio->leftbuf, audiomain->segmentsize);

	audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf
		= baudio->rightbuf;
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[1].buf
		= adsrbuf;
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[2].buf =
		baudio->leftbuf;
	(*baudio->sound[6]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[voice->index][6]);
/* AMP OVER */
	return(0);
}

int
operatePolyPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if ((voice->flags & BRISTOL_KEYDONE) || (baudio->mixflags & P_CHORD_OFF))
	{
/*printf("done: %x\n", baudio->mixflags); */
		if ((baudio->mixflags & P_CHORD_OFF)
			|| ((baudio->mixflags & P_CHORD) == 0))
		{
			((pmods *) baudio->mixlocals)->keydata[0].key = -1;
			((pmods *) baudio->mixlocals)->keydata[1].key = -1;
			((pmods *) baudio->mixlocals)->keydata[2].key = -1;
			((pmods *) baudio->mixlocals)->keydata[3].key = -1;

			voice->flags &= ~BRISTOL_VCO_MASK;
			baudio->mixflags &= ~P_CHORD_OFF;

			return(0);
		}
		if ((baudio->mixflags & P_CHORD) && ~(baudio->mixflags & P_UNISON))
			baudio->mixflags |= P_REON;
	}
	return(0);
}

int
bristolPolyDestroy(audioMain *audiomain, Baudio *baudio)
{
	printf("removing one poly\n");
	return(0);
}

int
bristolPolyInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one mono/poly\n");
	/*
	 * The Poly is relatively straightforward in terms of operators
	 *
	 * We will need the following and will use those from the prophet:
	 *
	 *	Two LFO for Mod groups
	 *	Four DCO - Each voice will only sound one of them though.
	 *  Noise
	 *	Filter
	 *	Two ENV
	 *	Amplifier
	 */
	baudio->soundCount = 11; /* Number of operators in this voice (MM) */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * The fourth Osc and two LFO are tagged on the end. Ordering is not
	 * important, but this will allow us to import much of the existing 
	 * code from the other synths that already use for first 8 operators:
	 * So many of these classic synths used the same sound routing and
	 * we use that here.
	 */
	initSoundAlgo(8, 0, baudio, audiomain, baudio->sound);
	initSoundAlgo(8, 1, baudio, audiomain, baudio->sound);
	initSoundAlgo(8, 2, baudio, audiomain, baudio->sound);
	/* An ADSR */
	initSoundAlgo(1, 3, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(3, 4, baudio, audiomain, baudio->sound);
	/* Another ADSR */
	initSoundAlgo(1, 5, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2, 6, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4, 7, baudio, audiomain, baudio->sound);
	/* Fourth Osc */
	initSoundAlgo(8, 8, baudio, audiomain, baudio->sound);
	/* Two LFO */
	initSoundAlgo(16, 9, baudio, audiomain, baudio->sound);
	initSoundAlgo(16, 10, baudio, audiomain, baudio->sound);

	baudio->param = polyController;
	baudio->destroy = bristolPolyDestroy;
	baudio->preops = operatePolyPreops; /* Mod Group LFO */
	baudio->operate = operateOnePoly; /* Mod application to single OSC */
	baudio->postops = operatePolyPostops; /* Env and Filter */

	if (mg1buf == 0)
		mg1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (mg2buf == 0)
		mg2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (freqbuf == 0)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc0buf == 0)
		osc0buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc1buf == 0)
		osc1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc2buf == 0)
		osc2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc3buf == 0)
		osc3buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == 0)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratchbuf == 0)
		scratchbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(pmods));

	((pmods *) baudio->mixlocals)->keydata[0].key = -1;
	((pmods *) baudio->mixlocals)->keydata[1].key = -1;
	((pmods *) baudio->mixlocals)->keydata[2].key = -1;
	((pmods *) baudio->mixlocals)->keydata[3].key = -1;

	((pmods *) baudio->mixlocals)->nextosc = 0;

	baudio->mixflags |= BRISTOL_MULTITRIG;
	return(0);
}

