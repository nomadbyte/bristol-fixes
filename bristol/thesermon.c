
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
 * The sermon generates the waves for all the oscillators required by a hammond.
 * Different sources put this at 91, 92, or 95 oscillators. By my reconning we
 * need one for every key on the hammond (61), plus the range of the drawbars,
 * (5 octaves = 62). In short, the lowest drawbar goes 12 notes below the 
 * bottom key, and the highest note goes 3 octaves (36 notes) above, or rather
 *		61 + 12 + 36 = 109 frequencies.
 * We pull back to 92 using "foldback", which is reusing different bus routings
 * on the uppermost and lowermost ranges. The original has 24 compartments with
 * 4 wheels each giving 96 wheels however 4 were 'dummies' with no teeth just
 * installed for balancing the mechanics. This is characterised in the bristol
 * tonewheel definition file.
 *
 * We also need to look at introducing "tonewheel damping", ie, when a given
 * wheel is used more than once then its gain does not increment linearly,
 * rather the gain tails off with more taps. This happens when a wheel is used
 * as a harmonic rather than a fundamental - the first time the gain is 1.0,
 * the second time it is not 2.0 after 2 uses, but perhaps 1.5, which should
 * be configurable. DONE 28/11/04.
 *
 * Want to introduce the following:
 *
 *	EQ: control the gains of each wave according to a profile. OK
 *	EQ: drawbus profiles. OK
 *	Distorts: control the harmonics of the wave, also with a profile. OK
 *	Rework chorus (other file) OK - need to take another filtered delay algo.
 *	Always pass through Leslie - have it stop. OK
 *	Added in random phase difference between teeth. OK
 *	Crosstalk: between teeth of same compartment. OK
 *	Crosstalk: between drawbars. OK
 *	Crosstalk: between box output filters. OK
 *	Drawbar leakage. OK
 *	tapering. OK
 *	Check foldback, seem to be too much - damaged for damping. OK (tapering)
 *	Drawbars to full 9 stage swing. OK
 *	Drawbars graphics to be numbered. OK
 *	Percussive bypass of chorus plus add in HPF. Bypass OK. HPF OK.
 *	New reverb (other file). Reworked reverb for 2600/Aks too. OK.
 *	Fixed ALSA Seq registered device name. OK.
 *	Fix parameter damage on lower manual. There were two, resolved. OK
 *	Put diverse defaults tables into a header file. OK
 *	Fix lost note issues. OK. Timing issue between threads.
 *	Fix memory linkage failure on exit. Effects mem allocation error. OK.
 *
 *	Optimisation: only generate a wave when it is tapped.
 *
 *		This is larger issue. We should have a prestaging that takes a list of
 *		all the notes that have been pressed. It will build a table of which
 *		tonewheels will be tapped for the notes, the drawbars and crosstalk 
 *		and then generate only those waves. We might consider just building a
 *		gain table basing it on crosstalk and the resistive network for damping
 *		and generate non-zero waves. At the moment (04/18/07) we generate all
 *		tonewheels even when not being tapped, then later apply the gains
 * 		to them, and crosstalk may be called multiple times for some waves.
 *		It will be more efficient to build a gain table, then generate only
 *		the waves needed already with those gains and mix them down.
 *		This requires a bit of rearchitecting. Currently the gearbox waves are
 *		built in the pre-ops and used in the runtime operation. Pre-ops would
 *		have to reset any variables, run time would build the volumes table
 *		as it is called once per note, and then post-ops should build the
 *		actual output. This would fit into the operate() code used in hammond.c
 *		would just have to watch out for the percussives. This would also allow
 *		for some neat key click options.
 *
 *	Axle jitter - hm.
 *	Extend the rotary speaker algorithm.
 *	Other key click
 *	Reorganise volumes (click too loud?)
 *
 * Finallly we need to build a set of defaults for the normal and bright tab.
 */

#include <math.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bristol.h"
#include "bristolmidi.h"
#include "hammond.h"

#define _THE_SERMON /* This takes a float for the click source */
#include "click.h"

/*
 * ToneMatrix is another way to generate the tonewheels. The preacher is called
 * for each key that is pressed and it builds an array of gain levels for its
 * wheels, crosstalks and leakages. In the post ops we call 'requiem' that then
 * generates the tonewheels required at the given volumes, a lot more efficient.
 *
 * We need to define the feature here as thesermon.h file depends on it too.
 */
/*#define TONEMATRIX */

#include "thesermon.h"

static void fillsquarewave(float *, int, double, int);
static void filltriwave(float *, int, double, int);

/*
 * We count make all these arrays into a single array or structure, something
 * like the following, but we would have to rearrange all the iterpolation
 * routines, so perhaps not, they are kind of nice and easy to do.
 *
 * So we keep the simple arrays. For the following I could introduce different
 * bright and normal settings but these are not really concerned with those
 * settings. FFS.
 */
typedef struct Crosstalk {
	int wheel;
	float gain;
} crosstalk;

typedef struct Tonewheels {
	float freq; /* Hz */
	float step; /* wavetable index step required to generate this frequency */
	float flutter;
	float EQ;
	float damping;
	float phase;
	crosstalk crosstalk[2][XT_COUNT]; /* crosstalk levels for compartment. */
	float stops[BUS_COUNT];
	float taper[BUS_COUNT];
	/* There are more, we could bury the wavetables and buffers in here. */
} tonewheels;

tonewheels gearbox[92];

/* Bright should be burried in the tonewheel structure */
static int bright, btnote, btdelay = 0, samplerate;

#ifdef TONEMATRIX
struct {
	float gain;
	int percussive;
	float numerator;
	int usecount;
} tonematrix[OSC_LIMIT];
#endif /* TONEMATRIX */

void
therequiem(register float *buf, register float *pbuf, int samplecount)
{
#ifdef TONEMATRIX
	register int wheel, count;
	register float index, *source, *dest, freq, gain;

	for (wheel = 0; wheel < OSC_LIMIT; wheel++)
	{
		/*
		 * See if we have this wheel tapped for output. If we do then we need
		 * to generate the tone. If not we should just increment its index
		 * as if it had been tapped just to keep the phase in sync.
		 */
		if (tonematrix[wheel].gain != 0.0)
		{
/*
if (tonematrix[wheel].usecount > 0)
printf("Wheel %i: %2.2f, %i, %2.2f, %2.2f %i: %2.2f\n", 
wheel,
tonematrix[wheel].gain,
tonematrix[wheel].percussive,
tonematrix[wheel].numerator,
(float) 24/tonematrix[wheel].usecount,
tonematrix[wheel].usecount,
((float) (24/tonematrix[wheel].usecount)) /
(tonematrix[wheel].numerator+24/tonematrix[wheel].usecount));
else
printf("Wheel %i: ct %2.2f\n", wheel, tonematrix[wheel].gain);
*/
			source = &wheeltemplates[bright][wheel][0];

			index = toneindeces[wheel];

			if (tonematrix[wheel].percussive != 0)
				dest = pbuf;
			else
				dest = buf;

			gain = tonematrix[wheel].gain * 4;
			if (tonematrix[wheel].usecount)
				gain *= ((float) (24/tonematrix[wheel].usecount)) /
					(tonematrix[wheel].numerator
						+ 24/tonematrix[wheel].usecount);

			if (tonematrix[wheel].percussive != 0) {
				dest = pbuf;
				gain *= 2;
			} else
				dest = buf;

			/*
			 * For the clutching system the diverse axles are not in sync. This
			 * can be reproduced by altering the frequencies by small amounts
			 * up then down by different times by groups of wheels.
			 */
			freq = gearbox[wheel].step; /*gearings[wheel]; */
			count = samplecount;

			/*
			 * This should be resampled, something that would be quite intensive
			 * so may leave it for future study since the current code already 
			 * uses about 350 MIPS.
			 */
			do {
				*dest++ += source[(int) index] * gain;
				if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
				*dest++ += source[(int) index] * gain;
				if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
				*dest++ += source[(int) index] * gain;
				if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
				*dest++ += source[(int) index] * gain;
				if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
				*dest++ += source[(int) index] * gain;
				if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
				*dest++ += source[(int) index] * gain;
				if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
				*dest++ += source[(int) index] * gain;
				if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
				*dest++ += source[(int) index] * gain;
				if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
			} while ((count -= 8) > 0);

			toneindeces[wheel] = index;
		}
	}

	bzero(tonematrix, sizeof(tonematrix));
#endif /* TONEMATRIX */
}

