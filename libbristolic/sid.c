
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
 * Emulator code for the C64 SID audio chip. All the register numbers and bit
 * masks are defined in bristolsid.h, code will accept the register addressess,
 * parse the register bit values into some internal tables and then allow IO to
 * generate all the signals. The SID chip was partly digital (Osc/Env/Noise)
 * and partly analogue (Filter). This is emulated as integer code and floating
 * point code respectively.
 *
 * This is not a SID player. This file only emulates the chip, a sid player
 * would also be emulating the 6502 CPU and C64 architecture to allow it to
 * correctly interpret .sid files and render them musically. There was a lot
 * of innovative use of the chip by certain programmes in order to create
 * sampling output, alternative sounds, stereo outputs. This * abuse of the
 * chip is naturally not done by the chip itself, and that is all that is
 * emulated here. If you are interested in playing SID files there are a number
 * of excellent SID players available.
 *
 * The emulation uses the same 16 bit phase accumulating 24 bit counters with
 * a 12 bit shift for ramp waves, comparative extraction of pulse waves and 
 * comparative exclusive or'ing for the tri waves.
 *
 * The oscillators only implement hardsync which is a true emulation but is a
 * bit harsh with some (pulse) waveforms. This may be extracted to correct
 * softsync as a bristol configured option in the CONTROL register (FFS).
 *
 * The oscillators respect the TEST bit. They have slightly more artifacts
 * than the original since they are clocked at the native sampling rate, not
 * at 1.02MHz but they respect the frequency tables. There should be emulation
 * of the oscillator leakage that plagued the original and was fixed/reduced
 * by stopping the oscillators when not generating signal by the TEST bit for
 * example or a phase incrementer of 0. The leakage level is programmable.
 *
 * The default oscillator mixing is using the same 'and' function leading to
 * the same strange harmonics if multiple are selected. As an option in the
 * bristol CONTROL register they can be summed for a warmer signal which 
 * when selected should not damage the noise generation code. When multiple
 * waveforms are summed rather than and'ed then they are generated separately
 * rather than extracted from the same phase accumulator and can then be
 * detuned slightly for some extra phasing sounds.
 *
 * Oscillator ringmod will replace the TRI wave output with the ringmod output
 * as per the original ONLY when anding the signals together. When set up for
 * MULTI oscillator from the bristol CONTROL register then ringmod will be
 * summed to the output and TRI can be mixed in separately, this is just a 
 * small change the to digital signal routing sequence.
 *
 * The envelope is generated with the same 8 bit up/down counter although at
 * typical sampling rates (44.1kHz, 48kHz) then the max ramp up has to use a
 * double step increment to maintain the 2ms linear leading edge. The trailing
 * edge is slightly exponential as per the original and changing the sustain
 * level downwards will track using decay, changing it upwards will have no
 * effect if already sustained, ie, the counter cannot count back up to an
 * increased sustain level since when it has transit from count up (attack) to
 * counting down (decay/release) it can only count down to a sustain level that 
 * has been lowered. This is as per the original, and whilst it would have 
 * been easy to change here it was emulated as per the SID design.
 *
 * The filter is a chamberlain 12dB multimode and is probably a bit colder
 * than the original. As an option the LPF can be selected as a houvilainen
 * 24dB/octave, warmer but more CPU intensive. The 24dB version can also give
 * different levels of feedforward mixing of the other poles to also alter the
 * roll-off characteristics. The different dB/octave versions can also be mixed
 * and since the original was very subjective for its filter capabilities by
 * batch the set of options is perhaps reasonable.
 *
 * The filter S/N ratio is configurable.
 */

/*
 * The GUI should give three voices with all the OSC, ENV, RM, SYNC options
 * and then separately the test, multi and detune.
 *
 * Tri/Ramp/Square/Noise
 * Att/Dec/Sust/Rel
 * PW/Tune/transpose(/Glide?)
 * RM/SYNC/Mute/Routing/Multi
 *
 * osc3 -> PWM osc 1/2
 *
 * Filter freq, res, HP/BP/LP inc 24dB and filter routing, should add in option
 * of osc3 and env3 to filter cutoff. Pole mixing level.
 * 
 * Must have S/N, leakage, global tuning. DC bias? Reset?.
 *
 * Voice options to include:
 *
 * 	mono (with detune then between voices in emulator)
 * 	three voice poly-1: all play same sound
 * 	three voice poly-2: all have their own sound
 * 	two voice poly, voice 3 arpeggio - argeggiate all other held notes
 * 	two voice poly, voice 3 mod only
 *
 * Arpeggiator to have rate, retrig and wavescanning, rates down to X samples.
 *
 * We should consider using two SID, the second one could be used just for
 * an extra Env and Osc(LF) to modulate the first one, later look at more 
 * voices or other layer options?
 */

/*
 * Osc ringmod and sync have not been tested.
 * Multi_V phase accumulation code is possibly broken.
 * Detune, leakage need testing.
 * Full routing needs to be confirmed.
 * Filter need to be integrated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>

#include "bristolsid.h"
#include "bristol.h"

extern void * bristolmalloc();
extern void bristolfree();

#define B_SID_RAMP		0
#define B_SID_TRI		1
#define B_SID_SQUARE	2

/*
 * These were millisecond rates for the envelope attack. Decay and release
 * were specified at 3 times all these values however I think that came more
 * from the scaled (exponential) deltas than the actual value, ie, decay times
 * varied by time since the attack was linear and the decay was somewhat
 * arbitrarily exponential which needs to be studied.
 */
static float bSidEnvRates[16] = {
	0.002,
	0.006,
	0.016,
	0.024,
	0.038,
	0.056,
	0.068,
	0.080,
	0.100,
	0.250,
	0.500,
	0.800,
	1.000,
	3.000,
	5.000,
	8.000
};

/*
 * The following table is actually based on a 2MHz divider circuit, it is not
 * actually equally tempered.
 */
