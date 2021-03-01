
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
#include <strings.h>
#include <sys/time.h>

#include "brightonMixer.h"
#include "brightoninternals.h"

#define DISPLAY1 (FUNCTION_COUNT - 6)
#define DISPLAY2 (FUNCTION_COUNT - 5)
#define DISPLAY3 (FUNCTION_COUNT - 4)
#define DISPLAY4 (FUNCTION_COUNT - 3)
#define DISPLAY5 (FUNCTION_COUNT - 2)
#define DISPLAY6 (FUNCTION_COUNT - 1)

#include "brighton.h"
#include "brightonMini.h"
#include "brightonMixer.h"
#include "brightonMixerMemory.h"

static char text[128];
/*static char scratch[MAX_CHAN_COUNT][16]; */
static char memscratch[16];

static char *removeInterface(guiSynth *);
static char *printInterface(guiSynth *);
static char *printTrackMenu(guiSynth *);
static char *printMidiMenu(guiSynth *);
static char *trackDown(guiSynth *);
static char *trackUp(guiSynth *);
static char *midiDown(guiSynth *);
static char *midiUp(guiSynth *);
static char *setTrackText(guiSynth *);

static char *printMemMenu(guiSynth *);
static char *memoryBuildList(guiSynth *);
static char *memoryList(guiSynth *);
static char *memoryShowList(guiSynth *);
static char *memoryListUp(guiSynth *);
static char *memoryListDown(guiSynth *);

static char *memorySel1(guiSynth *);
static char *memorySel2(guiSynth *);
static char *memorySel3(guiSynth *);
static char *memorySel4(guiSynth *);

static char *songSel1(guiSynth *);
static char *songSel2(guiSynth *);
static char *songSel3(guiSynth *);
static char *songSel4(guiSynth *);

static char *songList(guiSynth *);
static char *createSongDir(guiSynth *);

extern int loadMixerMemory(guiSynth *, char *, int);
extern int saveMixerMemory(guiSynth *, char *);
extern void displayPanel(guiSynth *, char *, int, int, int);
extern int setMixerMemory(mixerMem *, int, int, float *, char *);

/*
 * Ok, fixed table sizes, but a kilo of memories will do for now.
 */
#define MIXER_MEM_COUNT 1024
int memCount = -1, memIndex = 0;
char memList[MIXER_MEM_COUNT][32];

guiSynth *theSynth;

/*
 * We should not really have to rely on timers within the GUI: The engine will
 * evaluate RMS and Peak signal values for the channels, and save them. The 
 * Midi thread should take these values X times per second (ie, with a timer)
 * and send a message to the GUI over the interface tap.
 *
 * Whenever the GUI gets a VU message it should update the VU meter display.
 */
u_long vuInterval = 100000;

/*
 * Need a set of structures that allow a TitleBar strings, 5 substrings, 
 * 2 "next menus" for the Title, 2 "next menus" or otherwise functions.
 */
typedef char *(*MenuFunc)(guiSynth *);

typedef struct mLine {
	char title[32];
	int last, next;
	MenuFunc lmenuFunc;
	MenuFunc rmenuFunc;
} mLine;

#define line1 line[0]
#define line2 line[1]
#define line3 line[2]
#define line4 line[3]
#define line5 line[4]

typedef struct Menu {
	mLine title;
	mLine line[5];
} Menu;

#define MENU_COUNT 32

int currentMenu = -1;
int currentTrack = 0;

char *setInt0()
{
	vuInterval = 0;
	return(NULL);
}

char *setInt50()
{
	vuInterval = 50000;
	return(NULL);
}

char *setInt100()
{
	vuInterval = 100000;
	return(NULL);
}

char *setInt200()
{
	vuInterval = 200000;
	return(NULL);
}

static char * loadShim(guiSynth *synth)
{
	loadMixerMemory(synth, memscratch, 0);
	return(NULL);
}

static char * saveShim(guiSynth *synth)
{
	saveMixerMemory(synth, memscratch);
	return(NULL);
}
static char * undoShim(guiSynth *synth)
{
	loadMixerMemory(synth, "revert", 0);
	return(NULL);
}

