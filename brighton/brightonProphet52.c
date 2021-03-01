
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


#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightonKeys.h"
#include "brightoninternals.h"

static int initmem;

static int prophet52Init();
static int prophet52Configure();
static int prophet52Callback(brightonWindow *, int, int, float);
/*static int keyCallback(brightonWindow *, int, int, float); */
/*static int modCallback(brightonWindow *, int, int, float); */
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define LAST_DEV 51
#define DEV_OFFSET 2

#define FIRST_DEV 0
#define DEVICE_COUNT (69 + FIRST_DEV)
#define ACTIVE_DEVS (51 + FIRST_DEV)
#define DISPLAY_DEV (DEVICE_COUNT - 1 + FIRST_DEV)
#define RADIOSET_1 (LAST_DEV + 1)

#define KEY_PANEL 1

#define R1 140
#define RB1 160
#define R2 425
#define RB2 445
#define R3 710
#define RB3 730

#define C1 17
#define C2 66
#define CB1 60
#define CB2 88
#define C3 116
#define C4 143
#define C5 172

#define C6 217
#define CB6 258
#define CB7 287
#define C7 320
#define CB8 370

#define C9 262
#define CB9 310
#define CB10 340
#define CB11 370
#define C10 410
#define CB12 460
#define CB13 490

#define C11 418
#define C12 466
#define C13 513

#define C14 565
#define C15 613
#define C16 658
#define C17 705

#define C18 761
#define C19 810
#define C20 860
#define C21 910

#define C22 877
#define C23 964

#define S1 120

#define B1 14
#define B2 80

/*
 * This structure is for device definition. The structure is defined in 
 * include/brighton.h, further definitions in brighton/brightonDevtable.h and
 * include/brightoninternals.h
 *
 *	typedef int (*brightonCallback)(int, float);
 *	typedef struct BrightonLocations {
 *		int device; 0=rotary, 1=scale, etc.
 *		float relx, rely; relative position with regards to 1000 by 1000 window
 *		float relw, relh; relative height.
 *		int from, to;
 *		brightonCallback callback; specific to this dev
 *		char *image; bitmap. If zero take a device default.
 *		int flags;
 *	} brightonLocations;
 *
 * This example is for a prophet52Bristol type synth interface.
 */