unsigned short freqmap[128] = {
	(unsigned short) (8.175879 / B_SID_FREQ_DIV),
	(unsigned short) (8.661983 / B_SID_FREQ_DIV),
	(unsigned short) (9.177091 / B_SID_FREQ_DIV),
	(unsigned short) (9.722803 / B_SID_FREQ_DIV),
	(unsigned short) (10.300889 / B_SID_FREQ_DIV),
	(unsigned short) (10.913456 / B_SID_FREQ_DIV),
	(unsigned short) (11.562431 / B_SID_FREQ_DIV),
	(unsigned short) (12.249948 / B_SID_FREQ_DIV),
	(unsigned short) (12.978416 / B_SID_FREQ_DIV),
	(unsigned short) (13.750051 / B_SID_FREQ_DIV),
	(unsigned short) (14.567703 / B_SID_FREQ_DIV),
	(unsigned short) (15.434004 / B_SID_FREQ_DIV),
	(unsigned short) (16.351892 / B_SID_FREQ_DIV),
	(unsigned short) (17.324118 / B_SID_FREQ_DIV),
	(unsigned short) (18.354349 / B_SID_FREQ_DIV),
	(unsigned short) (19.445795 / B_SID_FREQ_DIV),
	(unsigned short) (20.601990 / B_SID_FREQ_DIV),
	(unsigned short) (21.826912 / B_SID_FREQ_DIV),
	(unsigned short) (23.125130 / B_SID_FREQ_DIV),
	(unsigned short) (24.500196 / B_SID_FREQ_DIV),
	(unsigned short) (25.957170 / B_SID_FREQ_DIV),
	(unsigned short) (27.500481 / B_SID_FREQ_DIV),
	(unsigned short) (29.135832 / B_SID_FREQ_DIV),
	(unsigned short) (30.868008 / B_SID_FREQ_DIV),
	(unsigned short) (32.704319 / B_SID_FREQ_DIV),
	(unsigned short) (34.648834 / B_SID_FREQ_DIV),
	(unsigned short) (36.709373 / B_SID_FREQ_DIV),
	(unsigned short) (38.892345 / B_SID_FREQ_DIV),
	(unsigned short) (41.204830 / B_SID_FREQ_DIV),
	(unsigned short) (43.654778 / B_SID_FREQ_DIV),
	(unsigned short) (46.251331 / B_SID_FREQ_DIV),
	(unsigned short) (49.000393 / B_SID_FREQ_DIV),
	(unsigned short) (51.915688 / B_SID_FREQ_DIV),
	(unsigned short) (55.002476 / B_SID_FREQ_DIV),
	(unsigned short) (58.271664 / B_SID_FREQ_DIV),
	(unsigned short) (61.736015 / B_SID_FREQ_DIV),
	(unsigned short) (65.410782 / B_SID_FREQ_DIV),
	(unsigned short) (69.300072 / B_SID_FREQ_DIV),
	(unsigned short) (73.421440 / B_SID_FREQ_DIV),
	(unsigned short) (77.784691 / B_SID_FREQ_DIV),
	(unsigned short) (82.413055 / B_SID_FREQ_DIV),
	(unsigned short) (87.313370 / B_SID_FREQ_DIV),
	(unsigned short) (92.506935 / B_SID_FREQ_DIV),
	(unsigned short) (98.000786 / B_SID_FREQ_DIV),
	(unsigned short) (103.831375 / B_SID_FREQ_DIV),
	(unsigned short) (110.011002 / B_SID_FREQ_DIV),
	(unsigned short) (116.550117 / B_SID_FREQ_DIV),
	(unsigned short) (123.472031 / B_SID_FREQ_DIV),
	(unsigned short) (130.821564 / B_SID_FREQ_DIV),
	(unsigned short) (138.600143 / B_SID_FREQ_DIV),
	(unsigned short) (146.842880 / B_SID_FREQ_DIV),
	(unsigned short) (155.569382 / B_SID_FREQ_DIV),
	(unsigned short) (164.826111 / B_SID_FREQ_DIV),
	(unsigned short) (174.641983 / B_SID_FREQ_DIV),
	(unsigned short) (185.013870 / B_SID_FREQ_DIV),
	(unsigned short) (196.001572 / B_SID_FREQ_DIV),
	(unsigned short) (207.684326 / B_SID_FREQ_DIV),
	(unsigned short) (220.022003 / B_SID_FREQ_DIV),
	(unsigned short) (233.100235 / B_SID_FREQ_DIV),
	(unsigned short) (246.974564 / B_SID_FREQ_DIV),
	(unsigned short) (261.643127 / B_SID_FREQ_DIV),
	(unsigned short) (277.238708 / B_SID_FREQ_DIV),
	(unsigned short) (293.685760 / B_SID_FREQ_DIV),
	(unsigned short) (311.138763 / B_SID_FREQ_DIV),
	(unsigned short) (329.706573 / B_SID_FREQ_DIV),
	(unsigned short) (349.283966 / B_SID_FREQ_DIV),
	(unsigned short) (370.096222 / B_SID_FREQ_DIV),
	(unsigned short) (392.003143 / B_SID_FREQ_DIV),
	(unsigned short) (415.454926 / B_SID_FREQ_DIV),
	(unsigned short) (440.140839 / B_SID_FREQ_DIV),
	(unsigned short) (466.200470 / B_SID_FREQ_DIV),
	(unsigned short) (494.071136 / B_SID_FREQ_DIV),
	(unsigned short) (523.286255 / B_SID_FREQ_DIV),
	(unsigned short) (554.631165 / B_SID_FREQ_DIV),
	(unsigned short) (587.544067 / B_SID_FREQ_DIV),
	(unsigned short) (622.277527 / B_SID_FREQ_DIV),
	(unsigned short) (659.630615 / B_SID_FREQ_DIV),
	(unsigned short) (698.812012 / B_SID_FREQ_DIV),
	(unsigned short) (740.192444 / B_SID_FREQ_DIV),
	(unsigned short) (784.313721 / B_SID_FREQ_DIV),
	(unsigned short) (831.255188 / B_SID_FREQ_DIV),
	(unsigned short) (880.281677 / B_SID_FREQ_DIV),
	(unsigned short) (932.835815 / B_SID_FREQ_DIV),
	(unsigned short) (988.142273 / B_SID_FREQ_DIV),
	(unsigned short) (1047.120361 / B_SID_FREQ_DIV),
	(unsigned short) (1109.877930 / B_SID_FREQ_DIV),
	(unsigned short) (1175.088135 / B_SID_FREQ_DIV),
	(unsigned short) (1245.329956 / B_SID_FREQ_DIV),
	(unsigned short) (1319.261230 / B_SID_FREQ_DIV),
	(unsigned short) (1398.601440 / B_SID_FREQ_DIV),
	(unsigned short) (1481.481445 / B_SID_FREQ_DIV),
	(unsigned short) (1569.858765 / B_SID_FREQ_DIV),
	(unsigned short) (1663.893555 / B_SID_FREQ_DIV),
	(unsigned short) (1760.563354 / B_SID_FREQ_DIV),
	(unsigned short) (1865.671631 / B_SID_FREQ_DIV),
	(unsigned short) (1976.284546 / B_SID_FREQ_DIV),
	(unsigned short) (2096.436035 / B_SID_FREQ_DIV),
	(unsigned short) (2222.222168 / B_SID_FREQ_DIV),
	(unsigned short) (2352.941162 / B_SID_FREQ_DIV),
	(unsigned short) (2493.765625 / B_SID_FREQ_DIV),
	(unsigned short) (2638.522461 / B_SID_FREQ_DIV),
	(unsigned short) (2801.120361 / B_SID_FREQ_DIV),
	(unsigned short) (2967.359131 / B_SID_FREQ_DIV),
	(unsigned short) (3144.654053 / B_SID_FREQ_DIV),
	(unsigned short) (3333.333252 / B_SID_FREQ_DIV),
	(unsigned short) (3521.126709 / B_SID_FREQ_DIV),
	/* This was the maximum frequency, the rest use octave foldback */
	(unsigned short) (3731.343262 / B_SID_FREQ_DIV),

	(unsigned short) (1976.284546 / B_SID_FREQ_DIV),
	(unsigned short) (2096.436035 / B_SID_FREQ_DIV),
	(unsigned short) (2222.222168 / B_SID_FREQ_DIV),
	(unsigned short) (2352.941162 / B_SID_FREQ_DIV),
	(unsigned short) (2493.765625 / B_SID_FREQ_DIV),
	(unsigned short) (2638.522461 / B_SID_FREQ_DIV),
	(unsigned short) (2801.120361 / B_SID_FREQ_DIV),
	(unsigned short) (2967.359131 / B_SID_FREQ_DIV),
	(unsigned short) (3144.654053 / B_SID_FREQ_DIV),
	(unsigned short) (3333.333252 / B_SID_FREQ_DIV),
	(unsigned short) (3521.126709 / B_SID_FREQ_DIV),
	(unsigned short) (3731.343262 / B_SID_FREQ_DIV),

	(unsigned short) (1976.284546 / B_SID_FREQ_DIV),
	(unsigned short) (2096.436035 / B_SID_FREQ_DIV),
	(unsigned short) (2222.222168 / B_SID_FREQ_DIV),
	(unsigned short) (2352.941162 / B_SID_FREQ_DIV),
	(unsigned short) (2493.765625 / B_SID_FREQ_DIV),
	(unsigned short) (2638.522461 / B_SID_FREQ_DIV),
	(unsigned short) (2801.120361 / B_SID_FREQ_DIV),
	(unsigned short) (2967.359131 / B_SID_FREQ_DIV),
	(unsigned short) (3144.654053 / B_SID_FREQ_DIV),
};

/*
 * Using the following definitions we can use calls such as the following:
 *
 *	sid_register(id, B_SID_V1_FREQ_LO, phasemap[69][FREQ_LO]);
 *	sid_register(id, B_SID_V1_FREQ_HI, phasemap[69][FREQ_HI]);
 *
 * Which would set voice zero oscillation to A_440, ie, for MIDI key 69.
 */
#define FREQ_LO 0
#define FREQ_HI 1

typedef unsigned char SIDPhase[2];
/* static SIDPhase *phasemap = (SIDPhase *) freqmap; */

/*
 * Structure for the phase accumulating oscillators. There are 3 per voices
 * however in normal operation only one is used and the other waveforms are
 * derived from the same phase accumulator then ANDed together.
 *
 * With MULTI_V set then they are all phased independently and mixed together
 * to widen the sound, specific to the emulator.
 */
typedef struct SidOsc {
	unsigned int current; /* Accumulated (ramp) output */
	unsigned int phase;	/* current phase (implies value...) */
	unsigned int frequency; /* phase increment */
	unsigned int pw;
} sidOsc;

#define B_SID_ADSR_ATTACK 1
#define B_SID_ADSR_DECAY 2
#define B_SID_ADSR_SUSTAIN 3
#define B_SID_ADSR_RELEASE 4

/*
 * Structure for the up/down counter used to generate the ADSR.
 */
typedef struct SidAdsr {
	unsigned int state; /* ADSR state */
	int UDcounter;
	int subcounter;
	int attack;
	int attackStep;
	int decay;
	int decayStep;
	int sustain;
	int release;
	int releaseStep;
	int decayAcc;
} sidAdsr;

/*
 * Noise generation. dngx 1 and 2 are stuffed on RESET then operate as per the
 * original. These is only a single noise source but it is used several times,
 * up to once per oscillator and then once for SN ratio/denormal control.
 */
typedef struct SidNoise {
	int dngx1;
	int dngx2;
	int value;
} sidNoise;

/* Seed for the noise generation */
#define DNGX1 0x67452301
#define DNGX2 0xefcdab89

