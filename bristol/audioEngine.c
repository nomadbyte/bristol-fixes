
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

#include <math.h>
#include <assert.h>

#include "bristol.h"
//#include "bristolmm.h"

#include "dco.h"

extern tableEntry defaultTable[];
extern void freeBristolAudio();
static void a440();

#ifndef BRISTOL_SEMAPHORE
extern void rbMidiNote(audioMain *, bristolMidiMsg *);
#endif

/*
 * Used by the a440 sine generator.
 */
int s440holder = 0;

static void
bristolUnlinkVoice(audioMain *audiomain, bristolVoice *v)
{
	bristolVoice *vtp;

	/*
	 * See if this is a monophonic voice with note preference
	 */
	if ((v->baudio != NULL) && (v->baudio->voicecount == 1)
		&& (v->baudio->notemap.flags & (BRISTOL_MNL_LNP|BRISTOL_MNL_HNP)))
	{
		v->flags &= ~BRISTOL_KEYDONE;
		v = v->next;
		return;
	}

	/*
	 * Move to the tail of the freelist, unlink it first:
	 */
	if (v->next != NULL)
		v->next->last = v->last;
	else
		audiomain->playlast = v->last;

	if (v->last != NULL)
		v->last->next = v->next;
	else
		audiomain->playlist = v->next;

	vtp = v;
	/* We have to move this to the next entry now */
	v = v->next;

	/* Unlinked, put it on the tail of the freelist */
	vtp->next = NULL;
	if ((vtp->last = audiomain->freelast) != NULL)
		vtp->last->next = vtp;
	else
		audiomain->freelist = vtp;
	audiomain->freelast = vtp;
}

/*
 * This should be organised to be a callback for the Jack and DSSI interfaces.
 * there may be issues of internal buffering that will have to be reviewed, and
 * potentially if we are called with less than our desired number of samples
 * then we buffer them internally and return data slightly behind the real
 * time requests.
 *
 * The syntax for the callback will have to accord to the API specifications
 * which means audiomain will be implicit.
 *
 * doAudioOps() should take at least 4 bufs? Arrays of buf pointers? Whatever
 * the final specification this should no longer take a pair of interleaved
 * sample buffers. Optimally have each synth just use its own output buffers
 * and pair of input buffers. The calling routine can decide how to mix them
 * down and what inputs to provide.
 */
int
doAudioOps(audioMain *audiomain, float *outbuf, float *startbuf)
{
	register bristolVoice *voice;
	register int i;
	register Baudio *thisaudio;
	register float *extmult, *leftch, *rightch, gain;
	bristolMidiMsg msg;

	/*
	 * Clear the output buffer at this point.
	 */
	bristolbzero(outbuf, audiomain->segmentsize * 2);

#ifdef DEBUG
	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG5)
		printf("doAudioOps\n");
#endif

	/*
	 * Configure flags to force one iteration of the preops, and to enable
	 * postops if a voice is active.
	 */
	if ((thisaudio = audiomain->audiolist) == NULL)
	{
#ifndef BRISTOL_SEMAPHORE
		/*
		 * This is not threadsafe. We should flag the rb as inactive.
		 */
		jack_ringbuffer_stop(audiomain->rb);
		jack_ringbuffer_reset(audiomain->rb);
#endif
		return(0);
	}

#ifndef BRISTOL_SEMAPHORE
	while (jack_ringbuffer_read_space(audiomain->rb) >= sizeof(bristolMidiMsg))
	{
		jack_ringbuffer_read(audiomain->rb, (char *) &msg,
			sizeof(bristolMidiMsg));

		rbMidiNote(audiomain, &msg);
	}