Menu functionMenu[MENU_COUNT] = {
	{
		{"       Main Menu            ", 3, 1, 0, 0},
		{{"Effects             Bus     ", 1, 2, 0, 0},
		{"Track                VU      ", 3, 12, 0, 0},
		{"Audio                Mem     ", 13, 14, 0, 0},
		{"Stats             Print     ", 15, -1, 0, printInterface},
		{"Midi                 Quit   ", 17, 16, 0, 0}},
	},
	{
		{"     Effects Menu           ", 0, 2, 0, 0},
		{{"Pre 1             Post 1    ", 4, 8, 0, 0},
		{"Pre 2            Post 2     ", 5, 9, 0, 0},
		{"Pre 3            Post 3     ", 6, 10, 0, 0},
		{"Pre 4            Post 4     ", 7, 11, 0, 0},
		{"                    Main       ", -1, 0, 0, 0}},
	},
	{
		{"       Bus Menu             ", 1, 3, 0, 0},
		{{"Effects             Bus     ", 1, 2, 0, 0},
		{"Stats                       ", 3, -1, 0, 0},
		{"                            ", -1, -1, 0, 0},
		{"                            ", -1, -1, 0, 0},
		{"                     Main    ", -1, 0, 0, 0}},
	},
	{
		{"       Track Menu           ", 2, 12, printTrackMenu, 0},
		{{"                            ", -1, -1, 0, 0},
		{"Text:                       ", -1, -1, 0, 0},
		{"Down                  UP    ", -1, -1, trackDown, trackUp},
		{"                            ", -1, -1, 0, 0},
		{"                     MAIN   ", -1, 0, setTrackText, 0}},
	},
/* 4: */
	{
		{"       Pre 1 Menu           ", 1, 1, 0, 0},
		{{"P 1                  P 4  ", -1, -1, 0, 0},
		{"P 2                  P 5  ", -1, -1, 0, 0},
		{"P 3                  P 6    ", -1, -1, 0, 0},
		{"Prev               Next    ", 11, 5, 0, 0},
		{"                    Main    ", -1, 0, 0, 0}},
	},
	{
		{"       Pre 2 Menu           ", 1, 1, 0, 0},
		{{"P 1                  P 4  ", -1, -1, 0, 0},
		{"P 2                  P 5  ", -1, -1, 0, 0},
		{"P 3                  P 6    ", -1, -1, 0, 0},
		{"Prev               next    ", 4, 6, 0, 0},
		{"                    Main    ", -1, 0, 0, 0}},
	},
	{
		{"       Pre 3 Menu           ", 1, 1, 0, 0},
		{{"P 1                  P 4  ", -1, -1, 0, 0},
		{"P 2                  P 5  ", -1, -1, 0, 0},
		{"P 3                  P 6    ", -1, -1, 0, 0},
		{"Prev               next    ", 5, 7, 0, 0},
		{"                    Main    ", -1, 0, 0, 0}},
	},
	{
		{"       Pre 4 Menu           ", 1, 1, 0, 0},
		{{"P 1                  P 4  ", -1, -1, 0, 0},
		{"P 2                  P 5  ", -1, -1, 0, 0},
		{"P 3                  P 6    ", -1, -1, 0, 0},
		{"Prev               next    ", 6, 8, 0, 0},
		{"                    Main    ", -1, 0, 0, 0}},
	},
/* 8: */
	{
		{"       Post 1 Menu          ", 1, 1, 0, 0},
		{{"P 1                  P 4  ", -1, -1, 0, 0},
		{"P 2                  P 5  ", -1, -1, 0, 0},
		{"P 3                  P 6    ", -1, -1, 0, 0},
		{"Prev               next    ", 7, 9, 0, 0},
		{"                    Main    ", -1, 0, 0, 0}},
	},
	{
		{"       Post 2 Menu          ", 1, 1, 0, 0},
		{{"P 1                  P 4  ", -1, -1, 0, 0},
		{"P 2                  P 5  ", -1, -1, 0, 0},
		{"P 3                  P 6    ", -1, -1, 0, 0},
		{"Prev               next    ", 8, 10, 0, 0},
		{"                    Main    ", -1, 0, 0, 0}},
	},
	{
		{"       Post 3 Menu          ", 1, 1, 0, 0},
		{{"P 1                  P 4  ", -1, -1, 0, 0},
		{"P 2                  P 5  ", -1, -1, 0, 0},
		{"P 3                  P 6    ", -1, -1, 0, 0},
		{"Prev               next    ", 9, 11, 0, 0},
		{"                    Main    ", -1, 0, 0, 0}},
	},
	{
		{"       Post 4 Menu          ", 1, 1, 0, 0},
		{{"P 1                  P 4  ", -1, -1, 0, 0},
		{"P 2                  P 5  ", -1, -1, 0, 0},
		{"P 3                  P 6    ", -1, -1, 0, 0},
		{"Prev               next    ", 10, 4, 0, 0},
		{"                    Main    ", -1, 0, 0, 0}},
	},
/* 12: */
	{
		{"       VU Menu             ", 3, 13, 0, 0},
		{{"Interval 50ms              ", -1, -1, setInt50, 0},
		{"Interval 100ms             ", -1, -1, setInt100, 0},
		{"Interval 200ms              ", -1, -1, setInt200, 0},
		{"                            ", -1, -1, 0, 0},
		{"Off                 MAIN    ", -1, 0, setInt0, 0}},
	},
	{
		{"       Audio Menu           ", 12, 14, 0, 0},
		{{"Effects             Bus     ", 0, 2, 0, 0},
		{"Stats                       ", 0, -1, 0, 0},
		{"                            ", -1, -1, 0, 0},
		{"                            ", -1, -1, 0, 0},
		{"                     MAIN    ", -1, 0, 0, 0}},
	},
	{
		{"       MEM Menu             ", 13, 15, printMemMenu, 0},
		{{"Name:                        ", -1, -1, 0, 0},
		{"Create Song                 ", -1, -1, createSongDir, 0},
		{"Load            Search   ", -1, 20, loadShim, memoryList},
		{"Save               Undo     ", -1, -1, saveShim, undoShim},
		{"Songs               MAIN   ", 21, 0, songList, 0}},
	},
	{
		{"       Stats Menu           ", 14, 0, 0, 0},
		{{"Effects             Bus     ", 0, 2, 0, 0},
		{"Stats                       ", 0, -1, 0, 0},
		{"                            ", -1, -1, 0, 0},
		{"                            ", -1, -1, 0, 0},
		{"                     MAIN    ", -1, 0, 0, 0}},
	},
/* 16: */
	{
		{"       Quit Menu         ", 0, 0, 0, 0},
		{{"                            ", -1, -1, 0, 0},
		{"                            ", -1, -1, 0, 0},
		{"      Really quit?          ", -1, -1, 0, 0},
		{"                            ", -1, -1, 0, 0},
		{"No                   Yes    ", 0, 3, 0, removeInterface}},
	},
	{
		{"       Midi Menu         ", 0, 0, printMidiMenu, 0},
		{{"                            ", -1, -1, 0, 0},
		{"Channel:                    ", -1, -1, 0, 0},
		{"                      Up    ", -1, -1, 0, midiUp},
		{"                      Down  ", -1, -1, 0, midiDown},
		{"                     Main   ", -1, 0, 0, 0}},
	},
	{
	},
	{
	},
/* 20: */
	{
		{"Back    Search    Main      ", 14, 0, 0, 0},
		{{"                            ", 14, 14, memorySel1, memorySel1},
		{"                            ", 14, 14, memorySel2, memorySel2},
		{"                            ", 14, 14, memorySel3, memorySel3},
		{"                            ", 14, 14, memorySel4, memorySel4},
		{"Down                  UP    ", 20, 20, memoryListUp, memoryListDown}},
	},
	{
		{"Back    Songs     Main      ", 14, 0, 0, 0},
		{{"                            ", 20, 20, songSel1, songSel1},
		{"                            ", 20, 20, songSel2, songSel2},
		{"                            ", 20, 20, songSel3, songSel3},
		{"                            ", 20, 20, songSel4, songSel4},
		{"Down                  UP    ", 21, 21, memoryListUp, memoryListDown}},
	},
	{
	},
	{
	},
/* 24: */
	{
	},
	{
	},
	{
	},
	{
	},
/* 28: */
	{
	},
	{
	},
	{
	},
	{
	}
};

