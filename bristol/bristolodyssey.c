
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

/* ARP Odyssey algorithm. */

/*
 * Due to the complex routing here, there will be some odd effects of voice
 * cross modulation, especially using a lot of SHMix information - the mix
 * may come from a previously sounded voice for this synth, there is not other
 * way to have a device modulate itself currently. It could be extende to use
 * per voice buffers, but this is FFS. Mono operation should be correct.
 */

/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolodyssey.h"

/*
 * Use of these odyssey global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */
static float *freqbuf = (float *) NULL;
static float *fmbuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *arbuf = (float *) NULL;
static float *mixbuf = (float *) NULL;
static float *shmix = (float *) NULL;
static float *zerobuf = (float *) NULL;
static float *lfosh = (float *) NULL;
static float *lfosine = (float *) NULL;
static float *lfosquare = (float *) NULL;
static float *osc1ramp = (float *) NULL;
static float *osc1puls = (float *) NULL;
static float *osc2ramp = (float *) NULL;
static float *osc2puls = (float *) NULL;
static float *ringmod = (float *) NULL;

static float *scratch = (float *) NULL;

static void
oSwitchMod(odysseymods *mods, int ind, float value, float *b1, float *b2)
{
	mods->mod[ind].oactive = value == 0? 0:1;
	/* Select the buffer to use for mod */
	if (value == 0) {
		mods->mod[ind].gain = mods->mod[ind].cgain * mods->mod[ind].o2gain;
		mods->mod[ind].buf = b2;
	} else {
		mods->mod[ind].gain = mods->mod[ind].cgain * mods->mod[ind].o1gain;
		mods->mod[ind].buf = b1;
	}
/*printf("switchmod %i %f = %f\n", ind, value, mods->mod[ind].gain); */
}

static void
oContMod(odysseymods *mods, int index, float value)
{
	mods->mod[index].cgain = value;
	if (mods->mod[index].oactive)
		mods->mod[index].gain = mods->mod[index].o2gain * value * value;
	else
		mods->mod[index].gain = mods->mod[index].o1gain * value * value;
/*printf("contmod %i %f = %f\n", index, value, mods->mod[index].gain); */
}