/*
 * Three voices are emulated, each voice with 3 osc, env, noise plus routing
 * for outputs, sync and RM. Routing switches are separately held in the 'reg'
 * table.
 */
typedef struct SidVoice {
	unsigned int mix;
	unsigned int hold;
	sidOsc osc[3];
	sidNoise noise;
	sidAdsr env;
	unsigned int lsample;
} sidVoice;

/*
 * Two different filter algorithms will be available. Both are floating point
 * as the original was analogue. One is the chamberlain, it is 12dB multimode
 * as per the original. The other is a houvilainen LPF as a option, a lot 
 * warmer. Only one can be selected at any given time.
 */
typedef struct SidFilter {
	/* General use */
	float cutoff;
	float resonance;
	float fmix;
	/* For the Chamberlain code */
	float delay1;
	float delay2;
	float delay3;
	float delay4;
	/* For the houvilainen code */
	float az1;
	float az2;
	float az3;
	float az4;
	float az5;
	float ay1;
	float ay2;
	float ay3;
	float ay4;
	float amf;
} sidFilter;

typedef struct BSid {
	int id;
	float nominalclock;
	float samplerate;
	float volume;
	float gn;
	float detune;
	float detune2;
	float leakage;
	float snratio;
	float dcbias;
	unsigned char reg[B_SID_REGISTERS]; /* Content of the 32 SID registers */
	sidVoice voice[3];
	sidFilter filter;
	float sfMix;
	float out;
	float cent_diff;
} bSid;

/* Some bit selection macros */
#define _B_SID_DEBUG SID[id]->reg[B_SID_CONTROL] & B_SID_C_DEBUG
#define _B_SID_DEBUG_D SID[id]->reg[B_SID_CONTROL] & B_SID_C_DEBUG_D
#define _B_SID_DEBUG_A SID[id]->reg[B_SID_CONTROL] & B_SID_C_DEBUG_A

/*
 * Integer (digital) dispatch routines demultiplexed based on register addresses
 */
typedef unsigned char (*bSidRoutine)(int, unsigned char, unsigned char);
typedef float (*bSidIORoutine)(int, float);
/* One default dispatch that just returns the target reg value */
static unsigned char bSidGet(int, unsigned char, unsigned char);

/*
 * Default dispatch table, this mostly gets overwritten on RESET
 */
bSidRoutine bSidDispatch[B_SID_REGISTERS] = {
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
	bSidGet,
};

/*
 * Analogue dispatch routines
 */
static float bSidIOInit(int, float);
static float bSidIODestroy(int, float);
static float bSidIOAnalogue(int, float);
static float bSidIOGain(int, float);
static float bSidIODetune(int, float);
static float bSidIOLeakage(int, float);
static float bSidIOSNRatio(int, float);
static float bSidIODCBias(int, float);
static float bSidIOOberheim(int, float);

bSidIORoutine bSidIODispatch[B_SID_IO] = {
	bSidIOInit,
	bSidIOInit,
	bSidIOInit,
	bSidIOInit,
	bSidIOInit,
	bSidIOInit,
	bSidIOInit,
	bSidIOInit,
	bSidIOInit,
};

/*
 * Up to B_SID_COUNT chips can be emulated on request
 */
bSid *SID[B_SID_COUNT];

static unsigned char
bSidGet(int id, unsigned char comm, unsigned char param)
{
	return(SID[id]->reg[comm]);
}

/*
 * Filter register dispatch, this takes the register settings and converts them
 * into something that can be used to actually filter the signal.
 */
static unsigned char
bSidFilter(int id, unsigned char comm, unsigned char param)
{
	SID[id]->reg[comm] = param;

	/*
	 * The filter cutoff will go from 0 to nyquist and this parameter is only
	 * supposed to have a range up to 12kHz.
	 */
	switch (comm) {
		case B_SID_FILT_LO:
		case B_SID_FILT_HI:
			SID[id]->filter.cutoff =
				((float) ((SID[id]->reg[B_SID_FILT_HI] << 3)
					+ (SID[id]->reg[B_SID_FILT_LO] & 0x07))) / 2048.0;

			/* We use 24K here since cutoff only goes up to nyquist */
			SID[id]->filter.cutoff =
				SID[id]->filter.cutoff
					* 24000.0 / SID[id]->samplerate;

			if (_B_SID_DEBUG)
				printf("v %i: cutoff: %x = %f (~%2.2fkHz)\n", id,
					((SID[id]->reg[B_SID_FILT_HI] << 3)
					+ (SID[id]->reg[B_SID_FILT_LO] & 0x07)),
					SID[id]->filter.cutoff,
					SID[id]->filter.cutoff * SID[id]->samplerate / 2);

			break;
		case B_SID_FILT_RES_F:
			SID[id]->filter.resonance =
				((float) (SID[id]->reg[B_SID_FILT_RES_F] >> 4)) / 15.0;

			if (_B_SID_DEBUG)
				printf("v %i: resonance: %f\n", id, SID[id]->filter.resonance);

			break;
		case B_SID_FILT_M_VOL:
			SID[id]->volume =
				((float) (SID[id]->reg[B_SID_FILT_M_VOL] & B_SID_F_VOLMASK))
					/ 128.0;

			if (_B_SID_DEBUG)
				printf("v %i: volume: %f\n", id, SID[id]->volume);

			break;
	}

	return(param);
}

/*
 * Voice register dispatcher. Configures oscillators and envelopes, noise
 * generation, ringmod, sync.
 */
static unsigned char
bSidVoice(int id, unsigned char comm, unsigned char param)
{
	int vid = 0;
	float rate;

	SID[id]->reg[comm] = param;

	if (comm >= B_SID_V3_FREQ_LO)
		vid = B_SID_VOICE_3;
	else if (comm >= B_SID_V2_FREQ_LO)
		vid = B_SID_VOICE_2;

	comm %= 7;

	switch (comm) {
		case B_SID_V1_FREQ_LO:
		case B_SID_V1_FREQ_HI:
			rate = ((SID[id]->reg[7 * vid + B_SID_V1_FREQ_HI] << 8)
				+ SID[id]->reg[7 * vid + B_SID_V1_FREQ_LO]);
			/*
			 * This is the 1MHz clock phase accumulation rate. We are not
			 * running at 1MHz, we are running at samplerate so we need to 
			 * convert this into our own phase accumulator.
			 */
			SID[id]->voice[vid].osc[B_SID_RAMP].frequency
				= (int) (rate * SID[id]->nominalclock / SID[id]->samplerate);

			if (_B_SID_DEBUG)
				printf("v %i: freq pre: %4.3f (%4.3f Hz), post: %i (%4.3f Hz) (d=%4.3f) (r=%5.0f)\n",
					id,

					rate,
					rate * B_SID_FREQ_DIV,

					SID[id]->voice[vid].osc[B_SID_RAMP].frequency,
					SID[id]->voice[vid].osc[B_SID_RAMP].frequency
						* SID[id]->samplerate / SID[id]->nominalclock
						* B_SID_FREQ_DIV,

					SID[id]->detune,
					SID[id]->samplerate
				);

			if (SID[id]->reg[B_SID_CONTROL] & B_SID_C_MULTI_V)
			{
				SID[id]->voice[vid].osc[B_SID_TRI].frequency
					= (int)
						(((float) SID[id]->voice[vid].osc[B_SID_RAMP].frequency)
						* SID[id]->detune);
				SID[id]->voice[vid].osc[B_SID_SQUARE].frequency
					= (int)
						(((float) SID[id]->voice[vid].osc[B_SID_RAMP].frequency)
						* SID[id]->detune2);
			} else {
				SID[id]->voice[vid].osc[B_SID_TRI].frequency
					= SID[id]->voice[vid].osc[B_SID_SQUARE].frequency
						= SID[id]->voice[vid].osc[B_SID_RAMP].frequency;
			}
			break;
		case B_SID_V1_PW_LO:
		case B_SID_V1_PW_HI:
			SID[id]->voice[vid].osc[B_SID_SQUARE].pw
				= (SID[id]->reg[7 * vid + B_SID_V1_PW_HI] << 8)
					+ SID[id]->reg[7 * vid + B_SID_V1_PW_LO];
			break;
		case B_SID_V1_CONTROL:
			/*
			 * These are mostly routing options and do not need special
			 * handling, routing will refer back to the register content.
			 */
			break;
		case B_SID_V1_ATT_DEC:
			rate = 230.0 / bSidEnvRates[param >> 4];
			if ((rate = rate / SID[id]->samplerate) > 1.0)
			{
				SID[id]->voice[vid].env.attackStep = (int) (rate + 0.5);
				SID[id]->voice[vid].env.attack = 1;
			} else {
				SID[id]->voice[vid].env.attackStep = 1;
				SID[id]->voice[vid].env.attack = (int) (1.0 / (rate));
			}

			rate = 80.0 / bSidEnvRates[param & 0x0f];
			if ((rate = rate / SID[id]->samplerate) > 1.0)
			{
				SID[id]->voice[vid].env.decayStep = (int) (rate + 0.5);
				SID[id]->voice[vid].env.decay = 1;
			} else {
				SID[id]->voice[vid].env.decayStep = 1;
				SID[id]->voice[vid].env.decay = (int) (1.0 / (rate));
			}

			if (_B_SID_DEBUG)
				printf("v %i: attack: %i/%i, decay: %i/%i\n", id,
					SID[id]->voice[vid].env.attack,
					SID[id]->voice[vid].env.attackStep,
					SID[id]->voice[vid].env.decay,
					SID[id]->voice[vid].env.decayStep
			);
			break;
		case B_SID_V1_SUS_REL:
			SID[id]->voice[vid].env.sustain = (param >> 4) * 16;

			rate = 80.0 / bSidEnvRates[param & 0x0f];
			if ((rate = rate / SID[id]->samplerate) > 1.0)
			{
				SID[id]->voice[vid].env.releaseStep = (int) (rate + 0.5);
				SID[id]->voice[vid].env.release = 1;
			} else {
				SID[id]->voice[vid].env.releaseStep = 1;
				SID[id]->voice[vid].env.release = (int) (1.0 / (rate));
			}

			if (_B_SID_DEBUG)
				printf("v %i: sustain: %i, release %i/%i\n", id,
					SID[id]->voice[vid].env.sustain,
					SID[id]->voice[vid].env.release,
					SID[id]->voice[vid].env.releaseStep
			);
			break;
	}

	return(param);
}

