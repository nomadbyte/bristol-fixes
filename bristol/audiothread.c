
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "bristol.h"
#include "bristolmidi.h"
#include "palette.h"

#ifdef BRISTOL_PA
extern int bristolPulseInterface();
#endif

#ifdef _BRISTOL_JACK
extern int bristolJackInterface();
#endif

/*int atStatus = 0; */

extern int dupfd;
extern char *outputfile;
extern int buildCurrentTable(Baudio *, float);
extern void initMicrotonalTable(fTab []);

static void initPalette();
static void freePalette();
static void resetAudioThread();
static void initMidiVoices();

void mapVelocityCurve(int, float []);

void llgain(float *, int, float);

/*Baudio *baudio; */

void
audioThread(audioMain *audiomain)
{
	int i, rr, pthreadstatus;
	float *outbuf, *startbuf;
	char *device;

	audiomain->atStatus = BRISTOL_WAIT;

	audiomain->debuglevel = 0;
#ifdef DEBUG
	audiomain->debuglevel = -1;
#endif

	if (outputfile != NULL)
	{
		if ((dupfd = open(outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0)
			printf("failed to open duplicate output file %s\n", outputfile);
	}

	if (audiomain->audiodev != (char *) NULL)
		device = audiomain->audiodev;
	else if ((audiomain->flags & BRISTOL_AUDIOMASK) == BRISTOL_OSS)
		device = bOAD;
#if (BRISTOL_HAS_ALSA == 1)
	else
		device = bAAD;
#endif

	audiomain->opCount = BRISTOL_SYNTHCOUNT;

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("starting audio thread\n");

	/*
	 * Due to the nature of the jack interface, we need to have this file be
	 * minimally jack aware. The rest of the code takes an audio device, does
	 * its own read and write calls, then dispatches to doAudioOps(). This is
	 * not the same with jack, we register a callback and let it do the work
	 * for us.
	 */

	if (audiomain->flags & BRISTOL_PULSE)
	{
#ifdef BRISTOL_PA
		while (audiomain->atReq != BRISTOL_REQSTOP)
		{
			/*
			 * This will not return except when problem arise with the jack
			 * interface. The returning code will decide whether to flag a
			 * reqstop to exit
			 */
			bristolPulseInterface(audiomain);

			audiomain->atStatus = BRISTOL_EXIT;

			printf("pulse audio interface returned\n");
#ifdef MONOLITHIC
			pthread_exit(&pthreadstatus);
#else
			_exit(0);
#endif
		}

		return;
#else
		printf("pulse requested but not compiled with engine\n");

		audiomain->atStatus = BRISTOL_EXIT;

		_exit(0);
#endif /* BRISTOL_PA */
	}

	if (audiomain->flags & BRISTOL_JACK)
	{
#ifdef _BRISTOL_JACK
		while (audiomain->atReq != BRISTOL_REQSTOP)
		{
			/*
			 * This will not return except when problem arise with the jack
			 * interface. The returning code will decide whether to flag a
			 * reqstop to exit
			 */
			bristolJackInterface(audiomain);

			audiomain->atStatus = BRISTOL_EXIT;

			printf("jack audio interface returned\n");

			_exit(0);
		}

		return;
#else
		printf("jack requested but not compiled with engine\n");

		audiomain->atStatus = BRISTOL_EXIT;

		_exit(0);
#endif /* _BRISTOL_JACK */
	}

	/*
	 * open the audio device with our desired buffer size, and take its
	 * returned fragment size.
	 */
	if ((audiomain->iosize = bristolAudioOpen(device, audiomain->samplerate,
		audiomain->samplecount, audiomain->flags|audiomain->preload))
			< 0)
	{
		/*
		 * If we have Jack support but have not request jack audio then this
		 * failure is typically due to not giving the -jack option. Chekc for
		 * it and report.
		 */
#ifdef _BRISTOL_JACK
		if (~audiomain->flags & BRISTOL_JACK)
		{
			printf("Failed to open audio device %s\n", device);
			printf("If jack is running then use 'startBristol -jack'\n");
		} else
#endif
			printf("Problem opening audio device %s, exiting audio thread\n",
				device);
		audiomain->atStatus = BRISTOL_FAIL;
		pthread_exit(&pthreadstatus);
	}

	/*
	 * This fragment size is in bytes. The audio will be stereo shorts, so
	 * find out how many samples this is:
	 *	samplecount/2
	 * Our samples are going to be floats, and Mono until the later stages?
	 */
	audiomain->segmentsize = audiomain->samplecount * sizeof(float);

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("segmentsize is %i\n", audiomain->segmentsize);

	/*
	 * A segment is enough for the given number of mono samples in float res.
	 * Our IO buffers deal with stereo, so we need twice as many.
	 *
	 * This should be samplecount * periodsize bytes.
	 */
	outbuf = (float *) bristolmalloc(audiomain->segmentsize * 2);

	/* This will give 2 deinterleaved sample streams. */
	startbuf = (float *) bristolmalloc(audiomain->segmentsize * 2);

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("IO %i, samples %i, segsize %i\n",
			audiomain->iosize, audiomain->samplecount, audiomain->segmentsize);

	initAudioThread(audiomain);

	bristolbzero(outbuf, audiomain->segmentsize * 2);

	/*
	 * Preload the output
	 */
	for (i = 0; i < audiomain->preload; i++)
		bristolAudioWrite(outbuf, audiomain->samplecount);

	bristolAudioStart();

	audiomain->atStatus = BRISTOL_OK;

	while (audiomain->atReq != BRISTOL_REQSTOP)
	{
		/*
		 * We should call here to operate the voices. Suggest we pass a single
		 * destination buffer, which we will write, on return, to the audio
		 * device.
		 */
		if (bristolAudioRead(startbuf, audiomain->samplecount) < 0)
		{
			printf("Audio device read issue\n");

#ifdef _BRISTOL_DRAIN
			/*
			 * This code avoids reopening of the audio interface which was
			 * pretty ugly.
			 */
			bristolAudioStart();
//			for (i = 1; i < audiomain->preload; i++)
//				bristolAudioWrite(outbuf, audiomain->samplecount);
			bristolAudioRead(startbuf, audiomain->samplecount);
#else
			bristolAudioClose();
			if (bristolAudioOpen(device, audiomain->samplerate,
				audiomain->samplecount,
				audiomain->flags|audiomain->preload) < 0)
				/*
				 * Then we have an issue.
				 */
				audiomain->atReq = BRISTOL_REQSTOP;
			else
				/*
				 * Preload the output
				 */
				for (i = 0; i < audiomain->preload; i++)
					bristolAudioWrite(outbuf, audiomain->samplecount);

			bristolAudioStart();
			bristolAudioRead(startbuf, audiomain->samplecount);
#endif
		}

		/*
		 * The startbuf is actually interleaved at the moment, something which
		 * needs to be changed.
		 */
		if (audiomain->ingain > 1)
			llgain(startbuf, audiomain->samplecount, audiomain->ingain);

		/*
		 * doAudioOps should not use baudio, these will become a linked list
		 */
		doAudioOps(audiomain, outbuf, startbuf);

		if (audiomain->outgain > 1)
			llgain(outbuf, audiomain->samplecount, audiomain->outgain);

		if ((rr = bristolAudioWrite(outbuf, audiomain->samplecount)) < 0)
		{
			if (rr != -4)
			{
				printf("Audio device write issue: restart pl %i\n",
					audiomain->preload);
#ifdef _BRISTOL_DRAIN
				/*
				 * This code avoids reopening of the audio interface which was
				 * pretty ugly.
				 */
				bristolAudioStart();
//				for (i = 1; i < audiomain->preload; i++)
//					bristolAudioWrite(outbuf, audiomain->samplecount);
#else
				bristolAudioClose();
				if (bristolAudioOpen(device, audiomain->samplerate,
					audiomain->samplecount,
					audiomain->flags|audiomain->preload) < 0)
					/*
					 * Then we have an issue.
					 */
					audiomain->atReq = BRISTOL_REQSTOP;
				else
					/*
					 * Preload the output
					 */
					for (i = 0; i < audiomain->preload; i++)
						bristolAudioWrite(outbuf, audiomain->samplecount);

				bristolAudioStart();
#endif
			}
		}
	}

	bristolfree(outbuf);
	bristolfree(startbuf);

	resetAudioThread(audiomain);

	bristolAudioClose();

	printf("audioThread exiting\n");

	audiomain->atStatus = BRISTOL_EXIT;
	pthread_exit(&pthreadstatus);
}

/*
 * This assumes a stereo channel.....
 */
#warning llgain() assumes a stereo channel.
void
llgain(register float *buf, register int count, register float gain)
{
	for (;count > 0; count -= 8)
	{
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
		*buf++ *= gain;
	}
}

extern void doPitchWheel(Baudio *);

int
bristolMidiController(Baudio *baudio, int NRP, float value)
{
	if (baudio->midiflags & BRISTOL_MIDI_DEBUG2)
		printf("bristolMidiController(%i, %f)\n", NRP, value);

	/*
	 * We are concerned with the GM RP values for
	 *
	 *	PW Range (0)
	 *	Fine Tune (1)
	 *	Coarse Tune (2)
	 *
	 * Due to the emulation methods where each emulation is actually a different
	 * synth I am not certain what abstraction to use here, it could be per
	 * emulation or globally? Since the messages are associated with a MIDI
	 * channel then I think we should maintain. These messages are sent by the
	 * library to each emulation on a given channel, implying that it should
	 * not be global.
	 *
	 * There should also be an RP for Mod Wheel depth.
	 * Pitch wheel depth is only implemented in notes, not cents.
	 */
	switch (NRP) {
		case MIDI_RP_PW:
			baudio->midi_pitch = value * 16384.0f;
			doPitchWheel(baudio);
			alterAllNotes(baudio);
			return(0);
		case MIDI_RP_FINETUNE:
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);
			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			return(0);
		case MIDI_RP_COARSETUNE:
			baudio->finetune = value * 16383.0f;
			return(0);
		case BRISTOL_NRP_VELOCITY:
			mapVelocityCurve(value * CONTROLLER_RANGE, &baudio->velocitymap[0]);
			return(0);
		case BRISTOL_NRP_NULL:
			return(0);
		case BRISTOL_NRP_DETUNE:
			baudio->detune = value;
			/* We should apply it to the list of voices that has this baudio */
			return(0);
		case BRISTOL_NRP_GAIN:
			baudio->gain = value * 64;
			return(0);
		case BRISTOL_NRP_GLIDE:
			baudio->glidemax = value * 30;
			return(0);
		case BRISTOL_NRP_LWF:
			if (baudio->midiflags & BRISTOL_MIDI_DEBUG2)
				printf("LWF requested as %i\n", (int) (value * 16384.0f));
			baudio->mixflags &= ~(BRISTOL_LW_FILTER|BRISTOL_LW_FILTER2);
			switch ((int) (value * 16384.0f)) {
				case 0: /* was heavyweight, now medium */
					break;
				case 1:
					/* Chamberlains - not changed */
					baudio->mixflags |= BRISTOL_LW_FILTER;
					break;
				case 2:
					/* -lwf2 */
					baudio->mixflags |= BRISTOL_LW_FILTER2;
					break;
				case 3:
					/* Now -hwf heavyweight */
					baudio->mixflags |= (BRISTOL_LW_FILTER|BRISTOL_LW_FILTER2);
					break;
			}
			return(0);
		case BRISTOL_NRP_MNL_PREF:
			baudio->notemap.flags &= ~(BRISTOL_MNL_LNP|BRISTOL_MNL_HNP);
			if ((int) (value * CONTROLLER_RANGE) == 1)
				baudio->notemap.flags |= BRISTOL_MNL_LNP;
			else if ((int) (value * CONTROLLER_RANGE) == 2)
				baudio->notemap.flags |= BRISTOL_MNL_HNP;
			baudio->notemap.extreme = -1;
			return(0);
		case BRISTOL_NRP_MNL_TRIG:
			if (value != 0)
				baudio->notemap.flags |= BRISTOL_MNL_TRIG;
			else
				baudio->notemap.flags &= ~BRISTOL_MNL_TRIG;
			return(0);
		case BRISTOL_NRP_MNL_VELOC:
			if (value != 0)
				baudio->notemap.flags |= BRISTOL_MNL_VELOC;
			else
				baudio->notemap.flags &= ~BRISTOL_MNL_VELOC;
			return(0);
		case BRISTOL_NRP_MIDI_GO:
			bristolMidiOption(0, BRISTOL_NRP_MIDI_GO,
				(value > 0)? (int) 1: (int) 0);
			return(0);
		case BRISTOL_NRP_FORWARD:
			bristolMidiOption(0, BRISTOL_NRP_FORWARD,
				value > 0? (int) 1: (int) 0);
			return(0);
		case BRISTOL_NRP_DEBUG:
			/*
			 * This is a little unfortunte, we do not have direct access to
			 * the midi library structures here so can only enable MIDI debug
			 * for the emulator. This does not have to be an issue, it allows
			 * us to view what each emulation is delivered by midi through the
			 * bristol API.
			 *
			 * 052009 - integrate code to request the MIDI library include byte
			 * debuging of interfaces.
			 */
			if ((value * C_RANGE_MIN_1) == 0)
				baudio->midiflags &= ~(BRISTOL_MIDI_DEBUG1|BRISTOL_MIDI_DEBUG2);
			else {
				if ((value * C_RANGE_MIN_1) >= 1.99)
					baudio->midiflags |= BRISTOL_MIDI_DEBUG2;
				else
					baudio->midiflags |= BRISTOL_MIDI_DEBUG1;
			}

			bristolMidiOption(0, BRISTOL_NRP_DEBUG,
				((int) (value * C_RANGE_MIN_1)));

			return(0);
		case BRISTOL_NRP_ENABLE_NRP:
			if (value != 0)
				baudio->midiflags |= BRISTOL_MIDI_NRP_ENABLE;
			else
				baudio->midiflags &= ~BRISTOL_MIDI_NRP_ENABLE;
			return(0);
		case BRISTOL_NRP_REQ_SYSEX:
			printf("		REQ SYSEX %f\n", value);
			return(0);
	}

	/*
	 * This should also consider indexing this NRP value into an arbitrary
	 * engine control? This is non-trivial since there is not a table to 
	 * link them together to would have to parse the complete parameter tables
	 * that go to each operator.
	 *
	 * It would make more sense (ie, be easier) to take the two GM2 integer
	 * parameters and break the Data Entry into MSB/LSB and map them to 
	 * operator/controller and then pass them the value.
	 */

	return(0);
}

/*
 * This could also go into the library so the engine and GUI use the same code?
 * It might have been possible other than that the GUI code works with integer
 * values (mappings the actual controller indeces) and this code is for the
 * float curves in the engine, will leave that FFS.
 *
 * We want to go through the midi controller mapping file for this synth and
 * search for directives for value maps. The names are taken from the midi
 * header file and we want to add a few others for preconfigured value tables.
 */
static void
invertMap(float map[128])
{
	int j;

	for (j = 0; j < 128; j++)
		map[j] = (1.0 - map[j]);
}

/*
static void
linearMap(float map[128])
{
	int j;

	for (j = 0; j < 128; j++)
		map[j] = ((float) j) / 127.0;
}
*/

/*
 * Takes a parameter for a start point, plus a ramp rate that is greater
 * than linear to build tables that map to different playing styles. These
 * should be extended to include curves as well.
 * Each parameter takes 8 value.
 */
static void
nonSensitiveMap(float map[128], int start, int end)
{
	int j;

	for (j = 0; j < 128; j++)
		map[j] = 0.71f;
}

static void
linearScaledMap(float map[128], int start, int end)
{
	int j;
	float inc;

	/* Start can be up to halfway through existing scale */
	start = start * 64 / 9;
	end = end * 64 / 9 + 64;

	inc = 1.0 / ((float) (end - start));

	for (j = 0; j < 128; j++)
	{
		if (j > start) {
			if ((map[j] = ((float) (j - start)) * inc) > 1.0)
				map[j] = 1.0;
		} else
			map[j] = 0.0;
	}
}

static void
logMap(float map[128])
{
	int i;
	float scaler = 127 / logf(128.0);

	/*
	 * We are going to take a logarithmic scale such that values go from 0 to
	 * 127
	 */
    for (i = 1; i < 129; i++)
		map[i - 1] = (logf((float) i) * scaler) / 127.0;
}

static void
exponentialMap(float map[128], float power)
{
	int i;
	float scaler = 1 / powf(127, power);

	/*
	 * We are going to take exponential scale
	 */
    for (i = 0; i < 128; i++)
		map[i] = powf((float) i, power) * scaler;
}

static void
parabolaMap(float map[128])
{
	int i;
	float scaler = 1.0 / (64.0 * 64.0);

	/*
	 * We are going to take a parabolic scale. Not sure if it will be use....
	 */
    for (i = 0; i < 128; i++)
		map[i] = (float) (((float) ((i - 64) * (i - 64))) * scaler);
}

#define V_COUNT 500
#define R_COUNT 1000
void
mapVelocityCurve(int velocity, float map[128])
{
	int v = velocity, r = 0;

	if (velocity > R_COUNT) {
		v = velocity - R_COUNT;
		r = 1;
	}

	/*
	 * The first 100 are linear scaled maps. The second 100 should be
	 * exponentially scaled ones?
	 */
	if (v == 0) {
		nonSensitiveMap(map, 0, 127);
	} else if (v < 100) {
		linearScaledMap(map, v / 10, v % 10);
	} else if (v < 200) {
		/*
		 * This are exponential maps from powers of 0.01 up to 1.0 (linear) in
		 * 100 steps.
		 */
		exponentialMap(map, ((float) (v - 100)) / 100.0);
	} else if (v < V_COUNT) {
		/*
		 * This are exponentials from ~linear up to quadratic in steps of 
		 * 100th's.
		 */
		exponentialMap(map, ((float) (v - 100)) / 100.0);
	} else {
		switch (v) {
			case V_COUNT + 0: exponentialMap(map, 0.05); break; /* Soft touch */
			case V_COUNT + 1: exponentialMap(map, 0.1); break;
			case V_COUNT + 2: exponentialMap(map, 0.2); break;
			case V_COUNT + 3: exponentialMap(map, 0.3); break;
			case V_COUNT + 4: exponentialMap(map, 0.4); break;
			case V_COUNT + 5: exponentialMap(map, 0.5); break;
			case V_COUNT + 6: exponentialMap(map, 0.6); break;
			case V_COUNT + 7: exponentialMap(map, 0.7); break;
			case V_COUNT + 8: exponentialMap(map, 0.8); break;
			case V_COUNT + 9: exponentialMap(map, 0.9); break;
			default:
			case V_COUNT + 10: exponentialMap(map, 1.0); break; /* Linear */
			case V_COUNT + 11: exponentialMap(map, 1.1); break;
			case V_COUNT + 12: exponentialMap(map, 1.2); break;
			case V_COUNT + 13: exponentialMap(map, 1.3); break;
			case V_COUNT + 14: exponentialMap(map, 1.4); break;
			case V_COUNT + 15: exponentialMap(map, 1.5); break;
			case V_COUNT + 16: exponentialMap(map, 1.6); break;
			case V_COUNT + 17: exponentialMap(map, 1.7); break;
			case V_COUNT + 18: exponentialMap(map, 1.8); break;
			case V_COUNT + 19: exponentialMap(map, 1.9); break;
			case V_COUNT + 20: exponentialMap(map, 2.0); break;
			case V_COUNT + 21: exponentialMap(map, 2.2); break;
			case V_COUNT + 22: exponentialMap(map, 2.4); break;
			case V_COUNT + 23: exponentialMap(map, 2.6); break;
			case V_COUNT + 24: exponentialMap(map, 2.8); break;
			case V_COUNT + 25: exponentialMap(map, 3.0); break; /* Hard touch */

			case V_COUNT + 50: logMap(map); break;
			case V_COUNT + 60: parabolaMap(map); break;
		}
	}

	if (r) invertMap(map);
}

/*
 * This was initiallly a call made in the MIDI thread. That naturally led to 
 * problems with timing and was eventually moved as a request from the MIDI
 * thread to the audio thread.
 */
void
initBristolAudio(audioMain *audiomain, Baudio *baudio)
{
	int i, j;
	bristolOP *op;
	char name[256];
	float tmap[128];

	if (baudio == NULL)
		return;

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("initBristolAudio()\n");

	if (audiomain->atStatus != BRISTOL_OK)
		return;

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG3)
		printf("using baudio: %p->%p\n", audiomain->audiolist,
			baudio->next);

	baudio->leftbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	baudio->rightbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->glidemax = 5;

	/*
	 * These will come from some configuration file, eventually.
	 */
	baudio->note_diff = pow((double) 2, (double) 1/12);
	baudio->gain_diff = pow((double) 13, ((double) 1)/DEF_TAB_SIZE);
	/* Pitchwheel defaults to 2 semitones although some synths override it. */
	baudio->midi_pitch = 2;
	baudio->midi_pitchfreq = 0.0;
	baudio->gtune = 1.0;
	baudio->glide = 0.0;
	buildCurrentTable(baudio, 1.0);

	baudio->contcontroller[1] = 0.5;
	baudio->pitchwheel = 0.0;

	/*
	 * This should call an init routine for any given baudio. If it is not
	 * initialised then we have an issue and should really return a negative
	 * response.
	 */
	if (bristolAlgos[(audiomain->midiflags & BRISTOL_PARAMMASK)].initialise)
		bristolAlgos[(audiomain->midiflags & BRISTOL_PARAMMASK)].initialise
			(audiomain, baudio);

	/*
	 * All of this could go into the midithread?
	 */
	if (bristolAlgos[(audiomain->midiflags & BRISTOL_PARAMMASK)].name != NULL)
	{
		char file[256];
		float velocity;
		int vint;

		/*
		 * Get hold of any local configuration, microtunings, etc.
		 * I would like these to come from the profile for that synth. We 
		 * should first try a local file, then a general one?
		 */
		sprintf(file, "%s.mcm", bristolAlgos[
			(audiomain->midiflags & BRISTOL_PARAMMASK)].name);

		bristolGetFreqMap(file, "microTonalMap",
			baudio->microtonalmap, 128, 0, audiomain->samplerate);

		/* initMicrotonalTable(&baudio->microtonalmap[0]); */

		if (baudio->microtonalmap[0].step == baudio->microtonalmap[127].step)
		{
			/*
			 * We could consider promoting the audiomain global mapping at
			 * this point? The main issue is that we will probably eventually
			 * have to support at least per key remapping so having a private
			 * map will be a requirement.
			 */
			if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG4)
				printf("no private microtonal mapping for %s\n",
					bristolAlgos[(audiomain->midiflags & BRISTOL_PARAMMASK)].name);
			initMicrotonalTable(&baudio->microtonalmap[0]);
		} else
			printf("micro first %f last %f (%s)\n",
				baudio->microtonalmap[0].step, baudio->microtonalmap[127].step,
				bristolAlgos[(audiomain->midiflags & BRISTOL_PARAMMASK)].name);

		/* 
		 * See if we have been requested a specific velocity curve. This 
		 * could be moved into the library.....
		 */
		if (bristolGetMap(file, "velocityTable", &velocity, 1, 0) != 0)
		{
			vint = (int) velocity;
			mapVelocityCurve(vint, &baudio->velocitymap[0]);
		} else if (bristolGetMap(file, "velocityMap",
			&baudio->velocitymap[0], 128, 0) == 0)
			/*
			 * If we do not have some mapping we should default to a linear
			 * map
			 */
			mapVelocityCurve(510, &baudio->velocitymap[0]);

		if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG4)
			printf("veloc first %1.2f last %1.2f\n",
				baudio->velocitymap[0], baudio->velocitymap[127]);
	}

	/*
	 * Put in a locals array pointer. This is two dimensional, since the locals
	 * are in the audio structure, but we need a copy for each individual voice.
	 *
	 * The net result is that locals at the baudio level is a pointer to an 
	 * array of pointers to arrays pointing to chars.
	 */
	baudio->locals = (char ***)
		bristolmalloc0(sizeof(void *) * BRISTOL_MAXVOICECOUNT);
	baudio->FXlocals = (char ***)
		bristolmalloc0(sizeof(void *) * BRISTOL_MAXVOICECOUNT);

	for (i = 0; i < audiomain->voiceCount; i++)
	{
		baudio->locals[i] =
			(void *) bristolmalloc0(baudio->soundCount * sizeof(void *));

		baudio->FXlocals[i] =
			(void *) bristolmalloc0(baudio->soundCount * sizeof(void *));

		if (audiomain->effects == NULL) {
			int i = 5;

			printf("Null audio FX list\n");

			while (audiomain->effects == NULL) {
				sleep(1);
				if (--i == 0)
				{
					printf("Null audio FX list unrecovered audio thread\n");
					exit(0);
				}
			}

			printf("recovered\n");
		}

		if ((baudio->effect != NULL) && (baudio->effect[0] != NULL)
			&& (baudio->soundCount != 0))
		{
			op = (audiomain->effects)[(*baudio->effect[0]).index];
			if (op && op->specs)
				baudio->FXlocals[i][0]
					= (char *) bristolmalloc0(op->specs->localsize);
			else
				baudio->FXlocals[i][0] = (char *) bristolmalloc0(256);
		}

		if ((baudio->soundCount > 1)
			&& (baudio->effect != NULL) && (baudio->effect[1] != NULL))
		{
			op = (audiomain->effects)[(*baudio->effect[1]).index];
			baudio->FXlocals[i][1]
				= (char *) bristolmalloc0(op->specs->localsize);
		}

		/*
		 * Put in the locals.
		 */
		for (j = 0; j < baudio->soundCount; j++)
		{
			if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
				printf("Adding new audio operator: %i\n", j);

			op = (audiomain->palette)[(*baudio->sound[j]).index];
			baudio->locals[i][j]
				= (char *) bristolmalloc0(op->specs->localsize);

			if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG4)
				printf("malloc locals %p %i\n", baudio->locals[i][j],
					op->specs->localsize);
		}
	}

	/*
	 * We give a NULL name in the engine: these are all GUI mappings but the
	 * call will init the tables to defaults (linear).
	 */
	bristolMidiValueMappingTable(baudio->valuemap, baudio->midimap, "null");