static void
displayMenu(guiSynth *synth, int panel, int menu)
{
	if (functionMenu[currentMenu].title.lmenuFunc != 0)
	{
		/*
		 * If we have a left function to draw the display, it must do all the 
		 * lines.
		 */
		functionMenu[menu].title.lmenuFunc(synth);
		return;
	} else if (functionMenu[currentMenu].title.rmenuFunc != 0) {
		displayPanel(synth,
			functionMenu[menu].title.rmenuFunc(synth),
			0, FUNCTION_PANEL, DISPLAY1);
	} else {
		displayPanel(synth,
			functionMenu[menu].title.title, 0, FUNCTION_PANEL, DISPLAY1);
	}

	displayPanel(synth,
		functionMenu[menu].line1.title, 0, FUNCTION_PANEL, DISPLAY2);
	displayPanel(synth,
		functionMenu[menu].line2.title, 0, FUNCTION_PANEL, DISPLAY3);
	displayPanel(synth,
		functionMenu[menu].line3.title, 0, FUNCTION_PANEL, DISPLAY4);
	displayPanel(synth,
		functionMenu[menu].line4.title, 0, FUNCTION_PANEL, DISPLAY5);
	displayPanel(synth,
		functionMenu[menu].line5.title, 0, FUNCTION_PANEL, DISPLAY6);
}