#endif

	while (thisaudio != NULL)
	{
		if (bristolActiveSenseCheck(audiomain, thisaudio) == 0)
			thisaudio->mixflags |= BRISTOL_REMOVE;

		/*
		 * See if we are finished with the baudio (typically gui has exit).
		 */
		if (thisaudio->mixflags & BRISTOL_REMOVE)
		{
			Baudio *holder = thisaudio;
			bristolVoice *vl;

#ifdef BRISTOL_SEMAPHORE
			sem_wait(audiomain->sem_long);
#endif

			thisaudio = thisaudio->next;

			freeBristolAudio(audiomain, holder);

			if ((audiomain->audiolist == NULL)
				&& (audiomain->flags & BRISTOL_MIDI_WAIT))
			{
				if (audiomain->debuglevel > 5)
					printf("Terminate\n");

				audiomain->atStatus = BRISTOL_TERM;
				holder->mixflags &= ~BRISTOL_REMOVE;
				return(0);
			}

			/*
			 * Otherwise we need to make sure none of the voices are using this
			 * removed audio.
			 */
			for (vl = audiomain->playlist; vl != NULL; vl = vl->next)
				if (vl->baudio == holder)
					vl->baudio = NULL;

			for (vl = audiomain->newlist; vl != NULL; vl = vl->next)
				if (vl->baudio == holder)
					vl->baudio = NULL;

			holder->mixflags &= ~BRISTOL_REMOVE;

#ifdef BRISTOL_SEMAPHORE
			sem_post(audiomain->sem_long);
#endif

			continue;
		}

		thisaudio->mixflags |= BRISTOL_MUST_PRE;
		thisaudio->mixflags &= ~BRISTOL_HAVE_OPED;
		thisaudio->lvoices = thisaudio->cvoices;
		thisaudio->cvoices = 0;

		/*
		 * See if we need to wind this emulation arpeggiator counters along
		 */
		bristolArpegReAudio(audiomain, thisaudio);

		thisaudio = thisaudio->next;
	}

	/*
	 * a440 has to be done here, since it must be active even when no keys
	 * are pressed.
	 */
	if ((audiomain->audiolist != NULL)
			&& (audiomain->audiolist->mixflags & A_440))
		a440(audiomain, outbuf, audiomain->samplecount);

	/*
	 * Get the long semaphore over the whole polyphonic process. We need to 
	 * replace this with some message passing algorithms using something that
	 * is a bit less risky. Minimally we should just have a short_sem that
	 * covers management of a secure messaging list. That can wait though, this
	 * code is already pretty recent (0.40 stream).
	 */
#ifdef BRISTOL_SEMAPHORE
	sem_wait(audiomain->sem_long);