//		bristolAlgos[(audiomain->midiflags & BRISTOL_PARAMMASK)].name);
	/* We should do a getmap for the modwheel */
	sprintf(name, "%s.mcm",
		bristolAlgos[(audiomain->midiflags & BRISTOL_PARAMMASK)].name);

	if (bristolGetMap(name, "modWheel", tmap, 128, 0) > 0)
	{
		for (i = 0; i < 128; i++)
			baudio->valuemap[1][i] = (int) tmap[i];
	}

	baudio->midi = bristolMidiController;
	baudio->mixflags &= ~BRISTOL_HOLDDOWN;
	baudio->lowkey = 0;
	baudio->highkey = 127;
	audiomain->flags &= ~BRISTOL_AUDIOWAIT;
}

void
initAudioThread(audioMain *audiomain)
{
	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("initAudioThread(%i)\n", audiomain->opCount);

	/*
	 * Assign an array of operator pointers.
	 */
	audiomain->palette = (bristolOP **)
		bristolmalloc0(sizeof(bristolOP *) * audiomain->opCount);

	/*
	 * Now that we have some basic stuctures we should start initiating the
	 * operators. This is done one by one, for the
	 *
	 *	DCO/DCF/DCA/ADSR/NOISE/LFO/FX/OTHERS
	 *
	 * For extensibility there should be some nice way to configure the list
	 * of operators that will be placed in our palette.
	 */
	initPalette(audiomain, audiomain->palette);

	/*
	 * The effect chain is identical to the palette - they can be used for the
	 * same purpose. The difference is that when they are used as effects, they
	 * will appear on a different list in a bristolAudio structure.
	 */
	audiomain->effects = audiomain->palette;

	/*
	 * Assign an array of voice pointers. Need to call "initMidiVoices()"
	 */
	initMidiVoices(audiomain);
}