/*
 * The ctab here is a correct equally tempered index couters to generate waves.
 * This does not really match the hammond as it was not actally ET. A separate
 * table is used by the sermon to give it the hammond generated frequencies
 */
void
thesermon(int samplecount, int sineform)
{
	register int wheel, count;
	register float index, *source, *dest, freq;

	bright = sineform == 0? 0: 1;

	/*
	 * We now have 8 waves, which are anything from sines, leading edge sines,
	 * tris and ramps. We now need to fill all 92 tables with the number of 
	 * samples required for the next buffer SAMPLE_COUNT samples from these
	 * wave tables. Choice of wave will initially be parameterised, but 
	 * eventually we should be looking to select the wave based on the octave
	 * being stuffed.
	 *
	 * We could consider only filling this table fully when a given key is 
	 * requested. That may have an effect on overall phasing though.
	 */
	for (wheel = 0; wheel < OSC_LIMIT; wheel++)
	{
#ifndef TONEMATRIX
		source = &wheeltemplates[bright][wheel][0];

		index = toneindeces[wheel];
		dest = tonewheel[wheel];
		/*
		 * For the clutching system the diverse axles are not in sync. This
		 * can be reproduced by altering the frequencies by small amounts up,
		 * then down by different times by groups of wheels.
		 */
		freq = gearbox[wheel].step; /*gearings[wheel]; */
		count = samplecount;

		/*
		 * This should be resampled, something that would be quite intensive
		 * so may leave it for future study since the current code already 
		 * uses about 350 MIPS.
		 */
		do {
			*dest++ = source[(int) index];
			if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
			*dest++ = source[(int) index];
			if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
			*dest++ = source[(int) index];
			if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
			*dest++ = source[(int) index];
			if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
			*dest++ = source[(int) index];
			if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
			*dest++ = source[(int) index];
			if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
			*dest++ = source[(int) index];
			if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
			*dest++ = source[(int) index];
			if ((index += freq) > WAVE_SIZE) index -= WAVE_SIZE;
		} while ((count -= 8) > 0);

		toneindeces[wheel] = index;
#endif

		/*
		 * Set up the gain level for tonewheel damping. Starts at full gain.
		 *
		 * This will change, it should start at -ve indicating the wave has 
		 * not been filled? Perhaps not due to phasing issues although the 
		 * hammond was not tightly phased anyway. What we do want is for the
		 * initial gain not to be '1', but based on some profile to allow for
		 * bottom heavy equalisation, top heavy and light notching.
		 * This will also be used for the 'bright' control in the GUI.
		 */
		tonegains[wheel] = toneEQ[bright][wheel];
	}
}

/*
 * This is the bussing algorithm. We should consider bus delays for each bus
 * on key_on, so that each bus is activated at a slightly different time, as
 * per a normal multitap mechanical key. This would probably improve the key
 * click effect.
 *
 * We should consider passing the voice structure as it would reduce the 
 * parameter count.
 */