#endif

	if (audiomain->newlist != NULL)
	{
		bristolVoice *v = audiomain->playlist, *vtp;

		/*
		 * Get the short semaphore over the new voice management sequences
		 */
#ifdef BRISTOL_SEMAPHORE
		sem_wait(audiomain->sem_short);
#endif

		/*
		 * Do a scan of the playlist and see if there is anything to be moved
		 * to the freelist (KEYDONE).
		 */
		while (v != NULL)
		{
#ifdef NOT_ANDROID_NATIVE_LIBRARY
#warning ANDROID PATCH, MAY HAVE TO BACK IT OUT
			if (v->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
				v->flags &= ~BRISTOL_KEYDONE;
			else
#endif
			if (v->flags & BRISTOL_KEYDONE)
			{
//printf("remove %x->%x/%x (0x%04x)\n", (size_t) v, (size_t) v->last,
//(size_t) v->next, v->flags);

				/*
				 * See if this is a monophonic voice with note preference
				 */
				if ((v->baudio != NULL) && (v->baudio->voicecount == 1)
					&& (v->baudio->notemap.flags &
						(BRISTOL_MNL_LNP|BRISTOL_MNL_HNP)))
				{
					v->flags &= ~BRISTOL_KEYDONE;
					v = v->next;
					continue;
				}

				v->flags &= ~BRISTOL_KEYOFFING;
				/*
				 * Move to the tail of the freelist, unlink it first:
				 */
				if (v->next != NULL)
					v->next->last = v->last;
				else
					audiomain->playlast = v->last;

				if (v->last != NULL)
					v->last->next = v->next;
				else
					audiomain->playlist = v->next;

				vtp = v;
				/* We have to move this to the next entry now */
				v = v->next;

				/* Unlinked, put it on the tail of the freelist */
				vtp->next = NULL;
				if ((vtp->last = audiomain->freelast) != NULL)
					vtp->last->next = vtp;
				else
					audiomain->freelist = vtp;
				audiomain->freelast = vtp;

				continue;
			}
			v->offset = 0;
			v = v->next;
		}

		/*
		 * The newlist is doubly linked but we don't care about that since we
		 * own access at the moment and are going to move all members across.
		 */
		while (audiomain->newlist != NULL)
		{
			v = audiomain->newlist;
			audiomain->newlist = v->next;

			if ((v->next = audiomain->playlist) != NULL)
				v->next->last = v;
			else
				audiomain->playlast = v;
			v->last = NULL;
			audiomain->playlist = v;

			v->flags &= ~(BRISTOL_KEYDONE
				|BRISTOL_KEYOFF
				|BRISTOL_KEYOFFING
				|BRISTOL_KEYSUSTAIN);

			if ((v->flags & BRISTOL_KEYREON) == 0)
				v->flags |= BRISTOL_KEYON;

			if (v->flags & BRISTOL_KEYREOFF)
			{
				v->flags &= ~BRISTOL_KEYREOFF;
				v->flags |= BRISTOL_KEYOFF|BRISTOL_KEYSUSTAIN;
			}
		}

		audiomain->newlast = NULL;

		/*
		 * Free the short semaphore over the new voice management sequences
		 */
#ifdef BRISTOL_SEMAPHORE
		sem_post(audiomain->sem_short);
#endif
	}

	voice = audiomain->playlist;

	/*
	 * We need to look through our voice list, and see if any are active. If
	 * so then start running the voice structures through the sound structures.
	 *
	 * This is the polyphonic mixing process.
	 */
	while (voice != NULL)
	{
		if ((voice->baudio == NULL)
			|| (voice->baudio->operate == NULL))
		{
			if ((voice->baudio != NULL)
				&& (voice->baudio->operate == NULL))
			{
				printf("this emulator is not implemented\n");
				voice->baudio->notemap.flags
					&= ~(BRISTOL_MNL_LNP|BRISTOL_MNL_HNP);
			}
			/*
			 * Unlink this voice
			 */
			bristolUnlinkVoice(audiomain, voice);
			voice = voice->next;
			continue;
		}

		if (voice->baudio->mixflags & (BRISTOL_HOLDDOWN|BRISTOL_REMOVE))
		{
			voice = voice->next;
			continue;
		}

		if (~voice->flags & BRISTOL_KEYDONE)
		{
			/*
			 * If this audio is mono, and count is zero and SUSTAIN is not
			 * active then signal KEY_OFF, clear extreme note;
			 */
			if ((voice->baudio->voicecount == 1)
				&& (voice->baudio->notemap.flags
					& (BRISTOL_MNL_LNP|BRISTOL_MNL_HNP))
				&& (voice->baudio->notemap.count == 0)
				&& (voice->baudio->notemap.extreme >= 0)
				&& (voice->baudio->contcontroller[BRISTOL_CC_HOLD1] < 0.5))
			{
				if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
					printf("engine cleared MNL note\n");
				voice->baudio->notemap.extreme = -1;
				voice->flags |= BRISTOL_KEYOFF;
				voice->flags &= ~BRISTOL_KEYSUSTAIN;
			}

			if (voice->baudio->mixflags & BRISTOL_MUST_PRE)
			{
				if (voice->baudio->preops)
					voice->baudio->preops(audiomain,
						voice->baudio, voice, startbuf);

				/*
				 * Keep a pointer to the first voice that was active on any 
				 * given bristolAudio sound. This is the most recent keypress.
				 */
				voice->baudio->firstVoice = voice;
				voice->baudio->mixflags &= ~BRISTOL_MUST_PRE;
				voice->baudio->mixflags |= BRISTOL_HAVE_OPED;
			}

			/*
			 * Do something - find the buffers and put them in the IO structures
			 * then get the locals and params, and call the operator, summing
			 * the outputs into our single float buf.
			 *
			 * Each voice, or channel if you want, has its own independant 
			 * limit on voice counts. This strange hack works only if we have
			 * allocated a new voice to the head of the list. If we stole an
			 * existing voice or two it may fail however that is unlikely
			 * since it requires a full set of notes for a single emulation
			 * and most of them only request a subset of those available.
			 * Having said that, if we steal voices then the count does not
			 * increase anyway.
			 */
			if (++voice->baudio->cvoices > voice->baudio->voicecount)
			{
				/*
				 * This looks a bit brutal however the assignement code should
				 * avoid this situation.
				 */
				if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG2)
					printf("overvoice keydone\n");

				voice->flags |= BRISTOL_KEYOFF;
				voice = voice->next;
				continue;
			}

			/*
			 * If we are arpeggiating or sequencing we need to check on rolling
			 * this voice along, and need a couple of sanity checks.
			 */
			if (((voice->baudio->mixflags
				& (BRISTOL_SEQUENCE|BRISTOL_ARPEGGIATE))
				&& (bristolArpegReVoice(voice->baudio, voice,
					(float) audiomain->samplerate) < 0))
				|| ((voice->locals == 0) || (voice->locals[voice->index] == 0)))
			{
				voice = voice->next;
				continue;
			}

			voice->baudio->operate(audiomain,
				voice->baudio, voice, startbuf);

			if ((voice->baudio->voicecount == 1)
				&& (voice->baudio->notemap.flags 
					& (BRISTOL_MNL_LNP|BRISTOL_MNL_HNP)))
				voice->flags &= ~BRISTOL_KEYDONE;

			/*
			 * We should not do this, startbuf should be maintained across
			 * all synths.....
			 */
			bristolbzero(startbuf, audiomain->iosize);
		}

		voice = voice->next;
	}

	/*
	 * See if any of the voices have postoperators configured.
	 */
	thisaudio = audiomain->audiolist;
	while (thisaudio != NULL)
	{
		if (thisaudio->postops != NULL)
		{
			if (((thisaudio->mixflags & (BRISTOL_HOLDDOWN|BRISTOL_REMOVE)) == 0)
				&& (thisaudio->mixflags & BRISTOL_HAVE_OPED))
				thisaudio->postops(audiomain,
					thisaudio, thisaudio->firstVoice, startbuf);
		}
		thisaudio = thisaudio->next;
	}

	voice = audiomain->playlist;
	while (voice != NULL)
	{
		voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);
		voice->offset = -1;
		voice = voice->next;
	}

	/*
	 * Free the long semaphore over the polyphonic process.
	 */