void
freeAudioMain(audioMain *audiomain)
{
	printf("freeAudioMain()\n");

	freePalette(audiomain, audiomain->palette);
/*	freePalette(audiomain, audiomain->effects); */

	if (audiomain->palette != (bristolOP **) NULL)
		bristolfree(audiomain->palette);
/*	if (audiomain->effects != (bristolOP **) NULL) */
/*		bristolfree(audiomain->effects); */
}

void
freeBristolAudio(audioMain *audiomain, Baudio *baudio)
{
	bristolVoice *voice = audiomain->freelist;

	if (baudio == (Baudio *) NULL)
		return;

	baudio->mixflags |= BRISTOL_HOLDDOWN;

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("freeBristolAudio(%p, %p)\n", audiomain, baudio);

	freeSoundAlgo(baudio, 0, baudio->sound);
	bristolfree(baudio->sound);

	if (baudio->effect != NULL)
	{
		freeSoundAlgo(baudio, 0, baudio->effect);
		bristolfree(baudio->effect);
		baudio->effect = NULL;
	}

	if (baudio->locals != NULL)
	{
		int i, j = 0;

		for (i = 0; i < audiomain->voiceCount; i++)
		{
			/*
			 * Free the inividual locals.
			 */
			if (baudio->FXlocals[i][0] == NULL)
				continue;

			for (j = 0; j < baudio->soundCount; j++) {
				bristolfree(baudio->locals[i][j]);
				bristolfree(baudio->FXlocals[i][j]);
			}

			/*
			 * Free the pointer for this voice.
			 */
			bristolfree(baudio->locals[i]);
			bristolfree(baudio->FXlocals[i]);
		}
	}

	bristolfree(baudio->leftbuf);
	bristolfree(baudio->rightbuf);

	/*
	 * Free the locals pointer itself.
	 */
	bristolfree(baudio->locals);
	bristolfree(baudio->FXlocals);

	if (baudio->next != NULL)
		baudio->next->last = baudio->last;

	if (baudio->last != NULL)
		baudio->last->next = baudio->next;
	else
		audiomain->audiolist = baudio->next;

	while (voice != NULL)
	{
		if (voice->baudio != NULL)
		{
			if (voice->baudio->controlid == baudio->controlid)
			{
				voice->flags |= BRISTOL_KEYDONE;
				voice->baudio = NULL;
			}
		} else
			voice->flags |= BRISTOL_KEYDONE;

		voice = voice->next;
	}

	if (baudio->destroy != NULL)
		baudio->destroy(audiomain, baudio);

	if (baudio->mixlocals != NULL)
		bristolfree(baudio->mixlocals);

	/*
	 * This can only be freed once exit handling has been done by the audio
	 * engine and MIDI event management code, refer to audioEngine.c and 
	 * bristolsystem.c
	bristolfree(baudio);
	 */
}