void
preach(register float *buf, register float *pbuf, int note, int *gains,
int *percs, register int count, float gn, float damp, float cg, int flags,
float velocity)
{
	register int bus, i, j, index;
	register float gain = 0, *dest = buf, *source;
#ifdef NEW_CLICK
	register float clickg;
#endif

	cg *= 0.125;

	/*
	 * We should check the note values and discard any requests outside of 
	 * our valid range (0 to 60. The reason we need to check this regards the
	 * toothwheel tables that are sized for 92 teeth.
	 */
	while (note < 0)
		note+=12;
	while (note > 60)
		note-=12;

	/*
	 * If we have a note on event then we need to reset our offset counters
	 * and evaluate the offset timers for bus delays.
	 *
	 * We then have some pretty ugly changes to make such that a given bus
	 * will not start producing sound until that bus delay offset has been
	 * reached, something that is a fraction of a buffer and affects all the
	 * crosstalk options although we could be lazy and leave crosstalk until
	 * the first full buffer when the bus should produce sound? That might
	 * work reasonably anyway, due to the staggered busses.
	 */
#ifdef NEW_CLICK
	/*
	 * We should not do this for REON as the bus delay will introduce another
	 * unrequested tick. REON for this emulator will not happen often as it
	 * has short decay and plenty of voices typically.
	if (flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
	 */
	if (flags & BRISTOL_KEYON)
	{
//printf("note on %f: %i %i\n", velocity, note, bright);
		drawbars.offset[note] = 0;

//printf("velocity %f = ", velocity);
//		velocity = velocity * velocity * velocity * velocity;
		velocity = 1.0f - velocity;
		velocity = velocity * velocity;
//printf("%f\n", velocity);

		/*
		 * If this is the same note then we do note not change the bus delay
		 * offsets: repetitive playing of the same notes will give the same
		 * set of transient delays. If we are playing different note sequences
		 * then this will ensure that any given drawbar bus contact profile
		 * changes over time.
		 *
		 * Without this code then the note busbar delays are always the same
		 * for every note on the keyboard (well, apart from the effect of
		 * velocity).
		 */
		if ((btnote != note) && (++btdelay > 8))
			btdelay = 0;

		for (bus = 0; bus < BUS_COUNT; bus++)
		{
			drawbars.current[btdelay][note] = 0;
			drawbars.tdelay[btdelay][note] = (int)
				drawbars.delay[bright][bus] * velocity;
			if (++btdelay >= BUS_COUNT)
				btdelay = 0;
		}

		btnote = note;

		/*
		printf("preacher note %i on event: %i %i %i %i %i %i %i %i %i\n", note,
			drawbars.tdelay[0][note], drawbars.tdelay[1][note],
			drawbars.tdelay[2][note], drawbars.tdelay[3][note],
			drawbars.tdelay[4][note], drawbars.tdelay[5][note],
			drawbars.tdelay[6][note], drawbars.tdelay[7][note],
			drawbars.tdelay[8][note]);
		printf("%f: %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			velocity,
			drawbars.gain[bright][0], drawbars.gain[bright][1],
			drawbars.gain[bright][2], drawbars.gain[bright][3], drawbars.gain[bright][4], drawbars.gain[bright][5],
			drawbars.gain[bright][6], drawbars.gain[bright][7],
			drawbars.gain[bright][8]);
		 */
		/*
		printf("%p, %p, %p, %p, %p, %p, %p, %p, %p\n", 
		&waves[drawbars.pulse[bright][0]][0],
		&waves[drawbars.pulse[bright][1]][0],
		&waves[drawbars.pulse[bright][2]][0],
		&waves[drawbars.pulse[bright][3]][0],
		&waves[drawbars.pulse[bright][4]][0],
		&waves[drawbars.pulse[bright][5]][0],
		&waves[drawbars.pulse[bright][6]][0],
		&waves[drawbars.pulse[bright][7]][0],
		&waves[drawbars.pulse[bright][8]][0]);
		printf("%i, %i, %i, %i, %i, %i, %i, %i, %i\n", 
		drawbars.pulse[bright][0], drawbars.pulse[bright][1],
		drawbars.pulse[bright][2], drawbars.pulse[bright][3],
		drawbars.pulse[bright][4], drawbars.pulse[bright][5],
		drawbars.pulse[bright][6], drawbars.pulse[bright][7],
		drawbars.pulse[bright][8]);
		 */
	} else
		drawbars.offset[note] += count;
#endif

	for (bus = 0; bus < BUS_COUNT; bus++)
	{
		/*
		 * We need to do foldback checks. If the note is zero then there is one
		 * octave of foldback for fundamentals below this note due to the index
		 * of the tonewheels.
		 */
		index = wheelnumbers[note + offsets[bus]];
#ifdef TONEMATRIX
		/*
		 * This should perhaps include some stuff on damping.
		 */
THE TONEMATRIX IS BROKEN FOR THE NEW BUSDELAYS
		if (gains[bus] > 0)
			tonematrix[index].usecount++;

			if (percs[bus])
				tonematrix[index].percussive++;

			/*
			 * Numerator and denominator only work if tapers are defined as R
			 * values, not gains, which for now is in the rtapers array.
			 */
			if (tonematrix[index].numerator == 0)
				tonematrix[index].numerator = rtapers[note][bus];
			else
				tonematrix[index].numerator = 1/(1/rtapers[note][bus]
					+ 1/tonematrix[index].numerator);

			tonematrix[index].gain +=
				gn * busEQ[bright][bus] * gearbox[note].stops[gains[bus]]
				* tonegains[index] * gearbox[note].taper[gains[bus]];

			tonegains[index] *= (damp * damping[bright][index]);
		}
#else /* ELSE NOT TONEMATRIX */
		/*
		 * This is the unoptimized pre-tonematrix code that is currently 
		 * compiled with the engine (0.10.2). The optimized code is not yet
		 * finished.
		 */
		if (percs[bus])
			dest = pbuf;
		else
			dest = buf;

		source = tonewheel[index];

		/*
		 * We now have a selected tonewheel index, but before we can build the
		 * wave into the output stream we need to evaluate the gain, which is a
		 * function of the number of times this wheel as been used, over a
		 * damping factor (as yet fixed). This is the tonewheel damping factor.
		 */
		if ((gain = gn * busEQ[bright][bus] * gearbox[note].stops[gains[bus]]
			* tonegains[index] * gearbox[note].taper[gains[bus]]) <= 0)
		{
			tonegains[index] *= (damp * damping[bright][index]);
			continue;
		}

		/*
		 * When we have taken from this tap then we need to reduce its next gain
		 * due to damping or 'volume stealing' or whatever people want to call
		 * it
		 */
		tonegains[index] *= (damp * damping[bright][index]);

#ifdef NEW_CLICK
		/*
		 * There are several cases, one where the bus is silent, ie, it
		 * has not yet made contact, one where the bus is partially silent
		 * one where it is totally mixed with click, and one where click
		 * is ending and the normal state of just the bus data.
		 */
		if ((drawbars.offset[note] > (drawbars.tdelay[bus][note] + CLICK_SIZE))
			|| percs[bus])
		{
#endif
			/*
			 * Tap off the bus.
			 */
			for (j = count; j; j-=8)
			{
				*dest++ += *source++ * gain;
				*dest++ += *source++ * gain;
				*dest++ += *source++ * gain;
				*dest++ += *source++ * gain;
				*dest++ += *source++ * gain;
				*dest++ += *source++ * gain;
				*dest++ += *source++ * gain;
				*dest++ += *source++ * gain;
			}
#ifdef NEW_CLICK
		}
		else if ((drawbars.offset[note] + count) < drawbars.tdelay[bus][note])
		{
//printf("no contact: %i: %f\n", bus, cg);
			/* 
			 * Contact not yet made
			 */
			continue;
		}
		else if ((drawbars.offset[note] + count) >
			(drawbars.tdelay[bus][note] + CLICK_SIZE))
		{
			/*
			 * Buffer will end after the end of the click. Mix with the click
			 * buffer up to its end, then just the data buffers.
			float *clickp = &click[drawbars.offset[note]
				- drawbars.tdelay[bus][note]];
			 */
			float *clickp = &(waves[drawbars.pulse[bright][bus]])
				[(int)drawbars.offset[note] - drawbars.tdelay[bus][note]];

			clickg = cg * drawbars.gain[bright][bus];

//printf("second half buffer: %i %f\n", bus, clickg);

			int halfway = drawbars.tdelay[bus][note] + CLICK_SIZE
				- drawbars.offset[note];

			for (j = 0; j > halfway; j++)
				*dest++ += *source++ * gain + *clickp++ * clickg;

			for (; j < count; j++)
				*dest++ += *source++ * gain;
		}
		else if ((drawbars.offset[note] < drawbars.tdelay[bus][note])
			&& ((drawbars.offset[note] + count) > drawbars.tdelay[bus][note]))
		{
			float *clickp = &(waves[drawbars.pulse[bright][bus]])[0];

			clickg = cg * drawbars.gain[bright][bus];
//printf("first half buffer: %i %f: %i - %i = %i\n", bus, clickg,
//drawbars.tdelay[bus][note], drawbars.offset[note],
//(drawbars.tdelay[bus][note] - drawbars.offset[note]));

			/* 
			 * Note started halfway through buffer
			 */
			dest += (drawbars.tdelay[bus][note] - drawbars.offset[note]);
			source += (drawbars.tdelay[bus][note] - drawbars.offset[note]);

			j = count - (drawbars.tdelay[bus][note] - drawbars.offset[note]);

			for (; j; j--)
				*dest++ += *source++ * gain + *clickp++ * clickg;
		}
		else
		{
			float *clickp = &(waves[drawbars.pulse[bright][bus]])
				[(int) drawbars.offset[note] - drawbars.tdelay[bus][note]];

			clickg = cg * drawbars.gain[bright][bus];
//printf("full buffer: %i %f\n", bus, clickg);

			/*
			 * Just mix with click
			 */
			for (j = count; j; j-=8)
			{
				*dest++ += *source++ * gain + *clickp++ * clickg;
				*dest++ += *source++ * gain + *clickp++ * clickg;
				*dest++ += *source++ * gain + *clickp++ * clickg;
				*dest++ += *source++ * gain + *clickp++ * clickg;
				*dest++ += *source++ * gain + *clickp++ * clickg;
				*dest++ += *source++ * gain + *clickp++ * clickg;
				*dest++ += *source++ * gain + *clickp++ * clickg;
				*dest++ += *source++ * gain + *clickp++ * clickg;
			}
		}
#endif /* NEW_CLICK */

#endif /* NOT TONEMATRIX */

/*
printf("gains %i are %f %f %f\n",
index,
gearbox[index].crosstalk[0].gain,
gearbox[index].crosstalk[1].gain,
gearbox[index].crosstalk[2].gain);
*/

		/*
		 * If this drawbar is stopped then we don't expect any further
		 * crosstalk, or if the bus is not producing sound then skip the
		 * crosstalk.
		 */
		if ((gains[bus] == 0)
			|| (drawbars.offset[note] < drawbars.tdelay[bus][note]))
			continue;

		for (i = 0; i < XT_COUNT; i++)
		{
			if ((i == XT_DRAWBAR) || (i == XT_FAR_2))
				continue;

			/*
			 * Then seven lots of crosstalk. We could consider collapsing all
			 * of this into a single loop, would be no less efficient.
			 */
			if (gearbox[index].crosstalk[bright][i].wheel >= 0) {
#ifdef TONEMATRIX
				tonematrix[gearbox[index].crosstalk[bright][i].wheel].gain
					+= gearbox[index].crosstalk[bright][i].gain;
#else /* TONEMATRIX */
				source = tonewheel[gearbox[index].crosstalk[bright][i].wheel];
				if (percs[bus])
					dest = pbuf;
				else
					dest = buf;
				gain = gearbox[index].crosstalk[bright][i].gain;

				for (j = count; j; j-=16)
				{
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
				}
#endif /* NOT TONEMATRIX */
			}
		}

		/*
		 * Then there are up to two more crosstalks from adjacent drawbars. 
		 * I think this was known as bus crosstalk or drawbar leakage and I am
		 * not certain if they were the same thing.
		 */
		switch (bus) {
			case 0:
#ifdef TONEMATRIX
				tonematrix[wheelnumbers[note + offsets[1]]].gain
					+= defct[bright][XT_DRAWBAR];
#else
				/* from one bus up */
				source = tonewheel[wheelnumbers[note + offsets[1]]];
				gain = defct[bright][XT_DRAWBAR];


				if (percs[bus])
					dest = pbuf;
				else
					dest = buf;

				for (j = count; j; j-=16)
				{
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
				}
#endif /* TONEMATRIX */
				break;
			case 8:
#ifdef TONEMATRIX
				tonematrix[wheelnumbers[note + offsets[7]]].gain
					+= defct[bright][XT_DRAWBAR];
#else
				/* from one bus down */
				source = tonewheel[wheelnumbers[note + offsets[7]]];
				gain = defct[bright][XT_DRAWBAR];


				if (percs[bus])
					dest = pbuf;
				else
					dest = buf;

				for (j = count; j; j-=16)
				{
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
				}
#endif /* TONEMATRIX */
				break;
			default:
#ifdef TONEMATRIX
				tonematrix[wheelnumbers[note + offsets[bus + 1]]].gain
					+= defct[bright][XT_DRAWBAR];
#else
				/* from two adjacent busses. */
				source = tonewheel[wheelnumbers[note + offsets[bus + 1]]];
				gain = defct[bright][XT_DRAWBAR];


				if (percs[bus])
					dest = pbuf;
				else
					dest = buf;

				for (j = count; j; j-=16)
				{
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
				}

				source = tonewheel[wheelnumbers[note + offsets[bus - 1]]];

				if (percs[bus])
					dest = pbuf;
				else
					dest = buf;

#endif /* else TONEMATRIX */
#ifdef TONEMATRIX
				tonematrix[wheelnumbers[note + offsets[bus - 1]]].gain
					+= defct[bright][XT_DRAWBAR];
#else

				for (j = count; j; j-=16)
				{
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
					*dest++ += *source++ * gain;
				}
#endif /* TONEMATRIX */
				break;
		}
	}
}