int
functionOp(guiSynth *synth, int panel, int index, int operator, float value)
{
	if ((synth->flags && OPERATIONAL) == 0)
		return(0);

/*	printf("functionOp(%x, %i, %i, %i, %f)\n", */
/*		synth, panel, index, operator, value); */

	if (currentMenu == -1)
	{
		/*
		 * This is to init to the first menu
		 */
		currentMenu = 0;
		displayMenu(synth, panel, currentMenu);

		/*
		 * And use the chance to seed the interupts for the VU meter.
		 */
		theSynth = synth;

		return(0);
	}

	if ((index == 13) || (index == 24) || (index == 25))
	{
		/*
		 * memory buttons, 13 is 'rezero', 24 is load and 25 is save?
		 *
		 * These are bespoke since the generic synth memories use a single
		 * panel of parameters whilst the mixer needs many?
		 */
		if (index == 25)
		{
			if (memCount < 0)
				memoryBuildList(synth);

			if (++memIndex > memCount)
				memIndex = 0;

			sprintf(memscratch, "%s", memList[memIndex]);
			loadMixerMemory(synth, memList[memIndex], 0);
			currentMenu = 14;
			printMemMenu(synth);

			return(0);
		}
		if (index == 24)
		{
			if (memCount < 0)
				memoryBuildList(synth);

			if (--memIndex < 0)
				memIndex = memCount;

			sprintf(memscratch, "%s", memList[memIndex]);
			loadMixerMemory(synth, memList[memIndex], 0);
			currentMenu = 14;
			printMemMenu(synth);

			return(0);
		}
		if (index == 13)
		{
			loadMixerMemory(synth, "revert", 0);
			return(0);
		}
	}

	if ((index >= DISPLAY1) && (index <= DISPLAY5))
	{
		/*
		 * If we have an event here it is going to be a keypress.
		 */
		if (currentMenu == 3)
		{
			int i;
			char scratch[32];

			/*
			 * Track menu, this will be track naming text. Value will be an
			 * ascii char, put it at the end of the string. char 08 and 7f are
			 * delete, ie, remove a character.
			 *
			 * Then print it on line2. 
			 */
			sprintf(scratch, "%s",
				getMixerMemory((mixerMem *) synth->mem.param,
					currentTrack + 4, 79));
			i = strlen(scratch);

			if ((value == 0xff08) || (value == -1))
			{
				/*
				 * Should be backspace/delete
				 */
				if (i > 0)
					scratch[i - 1] = '\0';
			} else if (i == 15)
				/*
				 * If max length then return now
				 */
				return(0);
			else {
				scratch[i] = value;
				scratch[i + 1] = '\0';
			}
			setMixerMemory((mixerMem *) synth->mem.param,
				currentTrack + 4, 79, 0, scratch);
			sprintf(text, "Text: %s               ", scratch);
			displayPanel(synth, text, 0, FUNCTION_PANEL, DISPLAY3);
			setTrackText(synth);

			return(0);
		}
		/*
		 * See if we are in the memory menu;
		 */
		else if (currentMenu == 14)
		{
			int i;
			/*
			 * Track menu, this will be track naming text. Value will be an
			 * ascii char, put it at the end of the string. char 08 and 7f are
			 * delete, ie, remove a character.
			 *
			 * Then print it on line2. 
			 */
			i = strlen(memscratch);

			if ((value == 0xff08) || (value == -1))
			{
				if (i > 0)
					memscratch[i - 1] = '\0';
			} else if (i == 15)
				return(0);
			else {
				memscratch[i] = value;
				memscratch[i + 1] = '\0';
			}
			sprintf(text, "Name: %s                 ", memscratch);
			displayPanel(synth, text, 0, FUNCTION_PANEL, DISPLAY2);

			return(0);
		} else if ((currentMenu == 20) || (currentMenu == 21)) {
			int k, j, i = memIndex;
			/*
			 * This is the search window. Go through the memList looking for
			 * the next entry that starts with the given letter.
			 */
			for (j = 0; j <= memCount; j ++)
			{
				if (++i > memCount)
					i = 0;

				if (memList[i][0] == value)
				{
					memIndex = i;
					memoryShowList(synth);
					return(0);
				}
			}
			/*
			 * If we get here then we could not find a match on the first 
			 * letter. Lets see about the second letter (not sure why, or if
			 * it really makes sense
			 */
			i = memIndex;
			for (j = 0; j <= memCount; j ++)
			{
				if (++i > memCount)
					i = 0;

				if (memList[i][1] == value)
				{
					memIndex = i;
					memoryShowList(synth);
					return(0);
				}
			}
			/*
			 * If that failed then I personally think we should look for any 
			 * entry that contains the given character. We might consider
			 * dropping the last search.
			 */
			i = memIndex;
			for (j = 0; j <= memCount; j++)
			{
				if (++i > memCount)
					i = 0;

				for (k = 0; memList[i][k] != '\0'; k++)
				{
					if (memList[i][k] == value)
					{
						memIndex = i;
						memoryShowList(synth);
						return(0);
					}
				}
			}
		}

		return(0);
	}

	/*
	 * Title back
	 *
	 * Apart from the title, the rest of the lines can be put into procedures
	 * so that we can call the code with an index, as it is now an array.
	 *
	 * This is damn ugly. How was it not made into a case or a structured
	 * array of calls?
	 */
	if (operator == 0)
	{
		if (functionMenu[currentMenu].title.last != -1)
		{
			currentMenu = functionMenu[currentMenu].title.last;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 1) {
		/*
		 * Title forward
		 */
		if (functionMenu[currentMenu].title.next != -1)
		{
			currentMenu = functionMenu[currentMenu].title.next;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 2) {
		/*
		 * Line 1 back
		 */
		if (functionMenu[currentMenu].line1.lmenuFunc != 0)
		{
			functionMenu[currentMenu].line1.lmenuFunc(synth);

			return(0);
		}
		if (functionMenu[currentMenu].line1.last != -1)
		{
			currentMenu = functionMenu[currentMenu].line1.last;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 7) {
		/*
		 * Line 1 forward
		 */
		if (functionMenu[currentMenu].line1.rmenuFunc != 0)
		{
			functionMenu[currentMenu].line1.rmenuFunc(synth);

			return(0);
		}
		if (functionMenu[currentMenu].line1.next != -1)
		{
			currentMenu = functionMenu[currentMenu].line1.next;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 3) {
		/*
		 * Line 2 back
		 */
		if (functionMenu[currentMenu].line2.lmenuFunc != 0)
		{
			functionMenu[currentMenu].line2.lmenuFunc(synth);

			return(0);
		}
		if (functionMenu[currentMenu].line2.last != -1)
		{
			currentMenu = functionMenu[currentMenu].line2.last;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 8) {
		/*
		 * Line 2 forward
		 */
		if (functionMenu[currentMenu].line2.rmenuFunc != 0)
		{
			functionMenu[currentMenu].line2.rmenuFunc(synth);

			return(0);
		}
		if (functionMenu[currentMenu].line2.next != -1)
		{
			currentMenu = functionMenu[currentMenu].line2.next;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 4) {
		/*
		 * Line 3 back
		 */
		if (functionMenu[currentMenu].line3.lmenuFunc != 0)
		{
			functionMenu[currentMenu].line3.lmenuFunc(synth);

			return(0);
		}
		if (functionMenu[currentMenu].line3.last != -1)
		{
			currentMenu = functionMenu[currentMenu].line3.last;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 9) {
		/*
		 * Line 3 forward
		 */
		if (functionMenu[currentMenu].line3.rmenuFunc != 0)
		{
			functionMenu[currentMenu].line3.rmenuFunc(synth);

			return(0);
		}
		if (functionMenu[currentMenu].line3.next != -1)
		{
			currentMenu = functionMenu[currentMenu].line3.next;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 5) {
		/*
		 * Line 4 back
		 */
		if (functionMenu[currentMenu].line4.lmenuFunc != 0)
		{
			functionMenu[currentMenu].line4.lmenuFunc(synth);

			return(0);
		}
		if (functionMenu[currentMenu].line4.last != -1)
		{
			currentMenu = functionMenu[currentMenu].line4.last;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 10) {
		if (functionMenu[currentMenu].line4.rmenuFunc != 0)
		{
			functionMenu[currentMenu].line4.rmenuFunc(synth);

			return(0);
		}
		/*
		 * Line 4 forward
		 */
		if (functionMenu[currentMenu].line4.next != -1)
		{
			currentMenu = functionMenu[currentMenu].line4.next;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 6) {
		/*
		 * Line 5 back
		 */
		if (functionMenu[currentMenu].line5.lmenuFunc != 0)
		{
			functionMenu[currentMenu].line5.lmenuFunc(synth);

			return(0);
		}
		if (functionMenu[currentMenu].line5.last != -1)
		{
			currentMenu = functionMenu[currentMenu].line5.last;
			displayMenu(synth, panel, currentMenu);
		}
	} else if (operator == 11) {
		if (functionMenu[currentMenu].line5.rmenuFunc != 0)
		{
			functionMenu[currentMenu].line5.rmenuFunc(synth);

			return(0);
		}
		/*
		 * Line 5 forward
		 */
		if (functionMenu[currentMenu].line5.next != -1)
		{
			currentMenu = functionMenu[currentMenu].line5.next;
			displayMenu(synth, panel, currentMenu);
		}
	}
	return(0);
}

static char *
printInterface(guiSynth *synth)
{
	/*
	 * Export image to bitmap file.
	 */
	brightonXpmWrite(synth->win, "/tmp/mixer.xpm");

	return(NULL);
}

#warning - fixed track configuration at 16 tracks.
static char *
trackDown(guiSynth *synth)
{
	if (--currentTrack < 0)
		currentTrack = 15;

	printTrackMenu(synth);
	return(NULL);
}

static char *
trackUp(guiSynth *synth)
{
	if (++currentTrack == 16)
		currentTrack = 0;

	printTrackMenu(synth);
	return(NULL);
}

static char *
midiUp(guiSynth *synth)
{
	if (++synth->midichannel == 16)
		synth->midichannel = 15;

	printMidiMenu(synth);
	return(NULL);
}

static char *
midiDown(guiSynth *synth)
{
	if (--synth->midichannel < 0)
		synth->midichannel = 0;

	printMidiMenu(synth);
	return(NULL);
}

static char *
setTrackText(guiSynth *synth)
{
	char scratch[32];

	sprintf(scratch, "%s", getMixerMemory((mixerMem *) synth->mem.param, currentTrack + 4, 79));

	displayPanel(synth, scratch, 0, currentTrack + 4, PARAM_COUNT - 1);
	/*
	 * Looks ugly? The issue is that the channels use contiguous store in the
	 * gui, but I have to separate this into the memories.
	 */
	setMixerMemory((mixerMem *) synth->mem.param, CHAN_PANEL, 79 + (currentTrack * 80), 0, scratch);
	return(NULL);
}

static char *
printTrackMenu(guiSynth *synth)
{
	char scratch[32];

	if ((currentTrack + 4) < 0)
		return(NULL);

	sprintf(scratch, "%s", getMixerMemory((mixerMem *) synth->mem.param, currentTrack + 4, 79));

	/*
	 * Create text for track display
	 */
	sprintf(text, "       Track %i Menu           ", currentTrack + 1);
	displayPanel(synth, text, 0, FUNCTION_PANEL, DISPLAY1);

	displayPanel(synth,
		functionMenu[3].line[0].title, 0, FUNCTION_PANEL, DISPLAY2);

	sprintf(text, "Text: %s                         ", scratch);
	displayPanel(synth, text, 0, FUNCTION_PANEL, DISPLAY3);

	displayPanel(synth,
		functionMenu[3].line[2].title, 0, FUNCTION_PANEL, DISPLAY4);
	displayPanel(synth,
		functionMenu[3].line[3].title, 0, FUNCTION_PANEL, DISPLAY5);
	displayPanel(synth,
		functionMenu[3].line[4].title, 0, FUNCTION_PANEL, DISPLAY6);
	return(NULL);
}


static char *
songSel1(guiSynth *synth)
{
	currentMenu = 20;
	/*
	 * Set the current song to the given name and then print the list of
	 * songs in that directory
	 */
	loadMixerMemory(synth, memList[memIndex], 1);
	memoryList(synth);
	return(NULL);
}
static char *
songSel2(guiSynth *synth)
{
	currentMenu = 20;

	memIndex += 1;
	if (memIndex > memCount) memIndex -= memCount + 1;
	loadMixerMemory(synth, memList[memIndex], 1);
	memoryList(synth);
	return(NULL);
}
static char *
songSel3(guiSynth *synth)
{
	currentMenu = 20;
	memIndex += 2;
	if (memIndex > memCount) memIndex -= memCount + 1;
	loadMixerMemory(synth, memList[memIndex], 1);
	memoryList(synth);
	return(NULL);
}
static char *
songSel4(guiSynth *synth)
{
	currentMenu = 20;
	memIndex += 3;
	if (memIndex > memCount) memIndex -= memCount + 1;
	loadMixerMemory(synth, memList[memIndex], 1);
	memoryList(synth);
	return(NULL);
}

static char *
createSongDir(guiSynth *synth)
{
	loadMixerMemory(synth, memscratch, 2);
	return(NULL);
}

static char *
memorySel1(guiSynth *synth)
{
	currentMenu = 14;

	sprintf(memscratch, "%s", memList[memIndex]);

	printMemMenu(synth);
	return(NULL);
}
static char *
memorySel2(guiSynth *synth)
{
	currentMenu = 14;

	memIndex += 1;
	if (memIndex > memCount) memIndex -= memCount + 1;
	sprintf(memscratch, "%s", memList[memIndex]);

	printMemMenu(synth);
	return(NULL);
}
static char *
memorySel3(guiSynth *synth)
{
	currentMenu = 14;

	memIndex += 2;
	if (memIndex > memCount) memIndex -= memCount + 1;
	sprintf(memscratch, "%s", memList[memIndex]);

	printMemMenu(synth);
	return(NULL);
}
static char *
memorySel4(guiSynth *synth)
{
	currentMenu = 14;

	memIndex += 3;
	if (memIndex > memCount) memIndex -= memCount + 1;
	sprintf(memscratch, "%s", memList[memIndex]);

	printMemMenu(synth);
	return(NULL);
}

static char *
songBuildList(guiSynth *synth)
{
	char *entry;

	memCount = -1;

	while ((entry = getMixerMemory((mixerMem *) synth->mem.param, MM_GETLIST, 1)) != 0)
		sprintf(memList[++memCount], "%s", entry);
	/*while ((entry = ((char *) mixerMemory(synth, MM_GET, MM_GETLIST, 1, 0)))
		!= 0)
		sprintf(memList[++memCount], "%s", entry); */
	return(NULL);
}

static char *
memoryBuildList(guiSynth *synth)
{
	char *entry;

	memCount = -1;

	while ((entry = (getMixerMemory((mixerMem *) synth->mem.param, MM_GETLIST, 0))) != 0)
		sprintf(memList[++memCount], "%s", entry);
	return(NULL);
}

static char *
songShowList(guiSynth *synth)
{
	int mi = memIndex;

	if (memCount < 0)
		songBuildList(synth);

	/*
	 * Then show the memory entries in the panel from the list with the 
	 * given index
	 */
	sprintf(text, "%s", functionMenu[21].title.title);
	displayPanel(synth, text, 0, FUNCTION_PANEL, DISPLAY1);

	displayPanel(synth, memList[mi], 0, FUNCTION_PANEL, DISPLAY2);
	if (mi++ >= memCount) mi = 0;
	displayPanel(synth, memList[mi], 0, FUNCTION_PANEL, DISPLAY3);
	if (mi++ >= memCount) mi = 0;
	displayPanel(synth, memList[mi], 0, FUNCTION_PANEL, DISPLAY4);
	if (mi++ >= memCount) mi = 0;
	displayPanel(synth, memList[mi], 0, FUNCTION_PANEL, DISPLAY5);

	displayPanel(synth,
		functionMenu[21].line[4].title, 0, FUNCTION_PANEL, DISPLAY6);
	return(NULL);
}

static char *
memoryShowList(guiSynth *synth)
{
	int mi = memIndex;

	if (memCount < 0)
		memoryBuildList(synth);

	/*
	 * Then show the memory entries in the panel from the list with the 
	 * given index
	 */
	sprintf(text, "%s", functionMenu[20].title.title);
	displayPanel(synth, text, 0, FUNCTION_PANEL, DISPLAY1);

	displayPanel(synth, memList[mi], 0, FUNCTION_PANEL, DISPLAY2);
	if (mi++ >= memCount) mi = 0;
	displayPanel(synth, memList[mi], 0, FUNCTION_PANEL, DISPLAY3);
	if (mi++ >= memCount) mi = 0;
	displayPanel(synth, memList[mi], 0, FUNCTION_PANEL, DISPLAY4);
	if (mi++ >= memCount) mi = 0;
	displayPanel(synth, memList[mi], 0, FUNCTION_PANEL, DISPLAY5);

	displayPanel(synth,
		functionMenu[20].line[4].title, 0, FUNCTION_PANEL, DISPLAY6);
	return(NULL);
}

static char *
songList(guiSynth *synth)
{
	/*
	 * We need to build a list of the memories in our default directory.
	 * This should become a few routines, one that builds that list and two
	 * that allow us to scroll up and down it.
	 */
	songBuildList(synth);

	songShowList(synth);

	currentMenu = 21;
	return(NULL);
}

static char *
memoryList(guiSynth *synth)
{
	/*
	 * We need to build a list of the memories in our default directory.
	 * This should become a few routines, one that builds that list and two
	 * that allow us to scroll up and down it.
	 */
	memoryBuildList(synth);

	memoryShowList(synth);

	currentMenu = 20;
	return(NULL);
}

static char *
memoryListDown(guiSynth *synth)
{
	/*
	 * Cycle upwards though our list and show the names.
	 */
	if (++memIndex > memCount)
		memIndex = 0;

	memoryShowList(synth);

	return(NULL);
}

static char *
memoryListUp(guiSynth *synth)
{
	/*
	 * Cycle downwards though our list and show the names.
	 */
	if (--memIndex < 0)
		memIndex = memCount;

	memoryShowList(synth);

	return(0);
}

static char *
printMemMenu(guiSynth *synth)
{
	/*
	 * Create text for track display
	displayPanel(synth, "        Mem Menu              ",
		0, FUNCTION_PANEL, DISPLAY1);
	 */
	displayPanel(synth, functionMenu[14].title.title,
		0, FUNCTION_PANEL, DISPLAY1);

	sprintf(text, "Name: %s                         ", memscratch);
	displayPanel(synth, text, 0, FUNCTION_PANEL, DISPLAY2);

	displayPanel(synth,
		functionMenu[14].line[1].title, 0, FUNCTION_PANEL, DISPLAY3);
	displayPanel(synth,
		functionMenu[14].line[2].title, 0, FUNCTION_PANEL, DISPLAY4);
	displayPanel(synth,
		functionMenu[14].line[3].title, 0, FUNCTION_PANEL, DISPLAY5);
	displayPanel(synth,
		functionMenu[14].line[4].title, 0, FUNCTION_PANEL, DISPLAY6);
	return(NULL);
}

static char *
printMidiMenu(guiSynth *synth)
{
	/*
	 * Create text for track display
	 */
	displayPanel(synth, "       Midi Menu              ",
		0, FUNCTION_PANEL, DISPLAY1);

	displayPanel(synth,
		functionMenu[17].line[0].title,
		0, FUNCTION_PANEL, DISPLAY2);

	sprintf(text, "Channel: %i                       ", synth->midichannel + 1);
	displayPanel(synth, text, 0, FUNCTION_PANEL, DISPLAY3);

	displayPanel(synth,
		functionMenu[17].line[2].title, 0, FUNCTION_PANEL, DISPLAY4);
	displayPanel(synth,
		functionMenu[17].line[3].title, 0, FUNCTION_PANEL, DISPLAY5);
	displayPanel(synth,
		functionMenu[17].line[4].title, 0, FUNCTION_PANEL, DISPLAY6);
	return(NULL);
}

static char *
removeInterface(guiSynth *synth)
{
	/*
	 * Exit application, completely.
	 */
	brightonRemoveInterface(synth->win);
	return(NULL);
}

brightonEvent event;

int
doAlarm()
{
	event.type = BRIGHTON_FLOAT;
	event.value = 0.0;

/*	brightonSendEvent((void *) theSynth->win, 2, DISPLAY1 - 1, &event); */
	return(0);
}