/*
 * Control register dispatcher. This is bristol specific using one of the
 * non-assigned registers on the SID chip. It has a few options for voice 
 * control for filter selection and waveform mixing, plus it acts as an emulator
 * for the Reset pin on the package to put the chip into a known state.
 */
static unsigned char
bSidControl(int id, unsigned char comm, unsigned char param)
{
	if (param & B_SID_C_PRINT)
	{
		printf("Bristol SID register control block:\n\n");
		printf("	B_SID_V1_FREQ_LO	0x%02x\n", SID[id]->reg[0]);
		printf("	B_SID_V1_FREQ_HI	0x%02x\n", SID[id]->reg[1]);
		printf("	B_SID_V1_PW_LO		0x%02x\n", SID[id]->reg[2]);
		printf("	B_SID_V1_PW_HI		0x%02x\n", SID[id]->reg[3]);
		printf("	B_SID_V1_CONTROL	0x%02x\n", SID[id]->reg[4]);
		printf("	B_SID_V1_ATT_DEC	0x%02x\n", SID[id]->reg[5]);
		printf("	B_SID_V1_SUS_REL	0x%02x\n", SID[id]->reg[6]);
		printf("	B_SID_V2_FREQ_LO	0x%02x\n", SID[id]->reg[7]);
		printf("	B_SID_V2_FREQ_HI	0x%02x\n", SID[id]->reg[8]);
		printf("	B_SID_V2_PW_LO		0x%02x\n", SID[id]->reg[9]);
		printf("	B_SID_V2_PW_HI		0x%02x\n", SID[id]->reg[10]);
		printf("	B_SID_V2_CONTROL	0x%02x\n", SID[id]->reg[11]);
		printf("	B_SID_V2_ATT_DEC	0x%02x\n", SID[id]->reg[12]);
		printf("	B_SID_V2_SUS_REL	0x%02x\n", SID[id]->reg[13]);
		printf("	B_SID_V3_FREQ_LO	0x%02x\n", SID[id]->reg[14]);
		printf("	B_SID_V3_FREQ_HI	0x%02x\n", SID[id]->reg[15]);
		printf("	B_SID_V3_PW_LO		0x%02x\n", SID[id]->reg[16]);
		printf("	B_SID_V3_PW_HI		0x%02x\n", SID[id]->reg[17]);
		printf("	B_SID_V3_CONTROL	0x%02x\n", SID[id]->reg[18]);
		printf("	B_SID_V3_ATT_DEC	0x%02x\n", SID[id]->reg[19]);
		printf("	B_SID_V3_SUS_REL	0x%02x\n", SID[id]->reg[20]);

		printf("	B_SID_FILT_LO		0x%02x\n", SID[id]->reg[21]);
		printf("	B_SID_FILT_HI		0x%02x\n", SID[id]->reg[22]);
		printf("	B_SID_FILT_RES_F	0x%02x\n", SID[id]->reg[23]);
		printf("	B_SID_FILT_M_VOL	0x%02x\n", SID[id]->reg[24]);

		printf("	B_SID_X_ANALOGUE	0x%02x\n", SID[id]->reg[25]);
		printf("	B_SID_Y_ANALOGUE	0x%02x\n", SID[id]->reg[26]);

		printf("	B_SID_OSC_3_OUT		0x%02x\n", SID[id]->reg[27]);
		printf("	B_SID_ENV_3_OUT		0x%02x\n", SID[id]->reg[28]);

		printf("	B_SID_CONTROL		0x%02x\n", SID[id]->reg[29]);
		printf("\n");
	}

	if (param & B_SID_C_RESET)
	{
		/*
		 * Set up the unregistered envelope values, set the envelope registers
		 * and then call each register.
		 */
		SID[id]->detune = 1.0;
		SID[id]->detune2 = 1.0;
		SID[id]->leakage = 0.0001;
		SID[id]->dcbias = -0.2;
		SID[id]->snratio = 0.000001;
		SID[id]->cent_diff = powf(2, 1.0/1200.0);
		SID[id]->gn = 1.0;
		SID[id]->filter.fmix = 0.0;

		SID[id]->voice[0].lsample = 0;
		SID[id]->voice[0].env.UDcounter = 0;
		SID[id]->voice[0].env.state = B_SID_ADSR_RELEASE;
		SID[id]->reg[B_SID_V1_ATT_DEC] = 0x44;
		SID[id]->reg[B_SID_V1_SUS_REL] = 0x84;
		sid_register(id, B_SID_V1_ATT_DEC, SID[id]->reg[B_SID_V1_ATT_DEC]);
		sid_register(id, B_SID_V1_SUS_REL, SID[id]->reg[B_SID_V1_SUS_REL]);

		SID[id]->voice[1].lsample = 0;
		SID[id]->voice[1].env.UDcounter = 0;
		SID[id]->voice[1].env.state = B_SID_ADSR_RELEASE;
		SID[id]->reg[B_SID_V2_ATT_DEC] = 0x44;
		SID[id]->reg[B_SID_V2_SUS_REL] = 0x84;
		sid_register(id, B_SID_V2_ATT_DEC, SID[id]->reg[B_SID_V2_ATT_DEC]);
		sid_register(id, B_SID_V2_SUS_REL, SID[id]->reg[B_SID_V2_SUS_REL]);

		SID[id]->voice[2].lsample = 0;
		SID[id]->voice[2].env.UDcounter = 0;
		SID[id]->voice[2].env.state = B_SID_ADSR_RELEASE;
		SID[id]->reg[B_SID_V3_ATT_DEC] = 0x44;
		SID[id]->reg[B_SID_V3_SUS_REL] = 0x84;
		sid_register(id, B_SID_V3_ATT_DEC, SID[id]->reg[B_SID_V3_ATT_DEC]);
		sid_register(id, B_SID_V3_SUS_REL, SID[id]->reg[B_SID_V3_SUS_REL]);

		SID[id]->voice[0].osc[0].current = 0;
		SID[id]->voice[0].osc[1].current = 0;
		SID[id]->voice[0].osc[2].current = 0;

		/*
		 * Clean up the oscillators:
		 *
		 * Fout = Fp * Fclock / 16777216 Hz
		 * Fout = 440, Fclock = 1023000 (SID[id]->nominalclock)
		 * Fp = 440 * 16777216 / 1023000; 
		 *
		 * Fp = 7381 = 1CD6H
		 *
		 * Alternatively, the 1Mhz phase accumulator is:
		 *
		 * Fp = Fout / 0.059604645
		 *
		 * This needs to be converted into a phase accumulator for the native
		 * samplerate, the one at which the emulator is clocked.
		 */
		SID[id]->reg[B_SID_V1_FREQ_LO] = 0xd6; /* Gen A440 */
		SID[id]->reg[B_SID_V1_FREQ_HI] = 0x1c;
		SID[id]->reg[B_SID_V1_PW_LO] = 0;
		SID[id]->reg[B_SID_V1_PW_HI] = 0x08;
		SID[id]->reg[B_SID_V1_CONTROL] = B_SID_V_RAMP; /* Ramp wave only */
		sid_register(id, B_SID_V1_FREQ_LO, SID[id]->reg[B_SID_V1_FREQ_LO]);
		sid_register(id, B_SID_V1_FREQ_HI, SID[id]->reg[B_SID_V1_FREQ_HI]);
		sid_register(id, B_SID_V1_PW_LO, SID[id]->reg[B_SID_V1_PW_LO]);
		sid_register(id, B_SID_V1_PW_HI, SID[id]->reg[B_SID_V1_PW_HI]);
		sid_register(id, B_SID_V1_CONTROL, SID[id]->reg[B_SID_V1_CONTROL]);

		SID[id]->reg[B_SID_V2_FREQ_LO] = 0xd6; /* Gen A440 */
		SID[id]->reg[B_SID_V2_FREQ_HI] = 0x1c;
		SID[id]->reg[B_SID_V2_PW_LO] = 0;
		SID[id]->reg[B_SID_V2_PW_HI] = 0x08;
		SID[id]->reg[B_SID_V2_CONTROL] = B_SID_V_RAMP; /* Ramp wave only */
		sid_register(id, B_SID_V2_FREQ_LO, SID[id]->reg[B_SID_V2_FREQ_LO]);
		sid_register(id, B_SID_V2_FREQ_HI, SID[id]->reg[B_SID_V2_FREQ_HI]);
		sid_register(id, B_SID_V2_PW_LO, SID[id]->reg[B_SID_V2_PW_LO]);
		sid_register(id, B_SID_V2_PW_HI, SID[id]->reg[B_SID_V2_PW_HI]);
		sid_register(id, B_SID_V2_CONTROL, SID[id]->reg[B_SID_V2_CONTROL]);

		SID[id]->reg[B_SID_V3_FREQ_LO] = 0xd6; /* Gen A440 */
		SID[id]->reg[B_SID_V3_FREQ_HI] = 0x1c;
		SID[id]->reg[B_SID_V3_PW_LO] = 0;
		SID[id]->reg[B_SID_V3_PW_HI] = 0x08;
		SID[id]->reg[B_SID_V3_CONTROL] = B_SID_V_RAMP; /* Ramp wave only */
		sid_register(id, B_SID_V3_FREQ_LO, SID[id]->reg[B_SID_V3_FREQ_LO]);
		sid_register(id, B_SID_V3_FREQ_HI, SID[id]->reg[B_SID_V3_FREQ_HI]);
		sid_register(id, B_SID_V3_PW_LO, SID[id]->reg[B_SID_V3_PW_LO]);
		sid_register(id, B_SID_V3_PW_HI, SID[id]->reg[B_SID_V3_PW_HI]);
		sid_register(id, B_SID_V3_CONTROL, SID[id]->reg[B_SID_V3_CONTROL]);

		/*
		 * Noise. Reset the seed values
		 */
		SID[id]->voice[B_SID_VOICE_1].noise.dngx1 = DNGX1;
		SID[id]->voice[B_SID_VOICE_1].noise.dngx2 = DNGX2;
		SID[id]->voice[B_SID_VOICE_1].noise.value = 0;
		SID[id]->voice[B_SID_VOICE_2].noise.dngx1 = DNGX1;
		SID[id]->voice[B_SID_VOICE_2].noise.dngx2 = DNGX2;
		SID[id]->voice[B_SID_VOICE_2].noise.value = 0;
		SID[id]->voice[B_SID_VOICE_3].noise.dngx1 = DNGX1;
		SID[id]->voice[B_SID_VOICE_3].noise.dngx2 = DNGX2;
		SID[id]->voice[B_SID_VOICE_3].noise.value = 0;

		/*
		 * Filter. Change type to LP of selected type. Reset any stored values,
		 * configure small amount of resonance and some arbitrary cutoff.
		 */
		SID[id]->reg[B_SID_FILT_LO] = 0;
		SID[id]->reg[B_SID_FILT_HI] = 0x20;
		SID[id]->reg[B_SID_FILT_RES_F] = 0x47; /* Res + filt all voices */
		SID[id]->reg[B_SID_FILT_M_VOL] = 0x1c; /* Low pass, vol 12/15 */
		sid_register(id, B_SID_FILT_LO, SID[id]->reg[B_SID_FILT_LO]);
		sid_register(id, B_SID_FILT_HI, SID[id]->reg[B_SID_FILT_HI]);
		sid_register(id, B_SID_FILT_RES_F, SID[id]->reg[B_SID_FILT_RES_F]);
		sid_register(id, B_SID_FILT_M_VOL, SID[id]->reg[B_SID_FILT_M_VOL]);

		SID[id]->filter.amf = SID[id]->filter.az1 = SID[id]->filter.az2
			= SID[id]->filter.az3 = SID[id]->filter.az4 = SID[id]->filter.az5
			= SID[id]->filter.ay1 = SID[id]->filter.ay2 = SID[id]->filter.ay3
			= SID[id]->filter.ay4 = SID[id]->filter.amf = SID[id]->filter.fmix
			= SID[id]->filter.delay1 = SID[id]->filter.delay2
			= SID[id]->filter.delay3 = SID[id]->filter.delay4 = 0;

		SID[id]->reg[B_SID_X_ANALOGUE] = 0;
		SID[id]->reg[B_SID_X_ANALOGUE] = 0; /* This should be with leakage */

		SID[id]->reg[B_SID_OSC_3_OUT] = 0;
		SID[id]->reg[B_SID_ENV_3_OUT] = 0; /* This should be with leakage */
	}

	if (param & B_SID_C_LPF)
		SID[id]->reg[B_SID_CONTROL] |= B_SID_C_LPF;
	else
		SID[id]->reg[B_SID_CONTROL] &= ~B_SID_C_LPF;

	if (param & B_SID_C_MULTI_V)
		SID[id]->reg[B_SID_CONTROL] |= B_SID_C_MULTI_V;
	else
		SID[id]->reg[B_SID_CONTROL] &= ~B_SID_C_MULTI_V;

	if (param & B_SID_C_DEBUG)
		SID[id]->reg[B_SID_CONTROL] |= B_SID_C_DEBUG;
	else
		SID[id]->reg[B_SID_CONTROL] &= ~B_SID_C_DEBUG;

	if (param & B_SID_C_DEBUG_A)
		SID[id]->reg[B_SID_CONTROL] |= B_SID_C_DEBUG_A;
	else
		SID[id]->reg[B_SID_CONTROL] &= ~B_SID_C_DEBUG_A;

	return(0);
}