static char *
strnext(char *source)
{
	char *offset;

	/*
	 * Scan through the source until we find a member of 'whitespace'. Then
	 * return a pointer to the next character that is not a member of the same
	 * string.
	 */
	if ((source == NULL) || (*source == '\0'))
		return(NULL);

	/*
	 * So we have two valid strings. Find next occurence of whitespace and
	 * then skip over it.
	 */
	if ((offset = strpbrk(source, " 	")) == NULL)
		return(NULL);

	return(offset + strspn(offset, " 	"));
}

static void
parseTaper(int comp, char *line)
{
	char *offset;
	int note, res, drawbar = 0;

	if ((note = atoi(line)) != comp)
		return;

	offset = line;

	/*
	 * We now have 9 values. If they start with 'R' it is a resitor index,
	 * otherwise a literal value.
	 */
	for (;drawbar < BUS_COUNT; drawbar++)
	{
		if ((offset = strnext(offset)) == NULL)
			return;

		/*
		 * There are 3 possiblities, a literal floating value, an R followed 
		 * by an array index, or R followed by the resistance if which we have
		 * just a few recognised values.
		 */
		if (*offset == 'R') {
			if ((res = atoi(++offset)) < 0)
				return;

			if (res < 6)
				tapers[note][drawbar] = tresistors[res];
			else {
				switch(res) {
					case 100:
/*						gearbox[note].taper[drawbar] = tresistors[0]; */
						tapers[note][drawbar] = tresistors[0];
						break;
					case 50:
						tapers[note][drawbar] = tresistors[1];
						break;
					case 34:
						tapers[note][drawbar] = tresistors[2];
						break;
					case 24:
						tapers[note][drawbar] = tresistors[3];
						break;
					case 15:
						tapers[note][drawbar] = tresistors[4];
						break;
					case 10:
						tapers[note][drawbar] = tresistors[5];
						break;
					default:
						tapers[note][drawbar]
							= 24.0 / (((float) res) + 24.0);
						break;
				}
			}
		} else {
			tapers[note][drawbar] = atof(offset);
		}
	}
	/*
	printf("%i: %f %f %f %f %f %f %f %f %f\n", note,
		tapers[note][0], tapers[note][1],
		tapers[note][2], tapers[note][3],
		tapers[note][4], tapers[note][5],
		tapers[note][6], tapers[note][7],
		tapers[note][8]);
	 */
}

static void
parseCompartment(int comp, char *line, int bright)
{
	char *offset;
	int wheel;
	float x1, x2, x3, x4;

	if (comp != atoi(line))
		return;

	if ((wheel = atoi(offset = strnext(line))) >= 0)
		compartments[comp][0] = wheel;
	if ((wheel = atoi(offset = strnext(offset))) >= 0)
		compartments[comp][1] = wheel;
	if ((wheel = atoi(offset = strnext(offset))) >= 0)
		compartments[comp][2] = wheel;
	if ((wheel = atoi(offset = strnext(offset))) >= 0)
		compartments[comp][3] = wheel;
	if (*offset == '-')
		compartments[comp][3] = -1;

	/*
	 * See if gain levels have been defined in the file.
	 */
	if ((offset = strnext(offset)) == 0)
		return;
	x1 = atof(offset);
	if ((offset = strnext(offset)) == 0)
		return;
	x2 = atof(offset);
	if ((offset = strnext(offset)) == 0)
		return;
	x3 = atof(offset);
	if ((offset = strnext(offset)) == 0)
		return;
	x4 = atof(offset);

	gearbox[compartments[comp][0]].crosstalk[bright][0].gain = x1;
	gearbox[compartments[comp][0]].crosstalk[bright][1].gain = x3;
	gearbox[compartments[comp][0]].crosstalk[bright][2].gain = x4;

	gearbox[compartments[comp][1]].crosstalk[bright][0].gain = x2;
	gearbox[compartments[comp][1]].crosstalk[bright][1].gain = x4;
	gearbox[compartments[comp][1]].crosstalk[bright][2].gain = x3;

	gearbox[compartments[comp][2]].crosstalk[bright][0].gain = x1;
	gearbox[compartments[comp][2]].crosstalk[bright][1].gain = x3;
	gearbox[compartments[comp][2]].crosstalk[bright][2].gain = x4;

	if (compartments[comp][3] >= 0)
	{
		gearbox[compartments[comp][3]].crosstalk[bright][0].gain = x2;
		gearbox[compartments[comp][3]].crosstalk[bright][1].gain = x4;
		gearbox[compartments[comp][3]].crosstalk[bright][2].gain = x3;
	}

/*
printf("%1.2f %1.2f %1.2f, %1.2f %1.2f %1.2f,\n%1.2f %1.2f %1.2f, %1.2f %1.2f %1.2f\n",
	gearbox[compartments[comp][0]].crosstalk[bright][0].gain,
	gearbox[compartments[comp][0]].crosstalk[bright][1].gain,
	gearbox[compartments[comp][0]].crosstalk[bright][2].gain,

	gearbox[compartments[comp][1]].crosstalk[bright][0].gain,
	gearbox[compartments[comp][1]].crosstalk[bright][1].gain,
	gearbox[compartments[comp][1]].crosstalk[bright][2].gain,

	gearbox[compartments[comp][2]].crosstalk[bright][0].gain,
	gearbox[compartments[comp][2]].crosstalk[bright][1].gain,
	gearbox[compartments[comp][2]].crosstalk[bright][2].gain,

	if (compartments[comp][3] >= 0)
	{
		gearbox[compartments[comp][3]].crosstalk[bright][0].gain,
		gearbox[compartments[comp][3]].crosstalk[bright][1].gain,
		gearbox[compartments[comp][3]].crosstalk[bright][2].gain);
	}

	printf("%i %i %i %i\n",
	compartments[comp][0],
	compartments[comp][1],
	compartments[comp][2],
	compartments[comp][3])
*/
}

static void
parseWheel(char *line, tonewheels *gear)
{
/*printf("parseWheel %s", line); */
	/*
	 * Expecting to stuff the gear with the following:
	 * [freq|-] <c1|-> <g1> <c2|-> <g2> <c3|-> <g3>
	 *
	 * We have to scan to the next parameter skipping the wheel and any 
	 * subsequent blank space (<Space><tab>) and get the next parameter.
	 */
	if (*line != '-')
		gear->freq = atof(line);

	/*
	 * Moved crosstalk to the compartments and crosstalk tables.
	 */
	if (*(line = strnext(line)) != '-')
		gear->phase = atof(line);

	/*
	printf("%i %f %i %f %i %f: %f %f\n",
		gear->crosstalk[0].wheel,
		gear->crosstalk[0].gain,
		gear->crosstalk[1].wheel,
		gear->crosstalk[1].gain,
		gear->crosstalk[2].wheel,
		gear->crosstalk[2].gain,
		gear->phase,
		gear->freq);
	 */
}