static brightonLocations locations[DEVICE_COUNT] = {
/* poly mod */
	{"PolyMod-Filt", 0, C1, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"PolyMod-OscB", 0, C2, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"PolyMod2Freq-A", 2, C3, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"PolyMod2PW-A", 2, C4, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"PolyMod2Filt", 2, C5, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* lfo */
	{"LFO-Freq", 0, C2, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"LFO-Ramp", 2, C3, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"LFO-Tri", 2, C4, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"LFO-Square", 2, C5, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* wheel mod */
	{"Wheel-Mix", 0, C1, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Whell-FreqA", 2, CB1, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Whell-FreqB", 2, CB2, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Wheel-PW-A", 2, C3, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Wheel-PW-B", 2, C4, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Wheel-Filt", 2, C5, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* osc a */
	{"OscA-Transpose", 0, C6, R1, S1, S1, 0, 5, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"OscA-Ramp", 2, CB6, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscA-Pulse", 2, CB7, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscA-PW", 0, C7, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"OscA-Sync", 2, CB8, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* osc b */
	{"OscB-Transpose", 0, C6, R2, S1, S1, 0, 5, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"OscB-Tunine", 0, C9, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, BRIGHTON_NOTCH},
	{"OscB-Ramp", 2, CB9, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscB-Tri", 2, CB10, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscB-Pulse", 2, CB11, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscB-PW", 0, C10, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"OscB-LFO", 2, CB12, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscB-KBD", 2, CB13, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* glide, etc */
	{"Glide", 0, C6, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Unison", 2, CB7, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* mixer */
	{"Mix-OscA", 0, C11, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Mix-OscB", 0, C12, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Mix-Noise", 0, C13, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
/* filter + env */
	{"VCF-Cutoff", 0, C14, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Resonance", 0, C15, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Envelope", 0, C16, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-KBD", 2, C17 + 6, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCF-Attack", 0, C14, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Decay", 0, C15, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Sustain", 0, C16, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Release", 0, C17, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
/* main env */
	{"VCA-Attack", 0, C18, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCA-Decay", 0, C19, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCA-Sustain", 0, C20, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCA-Release", 0, C21, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
/* tune + vol */
	{"MasterTuning", 0, C18, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob3.xpm", 0, BRIGHTON_NOTCH},
	{"MasterVolume", 0, C23, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob3.xpm", 0, 0},
/* Chorus */
	{"Chorus-Speed", 0, C18, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Chorus-Depth", 0, C19, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Chorus-Phase", 0, C20, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Chorus-Gain", 0, C21, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
/* opts */
	{"Release", 2, C23 + 7, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"A-440", 2, C19 + 7, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"Tune", 2, C23 + 7, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
/* memories */
	{"", 2, C14 + 10, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C14 + 30, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C14 + 50, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C14 + 70, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C14 + 90, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C14 + 110, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C14 + 130, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C14 + 150, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
/* L/S */
	{"", 2, CB8 - 10, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, CB8 + 20, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, CB8 + 50, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
/* Midi, perhaps eventually file import/export buttons */
	{"", 2, C20 + 10, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C21 + 10, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* */
	{"", 4, 840, 1025, 140, 130, 0, 1, 0, "bitmaps/images/prophet55.xpm", 0, 0},
	/* Display */
	{"", 3, CB8 + 75, RB3, 120, B2, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}
};

/*
 */

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp prophet52App = {
	"prophet52",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/wood7.xpm", /* was wood2.xpm */
	0, /* BRIGHTON_STRETCH, //flags */
	prophet52Init,
	prophet52Configure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{5, 100, 2, 2, 5, 520, 0, 0},
	753, 310, 0, 0,
	6, /* one panel only */
	{
		{
			"Prophet",
			"bitmaps/blueprints/prophet52.xpm",
			"bitmaps/textures/metal4.xpm",
			0, /*BRIGHTON_STRETCH, // flags */
			0,
			0,
			prophet52Callback,
			15, 25, 970, 540,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/kbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			110, 660, 900, 320,
			KEY_COUNT,
			keysprofile
		},
		{
			"Mods",
			"bitmaps/blueprints/prophetmod.xpm",
			"bitmaps/textures/metal4.xpm", /* flags */
			BRIGHTON_REVERSE,
			0,
			0,
			modCallback,
			15, 660, 95, 320,
			2,
			mods
		},
		{
			"Prophet",
			0,
			"bitmaps/textures/wood7.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 15, 1000,
			0,
			0
		},
		{
			"Prophet",
			0,
			"bitmaps/textures/wood7.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /*flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
		},
		{
			"Prophet",
			0,
			"bitmaps/textures/wood4.xpm",
			0, /*BRIGHTON_STRETCH|BRIGHTON_VERTICAL, //flags */
			0,
			0,
			0,
			15, 980, 970, 20,
			0,
			0
		}
	}
};

/*static dispatcher dispatch[DEVICE_COUNT]; */

static int
midiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, "prophet52", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int
prophet52MidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
multiTune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	/*
	 * Configures all the tune settings to zero (ie, 0.5). Do the Osc-A first,
	 * since it is not visible, and then request the GUI to configure Osc-B.
	 */
	bristolMidiSendMsg(fd, synth->sid, 0, 2, 8191);
	event.type = BRIGHTON_FLOAT;
	event.value = 0.5;
	brightonParamChange(synth->win, synth->panel, FIRST_DEV + 45,
		&event);
	brightonParamChange(synth->win, synth->panel, FIRST_DEV + 21,
		&event);
}

static void
midiRelease(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (!global.libtest)
	{
		bristolMidiSendMsg(fd, synth->sid, 127, 0,
			BRISTOL_ALL_NOTES_OFF);
	}
}

static void
prophet52Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

/* printf("prophet52Memory(%i)\n", c); */

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->dispatch[RADIOSET_1].other2)
	{
		synth->dispatch[RADIOSET_1].other2 = 0;
		return;
	}

	switch (c) {
		default:
		case 0:
			/*
			 * We want to make these into memory buttons. To do so we need to
			 * know what the last active button was, and deactivate its 
			 * display, then send any message which represents the most
			 * recently configured value. Since this is a memory button we do
			 * not have much issue with the message, but we are concerned with
			 * the display.
			 */
			if (synth->dispatch[RADIOSET_1].other1 != -1)
			{
				synth->dispatch[RADIOSET_1].other2 = 1;

				if (synth->dispatch[RADIOSET_1].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_1].other1 + LAST_DEV + 2, &event);
			}
			synth->dispatch[RADIOSET_1].other1 = o;

			if (synth->flags & BANK_SELECT) {
				if ((synth->bank * 10 + o * 10) >= 1000)
				{
					synth->location = o;
					synth->flags &= ~BANK_SELECT;

					if (loadMemory(synth, "prophet52", 0,
						synth->bank + synth->location, synth->mem.active,
							FIRST_DEV, BRISTOL_STAT) < 0)
						displayText(synth, "FRE", synth->bank + synth->location,
							DISPLAY_DEV);
					else
						displayText(synth, "PRG", synth->bank + synth->location,
							DISPLAY_DEV);
				} else {
					synth->bank = synth->bank * 10 + o * 10;
					displayText(synth, "BANK", synth->bank + synth->location,
						DISPLAY_DEV);
				}
			} else {
				if (synth->bank < 10)
					synth->bank = 10;
				synth->location = o;
				if (loadMemory(synth, "prophet52", 0,
					synth->bank + synth->location, synth->mem.active,
						FIRST_DEV, BRISTOL_STAT) < 0)
					displayText(synth, "FRE", synth->bank + synth->location,
						DISPLAY_DEV);
				else
					displayText(synth, "PRG", synth->bank + synth->location,
						DISPLAY_DEV);
			}
			break;
		case 1:
			if (synth->bank < 10)
				synth->bank = 10;
			if (synth->location == 0)
				synth->location = 1;
			if (loadMemory(synth, "prophet52", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
				displayText(synth, "FRE", synth->bank + synth->location,
					DISPLAY_DEV);
			else
				displayText(synth, "PRG", synth->bank + synth->location,
					DISPLAY_DEV);
			synth->flags &= ~BANK_SELECT;
			break;
		case 2:
			if (synth->bank < 10)
				synth->bank = 10;
			if (synth->location == 0)
				synth->location = 1;
			saveMemory(synth, "prophet52", 0, synth->bank + synth->location,
				FIRST_DEV);
			displayText(synth, "PRG", synth->bank + synth->location,
				DISPLAY_DEV);
			synth->flags &= ~BANK_SELECT;
			break;
		case 3:
			if (synth->flags & BANK_SELECT) {
				synth->flags &= ~BANK_SELECT;
				if (loadMemory(synth, "prophet52", 0,
					synth->bank + synth->location, synth->mem.active,
						FIRST_DEV, BRISTOL_STAT) < 0)
					displayText(synth, "FRE", synth->bank + synth->location,
						DISPLAY_DEV);
				else
					displayText(synth, "PRG", synth->bank + synth->location,
						DISPLAY_DEV);
			} else {
				synth->bank = 0;
				displayText(synth, "BANK", synth->bank, DISPLAY_DEV);
				synth->flags |= BANK_SELECT;
			}
			break;
	}
/*	printf("	prophet52Memory(B: %i L %i: %i)\n", */
/*		synth->bank, synth->location, o); */
}

static int
prophet52Midi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			return(0);
		}
	} else {
		if ((newchan = synth->midichannel + 1) >= 16)
		{
			synth->midichannel = 15;
			return(0);
		}
	}

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	displayText(synth, "MIDI", newchan + 1, DISPLAY_DEV);

	synth->midichannel = newchan;

/*	printf("P: going to display: %x, %x\n", synth, synth->win); */
/*		displayText(synth, "MIDI", synth->midichannel + 1, DISPLAY_DEV); */

	return(0);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
prophet52Callback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/* printf("prophet52Callback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (prophet52App.resources[0].devlocn[index].to == 1.0)
		sendvalue = value * (CONTROLLER_RANGE - 1);
	else
		sendvalue = value;

	synth->mem.param[index] = value;

	if ((!global.libtest) || (index >= ACTIVE_DEVS))
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

#ifdef DEBUG
	else
		printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#endif

	return(0);
}

static void
keyTracking(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid, 4, 3, v / 2);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
prophet52Init(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	dispatcher *dispatch;
	int i;

	if (synth == 0)
	{
		synth = findSynth(global.synths, (int) 0);
		if (synth == 0)
		{
			printf("cannot init\n");
			return(0);
		}
	}

	synth->win = win;

	if ((initmem = synth->location) == 0)
		initmem = 11;

	printf("Initialise the prophet52 link to bristol: %p\n", synth->win);

	synth->mem.param = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;
	synth->dispatch = (dispatcher *)
		brightonmalloc(DEVICE_COUNT * sizeof(dispatcher));
	dispatch = synth->dispatch;

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets.
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
	{
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);
	}

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		synth->dispatch[i].routine = prophet52MidiSendMsg;
	}

	dispatch[FIRST_DEV].controller = 126;
	dispatch[FIRST_DEV].operator = 6;
	dispatch[FIRST_DEV + 1].controller = 126;
	dispatch[FIRST_DEV + 1].operator = 7;
	dispatch[FIRST_DEV + 2].controller = 126;
	dispatch[FIRST_DEV + 2].operator = 8;
	dispatch[FIRST_DEV + 3].controller = 126;
	dispatch[FIRST_DEV + 3].operator = 9;
	dispatch[FIRST_DEV + 4].controller = 126;
	dispatch[FIRST_DEV + 4].operator = 10;

	dispatch[FIRST_DEV + 5].controller = 2;
	dispatch[FIRST_DEV + 5].operator = 0;
	dispatch[FIRST_DEV + 6].controller = 126;
	dispatch[FIRST_DEV + 6].operator = 24;
	dispatch[FIRST_DEV + 7].controller = 126;
	dispatch[FIRST_DEV + 7].operator = 25;
	dispatch[FIRST_DEV + 8].controller = 126;
	dispatch[FIRST_DEV + 8].operator = 26;

	dispatch[FIRST_DEV + 9].controller = 126;
	dispatch[FIRST_DEV + 9].operator = 11;
	dispatch[FIRST_DEV + 10].controller = 126;
	dispatch[FIRST_DEV + 10].operator = 12;
	dispatch[FIRST_DEV + 11].controller = 126;
	dispatch[FIRST_DEV + 11].operator = 13;
	dispatch[FIRST_DEV + 12].controller = 126;
	dispatch[FIRST_DEV + 12].operator = 14;
	dispatch[FIRST_DEV + 13].controller = 126;
	dispatch[FIRST_DEV + 13].operator = 15;
	dispatch[FIRST_DEV + 14].controller = 126;
	dispatch[FIRST_DEV + 14].operator = 16;

	dispatch[FIRST_DEV + 15].controller = 0;
	dispatch[FIRST_DEV + 15].operator = 1;
	dispatch[FIRST_DEV + 16].controller = 0;
	dispatch[FIRST_DEV + 16].operator = 4;
	dispatch[FIRST_DEV + 17].controller = 0;
	dispatch[FIRST_DEV + 17].operator = 6;
	dispatch[FIRST_DEV + 18].controller = 0;
	dispatch[FIRST_DEV + 18].operator = 0;
	dispatch[FIRST_DEV + 19].controller = 0;
	dispatch[FIRST_DEV + 19].operator = 7;

	dispatch[FIRST_DEV + 20].controller = 1;
	dispatch[FIRST_DEV + 20].operator = 1;
	dispatch[FIRST_DEV + 21].controller = 1;
	dispatch[FIRST_DEV + 21].operator = 2;
	dispatch[FIRST_DEV + 22].controller = 1;
	dispatch[FIRST_DEV + 22].operator = 4;
	dispatch[FIRST_DEV + 23].controller = 1;
	dispatch[FIRST_DEV + 23].operator = 5;
	dispatch[FIRST_DEV + 24].controller = 1;
	dispatch[FIRST_DEV + 24].operator = 6;
	dispatch[FIRST_DEV + 25].controller = 1;
	dispatch[FIRST_DEV + 25].operator = 0;
	dispatch[FIRST_DEV + 26].controller = 126;
	dispatch[FIRST_DEV + 26].operator = 18;
	dispatch[FIRST_DEV + 27].controller = 126;
	dispatch[FIRST_DEV + 27].operator = 19;

	dispatch[FIRST_DEV + 28].controller = 126;
	dispatch[FIRST_DEV + 28].operator = 0;
	dispatch[FIRST_DEV + 29].controller = 126;
	dispatch[FIRST_DEV + 29].operator = 1;

	dispatch[FIRST_DEV + 30].controller = 126;
	dispatch[FIRST_DEV + 30].operator = 20;
	dispatch[FIRST_DEV + 31].controller = 126;
	dispatch[FIRST_DEV + 31].operator = 21;
	dispatch[FIRST_DEV + 32].controller = 126;
	dispatch[FIRST_DEV + 32].operator = 22;

	dispatch[FIRST_DEV + 33].controller = 4;
	dispatch[FIRST_DEV + 33].operator = 0;
	dispatch[FIRST_DEV + 34].controller = 4;
	dispatch[FIRST_DEV + 34].operator = 1;
	dispatch[FIRST_DEV + 35].controller = 126;
	dispatch[FIRST_DEV + 35].operator = 23;
	dispatch[FIRST_DEV + 36].controller = 4;
	dispatch[FIRST_DEV + 36].operator = 3;
	dispatch[FIRST_DEV + 36].routine = (synthRoutine) keyTracking;
	dispatch[FIRST_DEV + 37].controller = 3;
	dispatch[FIRST_DEV + 37].operator = 0;
	dispatch[FIRST_DEV + 38].controller = 3;
	dispatch[FIRST_DEV + 38].operator = 1;
	dispatch[FIRST_DEV + 39].controller = 3;
	dispatch[FIRST_DEV + 39].operator = 2;
	dispatch[FIRST_DEV + 40].controller = 3;
	dispatch[FIRST_DEV + 40].operator = 3;

	dispatch[FIRST_DEV + 41].controller = 5;
	dispatch[FIRST_DEV + 41].operator = 0;
	dispatch[FIRST_DEV + 42].controller = 5;
	dispatch[FIRST_DEV + 42].operator = 1;
	dispatch[FIRST_DEV + 43].controller = 5;
	dispatch[FIRST_DEV + 43].operator = 2;
	dispatch[FIRST_DEV + 44].controller = 5;
	dispatch[FIRST_DEV + 44].operator = 3;

	dispatch[FIRST_DEV + 45].controller = 126;
	dispatch[FIRST_DEV + 45].operator = 2;
	dispatch[FIRST_DEV + 46].controller = 5;
	dispatch[FIRST_DEV + 46].operator = 4;

	dispatch[FIRST_DEV + 47].controller = 126;
	dispatch[FIRST_DEV + 47].operator = 32;
	dispatch[FIRST_DEV + 48].controller = 126;
	dispatch[FIRST_DEV + 48].operator = 31;
	dispatch[FIRST_DEV + 49].controller = 126;
	dispatch[FIRST_DEV + 49].operator = 30;
	dispatch[FIRST_DEV + 50].controller = 126;
	dispatch[FIRST_DEV + 50].operator = 33;

	dispatch[DEV_OFFSET + 49].controller = 1;
	dispatch[DEV_OFFSET + 49].operator = 1;
	dispatch[DEV_OFFSET + 49].routine = (synthRoutine) midiRelease;
	dispatch[DEV_OFFSET + 50].controller = 126;
	dispatch[DEV_OFFSET + 50].operator = 3;
	dispatch[DEV_OFFSET + 51].controller = 1;
	dispatch[DEV_OFFSET + 51].operator = 1;
	dispatch[DEV_OFFSET + 51].routine = (synthRoutine) multiTune;

	dispatch[DEV_OFFSET + 52].operator = 1;
	dispatch[DEV_OFFSET + 53].operator = 2;
	dispatch[DEV_OFFSET + 54].operator = 3;
	dispatch[DEV_OFFSET + 55].operator = 4;
	dispatch[DEV_OFFSET + 56].operator = 5;
	dispatch[DEV_OFFSET + 57].operator = 6;
	dispatch[DEV_OFFSET + 58].operator = 7;
	dispatch[DEV_OFFSET + 59].operator = 8;

	dispatch[DEV_OFFSET + 60].controller = 1;
	dispatch[DEV_OFFSET + 61].controller = 2;
	dispatch[DEV_OFFSET + 62].controller = 3;

	dispatch[DEV_OFFSET + 52].routine =
		dispatch[DEV_OFFSET + 53].routine =
		dispatch[DEV_OFFSET + 54].routine =
		dispatch[DEV_OFFSET + 55].routine =
		dispatch[DEV_OFFSET + 56].routine =
		dispatch[DEV_OFFSET + 57].routine =
		dispatch[DEV_OFFSET + 58].routine =
		dispatch[DEV_OFFSET + 59].routine =
		dispatch[DEV_OFFSET + 60].routine =
		dispatch[DEV_OFFSET + 61].routine =
		dispatch[DEV_OFFSET + 62].routine
			= (synthRoutine) prophet52Memory;

	dispatch[DEV_OFFSET + 63].routine = dispatch[DEV_OFFSET + 64].routine =
		(synthRoutine) prophet52Midi;
	dispatch[DEV_OFFSET + 63].controller = 1;
	dispatch[DEV_OFFSET + 64].controller = 2;

	dispatch[RADIOSET_1].other1 = -1;

	/* Tune osc-1 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	/* Gain on filter contour */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4,
		CONTROLLER_RANGE - 1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 4);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
prophet52Configure(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

	if (synth == 0)
	{
		printf("problems going operational\n");
		return(-1);
	}

	if (synth->flags & OPERATIONAL)
		return(0);

printf("going operational\n");

	synth->flags |= OPERATIONAL;
	synth->keypanel = 1;
	synth->keypanel2 = -1;
	synth->transpose = 36;

	if (synth->location == 0)
	{
		synth->bank = 10;
		synth->location = 1;
	}
	loadMemory(synth, "prophet52", 0, initmem, synth->mem.active, FIRST_DEV, 0);

	brightonPut(win,
		"bitmaps/blueprints/prophet52shade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	return(0);
}