int
odysseyController(Baudio *baudio, u_char operator, u_char controller, float value)
{
	int ivalue = value * CONTROLLER_RANGE;
	odysseymods *mods = (odysseymods *) baudio->mixlocals;

#ifdef DEBUG
	printf("bristolOdysseyControl(%i, %i, %f)\n", operator, controller, value);
#endif

	if (operator != 126)
		return(0);

	switch (controller) {
		case 0:
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 1:
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 2: /* Osc-1 FM Mod */
			oContMod(mods, 0, value);
			break;
		case 3: /* Osc-1 FM Mod */
			oContMod(mods, 1, value);
			break;
		case 4: /* Osc-1 PW Mod */
			oContMod(mods, 2, value);
			break;
		case 5: /* Osc-1 FM Mod Sel 1 */
			oSwitchMod(mods, 0, value, lfosine, lfosquare);
			break;
		case 6: /* Osc-1 FM Mod Sel 2 */
			oSwitchMod(mods, 1, value, lfosh, adsrbuf);
			break;
		case 7: /* Osc-1 PW mod Sel */
			oSwitchMod(mods, 2, value, lfosine, adsrbuf);
			break;

		case 8: /* Osc-2 FM Mod */
			oContMod(mods, 3, value);
			break;
		case 9: /* Osc-2 FM Mod */
			oContMod(mods, 4, value);
			break;
		case 10: /* Osc-2 PW Mod */
			oContMod(mods, 5, value);
			break;
		case 11: /* Osc-2 FM Mod Sel 1 */
			oSwitchMod(mods, 3, value, lfosine, shmix);
			break;
		case 12: /* Osc-2 FM Mod Sel 2 */
			oSwitchMod(mods, 4, value, lfosh, adsrbuf);
			break;
		case 13: /* Osc-2 PM Mod Sel */
			oSwitchMod(mods, 5, value, lfosine, adsrbuf);
			break;

		case 14: /* SH/Mixer */
			oContMod(mods, 6, value);
			break;
		case 15: /* SH/Mixer */
			oContMod(mods, 7, value);
			break;
		case 16: /* SH Output */
			oContMod(mods, 8, value);
			break;
		case 17: /* SH/Mixer switch */
			oSwitchMod(mods, 6, value, osc1ramp, osc1puls);
			break;
		case 18: /* SH/Mixer switch */
			oSwitchMod(mods, 7, value, noisebuf, osc2puls);
			break;
		case 19: /* SH Trigger - may need to be in algo. */
			oSwitchMod(mods, 8, value, NULL, shmix);
			break;

		case 20: /* Mixer */
			oContMod(mods, 9, value);
			break;
		case 21: /* Mixer */
			oContMod(mods, 10, value);
			break;
		case 22: /* MIXER */
			oContMod(mods, 11, value);
			break;

		case 23: /* VCF Mods */
			oContMod(mods, 12, value);
			break;
		case 24: /* VCF Mods */
			oContMod(mods, 13, value);
			break;
		case 25: /* VCF Mods */
			oContMod(mods, 14, value);
			break;

		case 26: /* VCA Mods */
			oContMod(mods, 15, value);
			break;
		case 27: /* VCA Mods - gain value */
			mods->a_gain = value;
			break;

		case 28: /* Mixer switch */
			oSwitchMod(mods, 9, value, noisebuf, ringmod);
			break;
		case 29: /* Mixer switch */
			oSwitchMod(mods, 10, value, osc1ramp, osc1puls);
			break;
		case 30: /* Mixer switch */
			oSwitchMod(mods, 11, value, osc2ramp, osc2puls);
			break;
		case 31: /* VCF switch */
			oSwitchMod(mods, 12, value, zerobuf, shmix);
			break;
		case 32: /* VCF switch */
			oSwitchMod(mods, 13, value, lfosh, lfosine);
			break;
		case 33: /* VCF switch */
			oSwitchMod(mods, 14, value, adsrbuf, arbuf);
			break;
		case 34: /* VCA switch */
			oSwitchMod(mods, 15, value, arbuf, adsrbuf);
			break;

		case 35: /* OSC Sync */
			oSwitchMod(mods, 16, value, osc1puls, NULL);
			break;

		case 49:
			if (baudio->voicecount == 1)
			{
				/* Multi Env trigger */
				if (ivalue == 0)
					baudio->mixflags |= BRISTOL_MULTITRIG;
				else
					baudio->mixflags &= ~BRISTOL_MULTITRIG;
			} else {
				/*
				 * Else multi lfo
				 */
				if (ivalue)
					baudio->mixflags &= ~A_LFO_MULTI;
				else
					baudio->mixflags |= A_LFO_MULTI;
			}
			break;
		case 57:
			/* PPC Flatten */
			if (ivalue)
				ivalue = -1;
			else
				ivalue = 0;
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (ivalue * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 58:
			/* PPC trill */
			if (ivalue)
				baudio->mixflags |= A_LFO_TRILL;
			else
				baudio->mixflags &= ~A_LFO_TRILL;
			break;
		case 59:
			/* PPC Sharpen */
			if (ivalue)
				ivalue = 1;
			else
				ivalue = 0;
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (ivalue * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 53:
			/* Auto ADSR Env repeat. */
			if (ivalue == 0)
				((odysseymods *) baudio->mixlocals)->adsrAuto = -1;
			else
				((odysseymods *) baudio->mixlocals)->adsrAuto = 0;
			break;
		case 55:
			/* Auto AR Env repeat. */
			if (ivalue == 0)
				((odysseymods *) baudio->mixlocals)->arAuto = 0;
			else
				((odysseymods *) baudio->mixlocals)->arAuto = 1;
			break;
		case 60:
			if (ivalue)
				baudio->mixflags &= ~A_VCO_DUAL;
			else
				baudio->mixflags |= A_VCO_DUAL;
			break;
	}
	return(0);
}

int
odysseyPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	odysseymods *mods = (odysseymods *) baudio->mixlocals;
	int sc = audiomain->samplecount;
	int i;

	/*
	 * The following could be optimised by checking for 'Dual', then checking
	 * for preference, then making a single search.
	 */
	if ((baudio->mixflags & A_VCO_DUAL) && (baudio->voicecount == 1))
	{
		if (baudio->notemap.flags & BRISTOL_MNL_HNP)
		{
			/*
			 * Find the lowest note, the library gave us the highest
			 */
			mods->low = voice->key.key;
			for (i = 0; i < 128; i++)
			{
				if ((baudio->notemap.key[i] != 0) && (i < mods->low))
				{
					mods->low = i;
					break;
				}
			}

			mods->dFreq = baudio->ctab[mods->low].step;
			mods->dfreq = baudio->ctab[mods->low].freq;

			if ((mods->low < mods->last)
				&& (baudio->notemap.flags & BRISTOL_MNL_TRIG))
				voice->flags |= BRISTOL_KEYREON;

			mods->last = mods->low;
		} else {
			/* Find the highest note */
			mods->high = voice->key.key;
			for (i = 127; i > 0; i--)
			{
				if ((baudio->notemap.key[i] != 0) && (i < mods->high))
				{
					mods->high = i;
					break;
				}
			}

			mods->dFreq = baudio->ctab[mods->high].step;
			mods->dfreq = baudio->ctab[mods->high].freq;

			if ((mods->high > mods->last)
				&& (baudio->notemap.flags & BRISTOL_MNL_TRIG))
				voice->flags |= BRISTOL_KEYREON;

			mods->last = mods->high;
		}
	}

	if (((odysseymods *) baudio->mixlocals)->lfolocals == 0)
	{
		((odysseymods *) baudio->mixlocals)->lfolocals =
			baudio->locals[voice->index][2];
	}

/* NOISE */
	bristolbzero(noisebuf, audiomain->segmentsize);
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf = noisebuf;
	(*baudio->sound[6]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[voice->index][6]);
/* NOISE DONE */

/* LFO produces 3 outputs, sine, square and S/H */
	if ((baudio->mixflags & A_LFO_MULTI) == 0)
	{
		/*
		 * Operate LFO if we have one per voice.
		 */
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf
			= shmix;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[4].buf
			= lfosine;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= lfosquare;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf
			= lfosh;

		/*
		 * The LFO almost has mods. The NOISE source that is give to sample
		 * and hold, triggered by the LFO may have other mods incorporated.
		 */
		bufmerge(mods->mod[6].buf, mods->mod[6].gain, shmix, 0.0, sc);
		bufmerge(mods->mod[7].buf, mods->mod[7].gain, shmix, 1.0, sc);

		(*baudio->sound[2]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[2]).param,
			((odysseymods *) baudio->mixlocals)->lfolocals);

		/*
		 * This is KBD trig for SH
		 * If the SH trigger is keyboard then we should take a random value and
		 * fill the SH buffer with this value. P/W is not important since we
		 * will scale the value anyway. If SH trig is keyboard then the lfosh
		 * buffer should be null anyway.
		 */
		if (mods->mod[8].buf == NULL)
		{
			int i;
			float v = mods->shkbd;

			if (voice->flags & (BRISTOL_KEYREON|BRISTOL_KEYON))
				v = mods->shkbd = *lfosh;

			for (i = sc; i > 0; i--)
				lfosh[i] = v;
		}
	}
	return(0);
}