void
freeMidiVoices(audioMain *audiomain, bristolVoice *voice)
{
/*	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1) */
/*		printf("freeMidiVoices(%x, %x)\n", audiomain, voice); */

	if (voice == NULL)
		return;

	freeMidiVoices(audiomain, voice->next);

	audiomain->playlist = NULL;
	audiomain->playlast = NULL;
	audiomain->freelist = NULL;
	audiomain->freelast = NULL;
	audiomain->newlist = NULL;
}

static void
resetAudioThread(audioMain *audiomain)
{
	Baudio *holder;

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("resetAudioThread()\n");

	while (audiomain->audiolist != NULL) {
		holder = audiomain->audiolist;
		freeBristolAudio(audiomain, audiomain->audiolist);
		bristolfree(holder);
	}

	/*
	 * Free the array of voice pointers.
	 */
	freeMidiVoices(audiomain, audiomain->playlist);
}

static void
initPalette(audioMain *audiomain, bristolOP *palette[])
{
	int i;

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("initPalette()\n");

	/* 
	 * Go through the palette, and initialise all our operators.
	 */
	for (i = 0; i < BRISTOL_SYNTHCOUNT; i++)
	{
		if (bristolPalette[i].initialise == NULL)
			continue;

		palette[i] = bristolPalette[i].initialise
			(&palette[i], i, audiomain->samplerate,
				audiomain->samplecount);

		if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG3)
			printf("Assigned operator %s to %i at %p\n",
				palette[i]->specs->opname, i, palette[i]);
		if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG4)
			bristolOPprint(palette[i]);
	}
	audiomain->opCount = i - 1;
}