/*
 * Phase accumulating oscillator code with waveform extraction and noise
 * generation.
 *
 * This code will also do the sync and ringmod however that has yet to be 
 * tested.
 */
static void
bSidDoOsc(sidVoice *voice, unsigned char cflags, unsigned char vflags,
unsigned int psample)
{
	int gain = 0;

	if (vflags & B_SID_V_TEST)
	{
		voice->osc[B_SID_TRI].current = voice->osc[B_SID_RAMP].current = 0;
		voice->osc[B_SID_SQUARE].current = 0x0fff;
		voice->mix = 0;
		voice->lsample = psample;
		return;
	}

	/*
	 * This only does hard sync, we could look to change that as it works
	 * pretty badly for pulse waveforms. We could test running the oscillator 
	 * backwords by negating the frequency.
	 */
	if ((vflags & B_SID_V_SYNC) && (voice->lsample == 0) && (psample != 0))
	{
		voice->osc[B_SID_RAMP].phase
			= voice->osc[B_SID_TRI].phase
			= voice->osc[B_SID_SQUARE].phase = 0;
	}

	voice->osc[B_SID_RAMP].phase =
		(voice->osc[B_SID_RAMP].phase + voice->osc[B_SID_RAMP].frequency)
			& 0x00ffffff;
	voice->osc[B_SID_RAMP].current = (voice->osc[B_SID_RAMP].phase>>12)&0x0fff;

	/*
	 * osc.current is now the ramp wave.
	 *
	 * Pulse is based on the PW value. At its extremes this delivers DC as per
	 * the original:
	 *
	 *	voice->osc[B_SID_RAMP].current > voice->osc[B_SID_RAMP].pw? 0x0fff:0
	 *
	 * Tri takes the MSB of the wave and uses that to XOR the remaining 11 bits
	 * then scaling it back to the same total number of bits.
	 *
	 *	voice->osc[B_SID_RAMP].current & 0x0800?
	 *		((voice->osc[B_SID_RAMP].current & 0x07ff) ^ 0x7ff) << 1:
	 *	(voice->osc[B_SID_RAMP].current & 0x07ff) << 1
	 *
	 * These generation methods accord the chip, same logic and everything but
	 * the next following step is not: if we configure a multivoice option then
	 * all waveforms are generated independently and mixed together. Otherwise
	 * the waves are generated and then 'and'ed together.
	 */
	if (cflags & B_SID_C_MULTI_V)
	{
		/* Redo the phases for each oscillator */

		/* Get the phase, build current as ramp, convert into tri */
		voice->osc[B_SID_TRI].phase =
			(voice->osc[B_SID_TRI].phase + voice->osc[B_SID_TRI].frequency)
				& 0x00ffffff;
		voice->osc[B_SID_TRI].current = (voice->osc[B_SID_TRI].phase >> 12);
		/* Extract Tri wave */
		voice->osc[B_SID_TRI].current =
			voice->osc[B_SID_TRI].current & 0x00800?
				((voice->osc[B_SID_TRI].current & 0x07ff) ^ 0x7ff) << 1:
			(voice->osc[B_SID_TRI].current & 0x07ff) << 1;

		/* And for the square wave - get phase, build current as ramp */
		voice->osc[B_SID_SQUARE].phase = (voice->osc[B_SID_SQUARE].phase
			+ voice->osc[B_SID_SQUARE].frequency) & 0x00ffffff;
		voice->osc[B_SID_SQUARE].current
			= (voice->osc[B_SID_SQUARE].phase >> 12);
		/* Then extract the pulse component */
		voice->osc[B_SID_SQUARE].current =
			voice->osc[B_SID_SQUARE].current > voice->osc[B_SID_SQUARE].pw?
				0x0fff:0;

		voice->mix = 0x0;

		if (vflags & B_SID_V_TRI) {
			voice->mix = voice->osc[B_SID_TRI].current;
			gain = 1;
		}
		if (vflags & B_SID_V_RAMP) {
			voice->mix = (voice->mix + voice->osc[B_SID_RAMP].current) >> gain;
			gain = 1;
		}
		if (vflags & B_SID_V_SQUARE) {
			voice->mix = ((voice->mix + voice->osc[B_SID_SQUARE].current)
				>> gain);
			gain = 1;
		}

		/* Generate noise. Copy into the mix if selected, may need scaling */
		voice->noise.dngx1 ^= voice->noise.dngx2;
		voice->noise.value = (voice->noise.dngx2 & 0x00ffffff) >> 12;
		if (vflags & B_SID_V_NOISE) {
			voice->mix = (voice->mix + voice->noise.value) >> gain;
			gain = 1;
		}
		voice->noise.dngx2 += voice->noise.dngx1;

		if (vflags & B_SID_V_RINGMOD)
			voice->mix = ((voice->mix * psample) >> (12 + gain));
	} else {
		/* If we are not multi then extract wave from RAMP */
		voice->osc[B_SID_TRI].current =
			voice->osc[B_SID_RAMP].current & 0x0800?
				((voice->osc[B_SID_RAMP].current & 0x07ff) ^ 0x7ff) << 1
				:(voice->osc[B_SID_RAMP].current & 0x07ff) << 1;

		voice->osc[B_SID_SQUARE].current =
			voice->osc[B_SID_RAMP].current > voice->osc[B_SID_SQUARE].pw?
				0x0fff:0;

		voice->mix = 0x0fff;

		if (vflags & B_SID_V_RAMP)
			voice->mix &= voice->osc[B_SID_RAMP].current;
		if (vflags & B_SID_V_SQUARE)
			voice->mix &= voice->osc[B_SID_SQUARE].current;
		if (vflags & B_SID_V_TRI)
		{
			/*
			 * Ring mod is a multiplication function which was not a part of
			 * this chip. As far as I know it did an exor on the MSB of the 
			 * input waves, this would extract a pair of square waves for an
			 * exor function which resolves to ring modulation.
			 */
			if (vflags & B_SID_V_RINGMOD)
				voice->mix &=
					psample & 0x800?
						((voice->osc[B_SID_RAMP].current & 0x07ff) ^ 0x7ff)
						:(voice->osc[B_SID_RAMP].current & 0x07ff);
			else
				voice->mix &= voice->osc[B_SID_TRI].current;
		}

		/* Generate noise. Copy into the mix if selected, may need scaling */
		voice->noise.dngx1 ^= voice->noise.dngx2;
		voice->noise.value = (voice->noise.dngx2 & 0x00ffffff) >> 12;
		if (vflags & B_SID_V_NOISE)
			voice->mix &= voice->noise.value;
		voice->noise.dngx2 += voice->noise.dngx1;
	}

	voice->lsample = psample;
}