int
operateOneOdyssey(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	odysseymods *mods = (odysseymods *) baudio->mixlocals;
	int sc = audiomain->samplecount, kflags = 0;

/*printf("operateOneOdyssey(%i, %x, %x)\n", voice->index, audiomain, baudio); */

	bristolbzero(filtbuf, audiomain->segmentsize);
	/*
	 * There are a lot of mods in this synth, each operator will produce its
	 * output buffer, and then they will modulate eachother.
	 *
	 * Noise is done, run the LFO and ADSR.
	 */

/* LFO produces 3 outputs, sine, square and S/H */
	if (baudio->mixflags & A_LFO_MULTI)
	{
		/*
		 * Operate LFO if we have one per voice.
		 */
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf
			= shmix;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[4].buf
			= lfosine;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= lfosquare;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf
			= lfosh;

		/*
		 * The LFO almost has mods. The NOISE source that is give to sample
		 * and hold, triggered by the LFO may use other then noise.
		 */
		bufmerge(mods->mod[6].buf, mods->mod[6].gain, shmix, 0.0, sc);
		bufmerge(mods->mod[7].buf, mods->mod[7].gain, shmix, 1.0, sc);

		(*baudio->sound[2]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[2]).param,
			baudio->locals[voice->index][2]);

		/*
		 * This is KBD trig for SH
		 * If the SH trigger is keyboard then we should take a random value and
		 * fill the SH buffer with this value. P/W is not important since we
		 * will scale the value anyway. If SH trig is keyboard then the lfosh
		 * buffer should be null anyway.
		 */
		if (mods->mod[8].buf == NULL)
		{
			int i;
			float v = mods->shkbd;

			if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
				v = mods->shkbd = *lfosh;

			for (i = sc; i > 0; i--)
				lfosh[i] = v;
		}
	}