#ifdef BRISTOL_SEMAPHORE
	sem_post(audiomain->sem_long);
#endif

	/*
	 * At this point all the voices have put their output onto the leftbuf and
	 * rightbuf (leftbuf only if mono). We should go through the baudio 
	 * structures and see if we have any effects active.
	 *
	 * It is not possible to use the HAVE_OPED flag for the effects processes.
	 * If they use reverb and echo they need to roll on way past the end of
	 * poly voice processing.
	 */
	thisaudio = audiomain->audiolist;

	while (thisaudio != NULL)
	{
		/*
		 * Need to ensure this audio structure is actually assigned
		 */
		if (thisaudio->mixflags & BRISTOL_HOLDDOWN)
		{
			thisaudio = thisaudio->next;
			continue;
		}

		/*
		 * nc-17/06/02:
		 * At the moment this only works for a single effect on the list. Need
		 * to add another for/while loop inside this if statement.
		 * There are larger considerations for stereo effects - these can be
		 * single input or stereo input. Taking a stereo effect (in/out) into
		 * an effect that takes mono input will not give the anticipated 
		 * results, obviously, but is beyond the scope of this piece of code.
		 */
		if ((thisaudio->effect != NULL) &&
			(thisaudio->effect[0] != NULL))
		{
			int index;

			index = (*thisaudio->effect[0]).index;
			/*
			 * One inbuf, two outbuf. The effect should have some method of
			 * indicating its desired data flows - mono in/stereo out, or
			 * fully stereo, etc. At the moment there is one flag for stereo
			 * which indicates stereo in/out. Without this flag the assumption 
			 * for an effect is mono in/stereo out.
			 */
			if (audiomain->palette[index]->flags & BRISTOL_FX_STEREO)
			{
				audiomain->palette[index]->specs->io[0].buf
					= thisaudio->leftbuf;
				audiomain->palette[index]->specs->io[1].buf
					= thisaudio->rightbuf;
				audiomain->palette[index]->specs->io[2].buf
					= thisaudio->leftbuf;
				audiomain->palette[index]->specs->io[3].buf
					= thisaudio->rightbuf;
			} else {
				audiomain->palette[index]->specs->io[0].buf
					= thisaudio->leftbuf;
				audiomain->palette[index]->specs->io[1].buf
					= thisaudio->leftbuf;
				audiomain->palette[index]->specs->io[2].buf
					= thisaudio->rightbuf;
			}

			if ((thisaudio->firstVoice == NULL)
				|| (thisaudio->firstVoice->baudio == 0))
			{
				thisaudio = thisaudio->next;
				continue;
			}
#warning - this voice may have been reassigned. check midi channel
			if (thisaudio->firstVoice != NULL)
				(*thisaudio->effect[0]).operate(
					audiomain->palette[index],
					thisaudio->firstVoice,
					(*thisaudio->effect[0]).param,
					thisaudio->FXlocals[0][0]);

			/*
			 * This is a quick hack to allow for two chained effects, it needs 
			 * to be generalised for arbitrary links, but the ARP 2600 needed
			 * two effects.
			 */
			if (thisaudio->effect[1] != NULL)
			{
				index = (*thisaudio->effect[1]).index;
				/*
				 * One inbuf, two outbuf. The effect should have some method of
				 * indicating its desired data flows - mono in/stereo out, or
				 * fully stereo, etc.
				 */
				if (audiomain->palette[index]->flags & BRISTOL_FX_STEREO)
				{
					audiomain->palette[index]->specs->io[0].buf
						= thisaudio->leftbuf;
					audiomain->palette[index]->specs->io[1].buf
						= thisaudio->rightbuf;
					audiomain->palette[index]->specs->io[2].buf
						= thisaudio->leftbuf;
					audiomain->palette[index]->specs->io[3].buf
						= thisaudio->rightbuf;
				} else {
					audiomain->palette[index]->specs->io[0].buf
						= thisaudio->leftbuf;
					audiomain->palette[index]->specs->io[1].buf
						= thisaudio->leftbuf;
					audiomain->palette[index]->specs->io[2].buf
						= thisaudio->rightbuf;
				}

				if ((thisaudio->firstVoice == NULL)
					|| (thisaudio->firstVoice->baudio == 0))
				{
					thisaudio = thisaudio->next;
					continue;
				}
#warning - this voice may have been reassigned. check midi channel
				if (thisaudio->firstVoice != NULL)
					(*thisaudio->effect[1]).operate(
						audiomain->palette[index],
						thisaudio->firstVoice,
						(*thisaudio->effect[1]).param,
						thisaudio->FXlocals[1][1]);
			}

			/*
			 * If we have FX, apply them. They will all be stereo, and will
			 * eventually also be chained.
			 */
			rightch = thisaudio->rightbuf;
		} else {
			if (thisaudio->mixflags & BRISTOL_STEREO)
				rightch = thisaudio->rightbuf;
			else
				/*
				 * Otherwise the synth is mono.
				 */
				rightch = thisaudio->leftbuf;
		}

		leftch = thisaudio->leftbuf;

		extmult = outbuf;

		gain = thisaudio->gain;

/*
		if ((gain = thisaudio->gain) == 0)
			gain = 1.0;
*/

		/*
		 * Interleave the results for output to a stereo device.
		 *
		 * This must change. It will consist of a couple of values that are
		 * going to be GM-2: gain and balance. These will be used per synth to
		 * position the output into a pair of buffers, interleaving will be
		 * handled by the bristol audio library if it is needed for an audio
		 * device.
		 */
		for (i = 0; i < audiomain->samplecount; i+=8)
		{
			*extmult++ += *leftch++ * gain; *extmult++ += *rightch++ * gain;
			*extmult++ += *leftch++ * gain; *extmult++ += *rightch++ * gain;
			*extmult++ += *leftch++ * gain; *extmult++ += *rightch++ * gain;
			*extmult++ += *leftch++ * gain; *extmult++ += *rightch++ * gain;
			*extmult++ += *leftch++ * gain; *extmult++ += *rightch++ * gain;
			*extmult++ += *leftch++ * gain; *extmult++ += *rightch++ * gain;
			*extmult++ += *leftch++ * gain; *extmult++ += *rightch++ * gain;
			*extmult++ += *leftch++ * gain; *extmult++ += *rightch++ * gain;
		}

		bristolbzero(thisaudio->leftbuf, audiomain->segmentsize);
		bristolbzero(thisaudio->rightbuf, audiomain->segmentsize);

		thisaudio = thisaudio->next;
	}
	return(0);
}