static void
bristolHammondGetGears(char *file, char *match, tonewheels *gearbox, int count)
{
	FILE *fd;
	char path[256], *offset;
	char param[256];

	/*
	 * Open and read configuration
	 */
	sprintf(path, "%s/memory/profiles/%s", getBristolCache(file), file);

	/*
	 * If we can open the file then process it.
	 */
	if ((fd = fopen(path, "r")) == NULL)
	{
		sprintf(path, "%s/memory/profiles/%s", getenv("BRISTOL"), file);
		if ((fd = fopen(path, "r")) == NULL)
			return;
	}

	while (fgets(param, 256, fd) != NULL)
	{
		if (param[0] == '#')
			continue;
		if (strlen(param) < 5)
			continue;

		if (strncmp(param, "taper", 5) == 0)
		{
			parseTaper(atoi(strnext(param)), strnext(param));
			continue;
		}

		if (strncmp(param, "compartment", 11) == 0)
		{
			parseCompartment(atoi(strnext(param)), strnext(param), NORMAL);
			continue;
		}

		if (strncmp(param, match, strlen(match)) == 0)
		{
			int wheel;

			/*
			 * We should now have a line with something like:
			 *
			 * "wheel: <int> <freq> <phase>"
			 *
			 * HOWEVER, it will include lots of other stuff eventually!
			 *
			 * Parse it and stuff the gearbox. Accept negative gears for
			 * the empty balancing plates. Freq is optional, we will use it
			 * for the 192-teeth wheels as they had slightly flat frequency.
			 */
			if ((offset = strnext(param)) == NULL)
				continue;

			if ((wheel = atoi(offset)) < 0)
				continue;

			if (wheel >= count)
				continue;

			if ((offset = strnext(offset)) == NULL)
				continue;

			parseWheel(offset, &gearbox[wheel]);
/* printf("%i becomes %f\n", wheel, gearbox[wheel].freq); */
		}
	}

	fclose(fd);
}

#ifdef NOT_USED
static void
bristolHammondGetMap(char *file, char *match, float points[], int count)
{
	int i, n = 0;
	FILE *fd;
	float from, delta;
	char path[256];
	char param[256];

	/*
	 * Open and read configuration. Should consider seaching
	 * $HOME/.bristol/memory and $BRISTOL_DB/memory.
	 */
	sprintf(path, "%s/memory/profiles/%s", getBristolCache(file), file);

	/*
	 * If we can open the file then clean the array and start again.
	 */
	if ((fd = fopen(path, "r")) == NULL)
	{
		sprintf(path, "%s/memory/profiles/%s", getenv("BRISTOL"), file);
		if ((fd = fopen(path, "r")) == NULL)
			return;
	}

	for (i = 0; i < count; i++)
		points[i] = 0;

	while (fgets(param, 256, fd) != NULL)
	{
		if (param[0] == '#')
			continue;
		if (strlen(param) < 5)
			continue;

		if (strncmp(param, match, strlen(match)) == 0)
		{
			int wheel;
			float value;
			char *offset;

			/*
			 * We should now have a line with something like:
			 * <match>: wheel value
			if ((offset = strpbrk(param, " 	")) == NULL)
			 */
			if ((offset = index(param, ' ')) == NULL)
				continue;

			if ((wheel = atoi(offset)) < 0)
				continue;
			if (wheel >= count)
				continue;

			/*if ((offset = strpbrk(param, " 	")) == NULL) */
			if ((offset = index(++offset, ' ')) == NULL)
				continue;

			value = atof(offset);

			if (value > 0)
				points[wheel] = value;
		}
	}

	from = points[0];

	for (i = 1; i < count; i++)
	{
		if (points[i] != 0.0f)
		{
			if (i == (n + 1))
			{
				n = i;
				from = points[n];
				continue;
			}
			
			/* 
			 * If a start point was not defined then take the first definition
			 * and fill from there.
			 */
			if (from == 0)
				from = points[0] = points[i];

			delta = (points[i] - points[n]) / (i - n); 

			for (++n; n < i; n++)
				points[n] = (from += delta);

			from = points[i];
		}
	}

	/*
	 * If no end point defined then fill from last value.
	 */
	for (++n; n < count; n++)
		points[n] = points[n - 1];

/*
	i = 0;
	printf("%s\n", match);
	printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
		points[i], points[i+1], points[i+2], points[i+3], 
		points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2], points[i+3], 
			points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < count) 
		printf("%1.2f %1.2f %1.2f\n",
			points[i], points[i+1], points[i+2]);
*/
}
#endif /* NOT_USED */

/*
 * This will build a default gearbox to include compartments for crosstalk,
 * frequencies, stepping rates throught the wavetables, etc. We should put in
 * the different wheel shapes, equalisation, jitter and additionally consider
 * doing this for two gearboxes, normal and bright.
 */