/* */

/* ADSR */
	/*
	 * Run the ADSR and AR.
	 */
	if ((mods->arAutoTransit < 0) && (*lfosquare > 0) &&
		((voice->flags & (BRISTOL_KEYOFF|BRISTOL_KEYOFFING)) == 0))
			kflags |= BRISTOL_KEYREON;
	mods->arAutoTransit  = *lfosquare;

	if (mods->arAuto)
		voice->flags |= kflags;
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf = arbuf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);

	if (mods->adsrAuto)
		voice->flags |= kflags;
	else
		voice->flags &= ~kflags;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);
/* ADSR - OVER */

	/*
	 * We now have our mods, Noise, ADSR, S/H, Sine, Square.
	 *
	 * Run the oscillators.
	 */
/* OSC */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = fmbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf = scratch;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf = osc1ramp;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[3].buf = NULL;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[4].buf = osc1puls;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[5].buf = NULL;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[6].buf = NULL;

	/*
	 * Do this twice? That looks broken, we should do it once then copy, if
	 * not then the glide information gets damaged.
	 */
	fillFreqTable(baudio, voice, fmbuf, sc, 1);

	/*
	 * If we are dual then put a different freq into the second osc. We will 
	 * have to avoid issues of glide for now, find dfreq for this low key and
	 * then apply that to freqbuf.
	 */
	if ((baudio->mixflags & A_VCO_DUAL) && (baudio->voicecount == 1))
	{
		float *buf = freqbuf;
		int i;

		for (i = 0; i < sc; i+=8)
		{
			*buf++ = mods->dFreq;
			*buf++ = mods->dFreq;
			*buf++ = mods->dFreq;
			*buf++ = mods->dFreq;
			*buf++ = mods->dFreq;
			*buf++ = mods->dFreq;
			*buf++ = mods->dFreq;
			*buf++ = mods->dFreq;
		}
	} else
		bufmerge(fmbuf, 1.0, freqbuf, 0.0, sc);

	/*
	 * Put in any desired mods. There are two, MG and Wheel mod. Wheel mod is
	 * actually quite weak, takes the LFO to freqbuf, same as MG without the
	 * env. Hm.
	 */
	if (baudio->mixflags & A_LFO_TRILL)
		bufmerge(lfosine, 0.2, fmbuf, 1.0, sc);

	if (mods->mod[0].gain != 0.0f)
		bufmerge(mods->mod[0].buf, mods->mod[0].gain, fmbuf, 1.0, sc);
	if (mods->mod[1].gain != 0.0f)
		bufmerge(mods->mod[1].buf, mods->mod[1].gain, fmbuf, 1.0, sc);
	bufmerge(mods->mod[2].buf, mods->mod[2].gain, scratch, 0.0, sc);

	bristolbzero(osc1ramp, audiomain->segmentsize);
	bristolbzero(osc1puls, audiomain->segmentsize);

	(*baudio->sound[0]).operate(
		(audiomain->palette)[19],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);

	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf = osc2ramp;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[3].buf = osc1ramp;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[4].buf = osc2puls;

	if (baudio->mixflags & A_LFO_TRILL)
		bufmerge(lfosine, 0.2, freqbuf, 1.0, sc);

	if (mods->mod[3].gain != 0.0f)
		bufmerge(mods->mod[3].buf, mods->mod[3].gain, freqbuf, 1.0, sc);
	if (mods->mod[4].gain != 0.0f)
		bufmerge(mods->mod[4].buf, mods->mod[4].gain, freqbuf, 1.0, sc);
	bufmerge(mods->mod[5].buf, mods->mod[5].gain, scratch, 0.0, sc);

	bristolbzero(osc2ramp, audiomain->segmentsize);
	bristolbzero(osc2puls, audiomain->segmentsize);

	/*
	 * This was called with sound 0 rather than 1, so had two different freq
	 * and mixed output buffers. Sounded like ring mod.
	 */
	(*baudio->sound[1]).operate(
		(audiomain->palette)[19],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
/* OSC done */

/* Ring Mod */
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf = osc1puls;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf = osc2puls;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[2].buf = ringmod;
	(*baudio->sound[8]).operate(
		(audiomain->palette)[20],
		voice,
		(*baudio->sound[8]).param,
		voice->locals[voice->index][8]);
/* Ring Mod done */

/* Mixer */
	/* Takes mods 9, 10 and 11. */
	bufmerge(mods->mod[9].buf, mods->mod[9].gain * 96.0, mixbuf, 0.0, sc);
	bufmerge(mods->mod[10].buf, mods->mod[10].gain * 96.0, mixbuf, 1.0, sc);
	bufmerge(mods->mod[11].buf, mods->mod[11].gain * 96.0, mixbuf, 1.0, sc);
/* */

/* Filter */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = mixbuf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf = scratch;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf = filtbuf;;

	bufmerge(mods->mod[12].buf, mods->mod[12].gain, scratch, 0.0, sc);
	bufmerge(mods->mod[13].buf, mods->mod[13].gain, scratch, 1.0, sc);
	bufmerge(mods->mod[14].buf, mods->mod[14].gain, scratch, 1.0, sc);

	(*baudio->sound[3]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
/* Filter DONE */

/* HP Filter */
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[0].buf = filtbuf;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[1].buf =
		baudio->rightbuf;
	(*baudio->sound[9]).operate(
		(audiomain->palette)[10],
		voice,
		(*baudio->sound[9]).param,
		voice->locals[voice->index][9]);
/* HP Filter DONE */

	// This bypasses the HPF that has been giving me noise
	//bufmerge(filtbuf, 1.0, baudio->rightbuf, 0.0, sc);

/* AMP */
	bufmerge(shmix, mods->mod[8].gain, baudio->rightbuf, 1.0, sc);
	/*
	bufmerge(lfosine, 0.0, mods->mod[15].buf, mods->mod[15].gain, sc);
	{
		register float gain = mods->a_gain, *ab = adsrbuf;
		register int i = 0;

		for (; i < sc; i += 8)
		{
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
		}
	}
	*/

	kflags = voice->flags;
	audiomain->palette[1]->specs->io[0].buf = scratch;
	(*baudio->sound[10]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[10]).param,
		voice->locals[voice->index][10]);
	voice->flags = kflags;

	bufmerge(scratch, mods->a_gain, mods->mod[15].buf, mods->mod[15].gain, sc);

	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf
		= baudio->rightbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
		= mods->mod[15].buf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf =
		baudio->leftbuf;

	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);