/*
 * The routines fillFreqTable and fillFreqBuf need to be consolidated to one
 * routine. Minimally cFreq/dFreq and dfreq/dfreq need to be maintained.
 *
 * This fills the table with frequencies. The alternative fillFreqTable() will
 * insert sample steps through the 1K wavetable oscillators. This call is used
 * for the tendency tables which are frequency based.
 */
int
fillFreqBuf(Baudio *baudio, register bristolVoice *voice, register float *buf,
	register int size, int glide)
{
	register int i;

	/*
	 * Take a buffer, and fill in some frequency value to make an oscillator
	 * reproduce the correct note. Will cater for polyphonic portamento, and
	 * eventually also for some generic mods.
	 */
	if ((voice->cfreq == voice->dfreq) || (glide == 0) || (voice->lastkey == 0)
		|| (baudio->glide == 0.0f))
	{
		/*
		 * Note has no glide, just fill the table.
		 */
		for (i = 0; i < size; i+=8)
		{
			*buf++ = voice->dfreq;
			*buf++ = voice->dfreq;
			*buf++ = voice->dfreq;
			*buf++ = voice->dfreq;
			*buf++ = voice->dfreq;
			*buf++ = voice->dfreq;
			*buf++ = voice->dfreq;
			*buf++ = voice->dfreq;
		}
	} else {
		if (voice->cfreq < voice->dfreq) {
			/*
			 * Glide frequency up at some rate.
			 */
			if (voice->cfreqmult < 1.0f)
				/*
				 * Frequency inversion, probably due to pitchbend, we have to
				 * recalculate cfreqmult. This does lead to some stretching of
				 * the overall glide.
				 */
				voice->cfreqmult = powf(M_E, logf(voice->dfreq/voice->cfreq)
					/ (baudio->glide * baudio->samplerate));

			if ((voice->cfreqmult >=    0.999999f)
				&& (voice->cfreqmult <= 1.000001f))
			{
				voice->cFreq = voice->dFreq;
				voice->cfreq = voice->dfreq;
				for (i = 0; i < size; i+=8)
				{
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
				}
				return(0);
			}

			for (i = 0; i < size; i++)
			{
				if (voice->cfreq > voice->dfreq)
				{
					voice->cfreq = voice->dfreq;
					voice->cFreq = voice->dFreq;
					for (; i < size; i++)
						*buf++ = voice->cfreq;
					break;
				}
				/*
				 * Hm, this is incorrect, it should be a multiplier.....
				 */
				*buf++ = (voice->cfreq *= voice->cfreqmult);
			}
		} else {
			/*
			 * Glide frequency down at some rate.
			 */
			if (voice->cfreqmult > 1.0f)
				/*
				 * Frequency inversion, probably due to pitchbend, we have to
				 * recalculate cfreqmult. This does lead to some stretching of
				 * the overall glide.
				 */
				voice->cfreqmult = powf(M_E, logf(voice->dfreq/voice->cfreq)
					/ (baudio->glide * baudio->samplerate));

			if ((voice->cfreqmult >=    0.999999f)
				&& (voice->cfreqmult <= 1.000001f))
			{
				voice->cFreq = voice->dFreq;
				voice->cfreq = voice->dfreq;
				for (i = 0; i < size; i+=8)
				{
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
					*buf++ = voice->dfreq;
				}
				return(0);
			}

			for (i = 0; i < size; i++)
			{
				if (voice->cfreq < voice->dfreq)
				{
					voice->cfreq = voice->dfreq;
					voice->cFreq = voice->dFreq;
					for (; i < size; i++)
						*buf++ = voice->cfreq;
					break;
				}
				*buf++ = (voice->cfreq *= voice->cfreqmult);
			}
		}
	}

//	printf("freqbuf: %f %f %f %f %i\n",
//		voice->cFreq, voice->dFreq, voice->cFreqmult,
//		baudio->glide, baudio->samplerate);

	return(0);
}