static void
initGearbox()
{
	int i, start = 21, j, b;

	/*
	 * These are the not quite Even Tempered Hammond tonewheel frequencies.
	 * This could be extended to use drivegear, drivengear, toothcount to do
	 * the same? Would make it easier to emulate different gearboxes. FFS.
	 */
	gearbox[start + 0].freq = 220.00;
	gearbox[start + 1].freq = 233.04;
	gearbox[start + 2].freq = 246.86;
	gearbox[start + 3].freq = 261.54;
	gearbox[start + 4].freq = 277.07;
	gearbox[start + 5].freq = 293.70;
	gearbox[start + 6].freq = 311.11;
	gearbox[start + 7].freq = 329.55;
	gearbox[start + 8].freq = 349.09;
	gearbox[start + 9].freq = 370.00;
	gearbox[start +10].freq = 392.00;
	gearbox[start +11].freq = 415.14;

	/*
	 * The lower keys
	 */
	for (i = start - 1; i >= 0; i--)
		gearbox[i].freq = gearbox[i + 12].freq / 2;

	/*
	 * The higher keys. This is a 'correctly' staged gearbox, however it needs
	 * to be damaged a little for the 192-toothed wheels. This is done in the
	 * tonewheel profile file later.
	 */
	for (i = start + 12; i < OSC_LIMIT; i++)
		gearbox[i].freq = gearbox[i - 12].freq * 2;

	for (i = 0; i < OSC_LIMIT; i++)
	{
		gearbox[i].flutter = 0;
		gearbox[i].EQ = 1.0;
		gearbox[i].damping = 0.7;
		gearbox[i].phase = ((float) ((rand() & 0x0ffff) / 0x10000)) * 360.0;
		/* This is a separate issue since we need to consider compartments. */
		/* The defaults are no crosstalk, when we implement it we will then */
		/* build the compartment table. */
		gearbox[i].crosstalk[NORMAL][0].wheel = -1;
		gearbox[i].crosstalk[NORMAL][0].gain = defct[NORMAL][XT_NEAR_1];
		gearbox[i].crosstalk[NORMAL][1].wheel = -1;
		gearbox[i].crosstalk[NORMAL][1].gain = defct[NORMAL][XT_NEAR_2];
		gearbox[i].crosstalk[NORMAL][2].wheel = -1;
		gearbox[i].crosstalk[NORMAL][2].gain = defct[NORMAL][XT_FAR_1];
		gearbox[i].crosstalk[NORMAL][3].wheel = -1;
		gearbox[i].crosstalk[NORMAL][3].gain = defct[NORMAL][XT_FAR_2];

		gearbox[i].crosstalk[NORMAL][5].gain = defct[NORMAL][XT_FILT_1];
		gearbox[i].crosstalk[NORMAL][6].gain = defct[NORMAL][XT_FILT_2];
		gearbox[i].crosstalk[NORMAL][7].gain = defct[NORMAL][XT_FILT_3];
		gearbox[i].crosstalk[NORMAL][8].gain = defct[NORMAL][XT_FILT_4];

		gearbox[i].crosstalk[BRIGHT][0].wheel = -1;
		gearbox[i].crosstalk[BRIGHT][0].gain = defct[BRIGHT][XT_NEAR_1];
		gearbox[i].crosstalk[BRIGHT][1].wheel = -1;
		gearbox[i].crosstalk[BRIGHT][1].gain = defct[BRIGHT][XT_NEAR_2];
		gearbox[i].crosstalk[BRIGHT][2].wheel = -1;
		gearbox[i].crosstalk[BRIGHT][2].gain = defct[BRIGHT][XT_FAR_1];
		gearbox[i].crosstalk[BRIGHT][3].wheel = -1;
		gearbox[i].crosstalk[BRIGHT][3].gain = defct[BRIGHT][XT_FAR_2];

		gearbox[i].crosstalk[BRIGHT][5].gain = defct[BRIGHT][XT_FILT_1];
		gearbox[i].crosstalk[BRIGHT][6].gain = defct[BRIGHT][XT_FILT_2];
		gearbox[i].crosstalk[BRIGHT][7].gain = defct[BRIGHT][XT_FILT_3];
		gearbox[i].crosstalk[BRIGHT][8].gain = defct[BRIGHT][XT_FILT_4];

		/*
		 * Default stops are 8 linear levels, tapers are fixed but will be
		 * overriden from somewhere?
		 */
		for (j = 0; j < BUS_COUNT; j++)
		{
			gearbox[i].stops[j] = defbg[j];
			gearbox[i].taper[j] = deftp[j];
		}
	}
	/*
	 * We now have the default gearbox but have to have a rework of the 
	 * crosstalk map for the compartments. It is not sufficient just to put
	 * each toothwheel into an array, the gear entry needs to know which of
	 * the wheels in the same compartment have the geater crosstalk.....
	 */
	for (b = 0; b < 2; b++)
	{
		for (i = 0; i < 24; i++)
		{
			/*
			 * First line of wheels are the low frequency ones, they should
			 * really have less crosstalk from other wheels due to the filters
			 * applied to clean up the waveforms.
			 */
			gearbox[compartments[i][0]].crosstalk[b][0].wheel
				= compartments[i][1];
			gearbox[compartments[i][0]].crosstalk[b][1].wheel
				= compartments[i][2];
			gearbox[compartments[i][0]].crosstalk[b][2].wheel
				= compartments[i][3];
			gearbox[compartments[i][0]].crosstalk[b][0].gain
				= defct[b][XT_NEAR_2];
			gearbox[compartments[i][0]].crosstalk[b][1].gain
				= defct[b][XT_FAR_2];
			gearbox[compartments[i][0]].crosstalk[b][2].gain
				= defct[b][XT_FAR_1];
			gearbox[compartments[i][1]].crosstalk[b][0].wheel
				= compartments[i][0];
			gearbox[compartments[i][1]].crosstalk[b][1].wheel
				= compartments[i][3];
			gearbox[compartments[i][1]].crosstalk[b][2].wheel
				= compartments[i][2];
			gearbox[compartments[i][1]].crosstalk[b][0].gain
				= defct[b][XT_NEAR_1];
			gearbox[compartments[i][1]].crosstalk[b][1].gain
				= defct[b][XT_FAR_1];
			gearbox[compartments[i][1]].crosstalk[b][2].gain
				= defct[b][XT_FAR_2];
			gearbox[compartments[i][2]].crosstalk[b][0].wheel
				= compartments[i][3];
			gearbox[compartments[i][2]].crosstalk[b][1].wheel
				= compartments[i][0];
			gearbox[compartments[i][2]].crosstalk[b][2].wheel
				= compartments[i][1];
			gearbox[compartments[i][2]].crosstalk[b][0].gain
				= defct[b][XT_NEAR_2];
			gearbox[compartments[i][2]].crosstalk[b][1].gain
				= defct[b][XT_FAR_1];
			gearbox[compartments[i][2]].crosstalk[b][2].gain
				= defct[b][XT_FAR_2];
	
			if (compartments[i][3] >= 0)
			{
				gearbox[compartments[i][3]].crosstalk[b][0].wheel
					= compartments[i][2];
				gearbox[compartments[i][3]].crosstalk[b][1].wheel
					= compartments[i][1];
				gearbox[compartments[i][3]].crosstalk[b][2].wheel
					= compartments[i][0];
				gearbox[compartments[i][3]].crosstalk[b][0].gain
					= defct[b][XT_NEAR_1];
				gearbox[compartments[i][3]].crosstalk[b][1].gain
					= defct[b][XT_FAR_2];
				gearbox[compartments[i][3]].crosstalk[b][2].gain
					= defct[b][XT_FAR_1];
			}
	
			/*
			 * For the filter crosstalk the relationships between the teeth
			 * are a little different. For the 1st and last 24 there are the
			 * two adjacent wheels, for the rest in the middle there are 4
			 * adjacencies.
			 *
			 * There are 2 edge cases we need to take care of.
			 */
			if (i == 0) {
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i + 1][0];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_2].wheel
					= gearbox[compartments[i][0]].crosstalk[b][XT_FILT_3].wheel
					= gearbox[compartments[i][0]].crosstalk[b][XT_FILT_4].wheel
					= -1;
	
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i + 1][0];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_2].wheel
					= compartments[i + 1][1];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_2].gain
					= defct[b][XT_FILT_2];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_3].wheel
					= gearbox[compartments[i][1]].crosstalk[b][XT_FILT_4].wheel
					= -1;
	
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i + 1][2];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_2].wheel
					= gearbox[compartments[i][2]].crosstalk[b][XT_FILT_3].wheel
					= gearbox[compartments[i][2]].crosstalk[b][XT_FILT_4].wheel
					= 0-1;
	
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i + 1][2];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_2].wheel
					= gearbox[compartments[i][3]].crosstalk[0][XT_FILT_3].wheel
					= gearbox[compartments[i][3]].crosstalk[b][XT_FILT_4].wheel
					= -1;
			} else if (i == 23) {
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][0];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][1];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_2].wheel
					= gearbox[compartments[i][0]].crosstalk[b][XT_FILT_3].wheel
					= gearbox[compartments[i][0]].crosstalk[b][XT_FILT_4].wheel
					= -1;
	
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][1];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_2].wheel
					= gearbox[compartments[i][1]].crosstalk[b][XT_FILT_3].wheel
					= gearbox[compartments[i][1]].crosstalk[b][XT_FILT_4].wheel
					= -1;
	
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][2];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_2].wheel
					= compartments[i - 1][3];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_2].gain
					= defct[b][XT_FILT_2];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_3].wheel
					= gearbox[compartments[i][2]].crosstalk[b][XT_FILT_4].wheel
					= -1;
	
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][3];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_2].wheel
					= gearbox[compartments[i][3]].crosstalk[b][XT_FILT_3].wheel
					= gearbox[compartments[i][3]].crosstalk[b][XT_FILT_4].wheel
					= -1;
	
			} else {
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][0];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_2].wheel
					= compartments[i + 1][0];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_2].gain
					= defct[b][XT_FILT_2];
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_3].gain
					= gearbox[compartments[i][0]].crosstalk[b][XT_FILT_4].gain
					= 0.0;
				gearbox[compartments[i][0]].crosstalk[b][XT_FILT_3].wheel
					= gearbox[compartments[i][0]].crosstalk[b][XT_FILT_4].wheel
					= -1;
	
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][0];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_2].wheel
					= compartments[i + 1][0];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_2].gain
					= defct[b][XT_FILT_2];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][1];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_3];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_2].wheel
					= compartments[i + 1][1];
				gearbox[compartments[i][1]].crosstalk[b][XT_FILT_2].gain
					= defct[b][XT_FILT_4];
	
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][2];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_2].wheel
					= compartments[i + 1][2];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_2].gain
					= defct[b][XT_FILT_2];
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_3].gain
					= gearbox[compartments[i][2]].crosstalk[b][XT_FILT_4].gain
					= 0.0;
				gearbox[compartments[i][2]].crosstalk[b][XT_FILT_3].wheel
					= gearbox[compartments[i][2]].crosstalk[b][XT_FILT_4].wheel
					= -1;
	
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][3];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_1];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_2].wheel
					= compartments[i + 1][3];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_2].gain
					= defct[b][XT_FILT_2];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_1].wheel
					= compartments[i - 1][2];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_1].gain
					= defct[b][XT_FILT_3];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_2].wheel
					= compartments[i + 1][2];
				gearbox[compartments[i][3]].crosstalk[b][XT_FILT_2].gain
					= defct[b][XT_FILT_4];
			}
		}
	}
}