/*
 * This emulates the SID 8 bit up/down counter that generated the envelope.
 *
 * Attack was always linear.
 *
 * In the first iteration this was a linear decay and release, then an 
 * accumulator was added in that fead more delay into the decays to extende
 * them.
 */
static void
bSidDoEnv(sidVoice *voice, unsigned char vflags)
{
	/*
	 * We need to decide which state we are in, that will define the action
	 * we have to perform
	 */
	if (vflags & B_SID_V_GATE) {
		switch (voice->env.state) {
			case B_SID_ADSR_ATTACK:
				if (--voice->env.subcounter <= 0)
				{
					if ((voice->env.UDcounter += voice->env.attackStep) >= 255)
					{
						voice->env.UDcounter = 255;
						voice->env.state = B_SID_ADSR_DECAY;
						voice->env.decayAcc = 0;
						voice->env.subcounter = voice->env.decay;
					} else
						voice->env.subcounter = voice->env.attack;
				}
				break;
			case B_SID_ADSR_DECAY:
			case B_SID_ADSR_SUSTAIN:
				if (--voice->env.subcounter <= 0)
				{
					if (voice->env.UDcounter > voice->env.sustain)
					{
						voice->env.UDcounter -= voice->env.decayStep;
						voice->env.state = B_SID_ADSR_DECAY;
						voice->env.subcounter = voice->env.decay * 16;
					} else {
						voice->env.subcounter = voice->env.decay +
							(++voice->env.decayAcc << 8);
					}
				}
				break;
			case B_SID_ADSR_RELEASE:
				voice->env.state = B_SID_ADSR_ATTACK;
				voice->env.subcounter = voice->env.attack;
				if ((voice->env.UDcounter += voice->env.attackStep) >= 255)
				{
					voice->env.UDcounter = 255;
					voice->env.state = B_SID_ADSR_DECAY;
					voice->env.subcounter = voice->env.decay;
				}
				break;
		}
	} else {
		/* If we were not in the release state then stuff the counter */
		if (voice->env.state != B_SID_ADSR_RELEASE)
		{
			voice->env.decayAcc = 0;
			voice->env.subcounter = voice->env.release;
		}

		/*
		 * Release state: gate has dropped, tend towards leakage.
		 */
		if (--voice->env.subcounter <= 0)
		{
			if ((voice->env.UDcounter -= voice->env.releaseStep) <= 0)
				voice->env.UDcounter = 0;
			voice->env.subcounter = voice->env.release +
				(++voice->env.decayAcc >> 5);
		}
		voice->env.state = B_SID_ADSR_RELEASE;
	}
}

#define V2 40000.0
#define OV2 0.000025

/*
 * The filter input s->sfMix contains the voices selected to be filtered, any
 * S/N ratio injected noise plus a constant component noise to prevent the
 * denormal that occur at zero input. This sample needs to be filtered on to
 * the s->out which already contains non filtered voices plus oscillator 
 * leakage if defined.
 *
 * We are going to look at two filter algorithms, both floating point.
 *
 * The default will be a chamberlain 12dB/Octave multimode HP/BP/LP.
 *
 * An optional Houvilainen 24dB/Octave LP will be integrated, and since the
 * cutoff is tied at 12kHz or lower then we will not bother with oversampling.
 *
 * If no filter poles are selected then we pass input to output, this is not 
 * per the original, added in a check for selection otherwise zero output.
 */
#define TANHF(x) x