/*
 * The implementation of glide is actually damaged for some synths, they may 
 * call this routine more than once.
 */
int
fillFreqTable(Baudio *baudio, register bristolVoice *voice, register float *buf,
	register int size, int glide)
{
	register int i;

	/*
printf("fft %f %f %1.12f\n", voice->cFreq, voice->dFreq, voice->cFreqmult);
	 * Take a buffer, and fill in some frequency value to make an oscillator
	 * reproduce the correct note. Will cater for polyphonic portamento, and
	 * eventually also for some generic mods.
	 */
	if ((voice->cFreq == voice->dFreq) || (glide == 0)
		|| (voice->lastkey == 0) || (baudio->glide == 0.0f))
	{
//printf("fft %f %f %i\n", voice->cFreq, voice->dFreq, voice->lastkey);
		/*
		 * Note has no glide, just fill the table.
		 */
		for (i = 0; i < size; i+=8)
		{
			*buf++ = voice->dFreq;
			*buf++ = voice->dFreq;
			*buf++ = voice->dFreq;
			*buf++ = voice->dFreq;
			*buf++ = voice->dFreq;
			*buf++ = voice->dFreq;
			*buf++ = voice->dFreq;
			*buf++ = voice->dFreq;
		}
	} else {
		if (voice->cFreq < voice->dFreq) {
			/*
			 * Glide frequency up at some rate.
			 */
			if (voice->cFreqmult < 1.0f)
				/*
				 * Frequency inversion, probably due to pitchbend, we have to
				 * recalculate cfreqmult. This does lead to some stretching of
				 * the overall glide.
				 */
				voice->cFreqmult = powf(M_E, logf(voice->dFreq/voice->cFreq)
					/ (baudio->glide * baudio->samplerate));

//printf("1: inversion %f %i\n", voice->cFreqmult, voice->cFreqmult == 1.0);
//limit 0.99999999
			if ((voice->cFreqmult >=    0.999999f)
				&& (voice->cFreqmult <= 1.000001f))
			{
				voice->cFreq = voice->dFreq;
				for (i = 0; i < size; i+=8)
				{
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
				}
				return(0);
			}

			for (i = 0; i < size; i++)
			{
				if (voice->cFreq >= voice->dFreq)
				{
					voice->cFreq = voice->dFreq;
					voice->cfreq = voice->dfreq;
					for (; i < size; i++)
						*buf++ = voice->cFreq;
					break;
				}
				*buf++ = (voice->cFreq *= voice->cFreqmult);
			}
		} else {
			/*
			 * Glide frequency down at some rate.
			 */
			if (voice->cFreqmult > 1.0f)
				/*
				 * Frequency inversion, probably due to pitchbend, we have to
				 * recalculate cfreqmult. This does lead to some stretching of
				 * the overall glide.
				 */
				voice->cFreqmult = powf(M_E, logf(voice->dFreq/voice->cFreq)
					/ (baudio->glide * baudio->samplerate));

//printf("2: inversion %f %i\n", voice->cFreqmult, voice->cFreqmult == 1.0);
			if ((voice->cFreqmult >=    0.999999f)
				&& (voice->cFreqmult <= 1.000001f))
			{
				voice->cFreq = voice->dFreq;
				for (i = 0; i < size; i+=8)
				{
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
					*buf++ = voice->dFreq;
				}
				return(0);
			}

			for (i = 0; i < size; i++)
			{
				if (voice->cFreq < voice->dFreq)
				{
					voice->cFreq = voice->dFreq;
					voice->cfreq = voice->dfreq;
					for (; i < size; i++)
						*buf++ = voice->cFreq;
					break;
				}
				*buf++ = (voice->cFreq *= voice->cFreqmult);
			}
		}
	}

//	printf("freqtab: %f %f %f %f %i\n",
//		voice->cFreq, voice->dFreq, voice->cFreqmult,
//		baudio->glide, baudio->samplerate);

	return(0);
}

/*
 * Take a buffer and fill it with a sinewave at 440 hz.
 */
static void
a440(audioMain *audiomain, float *buf, int count)
{
	float *sine, index = s440holder, step;
	int obp;

	/*
	 * There is a sine wave in buffer zero of the first operator - the DCO.
	 */
	sine = ((bristolDCO *)audiomain->palette[0]->specs)->wave[0];
	step = defaultTable[69].defnote;

	for (obp = 0; obp < count * 2;)
	{
		buf[obp++] = sine[(int) index] * 48;
		buf[obp++] = sine[(int) index] * 48;
		if ((index += step) > DCO_WAVE_SZE)
			index -= DCO_WAVE_SZE;
	}

	s440holder = index;
}