static void
finishGearings(float rate)
{
	int i, j;

/*printf("finishGearings(%f)\n", rate); */

	/*
	 * The frequencies have been stuffed separately, we now just calculate the
	 * step rates through the wavetables to generate these frequencies.
	 */
	for (i = 0; i < OSC_LIMIT; i++)
	{
		/*
		 * Initialise the step through the wavetemplate then the random
		 * index into this table. The randomness is a little awkward, it is
		 * defined to be different over any two invocations and that will
		 * subtly change the sound between them. It is also possible with
		 * srand to seed with a fixed value and that would have given a fixed
		 * phase difference per wheel and have made the tonal qualities a bit
		 * more predictable. This same effect can be reproduced by specifying
		 * the complete gearbox, it works but is quite a lot of work.
		 */
		gearbox[i].step = ((float) 1024.0) / (rate / gearbox[i].freq);
		toneindeces[i] = gearbox[i].phase / 360.0 * 1024.0;

		for (j = 0; j < BUS_COUNT; j++)
		{
			gearbox[i].stops[j] = defbg[j];
			gearbox[i].taper[j] = tapers[i][j];
		}
	}
}

void
initthesermon(int sc, int sr, int compress)
{
	int i;
	float wavepoints[2][OSC_LIMIT];

	/* printf("initthesermon(%i, %i, %i)\n", sc, sr, compress); */

	samplerate = sr; /* We need this for bus delay timing */

	/*
	 * Build the basic runtime wavetable.
	 */
	if (oscillators == NULL)
	{
		oscillators
			= (float *) bristolmalloc0(OSC_COUNT * sc * sizeof(float));

		for (i = 0; i < OSC_LIMIT; i++)
		{
			tonewheel[i] = i * sc + oscillators;
			toneindeces[i] = 0;
		}
	}

	/*
	 * This will now open a configuration file that will define the wheel
	 * templates, the EQ map, crosstalk, anything else. Where possible I would
	 * like to keep configurations available from the GUI, but the wheel and
	 * EQ profiles are not easy to interface. I want them to be a set of points
	 * which are then interpolated to give gain levels and waveform distortions.
	 *
	 * bright should perhaps not re-eq the tonewheels but the busses?
	 */
	for (i = 0; i < OSC_LIMIT; i++)
		toneEQ[NORMAL][i] = toneEQ[BRIGHT][i] = 0;
	/* This is the default EQ map, overrides come from the 'tonewheel' file. */
	toneEQ[NORMAL][0] = 3.0;
	toneEQ[NORMAL][20] = 3.0;
	toneEQ[NORMAL][32] = 2.0;
	toneEQ[NORMAL][50] = 1.1;
	toneEQ[NORMAL][60] = 0.8;
	toneEQ[NORMAL][80] = 2.0;
	toneEQ[NORMAL][90] = 3.0;
	bristolGetMap("tonewheel", "EQNormal", &toneEQ[NORMAL][0], OSC_LIMIT, 0);
	toneEQ[BRIGHT][0] = 2.0;
	toneEQ[BRIGHT][20] = 2.0;
	toneEQ[BRIGHT][32] = 1.2;
	toneEQ[BRIGHT][50] = 0.8;
	toneEQ[BRIGHT][60] = 1.6;
	toneEQ[BRIGHT][80] = 2.5;
	toneEQ[BRIGHT][90] = 3.0;
	bristolGetMap("tonewheel", "EQBright", &toneEQ[BRIGHT][0], OSC_LIMIT, 0);

	damping[NORMAL][0] = 2.0;
	damping[NORMAL][20] = 2.0;
	damping[NORMAL][32] = 1.2;
	damping[NORMAL][50] = 0.8;
	damping[NORMAL][60] = 1.6;
	damping[NORMAL][80] = 2.5;
	damping[NORMAL][90] = 3.0;
	bristolGetMap("tonewheel", "DampNormal", &damping[NORMAL][0], OSC_LIMIT, 0);
	damping[BRIGHT][0] = 2.0;
	damping[BRIGHT][20] = 2.0;
	damping[BRIGHT][32] = 1.2;
	damping[BRIGHT][50] = 0.8;
	damping[BRIGHT][60] = 1.6;
	damping[BRIGHT][80] = 2.5;
	damping[BRIGHT][90] = 3.0;
	bristolGetMap("tonewheel", "DampBright", &damping[BRIGHT][0], OSC_LIMIT, 0);

	for (i = 0; i < BUS_COUNT; i++)
	{
		busEQ[NORMAL][i] = busEQ[BRIGHT][i] = 0;

/*		stops[0][i] = stops[1][i] = stops[2][i] = stops[3][i] = */
/*			stops[4][i] = stops[5][i] = stops[6][i] = stops[7][i] = */
/*			stops[8][i] = ((float) i) / 8.0; */
	}
	/* This is the default EQ map, overrides come from the 'tonewheel' file. */
	busEQ[NORMAL][0] = 1.5;
	busEQ[NORMAL][5] = 1.0;
	busEQ[NORMAL][8] = 1.0;
	bristolGetMap("tonewheel", "BusNormal", &busEQ[NORMAL][0], BUS_COUNT, 0);
	busEQ[BRIGHT][0] = 1.5;
	busEQ[BRIGHT][4] = 1.0;
	busEQ[BRIGHT][8] = 2.0;
	bristolGetMap("tonewheel", "BusBright", &busEQ[BRIGHT][0], BUS_COUNT, 0);

	bristolGetMap("tonewheel", "BusDelayNormal", &drawbars.delay[NORMAL][0],
		BUS_COUNT, 0);
	bristolGetMap("tonewheel", "BusDelayBright", &drawbars.delay[BRIGHT][0],
		BUS_COUNT, 0);

	bristolGetMap("tonewheel", "ClickNormal", &drawbars.gain[NORMAL][0],
		BUS_COUNT, 0);
	bristolGetMap("tonewheel", "ClickPulseNormal", &drawbars.wave[NORMAL][0],
		BUS_COUNT, 0);
	bristolGetMap("tonewheel", "ClickInvertNormal", &drawbars.invert[NORMAL][0],
		BUS_COUNT, 0);
	bristolGetMap("tonewheel", "ClickBright", &drawbars.gain[BRIGHT][0],
		BUS_COUNT, 0);
	bristolGetMap("tonewheel", "ClickPulseBright", &drawbars.wave[BRIGHT][0],
		BUS_COUNT, 0);
	bristolGetMap("tonewheel", "ClickInvertBright", &drawbars.invert[BRIGHT][0],
		BUS_COUNT, 0);

	/*
	 * We have to fix the delays as samplecounts and other stuff.
	 * Note: the actual delay is calculated at run time based on velocity
	 */
	for (i = 0; i < BUS_COUNT; i++)
	{
		drawbars.delay[NORMAL][i]
			= ((int) drawbars.delay[NORMAL][i]) * sr / 1024;
		drawbars.delay[BRIGHT][i]
			= ((int) drawbars.delay[BRIGHT][i]) * sr / 1024;

		if (drawbars.invert[NORMAL][i] >= 0.9)
			drawbars.gain[NORMAL][i] = -drawbars.gain[NORMAL][i];
		if (drawbars.invert[BRIGHT][i] >= 0.9)
			drawbars.gain[BRIGHT][i] = -drawbars.gain[BRIGHT][i];

		if ((drawbars.pulse[NORMAL][i] = (int) drawbars.wave[NORMAL][i])
			>= C_PULSE_LIMIT)
			drawbars.pulse[NORMAL][i] = C_PULSE_LIMIT - 1;

		if ((drawbars.pulse[BRIGHT][i] = (int) drawbars.wave[BRIGHT][i])
			>= C_PULSE_LIMIT)
			drawbars.pulse[BRIGHT][i] = C_PULSE_LIMIT - 1;
	}
/*
printf("config:\n");
printf("delay: %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
drawbars.delay[NORMAL][0], drawbars.delay[NORMAL][1],
drawbars.delay[NORMAL][2], drawbars.delay[NORMAL][3],
drawbars.delay[NORMAL][4], drawbars.delay[NORMAL][5],
drawbars.delay[NORMAL][6], drawbars.delay[NORMAL][7],
drawbars.delay[NORMAL][8]);
printf("delay: %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
drawbars.delay[BRIGHT][0], drawbars.delay[BRIGHT][1],
drawbars.delay[BRIGHT][2], drawbars.delay[BRIGHT][3],
drawbars.delay[BRIGHT][4], drawbars.delay[BRIGHT][5],
drawbars.delay[BRIGHT][6], drawbars.delay[BRIGHT][7],
drawbars.delay[BRIGHT][8]);

printf("pulse: %i %i %i %i %i %i %i %i %i\n",
drawbars.pulse[NORMAL][0], drawbars.pulse[NORMAL][1],
drawbars.pulse[NORMAL][2], drawbars.pulse[NORMAL][3],
drawbars.pulse[NORMAL][4], drawbars.pulse[NORMAL][5],
drawbars.pulse[NORMAL][6], drawbars.pulse[NORMAL][7],
drawbars.pulse[NORMAL][8]);
printf("pulse: %i %i %i %i %i %i %i %i %i\n",
drawbars.pulse[BRIGHT][0], drawbars.pulse[BRIGHT][1],
drawbars.pulse[BRIGHT][2], drawbars.pulse[BRIGHT][3],
drawbars.pulse[BRIGHT][4], drawbars.pulse[BRIGHT][5],
drawbars.pulse[BRIGHT][6], drawbars.pulse[BRIGHT][7],
drawbars.pulse[BRIGHT][8]);

printf("gain: %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
drawbars.gain[NORMAL][0], drawbars.gain[NORMAL][1],
drawbars.gain[NORMAL][2], drawbars.gain[NORMAL][3],
drawbars.gain[NORMAL][4], drawbars.gain[NORMAL][5],
drawbars.gain[NORMAL][6], drawbars.gain[NORMAL][7],
drawbars.gain[NORMAL][8]);
printf("gain: %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
drawbars.gain[BRIGHT][0], drawbars.gain[BRIGHT][1],
drawbars.gain[BRIGHT][2], drawbars.gain[BRIGHT][3],
drawbars.gain[BRIGHT][4], drawbars.gain[BRIGHT][5],
drawbars.gain[BRIGHT][6], drawbars.gain[BRIGHT][7],
drawbars.gain[BRIGHT][8]);
*/

	for (i = 0; i < OSC_LIMIT; i++)
		wavepoints[0][i] = wavepoints[1][i] = 0;
	/* This is the default tonewheel map, they also comes from the same file. */
	wavepoints[NORMAL][0] = 0.8; /* wheel map, < 1.0 = Square, > 1.0 = ramp */
	wavepoints[NORMAL][30] = 0.8;
	wavepoints[NORMAL][40] = 1.0;
	wavepoints[NORMAL][60] = 1.0;
	wavepoints[NORMAL][80] = 2.0;
	bristolGetMap("tonewheel", "ToneNormal", wavepoints[NORMAL], OSC_LIMIT, 0);
	wavepoints[BRIGHT][0] = 0.8; /* wheel map, < 1.0 = Square, > 1.0 = ramp */
	wavepoints[BRIGHT][30] = 0.8;
	wavepoints[BRIGHT][40] = 1.0;
	wavepoints[BRIGHT][60] = 1.0;
	wavepoints[BRIGHT][80] = 2.0;
	bristolGetMap("tonewheel", "ToneBright", wavepoints[BRIGHT], OSC_LIMIT, 0);

	/*
	 * Stuff the two main wavetables, one for bright, one for normal
	 */
	for (i = 0; i < OSC_LIMIT; i++)
	{
		if (wavepoints[NORMAL][i] > 1.0)
			filltriwave(&wheeltemplates[NORMAL][i][0],
				WAVE_SIZE, wavepoints[NORMAL][i], compress);
		else
			fillsquarewave(&wheeltemplates[0][i][0],
				WAVE_SIZE, wavepoints[NORMAL][i],
				compress); //wavepoints[NORMAL][i] > 0.55? 1:2);
	}

	for (i = 0; i < OSC_LIMIT; i++)
	{
		if (wavepoints[BRIGHT][i] >= 1.0)
			filltriwave(&wheeltemplates[BRIGHT][i][0],
				WAVE_SIZE, wavepoints[BRIGHT][i], 0);
		else
			fillsquarewave(&wheeltemplates[BRIGHT][i][0],
				WAVE_SIZE, wavepoints[BRIGHT][i],
				wavepoints[BRIGHT][i] > 0.55? 1:2);
	}

	bristolGetMap("tonewheel", "crosstalkNormal", defct[0], XT_COUNT, 0);
	bristolGetMap("tonewheel", "crosstalkBright", defct[1], XT_COUNT, 0);

	initGearbox();

	bristolGetMap("tonewheel", "resistors", tresistors, 6, 0);
	bristolGetMap("tonewheel", "stops", defbg, BUS_COUNT, 0);
	bristolHammondGetGears("tonewheel", "wheel", gearbox, OSC_LIMIT);

	finishGearings((float) sr);
}