static void
bSidDoFilter(bSid *s)
{
	float kfc, highpass, qres;

	if (((s->reg[B_SID_CONTROL] & B_SID_C_LPF) == 0) &&
		((s->reg[B_SID_FILT_M_VOL] & (B_SID_F_HP|B_SID_F_BP|B_SID_F_LP)) == 0))
	{
		s->sfMix = 0;
		return;
	}

	/* chamberlain */
	kfc = s->filter.cutoff; // * 12000 / s->samplerate;

	if (kfc <= 0.000001)
		kfc = 0.000001;

	qres = 2.0 - s->filter.resonance * 1.95;

	/* delay2/4 = lowpass output */
	s->filter.delay2 = s->filter.delay2 + kfc * s->filter.delay1;

	highpass = s->sfMix * 0.25 - s->filter.delay2 - qres * s->filter.delay1;

	/* delay1/3 = bandpass output */
	s->filter.delay1 = kfc * highpass + s->filter.delay1;

	s->filter.delay4 = s->filter.delay4 + kfc * s->filter.delay3;
	highpass = s->filter.delay2 - s->filter.delay4 - qres * s->filter.delay3;
	s->filter.delay3 = kfc * highpass + s->filter.delay3;

	/* Houvilainen LPF-24 */
	if (s->reg[B_SID_CONTROL] & B_SID_C_LPF)
	{
		float kfcr;
		float kacr;
		float k2vg;

		/* Max filter at 12kHz */
		kfc = s->filter.cutoff/2; //* s->filter.cutoff * 12000/s->samplerate;

		// frequency & amplitude correction
		kfcr = kfc * (kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
		kacr = kfc * (-3.9364 * kfc + 1.8409) + 0.9968;

		k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc));

		// cascade of 4 1st order sections
		s->filter.ay1 = s->filter.az1 + k2vg * (tanhf(s->sfMix * OV2
			- 4*s->filter.resonance*s->filter.amf*kacr)
			- TANHF(s->filter.az1));
		s->filter.az1 = s->filter.ay1;

		s->filter.ay2 = s->filter.az2
			+ k2vg * (TANHF(s->filter.ay1) - TANHF(s->filter.az2));
		s->filter.az2 = s->filter.ay2;

		s->filter.ay3 = s->filter.az3
			+ k2vg * (TANHF(s->filter.ay2) - TANHF(s->filter.az3));
		s->filter.az3 = s->filter.ay3;

		s->filter.ay4 = s->filter.az4
			+ k2vg * (TANHF(s->filter.ay3) - TANHF(s->filter.az4));
		s->filter.az4 = s->filter.ay4;

		// 1/2-sample delay for phase compensation
		s->filter.amf = (s->filter.ay4+s->filter.az5)*0.5;
		s->filter.az5 = s->filter.ay4;

		/*
		 * This remixes the different poles to give a fatter, warmer sound.
		 * Other algorithms do a premixing of this however that can prevent
		 * the filter from resonating. This also spreads the cutoff away
		 * from a pure 24dB/Octave, the original only having a 12dB rolloff.
		s->sfMix = (s->filter.amf * V2);
		 */
		s->sfMix = (s->filter.amf
			+ (s->filter.ay3-s->filter.az3
			- s->filter.ay2+s->filter.az4
			- s->filter.ay1-s->filter.az2) * s->filter.fmix) * V2 * 0.5;
	} else
		s->sfMix = 0;

	/* mix filter output into output buffer */
	if (s->reg[B_SID_FILT_M_VOL] & B_SID_F_LP)
		s->sfMix += s->filter.delay4;
	if (s->reg[B_SID_FILT_M_VOL] & B_SID_F_HP)
		s->sfMix += highpass;
	if (s->reg[B_SID_FILT_M_VOL] & B_SID_F_BP)
		s->sfMix += s->filter.delay3;
}

static float
bSidIOAnalogue(int id, float a_in)
{
	bSid *s = SID[id];

	/*
	 * This will be the workhorse that takes a sample in, generates all the
	 * voices, envelopes and filter, and generates an output sample that it 
	 * returns, copying OSC3/RND and ENV3 to the relevant registers.
	 *
	 * This is effectively the clock signal for the emulation.
	 *
	 * Start with voice-3 as it might not even be sent to output, just used as
	 * a modulator. We can also stuff the output registers now with this data,
	 * not that it helps the interface since this IO is an atomic operation.
	 */

	/*
	 * We need to bleed the oscillators through to the output, something that
	 * happened in the original. Unfortunately it is not clear what was leaked,
	 * the notes from Yannes indicate it was oscillator noise however these
	 * were digital so would have appeared as pure noise rather than leaked
	 * signal when leaked into the analogue domain - digital oscillators cannot
	 * 'leak' their waveforms.
	 *
	 * Now the generation of the analogue signal that was driven into the 
	 * filter needed D/A converters, naturally, but for whatever reason Yannes
	 * decided that the gain of the D/A would be under oscillator control: the
	 * oscillator modulated the gain of the envelope. The net effect should be
	 * the same as the more logical env modulating the gain of the oscillator
	 * however since the oscillator was constantly changing this probably lead
	 * to the noise in the analogue domain when going through an opamp on the
	 * DA circuit. For this reason we are going to leak the actual oscillators
	 * rather than the voice - the voice already has the envelope applied so
	 * if we do not apply this leakage to the oscillator it will only be 
	 * audible when the env is open.
	 *
	 * We should also consider whether this should go into the sfMix as well.
	 */
	s->out = (SID[id]->voice[0].osc[B_SID_RAMP].current
			+ SID[id]->voice[1].osc[B_SID_RAMP].current
			+ SID[id]->voice[2].osc[B_SID_RAMP].current)
			* B_S_F_SCALER * SID[id]->leakage;

	/*
	 * We are going to introduce signal to noise for the filter here also but
	 * it will use the previous noise sample. The results need to be seen.
	 *
	 * The S/N ratio is applied here plus a value that reflects the need to 
	 * denormal the filter on zero input. We use noise here although we could
	 * also prevent denormal of the filter by having some addition to the 
	 * oscillator leakage, that could be optional: if leakage and snratio
	 * are zero then use a denormal noise value, otherwise not. The optimisation
	 * is minimal.
	 */
	s->sfMix = ((float) SID[id]->voice[0].noise.value)
		* SID[id]->snratio * B_S_F_SCALER
		+ s->out;

	/* The 'sync/ringmod' sample needs to be scaled back from env expansion */
	bSidDoOsc(&(s->voice[B_SID_VOICE_1]),
		s->reg[B_SID_CONTROL], s->reg[B_SID_V1_CONTROL],
		s->voice[B_SID_VOICE_3].hold);

	bSidDoOsc(&(s->voice[B_SID_VOICE_2]),
		s->reg[B_SID_CONTROL], s->reg[B_SID_V2_CONTROL],
		s->voice[B_SID_VOICE_1].mix);

	bSidDoOsc(&(s->voice[B_SID_VOICE_3]),
		s->reg[B_SID_CONTROL], s->reg[B_SID_V3_CONTROL],
		s->voice[B_SID_VOICE_2].mix);

	s->reg[B_SID_X_ANALOGUE] = (s->voice[B_SID_VOICE_1].mix >> 4);
	s->reg[B_SID_Y_ANALOGUE] = (s->voice[B_SID_VOICE_2].mix >> 4);
	s->reg[B_SID_OSC_3_OUT] = (s->voice[B_SID_VOICE_3].mix >> 4);

	bSidDoEnv(&(s->voice[B_SID_VOICE_3]), s->reg[B_SID_V3_CONTROL]);
	bSidDoEnv(&(s->voice[B_SID_VOICE_2]), s->reg[B_SID_V2_CONTROL]);
	bSidDoEnv(&(s->voice[B_SID_VOICE_1]), s->reg[B_SID_V1_CONTROL]);

	s->reg[B_SID_ENV_3_OUT] = s->voice[B_SID_VOICE_3].env.UDcounter;

	/*
	 * We now have voices and envelopes, multiply them out and add them 
	 * together for testing.
	 */
	s->voice[B_SID_VOICE_1].mix *= ((s->voice[B_SID_VOICE_1].env.UDcounter
		* s->voice[B_SID_VOICE_1].env.UDcounter) >> 8);
	s->voice[B_SID_VOICE_2].mix *= ((s->voice[B_SID_VOICE_2].env.UDcounter
		* s->voice[B_SID_VOICE_2].env.UDcounter) >> 8);
	s->voice[B_SID_VOICE_3].hold = s->voice[B_SID_VOICE_3].mix;
	s->voice[B_SID_VOICE_3].mix *= ((s->voice[B_SID_VOICE_3].env.UDcounter
		* s->voice[B_SID_VOICE_3].env.UDcounter) >> 8);

	/*
	 * This mixing might be done well with some normalisation around zero
	 * however that is not trivial. All the waves accumulate up from zero.
	 */
	if (s->reg[B_SID_FILT_RES_F] & B_SID_F_MIX_V_1)
		s->sfMix += (s->voice[B_SID_VOICE_1].mix >> 8) * B_S_F_SCALER;
	else
		s->out += (s->voice[B_SID_VOICE_1].mix >> 8) * B_S_F_SCALER;

	if (s->reg[B_SID_FILT_RES_F] & B_SID_F_MIX_V_2)
		s->sfMix += (s->voice[B_SID_VOICE_2].mix >> 8) * B_S_F_SCALER;
	else
		s->out += (s->voice[B_SID_VOICE_2].mix >> 8) * B_S_F_SCALER;

	/* We need to test the MIX_3 bit here as well */
	if ((s->reg[B_SID_FILT_M_VOL] & B_SID_F_3_OFF) == 0)
	{
		if (s->reg[B_SID_FILT_RES_F] & B_SID_F_MIX_V_3)
			s->sfMix += (s->voice[B_SID_VOICE_3].mix >> 8) * B_S_F_SCALER;
		else
			s->out += (s->voice[B_SID_VOICE_3].mix >> 8) * B_S_F_SCALER;
	}

	/*
	 * We now have to pass sfMix through the filter back into s->out and return
	 * that as the result. We could consider bypassing the filter if all the
	 * voices are directed to the output however that would affect the snratio
	 * as it is only applied to the filter and it would be a pretty sparse 
	 * optimisation since the filter is going to be used if only for one voice
	 * at a time.
	 */
	s->sfMix *= 10000;
	bSidDoFilter(s);
	s->out += s->sfMix * 0.0004;

	return(s->out * s->volume * s->gn + s->dcbias); 
}