static void
freePalette(audioMain *audiomain, bristolOP *palette[])
{
	int i;

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("freePalette()\n");

	/* 
	 * This needs to be sorted out when we move on to operators other than the
	 * DCO!
	 */
	for (i = 0; i < audiomain->opCount; i++)
	{
		if (palette[i] != (bristolOP *) NULL)
		{
			if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG3)
				printf("Removed operator %s from %i at %p\n",
					palette[i]->specs->opname, i, palette[i]);
//printf("Removed operator %s from %i at %x\n",
//palette[i]->specs->opname, i, (size_t) palette[i]);

			palette[i]->destroy(palette[i]);

			palette[i] = (bristolOP *) NULL;
		}
	}
}

static void
initMidiVoices(audioMain *audiomain)
{
	int i;
	bristolVoice *voice;

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
		printf("initMidiVoices(%p, %i)\n", audiomain, audiomain->voiceCount);

	audiomain->playlist = NULL;
	audiomain->playlast = NULL;
	audiomain->freelist = NULL;
	audiomain->freelast = NULL;
	audiomain->newlist = NULL;

	/*
	 * Create the voice structures, put them on the playlist with some default
	 * flags, etc.
	 */
	for (i = 0; i < audiomain->voiceCount; i++)
	{
		if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG1)
			printf("Adding new MIDI Voice: %i\n", i);

		voice = (bristolVoice *) bristolmalloc0(sizeof(bristolVoice));

		voice->flags = BRISTOL_KEYDONE;
		voice->last = NULL;
		voice->next = audiomain->freelist;

		/*
		 * Chain it into the voice list.
		 */
		voice->next = audiomain->freelist;
		voice->last = NULL;
		if (audiomain->freelist == NULL)
			audiomain->freelast = voice;
		else
			audiomain->freelist->last = voice;
		audiomain->freelist = voice;

		voice->index = i;
	}
}

/*
 * Change the frequencies of all active notes. Called when global tuning is
 * changed.
 */
extern bristolMidiHandler bristolMidiRoutines;

void
doNoteChanges(bristolVoice *voice)
{
	if (~voice->flags & BRISTOL_KEYDONE)
	{
		/*
		 * First make sure we have no microtonal map.
		 */
		if (voice->baudio->microtonalmap[voice->key.key].step > 0.0) {
			voice->dFreq = voice->baudio->microtonalmap[voice->key.key].step;
			voice->dfreq = voice->baudio->microtonalmap[voice->key.key].freq;
		} else if (bristolMidiRoutines.bmr[7].floatmap[voice->key.key] > 0.0) {
			/*
			 * This is the synth global microtonal map
			 */
			voice->dFreq = bristolMidiRoutines.freq[voice->key.key].step;
			voice->dfreq = bristolMidiRoutines.freq[voice->key.key].freq;
		} else {
			voice->dFreq = voice->baudio->ctab[voice->key.key].step;
			voice->dfreq = voice->baudio->ctab[voice->key.key].freq;
		}

		if (voice->detune != 0.0) {
			voice->dFreq *= voice->detune;
			voice->dfreq *= voice->detune;
		}
	}
}