/* AMP OVER */
	/*
	 * It is bad practice to use the rightbuf. It can be done, but must be
	 * zeroed - most efficiently in postOps but otherwise.....
	 */
	bristolbzero(baudio->rightbuf, audiomain->segmentsize);
	return(0);
}

static int
bristolOdysseyDestroy(audioMain *audiomain, Baudio *baudio)
{
printf("removing one odyssey\n");
	return(0);
	bristolfree(freqbuf);
	bristolfree(fmbuf);
	bristolfree(filtbuf);
	bristolfree(noisebuf);
	bristolfree(adsrbuf);
	bristolfree(arbuf);
	bristolfree(mixbuf);
	bristolfree(shmix);
	bristolfree(zerobuf);
	bristolfree(lfosine);
	bristolfree(lfosquare);
	bristolfree(lfosh);
	bristolfree(osc1ramp);
	bristolfree(osc1puls);
	bristolfree(osc2ramp);
	bristolfree(osc2puls);
	bristolfree(ringmod);
	return(0);
}

int
bristolOdysseyInit(audioMain *audiomain, Baudio *baudio)
{
	odysseymods *mods;
	int i;

printf("initialising one odyssey\n");
	baudio->soundCount = 11; /* Number of operators in this voice */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/* oscillator 1 */
	initSoundAlgo(	19,	0, baudio, audiomain, baudio->sound);
	/* oscillator 2 */
	initSoundAlgo(	19,	1, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(	16,	2, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(	3,	3, baudio, audiomain, baudio->sound);
	/* One ADSR */
	initSoundAlgo(	1,	4, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(	2,	5, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(	4,	6, baudio, audiomain, baudio->sound);
	/* Second ADSR to be AR envelope */
	initSoundAlgo(	1,	7, baudio, audiomain, baudio->sound);
	/* Ring Mod */
	initSoundAlgo(	20,	8, baudio, audiomain, baudio->sound);
	/* HPF */
	initSoundAlgo(	10,	9, baudio, audiomain, baudio->sound);
	/* Third key grooming envelope */
	initSoundAlgo(	1,	10, baudio, audiomain, baudio->sound);

	baudio->param = odysseyController;
	baudio->destroy = bristolOdysseyDestroy;
	baudio->operate = operateOneOdyssey;
	baudio->preops = odysseyPreops;

	if (freqbuf == NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (fmbuf == NULL)
		fmbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == NULL)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (noisebuf == NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == NULL)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (arbuf == NULL)
		arbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (mixbuf == NULL)
		mixbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (shmix == NULL)
		shmix = (float *) bristolmalloc0(audiomain->segmentsize);
	if (zerobuf == NULL)
		zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosh == NULL)
		lfosh = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosquare == NULL)
		lfosquare = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosine == NULL)
		lfosine = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc1ramp == NULL)
		osc1ramp = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc1puls == NULL)
		osc1puls = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc2ramp == NULL)
		osc2ramp = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc2puls == NULL)
		osc2puls = (float *) bristolmalloc0(audiomain->segmentsize);
	if (ringmod == NULL)
		ringmod = (float *) bristolmalloc0(audiomain->segmentsize);

	if (scratch == NULL)
		scratch = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(odysseymods));
	mods = (odysseymods *) baudio->mixlocals;

	for (i = 0; i < 20; i++)
		mods->mod[i].o1gain = mods->mod[i].o2gain = 1.0;
	mods->mod[2].o1gain = mods->mod[2].o2gain = 512.0;
	mods->mod[5].o1gain = mods->mod[5].o2gain = 512.0;
	return(0);
}