/*
 * Detune should be a number of cents since it is supposed to be mild rather
 * than a wild effect. Assume we are after up to 20 cents of difference.
 *
 * May turn this into 0..1, this gives us up to 2 octaves of spread on the
 * waves in multi which could be very nice.
 */
static float
bSidIODetune(int id, float param)
{
	if (_B_SID_DEBUG_A)
		printf("s %i: DETUNE %f cents\n", id, param);

	SID[id]->detune = powf(SID[id]->cent_diff, param);
	SID[id]->detune2 = 1 / SID[id]->detune;

	return(SID[id]->detune);
}

static float
bSidIOGain(int id, float param)
{
	if (_B_SID_DEBUG_A)
		printf("s %i: GAIN %f\n", id, param);
	return(SID[id]->gn = param);
}

static float
bSidIOLeakage(int id, float param)
{
	if (_B_SID_DEBUG_A)
		printf("s %i: LEAKAGE %f\n", id, param);
	return(SID[id]->leakage = param * param * 0.01);
}

static float
bSidIODCBias(int id, float param)
{
	if (_B_SID_DEBUG_A)
		printf("s %i: DC Bias %f\n", id, -0.5 + param);
	return(SID[id]->dcbias = -0.5 + param);
}

static float
bSidIOOberheim(int id, float param)
{
	if (_B_SID_DEBUG_A)
		printf("s %i: OB fmix %f\n", id, param * param);
	return(SID[id]->filter.fmix = param * param);
}

static float
bSidIOClockrate(int id, float param)
{
	if (_B_SID_DEBUG_A)
		printf("s %i: Clock %f\n", id, param);

	if ((param > 0.5) || (param < 3))
	{
		SID[id]->nominalclock = param * 1000000;
		/* And reload the frequency registers from current value to retune */
		sid_register(id, B_SID_V1_FREQ_LO, SID[id]->reg[B_SID_V1_FREQ_LO]);
		sid_register(id, B_SID_V1_FREQ_HI, SID[id]->reg[B_SID_V1_FREQ_HI]);
		sid_register(id, B_SID_V2_FREQ_LO, SID[id]->reg[B_SID_V2_FREQ_LO]);
		sid_register(id, B_SID_V2_FREQ_HI, SID[id]->reg[B_SID_V2_FREQ_HI]);
		sid_register(id, B_SID_V3_FREQ_LO, SID[id]->reg[B_SID_V3_FREQ_LO]);
		sid_register(id, B_SID_V3_FREQ_HI, SID[id]->reg[B_SID_V3_FREQ_HI]);
	}

	return(SID[id]->nominalclock);
}

static float
bSidIOSNRatio(int id, float param)
{
	if (_B_SID_DEBUG_A)
		printf("s %i: S/N ratio %f\n", id, param * param + 0.000000001);

	return(SID[id]->snratio = 0.1 * param * param + 0.000000001);
}

static float
bSidIODestroy(int id, float samplerate)
{
	bSid *tmp;

	if (_B_SID_DEBUG_A)
		printf("destroy %i\n", id);

	tmp = SID[id];
	SID[id] = NULL;

	bristolfree(tmp);

	return(0);
}

static float
bSidIOInit(int id, float samplerate)
{
	static int sidID = 0;

	while (SID[sidID] != 0)
		if (++sidID >= B_SID_COUNT)
			break;

	if (sidID == B_SID_COUNT)
		return(-1);

	SID[sidID] = (bSid *) bristolmalloc((size_t) sizeof(bSid));
	bzero((void *) SID[sidID], (size_t) sizeof(bSid));

	SID[sidID]->id = sidID;
	SID[sidID]->samplerate = samplerate;
	SID[sidID]->nominalclock = B_SID_MASTER_CLOCK;
	SID[sidID]->detune = 1.0;
	SID[sidID]->leakage = 0.01;
	SID[sidID]->snratio = 0.000001;

	bSidDispatch[B_SID_V1_FREQ_LO] = bSidVoice;
	bSidDispatch[B_SID_V1_FREQ_HI] = bSidVoice;
	bSidDispatch[B_SID_V1_PW_LO] = bSidVoice;
	bSidDispatch[B_SID_V1_PW_HI] = bSidVoice;
	bSidDispatch[B_SID_V1_CONTROL] = bSidVoice;
	bSidDispatch[B_SID_V1_ATT_DEC] = bSidVoice;
	bSidDispatch[B_SID_V1_SUS_REL] = bSidVoice;

	bSidDispatch[B_SID_V2_FREQ_LO] = bSidVoice;
	bSidDispatch[B_SID_V2_FREQ_HI] = bSidVoice;
	bSidDispatch[B_SID_V2_PW_LO] = bSidVoice;
	bSidDispatch[B_SID_V2_PW_HI] = bSidVoice;
	bSidDispatch[B_SID_V2_CONTROL] = bSidVoice;
	bSidDispatch[B_SID_V2_ATT_DEC] = bSidVoice;
	bSidDispatch[B_SID_V2_SUS_REL] = bSidVoice;

	bSidDispatch[B_SID_V3_FREQ_LO] = bSidVoice;
	bSidDispatch[B_SID_V3_FREQ_HI] = bSidVoice;
	bSidDispatch[B_SID_V3_PW_LO] = bSidVoice;
	bSidDispatch[B_SID_V3_PW_HI] = bSidVoice;
	bSidDispatch[B_SID_V3_CONTROL] = bSidVoice;
	bSidDispatch[B_SID_V3_ATT_DEC] = bSidVoice;
	bSidDispatch[B_SID_V3_SUS_REL] = bSidVoice;

	bSidDispatch[B_SID_FILT_LO] = bSidFilter;
	bSidDispatch[B_SID_FILT_HI] = bSidFilter;
	bSidDispatch[B_SID_FILT_RES_F] = bSidFilter;
	bSidDispatch[B_SID_FILT_M_VOL] = bSidFilter;

	bSidDispatch[B_SID_X_ANALOGUE] = bSidGet;
	bSidDispatch[B_SID_Y_ANALOGUE] = bSidGet;

	bSidDispatch[B_SID_OSC_3_OUT] = bSidGet;
	bSidDispatch[B_SID_ENV_3_OUT] = bSidGet;

	bSidDispatch[B_SID_CONTROL] = bSidControl;

	/*
	 * Set up the analogue (float) dispatch routines
	 */
	bSidIODispatch[B_SID_DESTROY] = bSidIODestroy;
	bSidIODispatch[B_SID_ANALOGUE_IO] = bSidIOAnalogue;
	bSidIODispatch[B_SID_GAIN] = bSidIOGain;
	bSidIODispatch[B_SID_DETUNE] = bSidIODetune;
	bSidIODispatch[B_SID_SN_RATIO] = bSidIOSNRatio;
	bSidIODispatch[B_SID_SN_LEAKAGE] = bSidIOLeakage;
	bSidIODispatch[B_SID_DC_BIAS] = bSidIODCBias;
	bSidIODispatch[B_SID_OBERHEIM] = bSidIOOberheim;
	bSidIODispatch[B_SID_CLOCKRATE] = bSidIOClockrate;

	/*
	 * We continue with a B_SID_C_RESET call although this should be made by
	 * the initialising/calling code anyway.
	 */
	sid_register(sidID, B_SID_CONTROL, B_SID_C_RESET);

	return((float) sidID);
}

/*
 * Demultiplexor for address being given, parse the value into the registers and
 * interpret them into our local structure, finally return the current content.
 */
int
sid_register(int id, unsigned char address, unsigned char value)
{
	if ((id >= B_SID_COUNT) || (id < 0) || (SID[id] == NULL)
		|| (address >= B_SID_REGISTERS))
		return(0xff);

	if (_B_SID_DEBUG)
		printf("id %x: address 0x%02x(%02i) := 0x%02x\n",
			id, address, address, value);

	return(bSidDispatch[address](id, address, value));
}

float
sid_IO(int id, int command, float param)
{
	if (command == 0)
		return(bSidIOInit(-1, param));

	if ((id >= B_SID_COUNT)  || (id < 0) || (SID[id] == NULL) || (command < 0)
		|| (command >= B_SID_IO) || (bSidIODispatch[command] == NULL))
		return(-1);

	return(bSidIODispatch[command](id, param));
}