/*
 * So, rather than tri waves we also need squares, or perhaps more so. This is
 * going to be a distorted cosine wave: depending on the 'reach' then we
 * scan through the cosine faster, filling the space with "1's".
 */
static void
fillsquarewave(register float *mem, register int count,
register double reach, int compress)
{
	register int i, newcount;
	register float increment = 0.0, l;

	/* 
	 * Change the number of samples we will actually fill with the cosine.
	 */
	newcount = reach * count;
	reach = 2.0 / (float) newcount;

	for (i = 0;i < newcount; i++)
		mem[i] = cosf(M_PI * (increment += reach)) * HAMMOND_WAVE_GAIN;

	for (;i < count; i++)
		mem[i] = HAMMOND_WAVE_GAIN;

	if (compress)
	{
		for (i = 0;i < count; i++)
		{
			//mem[i] = (-mem[i] + 8.0f) * (-mem[i] + 8.0f) * (-mem[i] + 8.0f)
			//	/ 256.0f - 8.0f;
			l = mem[i] + 8;
			mem[i] = l * l * l * 0.00390625f - 8.0f;
		}
	}

/*
	printf("\n%f, %f\n", j, inc);
	for (i = 0; i < count; i+=8)
	{
		printf("\n%i: ", i + k);
		for (k = 0; k < 8; k++)
			printf("%03.1f, ", mem[i + k]);
	}
	printf("\n");
*/
}

static void
filltriwave(register float *mem, register int count,
register double reach, int compress)
{
	register int i, recalc1 = 1, recalc2 = 1;
	float j = 0, inc = reach;
	float l;

	for (i = 0;i < count; i++)
	{
		if ((j > (count * 3 / 4)) && recalc2)
		{
			recalc2 = 0;
			inc = count / 4 / (count - i);
		} else if ((j > (count / 4)) && recalc1) {
			recalc1 = 0;
			/* 
			 * J now has the sine on a peak, we need to calculate the number
			 * of steps required to get the sine back to zero crossing by
			 * the time i = count / 2, ie, count /4 steps in total.
			 */
			inc = count / 4 / (count / 2 - i);
		}

		j += inc;

		mem[i] = sinf((2 * M_PI * j) / count) * HAMMOND_WAVE_GAIN;
	}

	if (compress)
	{
		for (i = 0;i < count; i++)
		{
			//mem[i] = (mem[i] + 8.0f) * (mem[i] + 8.0f) * (mem[i] + 8.0f)
			//	/ 256.0f - 8.0f;
			l = mem[i] + 8;
			mem[i] = l * l * l * 0.00390625f - 8.0f;
		}
	}


//			mem[i] =
//				(mem[i] * (mem[i] * (mem[i] + 24) + 192) + 512) / 256.0f - 8.0f;
//			l = mem[i] + 8;
//			mem[i] = l * l * l * 0.00390635f - 8.0f;

/*
	printf("\n%f, %f\n", j, inc);
	for (i = 0; i < count; i+=8)
	{
		printf("\n%i: ", i + k);
		for (k = 0; k < 8; k++)
			printf("%03.1f, ", mem[i + k]);
	}
	printf("\n");
*/
}

