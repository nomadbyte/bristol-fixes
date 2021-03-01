
/*
 *  Diverse Bristol midi routines.
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

#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>

#include <string.h>

#include "brighton.h"
#include "brightoninternals.h"
#include "brightonMini.h"

extern char *getBristolCache(char *);
extern void brightonChangeParam(guiSynth *, int, int, float);
extern void brightonKeyInput(brightonWindow *, int, int);

static fd_set stdioset[32];
static struct timeval timeout;

static float lp = -1;

struct termios resetattr;

#define B_TTY_ACT_COUNT	80
#define B_TTY_LINE_LEN	256

#define B_TTY_RAW		0x00000001
#define B_TTY_RAW_P		0x00000002
#define B_TTY_RAW_P2	0x00000004
#define B_TTY_COOKED	0x00000008
#define B_TTY_RAW_ESC	0x00000010
#define B_TTY_RAW_ESC2	0x00000020
#define B_TTY_SEARCH	0x00000040
#define B_TTY_PLAY		0x00000080
#define B_TTY_MMASK		0x00000fff

#define B_TTY_INIT		0x00001000
#define B_TTY_DEBUG		0x00002000
#define B_TTY_AWV		0x00004000
#define B_TTY_AUV		0x00008000
#define B_TTY_ALIASING	0x00010000
#define B_TTY_SAVE_HIST	0x00020000
#define B_TTY_PLAY_LINE	0x00040000
#define B_TTY_CLEAR_KEY	0x00080000
#define B_TTY_SEQ		0x00100000

#define B_TTY_ARGC 32

static char hist[51][B_TTY_LINE_LEN];
static char pbuf[B_TTY_LINE_LEN];
static char cbuf[B_TTY_LINE_LEN];

#define B_SEQ_COUNT 16

static struct {
	int count, rate, step[B_SEQ_COUNT];
} sequences[B_SEQ_COUNT] = {
	{8, 30, {52 + 0, 52 + 7, 52 + 3, 52 + 7, 52 + 0, 52 + 5, 52 + 9, 52 + 5}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
	{0, 0, {0}},
};

static char *unknown = "unnamed parameter";
static char *b_blank = "                                                                            ";

#define B_TTY_A_LEFT	0
#define B_TTY_A_RIGHT	1
#define B_TTY_A_INCMIN	2
#define B_TTY_A_INC		3
#define B_TTY_A_INCMAX	4
#define B_TTY_A_DECMIN	5
#define B_TTY_A_DEC		6
#define B_TTY_A_DECMAX	7
#define B_TTY_A_M_UP	8
#define B_TTY_A_M_DOWN	9
#define B_TTY_A_M_READ	10
#define B_TTY_A_M_WRITE	11
#define B_TTY_A_M_TOG	12
#define B_TTY_A_INSERT	13
#define B_TTY_A_INC1	14
#define B_TTY_A_INC4	15
#define B_TTY_A_DEC1	16
#define B_TTY_A_DEC4	17
#define B_TTY_A_UPDATE	18
#define B_TTY_A_FIND	19
#define B_TTY_A_NULL	-1

#define RESOURCES global->synths->resources
#define SYNTHS global->synths
#define paramname(x) RESOURCES->resources[btty.p].devlocn[x].name
//#define DEVICE(x) RESOURCES->resources[btty.p].devlocn[x]
#define DEVICE(x) global->synths->win->app->resources[btty.p].devlocn[x]
#define PDEV(x) ((brightonDevice *) global->synths->win->app->resources[btty.p].devlocn[x].dev)

#define unnamed(x) ((paramname(x)[0] == '\0') || (DEVICE(x).flags & BRIGHTON_WITHDRAWN))

//	if (RESOURCES->resources[btty.p].devlocn[btty.i].to == 1.0f)
//	if (DEVICE(btty.i).to == 1.0f)

brightonEvent event;
extern void printBrightonHelp(int);

typedef int (*clicom)();

static int bttyInterpret(guimain *, char *);
static int execHelp(guimain *, int, char **);
static int execMemory(guimain *, int, char **);
static int execLoad(guimain *, int, char **);
static int execDebug(guimain *, int, char **);
static int execImport(guimain *, int, char **);
static int execSet(guimain *, int, char **);
static int execBristol(guimain *, int, char **);
static int execBrighton(guimain *, int, char **);
static int execMidi(guimain *, int, char **);
static int execAlias(guimain *, int, char **);
static int execCLI(guimain *, int, char **);
static int execQuit(guimain *, int, char **);
static void bttyMemSave(guimain *);

/* Error codes */
#define B_ERR_EXIT		-1
#define B_ERR_OK		0
#define B_ERR_NOT_FOUND	1
#define B_ERR_PARAM		2
#define B_ERR_VALUE		3
#define B_ERR_INV_ARG	4
#define B_ERR_EARLY		5

/* global command codes */
#define B_COM_LAST		-1
#define B_COM_NOT_USED	0
#define B_COM_FREE		128

/* 'set memory' commands */
#define B_COM_FIND		1
#define B_COM_READ		2
#define B_COM_WRITE		3
#define B_COM_IMPORT	4
#define B_COM_EXPORT	5
#define B_COM_FORCE		6

typedef struct CommSet {
	char name[12];
	int map;
	char help[B_TTY_LINE_LEN];
	clicom exec;
	struct CommSet *subcom;
} comSet;

comSet brightoncom[4] = {
	{"",		B_COM_NOT_USED, "", 0, 0},
	{"panel",	B_COM_FIND,
		"<n> Set the active emulator parameter panel",
		0, 0},
	{"device",	B_COM_FIND,
		"<panel> <dev> <value> send parameter change message",
		0, 0},
	{"", B_COM_LAST, "", 0, 0},
};

comSet debugcomm[6] = {
	{"",		B_COM_NOT_USED, "", 0, 0},
	{"cli",	B_COM_FIND,
		"Command Line Debuging on/off",
		0, 0},
	{"midi",	B_COM_FIND,
		"debug level midi interface libraries 0..3",
		0, 0},
	{"frontend",	B_COM_FIND,
		"debug engine interface libraries on/off",
		0, 0},
	{"engine",	B_COM_FIND,
		"engine debuging level 0..15 (0 = off, >9 = verbose)",
		0, 0},
	{"", B_COM_LAST, "", 0, 0},
};

comSet bristolcom[4] = {
	{"",		B_COM_NOT_USED, "", 0, 0},
	{"register",	B_COM_FIND,
		"send MIDI surface controller registration request",
		0, 0},
	{"control",	B_COM_FIND,
		"<op> <cont> <value> send parameter change message to engine",
		0, 0},
	{"", B_COM_LAST, "", 0, 0},
};

comSet midicomm[24] = {
	{"",		B_COM_NOT_USED, "", 0, 0},
	{"channel",	B_COM_FIND,
		"configure engine midi channel 1..16",
		0, 0},
	{"sid",	B_COM_FIND,
		"select internal messaging id",
		0, 0},
	{"debug",	B_COM_FIND,
		"engine debuging level 0..16",
		0, 0},
	{"lowkey",	B_COM_FIND,
		"lower midi keyboard split point 0..127",
		0, 0},
	{"highkey",	B_COM_FIND,
		"higher midi keyboard split point 0..127",
		0, 0},
	{"chanpress",	B_COM_FIND,
		"send a channel pressure event value p=0..127",
		0, 0},
	{"polypress",	B_COM_FIND,
		"send a key pressure event k=0..127 p=0..127",
		0, 0},
	{"filter",	B_COM_FIND,
		"engine filters: lwf/nwf/wwf/hwf",
		0, 0},
	{"notepref",	B_COM_FIND,
		"monophonic note preference logic: hnp/lnp/nnp",
		0, 0},
	{"velocity",	B_COM_FIND,
		"midi velocity curve 0..1000",
		0, 0},
	{"detune",	B_COM_FIND,
		"engine 'temperature sensitivity' detuning",
		0, 0},
	{"glide",	B_COM_FIND,
		"maximum glide rate (5s)",
		0, 0},
	{"legato",	B_COM_FIND,
		"monophonic legato velocity logic on/off",
		0, 0},
	{"trigger",	B_COM_FIND,
		"monophonic trig/retrig logic on/off",
		0, 0},
	{"gain",	B_COM_FIND,
		"emulator global gain control",
		0, 0},
	{"pwd",	B_COM_FIND,
		"midi pitch wheel depth semitones",
		0, 0},
	{"nrp", B_COM_FIND,
		"enable user interface NRP response on/off",
		0, 0},
	{"enginenrp",	B_COM_FIND,
		"enable engine NRP response on/off",
		0, 0},
	{"forwarding",	B_COM_FIND,
		"enable engine midi message forwarding on/off",
		0, 0},
	{"tuning",	B_COM_FIND,
		"coarse/fine: global tuning 0..1.0",
		0, 0},
	{"controller",	B_COM_FIND,
		"send a value to a continuous controller value",
		0, 0},
	{"panic",	B_COM_FIND,
		"midi all-notes-off, etc",
		0, 0},
	{"", B_COM_LAST, "", 0, 0},
};

comSet cliComm[4] = {
	{"",		B_COM_NOT_USED, "", 0, 0},
	{"cycle",	B_COM_FIND,
		"CLI cycle response time (ms)",
		0, 0},
	{"list",	B_COM_FIND,
		"list the CLI ESC command key templates",
		0, 0},
	{"", B_COM_LAST, "", 0, 0},
};

comSet memcomm[8] = {
	/* Find/read/write/import/export should move to set memory */
	{"",		B_COM_NOT_USED, "", 0, 0},
	{"find",	B_COM_FIND,
		"'find', 'find free', 'find load' synth memory search",
		execLoad, 0},
	{"read",	B_COM_FIND,
		"[<n>|undo] load synth memory at the configured index",
		execLoad, 0},
	{"write",	B_COM_FIND,
		"[<n>] save current synth settings to memory",
		execLoad, 0},
	{"import",	B_COM_FIND,
		"[path [mem]] read a file and save to memory",
		execImport, 0},
	{"export",	B_COM_FIND,
		"[path [mem]] save memory to external file",
		execImport, 0},
	{"force",	B_COM_FIND,
		"force the loading of an otherwise erroneous memory, on/off",
		execLoad, 0},
	{"", B_COM_LAST, "", 0, 0},
};

/*
 * Need to build more structures to represent the parameters set of each
 * command, this is needed for better help and command completion.
static struct {
	char name[12];
	int map;
	char help[B_TTY_LINE_LEN];
	clicom exec;
	comSet *subcom;
} tmp;
 */

/* 'set <comm> and top level interpretation */
#define B_COM_SET		1
#define B_COM_MEMORY	2
#define B_COM_MIDI		3
#define B_COM_DEBUG		4
#define B_COM_BRISTOL	5
#define B_COM_BRIGHTON	6
#define B_COM_CLI		7
#define B_COM_ALIAS		8
#define B_COM_SETALIAS	9
#define B_COM_HELP		10
#define B_COM_QUIT		11
#define B_COM_PLAY		12

comSet setcomm[B_TTY_ACT_COUNT + 1] = {
	{"",		B_COM_NOT_USED, "", 0, 0},
	/* Find/read/write/import/export should move to set memory */
	{"memory",	B_COM_MEMORY,
		"'set memory [find|read|write|import|export]",
		execMemory, memcomm},
	{"midi",	B_COM_MIDI,
		"[channel|debug|help] midi control commands",
		execMidi, midicomm},
	{"debug",	B_COM_DEBUG,
		"[on|off|engine [0..15]] debug settings",
		execDebug, debugcomm},
	{"bristol",	B_COM_BRISTOL,
		"[cont/op/value|register] - operator commands",
		execBristol, bristolcom},
	{"brighton",B_COM_BRIGHTON,
		"[panel|p/d/v] - GUI management commands",
		execBrighton, brightoncom},
	{"cli",	    B_COM_CLI,
		"[list|<key> action|cycle] navigation key management",
		execCLI, cliComm},
	{"alias",	B_COM_SETALIAS,
		"command aliases, %/$ signs will be remapped parameters",
		execAlias, 0},
	{"play",	B_COM_PLAY,
		"set play mode|stop, set play sequence <id>, to test synth patches",
		execAlias, 0},
	{"noalias",	B_COM_SETALIAS,
		"noalias <name>: remove an alias called 'name'",
		execAlias, 0},
	{"line",	B_COM_NOT_USED,
		"command line length",
		0, 0},
	{"history",	B_COM_NOT_USED,
		"history table size (max 50)",
		0, 0},
	{"savehistory",	B_COM_NOT_USED,
		"autosave history and parameters on exit",
		0, 0},
	{"prompt",	B_COM_NOT_USED,
		"CLI Prompt",
		0, 0},
	{"accelerator",	B_COM_NOT_USED,
		"Default ESC mode Up/Down parameter change value",
		0, 0},
	{"awv",	B_COM_NOT_USED,
		"access withdrawn variables",
		0, 0},
	{"", B_COM_LAST, "", 0, 0},
};

comSet commands[B_TTY_ACT_COUNT + 1] = {
	{"",		B_COM_NOT_USED, "", 0, 0},
	/* Find/read/write/import/export should move to set memory */
	{"set",		B_COM_SET,
		"[line|panel|awv|cli|noalias|hist|prompt]",
		execSet, setcomm},
	{"memory",	B_COM_MEMORY,
		"'set memory [find|read|write|import|export]",
		execMemory, memcomm},
	{"midi",	B_COM_MIDI,
		"[channel|debug|help] midi control commands",
		execMidi, midicomm},
	{"debug",	B_COM_DEBUG,
		"[on|off|engine [0..15]] debug settings",
		execDebug, debugcomm},
	{"bristol",	B_COM_BRISTOL,
		"[cont/op/value|register] - operator commands",
		execBristol, bristolcom},
	{"brighton",B_COM_BRIGHTON,
		"[panel|p/d/v] - GUI management commands",
		execBrighton, brightoncom},
	{"cli",	    B_COM_CLI,
		"[list|<key> action|cycle] navigation key management",
		execCLI, cliComm},
	{"alias",	B_COM_SETALIAS,
		"command aliases, %/$ signs will be remapped parameters",
		execAlias, 0},
	{"help",	B_COM_HELP,
		"'set <command> help', 'help <command>, this screen",
		execHelp, 0},
	{"quit",	B_COM_QUIT,
		"exit application, terminate the emulator",
		execQuit, NULL},
	{"", B_COM_LAST, "", 0, 0},
};

#define B_TTY_A_PREDEF 20

static struct {
	char opname[12];
	int imap;
} templates[B_TTY_ACT_COUNT + 1] = {
	{"left",	B_TTY_A_LEFT},
	{"right",	B_TTY_A_RIGHT},
	{"incmin",	B_TTY_A_INCMIN},
	{"inc",		B_TTY_A_INC},
	{"incmax",	B_TTY_A_INCMAX},
	{"decmin",	B_TTY_A_DECMIN},
	{"dec",		B_TTY_A_DEC},
	{"decmax",	B_TTY_A_DECMAX},
	{"memUp",	B_TTY_A_M_UP},
	{"memDown",	B_TTY_A_M_DOWN},
	{"read",	B_TTY_A_M_READ},
	{"write",	B_TTY_A_M_WRITE},
	{"toggle",	B_TTY_A_M_TOG},
	{"insert",	B_TTY_A_INSERT},
	{"fineup",	B_TTY_A_INC4},
	{"up",		B_TTY_A_INC1},
	{"finedown",B_TTY_A_DEC4},
	{"down",	B_TTY_A_DEC1},
	{"update",	B_TTY_A_UPDATE},
	{"search",	B_TTY_A_FIND},
	{"off",		B_TTY_A_NULL},
	{"", -1},
};

static struct {
	char map;
	int imap;
} action[B_TTY_ACT_COUNT + 1] = {
	{'h', B_TTY_A_LEFT},
	{'l', B_TTY_A_RIGHT},
	{0x0b, B_TTY_A_INCMIN},
	{'k', B_TTY_A_INC},
	{'K', B_TTY_A_INCMAX},
	{0x0a, B_TTY_A_DECMIN},
	{'j', B_TTY_A_DEC},
	{'J', B_TTY_A_DECMAX},
	{'M', B_TTY_A_M_UP},
	{'m', B_TTY_A_M_DOWN},
	{'r', B_TTY_A_M_READ},
	{'w', B_TTY_A_M_WRITE},
	{'x', B_TTY_A_M_TOG},
	{'u', B_TTY_A_INC1},
	{'d', B_TTY_A_DEC1},
	{'f', B_TTY_A_UPDATE},
	{'U', B_TTY_A_INC4},
	{'D', B_TTY_A_DEC4},
	{':', B_TTY_A_INSERT},
	{'/', B_TTY_A_FIND},
	{'\0', -1},
};

static struct {
	unsigned int flags;
	guimain *global;
	int cycle;
	int i;
	int p;
	char promptText[64];
	char prompt[64];
	char *hist[51];
	int hist_tmp;
	int hist_c;
	int edit_p;
	char b;
	char delim;
	int len;
	int fd[2];
	int lastmem;
	char ichan;
	char mdbg;
	char edbg;
	char lowkey;
	char highkey;
	char preference;
	float accel;
	int fl;
	int po;
	unsigned int key;
	int pcycle;
	int acycle;
	struct {
		int id;
		int step;
		int count;
		int transpose;
		unsigned int flags;
	} sequence;
} btty = {
	B_TTY_INIT|B_TTY_COOKED|B_TTY_PLAY_LINE,
	NULL,
	500,
	0,
	0,
	"%algo% (m %memory% ch %channel%): ",
	"%algo%: ",
	{hist[0], hist[1], hist[2], hist[3], hist[4],
	hist[5], hist[6], hist[7], hist[8], hist[9], 
	hist[10], hist[11], hist[12], hist[13], hist[14],
	hist[15], hist[16], hist[17], hist[18], hist[19], 
	hist[20], hist[21], hist[22], hist[23], hist[24],
	hist[25], hist[26], hist[27], hist[28], hist[29], 
	hist[30], hist[31], hist[32], hist[33], hist[34],
	hist[35], hist[36], hist[37], hist[38], hist[39], 
	hist[40], hist[41], hist[42], hist[43], hist[44],
	hist[45], hist[46], hist[47], hist[48], hist[49], hist[50]}, 
	0, 50, 0,
	'\0',
	':',
	80,
	{0, 0},
	0,
	/* These all default to zero, the MIDI options */
	0, 0, 0, 0, 127, 0,
	/* Default Cursor key accelerator */
	0.05f,
	0,
	0,
	/* Keystroke management */
	0, 500, 500
};

static void
print(char *str)
{
	int i;
	char tbuf[B_TTY_LINE_LEN];

	if (btty.flags & B_TTY_DEBUG)
	{
		snprintf(tbuf, btty.len, "d: %s\r\n", str);
		i = write(btty.fd[1], tbuf, strlen(tbuf));
	}
}

static void
printChar(char ch)
{
	int i;

	if (btty.flags & B_TTY_DEBUG)
	{
		snprintf(pbuf, btty.len, "char 0x%x\r\n", ch);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
	}
}

static void
seqStop(guimain *global)
{
	print("seqStop");

	if (btty.flags & B_TTY_SEQ)
	{
		bristolMidiSendMsg(global->controlfd, SYNTHS->midichannel,
			BRISTOL_EVENT_KEYOFF, 0,
			sequences[btty.sequence.id].step[btty.sequence.step]
			+ btty.sequence.transpose);
		bristolMidiSendMsg(global->controlfd, SYNTHS->sid, 127, 0,
			BRISTOL_ALL_NOTES_OFF);
	}

	btty.flags &= ~B_TTY_SEQ;
	btty.sequence.step = 0;
}

static void
bttyManageHistory(char *comm)
{
	if (btty.flags & B_TTY_ALIASING)
		return;
	if ((strcmp(comm, btty.hist[1]) != 0) && (comm[0] != '\0'))
	{
		int i;
		char *curr = btty.hist[50];

		snprintf(btty.hist[0], btty.len, "%s", comm);
		/* different history, cycle them and clean current */
		for (i = 50; i > 0; i--)
			btty.hist[i] = btty.hist[i - 1];
		btty.hist[0] = curr;
		memset(btty.hist[0], 0, B_TTY_LINE_LEN);
	}
	btty.hist_tmp = 0;
}

void
brightonCLIcleanup()
{
	if (btty.flags & B_TTY_SAVE_HIST)
	{
		btty.SYNTHS->location = 9999;
		bttyMemSave(btty.global);
	}
	tcsetattr(btty.fd[0], TCSANOW, &resetattr);
}

void
brightonGetCLIcodes(int fd)
{
	int i, n;

	snprintf(pbuf, btty.len, "CLI: set history %i\n", btty.hist_c);
	n = write(fd, pbuf, strlen(pbuf));

	snprintf(pbuf, btty.len, "CLI: set line %i\n", btty.len);
	n = write(fd, pbuf, strlen(pbuf));

	snprintf(pbuf, btty.len, "CLI: set accel %f\n", btty.accel);
	n = write(fd, pbuf, strlen(pbuf));

	snprintf(pbuf, btty.len, "CLI: set prompttext \"%s\"\n", btty.promptText);
	n = write(fd, pbuf, strlen(pbuf));

	snprintf(pbuf, btty.len, "CLI: set cli cycle %i\n", btty.cycle);
	n = write(fd, pbuf, strlen(pbuf));

	if (btty.flags & B_TTY_DEBUG)
	{
		snprintf(pbuf, btty.len, "CLI: debug on\n");
		n = write(fd, pbuf, strlen(pbuf));
	}

	snprintf(pbuf, btty.len, "CLI: set panel %i\n", btty.p);
	n = write(fd, pbuf, strlen(pbuf));

	for (i = 0; commands[i].map != -1; i++)
	{
		if (commands[i].map == B_COM_ALIAS)
		{
			snprintf(pbuf, btty.len, "CLI: set alias %s %s\n",
				commands[i].name, commands[i].help);
			n = write(fd, pbuf, strlen(pbuf));
		}
	}

	for (i = 0; i < B_TTY_ACT_COUNT; i++)
	{
		if ((action[i].imap < 0) || (action[i].map == '\0'))
			continue;

		if (action[i].map < 26)
		{
			snprintf(pbuf, btty.len,
				"CLI: set cli ^X %s\n", templates[action[i].imap].opname);
			pbuf[14] = action[i].map + 96;
		} else {
			snprintf(pbuf, btty.len,
				"CLI: set cli X %s\n", templates[action[i].imap].opname);
			pbuf[13] = action[i].map;
		}

		n = write(fd, pbuf, strlen(pbuf));
	}

	if (btty.flags & B_TTY_SAVE_HIST)
	{
		snprintf(pbuf, btty.len, "CLI: set savehistory on\n");
		n = write(fd, pbuf, strlen(pbuf));

		for (i = 50; i >= 0; i--)
		{
			if (btty.hist[i][0] == '\0')
				continue;
			snprintf(pbuf, btty.len, "CLI: set history comm %s\n",
				btty.hist[i]);
			n = write(fd, pbuf, strlen(pbuf));
		}
	}

}

void
brightonSetCLIcode(char *input)
{
	int map, i, stuff = 0;
	int act;
	char c = input[0];
	char *comm = &input[2];

	if (btty.global == NULL)
		return;

	print("SetCLIcode");

	if ((input[strlen(input) -1] == '\n')
		|| (input[strlen(input) -1] == '\r'))
		input[strlen(input) -1] = '\0';

	if (strncmp("set", input, 3) == 0)
	{
		snprintf(cbuf, btty.len, "%s", input);
		bttyInterpret(btty.global, cbuf);
		return;
	}

	if (c == '^')
	{
		c = input[1] - 96;
		comm++;
	}

	print(comm);

	for (act = 0; act < B_TTY_ACT_COUNT; act++)
	{
		/* return if we cannot find the action */
		if (templates[act].imap == -1)
			break;
		if (templates[act].opname[0] == '\0')
			break;
		if (strcasecmp(comm, templates[act].opname) == 0)
		{
			stuff = act;
			break;
		}
	}
	if ((stuff) || (templates[act].imap == -1))
	{
		int i;

		/*
		 * Search the ommmand table, add entry to templates
		 */
		for (i = 0 ;
			(commands[i].map != B_COM_LAST) && (i < B_TTY_ACT_COUNT);
			i++)
		{
			if (strcasecmp(comm, commands[i].name) == 0)
			{
				/*
				 * Found a command.
				 */
				if (templates[act].imap == -1)
				{
					templates[act + 1].imap = -1;
					templates[act + 1].opname[0] = '\0';
				}
				templates[act].imap = act;
				snprintf(templates[act].opname, 12, "%s", comm);
				break;
			}
		}
	}

	print(templates[act].opname);

	if (act >= B_TTY_ACT_COUNT)
		return;

	act = templates[act].imap;

	for (map = 0; map < B_TTY_ACT_COUNT; map++)
	{
		if (action[map].map == '\0')
			break;
		if (action[map].map == c)
			break;
	}
	if (map == B_TTY_ACT_COUNT)
		return;

	if (action[map].map == '\0')
	{
		action[map + 1].map = '\0';
		action[map + 1].imap = B_COM_LAST;
	}

	action[map].map = c;
	action[map].imap = act;

	if (btty.flags & B_TTY_DEBUG)
	{
		if (input[0] == '^')
		{
			snprintf(pbuf, btty.len, "^X is %s\n\r", templates[act].opname);
			pbuf[1] = c + 96;
		} else {
			snprintf(pbuf, btty.len, "X is %s %i %i\n\r", templates[act].opname,
				map, action[map].imap);
			pbuf[0] = action[map].map;
		}
		i = write(btty.fd[1], pbuf, strlen(pbuf));
	}
}

static int
execHelpCheck(int c, char **v)
{
	int i, j, n;

	if ((c == 2) && (strncmp("help", v[1], strlen(v[1])) == 0))
	{
		for (i = 0; commands[i].map != B_COM_LAST; i++)
		{
			if (strncmp(commands[i].name, v[0], strlen(v[0])) == 0)
			{
				if (commands[i].subcom == 0)
				{
					snprintf(pbuf, B_TTY_LINE_LEN, "%12s: %s\n\r",
						commands[i].name, commands[i].help);
					n = write(btty.fd[1], pbuf, strlen(pbuf));
					return(1);
				} else {
					for (j = 0; commands[i].subcom[j].map != B_COM_LAST; j++)
					{
						if ((commands[i].subcom[j].name[0] == '\0')
							|| (commands[i].subcom[j].help[0] == '\0'))
							continue;
						snprintf(pbuf, B_TTY_LINE_LEN, "%13s: %s\n\r",
							commands[i].subcom[j].name,
							commands[i].subcom[j].help);
						n = write(btty.fd[1], pbuf, strlen(pbuf));
					}
					return(1);
				}
			}
		}
	}

	return(0);
}

static int
isparam(int set, int comm, char *name)
{
	if (strncmp(commands[set].subcom[comm].name, name, strlen(name)) == 0)
		return(1);
	return(0);
}

static void
bttyMemSave(guimain *global)
{
	int i;

	SYNTHS->cmem = SYNTHS->lmem;
	if (SYNTHS->saveMemory != NULL)
		SYNTHS->saveMemory(SYNTHS,
			RESOURCES->name, 0,
			SYNTHS->location, 0);
	else
		saveMemory(SYNTHS, RESOURCES->name,
			0, SYNTHS->location, 0);
	if (btty.flags & B_TTY_DEBUG)
	{
		snprintf(pbuf, btty.len, "write: %i\n\r", SYNTHS->location);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
	}
}

static void
bttyMemLoad(guimain *global)
{
	int i;

	SYNTHS->lmem = SYNTHS->cmem;
	if (SYNTHS->loadMemory != NULL)
		SYNTHS->loadMemory(SYNTHS,
			RESOURCES->name,
			0, SYNTHS->location, SYNTHS->mem.active,
			0, btty.fl);
	else
		loadMemory(SYNTHS, RESOURCES->name,
			0, SYNTHS->location, SYNTHS->mem.active,
			0, btty.fl);
	SYNTHS->cmem = SYNTHS->location;

	if (SYNTHS->location != 9999)
	{
		snprintf(pbuf, btty.len, "read: %i\r\n", SYNTHS->location);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
	} else {
		snprintf(pbuf, btty.len, "read: unread\r\n");
		i = write(btty.fd[1], pbuf, strlen(pbuf));
	}
}

static int
execMemory(guimain *global, int c, char **argv)
{
	int n;

	if (c == 1)
		return(B_ERR_PARAM);

	if (execHelpCheck(c, argv)) return(0);

	if (c == 2)
	{
		/* <n> find, read, write */
		if (isparam(B_COM_MEMORY, B_COM_FIND, argv[1]))
		{
			int location = SYNTHS->location + 1;
			/* find mem */
			while (loadMemory(SYNTHS, RESOURCES->name, 0,
				location, SYNTHS->mem.active, 0, BRISTOL_STAT) != 0)
			{
				if (++location >= 1024)
					location = 0;
				if (location == SYNTHS->location)
					break;
			}
			if (location != SYNTHS->location)
			{
				SYNTHS->location = location;
				snprintf(pbuf, btty.len, "mem: %i\r\n", SYNTHS->location);
				n = write(btty.fd[1], pbuf, strlen(pbuf));
			} else {
				snprintf(pbuf, btty.len, "no memories\r\n");
				n = write(btty.fd[1], pbuf, strlen(pbuf));
			}
			return(B_ERR_OK);
		}
		if (isparam(B_COM_MEMORY, B_COM_READ, argv[1]))
		{
			bttyMemLoad(global);
			return(B_ERR_OK);
		}
		if (isparam(B_COM_MEMORY, B_COM_WRITE, argv[1]))
		{
			bttyMemSave(global);
			return(B_ERR_OK);
		}
		if (isdigit(argv[1][0]))
		{
			if ((n = atoi(argv[1])) < 0)
				return(B_ERR_VALUE);
			if (n > 1023)
				return(B_ERR_VALUE);

			SYNTHS->location = n;
			return(B_ERR_OK);
		}
		return(B_ERR_PARAM);
	}

	if (c == 3)
	{
		/* find free, find read, read undo, read n, write n */ 
		if (isparam(B_COM_MEMORY, B_COM_FIND, argv[1]))
		{
			if (strncmp("free", argv[2], strlen(argv[2])) == 0)
			{
				int location = SYNTHS->location + 1;
				/* find mem */
				while (loadMemory(SYNTHS, RESOURCES->name, 0,
					location, SYNTHS->mem.active, 0, BRISTOL_STAT) == 0)
				{
					if (++location >= 1024)
						location = 0;
					if (location == SYNTHS->location)
						break;
				}
				if (location != SYNTHS->location)
				{
					SYNTHS->location = location;
					snprintf(pbuf, btty.len, "mem: %i\r\n", SYNTHS->location);
					n = write(btty.fd[1], pbuf, strlen(pbuf));
				} else {
					snprintf(pbuf, btty.len, "no memories\r\n");
					n = write(btty.fd[1], pbuf, strlen(pbuf));
					return(B_ERR_VALUE);
				}
				return(B_ERR_OK);
			}
			if ((strncmp("load", argv[2], strlen(argv[2])) == 0)
				||(strncmp("read", argv[2], strlen(argv[2])) == 0))
			{
				int location = SYNTHS->location + 1;
				/* find mem */
				while (loadMemory(SYNTHS, RESOURCES->name, 0,
					location, SYNTHS->mem.active, 0, BRISTOL_STAT) != 0)
				{
					if (++location >= 1024)
						location = 0;
					if (location == SYNTHS->location)
						break;
				}
				if (location != SYNTHS->location)
				{
					SYNTHS->location = location;
					bttyMemLoad(global);
					return(B_ERR_OK);
				} else {
					snprintf(pbuf, btty.len, "no memories\r\n");
					n = write(btty.fd[1], pbuf, strlen(pbuf));
					return(B_ERR_VALUE);
				}
			}
		} else if (isparam(B_COM_MEMORY, B_COM_READ, argv[1])) {
			if (strncmp("undo", argv[2], strlen(argv[2])) == 0)
			{
				n = SYNTHS->location;
				SYNTHS->location = 9999;
				bttyMemLoad(global);
				SYNTHS->location = n;
				return(B_ERR_OK);
			}

			if ((n = atoi(argv[2])) < 0)
				return(B_ERR_VALUE);
			if (n > 1023)
				return(B_ERR_VALUE);

			SYNTHS->location = n;
			bttyMemLoad(global);
			return(B_ERR_OK);
		} else if (isparam(B_COM_MEMORY, B_COM_FORCE, argv[1])) {
			if (strcmp(argv[2], "on") == 0)
			{
				btty.fl = BRISTOL_FORCE;
				return(B_ERR_OK);
			} else if (strcmp(argv[2], "off") == 0) {
				btty.fl = 0;
				return(B_ERR_OK);
			}
			return(B_ERR_PARAM);
		} else if (isparam(B_COM_MEMORY, B_COM_WRITE, argv[1])) {
			if ((n = atoi(argv[2])) < 0)
				return(B_ERR_VALUE);
			if (n > 1023)
				return(B_ERR_VALUE);

			SYNTHS->location = n;
			bttyMemSave(global);
			return(B_ERR_OK);
		}
		return(B_ERR_PARAM);
	}

	if (c == 4)
	{
		int loc;

		/* import, export */
		if (isparam(B_COM_MEMORY, B_COM_IMPORT, argv[1]))
		{
			loc = atoi(argv[3]);
			bristolMemoryImport(loc, argv[2], RESOURCES->name);
			return(B_ERR_OK);
		} else if (isparam(B_COM_MEMORY, B_COM_EXPORT, argv[1])) {
			loc = atoi(argv[2]);
			bristolMemoryExport(loc, argv[1], RESOURCES->name);
			return(B_ERR_OK);
		}
		return(B_ERR_PARAM);
	}

	return(B_ERR_PARAM);
}

static int
execCLI(guimain *global, int c, char **argv)
{
	int n;

	if (c == 1)
	{
		int i;

		snprintf(pbuf, btty.len, "cycle:                      %i\r\n", btty.cycle);
		n = write(btty.fd[1], pbuf, strlen(pbuf));

		snprintf(pbuf, btty.len, "CLI <ESC> mode navigation keys:\n\r");
		n = write(btty.fd[1], pbuf, strlen(pbuf));

		for (i = 0; action[i].imap != -1; i++)
		{
			snprintf(pbuf, btty.len, "%2i    : %s\n\r",
				i,
				templates[action[i].imap].opname);
			if (action[i].map <= 27)
			{
				pbuf[3] = '^';
				pbuf[4] = action[i].map + 96;
			} else
				pbuf[4] = action[i].map;
			n = write(btty.fd[1], pbuf, strlen(pbuf));
		}
		return(0);
	}

	if (c == 2)
	{
		if (strncmp("cycle", argv[1], strlen(argv[1])) == 0) {
			snprintf(pbuf, btty.len, "cycle:                      %i\r\n",
				btty.cycle);
			n = write(btty.fd[1], pbuf, strlen(pbuf));
			return(0);
		} else if (strncmp("listcommands", argv[1], strlen(argv[1])) == 0) {
			int i;

			for (i = 0; templates[i].imap != -1; i++)
			{
				snprintf(pbuf, btty.len, "    %s\n\r", templates[i].opname);
				n = write(btty.fd[1], pbuf, strlen(pbuf));
			}
			return(0);
		}
	}

	if (c == 3)
	{
		if (strncmp("cycle", argv[1], strlen(argv[1])) == 0) {
			if ((btty.cycle = atoi(argv[2])) < 50)
				btty.cycle = 50;
			else if (btty.cycle > 1000)
				btty.cycle = 1000;
			snprintf(pbuf, btty.len, "cycle:                      %i\r\n",
				btty.cycle);
			n = write(btty.fd[1], pbuf, strlen(pbuf));
		} else {
			char comm[B_TTY_LINE_LEN];
			snprintf(comm, B_TTY_LINE_LEN, "%s %s\n", argv[1], argv[2]);
print(comm);
			brightonSetCLIcode(comm);
		}
		return(0);
	}
	return(2);
}

static int
execAlias(guimain *global, int c, char **argv)
{
	char *comm, alias[B_TTY_LINE_LEN];
	int i;

	print("execAlias");

	if ((c == 1) && (strncmp(argv[0], "alias", strlen(argv[0])) == 0))
	{
		int n;
		for (i = 0; commands[i].map != B_COM_LAST; i++)
		{
			if (commands[i].map == B_COM_ALIAS)
			{
				snprintf(pbuf, B_TTY_LINE_LEN, "%12s: %s\n\r",
					commands[i].name, commands[i].help);
				n = write(btty.fd[1], pbuf, strlen(pbuf));
			}
		}
		return(0);
	}

	/*
	 * Two calls, one with the actual 'alias' token, if not we should search
	 * the alias list.
	 */
	if (strncmp("alias", argv[0], strlen(argv[0])) == 0)
	{
		if (c < 3)
			return(2);
		/*
		 * the alias is in our cbuf, we just have to find it.
		 */
		if (cbuf[0] == 's')
		{
			if ((comm = index(cbuf, ' ')) == NULL)
				return(3);
			comm++;
			if ((comm = index(comm, ' ')) == NULL)
				return(3);
			comm++;
			if ((comm = index(comm, ' ')) == NULL)
				return(3);
			comm++;
			if (comm[0] == '\0')
				return(3);
			snprintf(alias, btty.len, "%s", comm);
		} else if (cbuf[0] == 'a') {
			if ((comm = index(cbuf, ' ')) == NULL)
				return(3);
			comm++;
			if ((comm = index(comm, ' ')) == NULL)
				return(3);
			comm++;
			if (comm[0] == '\0')
				return(3);
			snprintf(alias, btty.len, "%s", comm);
		} else
			return(2);

		comm = argv[1];
		if (strncmp(comm, alias, strlen(comm)) == 0)
		{
			print("cannot apply alias: recursive");
			return(2);
		}

		/* Find free command */
		for (i = 0; commands[i].map != -1; i++)
		{
			if (i == (B_TTY_ACT_COUNT - 1))
			{
				snprintf(pbuf, btty.len, "command table full\r\n");
				i = write(btty.fd[1], pbuf, strlen(pbuf));
				return(4);
			}

			if (strcmp(commands[i].name, comm) == 0)
			{
				if (commands[i].map != B_COM_ALIAS)
				{
					print("cannot apply alias: name in use");
					return(2);
				}
				print("remapping alias");
				snprintf(commands[i].name, 11, "%s", comm);
				snprintf(commands[i].help, B_TTY_LINE_LEN, "%s", alias);
				commands[i].exec = execAlias;
				return(0);
			}
			if (commands[i].map == B_COM_FREE)
				break;
		}

		print(comm);
		print(alias);

		/* Flag next entry as last */
		if (commands[i].map != B_COM_FREE)
		{
			commands[i + 1].name[0] = '\0';
			commands[i + 1].map = B_COM_LAST;
		}

		snprintf(commands[i].name, 11, "%s", comm);
		snprintf(commands[i].help, B_TTY_LINE_LEN, "%s", alias);
		commands[i].map = B_COM_ALIAS;
		commands[i].exec = execAlias;
		return(0);
	} else {
		int j = 1, k = 0;

		print("searchAlias");

		/*
		 * Build a new command line and exec it. Find the alias, take it's
		 * help line, copy over byte by byte to cbuf, replace each % with
		 * increasing argv (if we have enough)
		 */
		for (i = 0; commands[i].map != -1; i++)
		{
			if (strncmp(argv[0], commands[i].name, strlen(argv[0])) == 0)
				break;
		}
		if (commands[i].map != B_COM_ALIAS)
			return(B_ERR_PARAM);
		comm = commands[i].help;

		print(comm);

		btty.flags |= B_TTY_ALIASING;
		for (i = 0; comm[i] != '\0'; i++)
		{
			if  (comm[i] == '$')
			{
				int l, m;

				i++;

				if (((m = atoi(&comm[i])) < 1) || (m >= c))
				{
					print("alias failed: too few arguments");
					btty.flags &= ~B_TTY_ALIASING;
					return(B_ERR_INV_ARG);
				}
				for (l = 0; argv[m][l] != '\0'; l++)
					alias[k++] = argv[m][l];
			}
			if (comm[i] == '%')
			{
				int l;

				if (j == c)
				{
					print("alias failed: too few arguments");
					btty.flags &= ~B_TTY_ALIASING;
					return(B_ERR_INV_ARG);
				}
				for (l = 0; argv[j][l] != '\0'; l++)
					alias[k++] = argv[j][l];
				if (comm[i] == '%')
					j++;
				i++;
			}
			if (comm[i] == ';')
			{
				alias[k] = '\0';
				print(alias);
				bttyInterpret(global, alias);
				memset(alias, 0, B_TTY_LINE_LEN);
				k = 0;
			} else
				alias[k++] = comm[i];
		}
		alias[k++] = '\0';
		print(alias);
		bttyInterpret(global, alias);
		btty.flags &= ~B_TTY_ALIASING;
		return(0);
	}
	return(2);
}

/*
 * controller c o v - very expert usage.
 */
static int
execBristol(guimain *global, int c, char **argv)
{
	int o, p;
	float v;

	print("execBristol");

	if (execHelpCheck(c, argv)) return(0);

	if (c == 2)
	{
		if (strncmp(bristolcom[1].name, argv[1], strlen(argv[1])) == 0)
		{
			snprintf(pbuf, btty.len, "register request %i %p\n\r",
				btty.i, DEVICE(btty.i).dev);
			o = write(btty.fd[1], pbuf, strlen(pbuf));
			brightonRegisterController((brightonDevice *)
				DEVICE(btty.i).dev);
			return(0);
		}
	}

	if (c == 5)
	{
		if (strncmp(bristolcom[2].name, argv[1], strlen(argv[1])) == 0)
		{
			int sid;

			if ((o = atoi(argv[2])) < 0) return(3); if (o > 127) return(3);
			if ((p = atoi(argv[3])) < 0) return(3); if (p > 127) return(3);
			if ((v = atof(argv[4])) < 0.0f) return(3); if (v > 1.0f) return(3); 

			if (btty.ichan == 0)
				sid = SYNTHS->sid;
			else
				sid = SYNTHS->sid2;
			bristolMidiSendMsg(global->controlfd, btty.ichan,
				o, p, (int) (v * C_RANGE_MIN_1));
			return(0);
		}
	}

	return(B_ERR_PARAM);
}

extern int bristolPressureEvent(int, int, int, int);
extern int bristolPolyPressureEvent(int, int, int, int, int);

/*
 * This is probably going to become a large part of the interface, it can give
 * access to loads of stuff that many of the GUI may not use because there was
 * no other way to control them.
 *
 * midi channel 1..16
 * midi debug 0..3
 * midi lowkey|highkey 0..127
 * midi filters lwf|nwf|wwf|hwf
 * midi notepref hnp|lnp|nnp
 * midi velocity 1..1000
 * midi chanpressure 0..127
 * midi polypressure 0..127 0..127
 * midi detune 0..500
 * midi glide 0..30
 * midi legato on/off
 * midi trig on/off
 * midi gain 1..
 * midi pwd 0..
 * midi nrp on/off
 * midi forwarding on/off
 * midi tuning fine 0..1.0
 * midi tuning coarse 0..1.0
 * midi panic
 */
static int
execMidi(guimain *global, int c, char **v)
{
	int i, n, k;

	if (execHelpCheck(c, v)) return(0);
	//if (midiHelpCheck(c, v)) return(0);

	for (i = 0; i < c; i++)
		print(v[i]);

	if (c == 1)
	{
		snprintf(pbuf, btty.len, "MIDI channel:               %i\r\n",
			SYNTHS->midichannel+1);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		snprintf(pbuf, btty.len, "SID channel:                %i\r\n",
			btty.ichan);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		snprintf(pbuf, btty.len, "MIDI debug:                 %i\r\n",
			btty.mdbg);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		snprintf(pbuf, btty.len, "MIDI lowkey                 %i\r\n",
			btty.lowkey);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		snprintf(pbuf, btty.len, "MIDI highkey:               %i\r\n",
			btty.highkey);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		return(B_ERR_OK);
	}

	if (c == 4)
	{
		if (strncmp("tuning", v[1], strlen(v[1])) == 0)
		{
			float tune = 0.5;
			if (strncmp("fine", v[2], strlen(v[2])) == 0)
			{
				i = MIDI_RP_FINETUNE;

				if ((tune = atof(v[3])) < 0.0f) return(3);
				if (tune > 1.0f) return(3); 
			} else if (strncmp("coarse", v[2], strlen(v[3])) == 0) {
				i = MIDI_RP_COARSETUNE;

				if ((tune = atof(v[3])) < 0.0f) return(3);
				if (tune > 1.0f) return(3); 
			}
			tune *= C_RANGE_MIN_1;
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel, i, tune);
			return(B_ERR_OK);
		}
		if (strncmp("polypressure", v[1], strlen(v[1])) == 0)
		{
			int k, p;

			if ((k = atoi(v[2])) < 0) return(3); if (k > 127) return(3); 
			if ((p = atoi(v[3])) < 0) return(3); if (p > 127) return(3); 

			bristolPolyPressureEvent(global->controlfd, 0, SYNTHS->midichannel,
				k, p);

			return(B_ERR_OK);
		}
		if (strncmp("controller", v[1], strlen(v[1])) == 0)
		{
			int id, value;

			if ((id = atoi(v[2])) < 0) return(B_ERR_VALUE);
			if (id > 127) return(B_ERR_VALUE);
			if ((value = atoi(v[3])) < 0) return(B_ERR_VALUE);
			if (value > 127) return(B_ERR_VALUE);
			bristolMidiSendControlMsg(global->controlfd,
				SYNTHS->midichannel, id, value);
			return(B_ERR_OK);
		}
		return(B_ERR_PARAM);
	}

	if (c == 2)
	{
		if (strncmp(v[1], "panic", strlen(v[1])) == 0) {
			bristolMidiSendMsg(global->controlfd, SYNTHS->sid2, 127, 0,
				BRISTOL_ALL_NOTES_OFF);
			bristolMidiSendMsg(global->controlfd, SYNTHS->sid, 127, 0,
				BRISTOL_ALL_NOTES_OFF);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "sid", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "SID channel :               %i\r\n",
				btty.ichan);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "channel", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "MIDI channel:               %i\r\n",
				SYNTHS->midichannel+1);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "debug", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "MIDI debug:                 %i\r\n",
				btty.mdbg);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		}
		return(B_ERR_PARAM);
	}

	if (c == 3)
	{
		if (strncmp(v[1], "sid", strlen(v[1])) == 0) {
			if ((n = atoi(v[2])) < 1)
				n = 0;
			else
				n = 1;
			btty.ichan = n;
			snprintf(pbuf, btty.len, "SID channel:                %i\r\n", btty.ichan);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "charpressure", strlen(v[1])) == 0) {
			if ((n = atoi(v[2])) < 0) return(B_ERR_VALUE);
			if (n > 127) return(B_ERR_VALUE);

			bristolPressureEvent(global->controlfd, 0, SYNTHS->midichannel, n);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "channel", strlen(v[1])) == 0) {
			if ((n = atoi(v[2])) < 1)
				return(3);
			else if (n > 16)
				return(3);
			SYNTHS->midichannel = n - 1;
			snprintf(pbuf, btty.len, "MIDI channel:               %i\r\n",
				SYNTHS->midichannel+1);
			i = write(btty.fd[1], pbuf, strlen(pbuf));

			SYNTHS->midichannel = SYNTHS->midichannel;
			if (global->libtest == 0)
				bristolMidiSendMsg(global->controlfd, SYNTHS->sid,
		            127, 0, BRISTOL_MIDICHANNEL|SYNTHS->midichannel);
			if ((k = atoi(v[2])) < 0) return(3); if (k > 127) return(3);

			SYNTHS->highkey = btty.highkey = k;

			bristolMidiSendMsg(global->controlfd, SYNTHS->sid, 127, 0,
				BRISTOL_HIGHKEY|SYNTHS->highkey);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "lowkey", strlen(v[1])) == 0) {
			if ((k = atoi(v[2])) < 0) return(3); if (k > 127) return(3);

			SYNTHS->lowkey = btty.lowkey = k;

			bristolMidiSendMsg(global->controlfd, SYNTHS->sid, 127, 0,
				BRISTOL_LOWKEY|SYNTHS->lowkey);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "filter", strlen(v[1])) == 0) {
			k = 0;
			if (strcmp(v[2], "lwf") == 0) k = 1;
			else if (strcmp(v[2], "nwf") == 0) k = 0;
			else if (strcmp(v[2], "wwf") == 0) k = 2;
			else if (strcmp(v[2], "hwf") == 0) k = 3;
			else return(B_ERR_INV_ARG);
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_LWF, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "detune", strlen(v[1])) == 0) {
			if ((k = atoi(v[2])) < 0) return(3); if (k > 16383) return(3);
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_DETUNE, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "velocity", strlen(v[1])) == 0) {
			if ((k = atoi(v[2])) < 0) return(3); if (k > 16383) return(3);
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_VELOCITY, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "glide", strlen(v[1])) == 0) {
			if ((k = atoi(v[2])) < 0) return(3); if (k > 16383) return(3);
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_GLIDE, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "pwd", strlen(v[1])) == 0) {
			if ((k = atoi(v[2])) < 0) return(3); if (k > 16383) return(3);
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				MIDI_RP_PW, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "gain", strlen(v[1])) == 0) {
			if ((k = atoi(v[2])) < 0) return(3); if (k > 16383) return(3);
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_GAIN, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "forwarding", strlen(v[1])) == 0) {
			k = 1;
			if (strcmp(v[2], "on") == 0) k = 1;
			if (strcmp(v[2], "off") == 0) k = 0;
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_FORWARD, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "nrp", strlen(v[1])) == 0) {
			if (strcmp(v[2], "on") == 0)
				SYNTHS->flags |= GUI_NRP;
			else if (strcmp(v[2], "off") == 0)
				SYNTHS->flags &= ~GUI_NRP;
			return(B_ERR_OK);
		} else if (strncmp(v[1], "enginenrp", strlen(v[1])) == 0) {
			k = 0;
			if (strcmp(v[2], "on") == 0) k = 1;
			if (strcmp(v[2], "off") == 0) k = 0;
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_ENABLE_NRP, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "legato", strlen(v[1])) == 0) {
			k = 1;
			if (strcmp(v[2], "on") == 0) k = 1;
			if (strcmp(v[2], "off") == 0) k = 0;
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_MNL_VELOC, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "trigger", strlen(v[1])) == 0) {
			k = 1;
			if (strcmp(v[2], "on") == 0) k = 1;
			if (strcmp(v[2], "off") == 0) k = 0;
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_MNL_TRIG, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "precedence", strlen(v[1])) == 0) {
			k = BRIGHTON_LNP;
			if (strcmp(v[2], "hnp") == 0) k = BRIGHTON_HNP;
			if (strcmp(v[2], "lnp") == 0) k = BRIGHTON_LNP;
			if (strcmp(v[2], "nnp") == 0) k = BRIGHTON_POLYPHONIC;
			bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
				BRISTOL_NRP_MNL_PREF, k);
			return(B_ERR_OK);
		} else if (strncmp(v[1], "debug", strlen(v[1])) == 0) {
			n = atoi(v[2]);

			switch (n) {
				default:
				case 0:
					n = 0;
					break;
				case 1:
					break;
				case 2:
					break;
				case 3:
					break;
			}
			btty.mdbg = n;
			if (global->libtest == 0)
				bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
		            BRISTOL_NRP_DEBUG, n);
			snprintf(pbuf, btty.len, "MIDI debug:                   %i\r\n",
				btty.mdbg);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		}
		return(B_ERR_PARAM);
	}

	return(B_ERR_PARAM);
}

/*
 * Set line width. Set debug on/off.
 */
static int
execSet(guimain *global, int c, char **v)
{
	int i, n;

	if (execHelpCheck(c, v)) return(0);

	for (i = 0; i < c; i++)
		print(v[i]);

	if (c > 1)
	{
		if (strncmp("memory", v[1], strlen(v[1])) == 0)
			return(execMemory(global, c - 1, v += 1));
		if (strncmp("midi", v[1], strlen(v[1])) == 0)
			return(execMidi(global, c - 1, v += 1));
		if (strncmp("debug", v[1], strlen(v[1])) == 0)
			return(execDebug(global, c - 1, v += 1));
		if (strncmp("bristol", v[1], strlen(v[1])) == 0)
			return(execBristol(global, c - 1, v += 1));
		if (strncmp("control", v[1], strlen(v[1])) == 0)
			return(execBristol(global, c - 1, v += 1));
		if (strncmp("brighton", v[1], strlen(v[1])) == 0)
			return(execBrighton(global, c - 1, v += 1));
		if (strncmp("alias", v[1], strlen(v[1])) == 0)
			return(execAlias(global, c - 1, v += 1));
		if (strncmp("cli", v[1], strlen(v[1])) == 0)
			return(execCLI(global, c - 1, v += 1));
	}


	if (c == 1)
	{
		snprintf(pbuf, btty.len, "line width:                 %i\r\n",
			btty.len);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		snprintf(pbuf, btty.len, "history:                    %i\r\n",
			btty.hist_c);
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		if (btty.flags & B_TTY_SAVE_HIST)
			snprintf(pbuf, btty.len, "savehistory:                on\r\n");
		else
			snprintf(pbuf, btty.len, "savehistory:                off\r\n");
		i = write(btty.fd[1], pbuf, strlen(pbuf));

		snprintf(pbuf, btty.len, "prompt:                     %s\r\n",
			btty.prompt);
		i = write(btty.fd[1], pbuf, strlen(pbuf));

		snprintf(pbuf, btty.len, "prompttext:                 %s\r\n",
			btty.promptText);
		i = write(btty.fd[1], pbuf, strlen(pbuf));

		snprintf(pbuf, btty.len, "accelerator:                %0.3f\r\n",
			btty.accel);
		i = write(btty.fd[1], pbuf, strlen(pbuf));

		snprintf(pbuf, btty.len, "active panel:               %i\r\n",
			btty.p);
		i = write(btty.fd[1], pbuf, strlen(pbuf));

		if (btty.flags & B_TTY_DEBUG)
			snprintf(pbuf, btty.len, "debug (cli):                on\r\n");
		else
			snprintf(pbuf, btty.len, "debug (cli):                off\r\n");
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		if (btty.flags & B_TTY_AWV)
			snprintf(pbuf, btty.len, "access withdrawn var        on\r\n");
		else
			snprintf(pbuf, btty.len, "access withdrawn var        off\r\n");
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		if (btty.flags & B_TTY_AUV)
			snprintf(pbuf, btty.len, "access unnamed var          on\r\n");
		else
			snprintf(pbuf, btty.len, "access unnamed var          off\r\n");
		i = write(btty.fd[1], pbuf, strlen(pbuf));
		return(B_ERR_OK);
	}

	if (c == 2)
	{
		if (strncmp(v[1], "line", strlen(v[1])) == 0)
		{
			snprintf(pbuf, btty.len, "line width:                 %i\r\n",
				btty.len);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "prompt", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "prompt:                     %s\r\n",
				btty.prompt);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "prompttext", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "prompttext:          %s\r\n",
				btty.promptText);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "panel", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "active panel:               %i\r\n",
				btty.p);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "accelerator", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "accelerator:                %0.3f\r\n",
				btty.accel);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "awv", strlen(v[1])) == 0) {
			if (btty.flags & B_TTY_AWV)
				snprintf(pbuf, btty.len, "access withdrawn var on\r\n");
			else
				snprintf(pbuf, btty.len, "access withdrawn var off\r\n");
			return(B_ERR_OK);
		} else if (strncmp(v[1], "prompt", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "prompt:                     %s\r\n",
				btty.prompt);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "prompttext", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "prompttext:          %s\r\n",
				btty.promptText);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "panel", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "active panel:               %i\r\n",
				btty.p);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "accelerator", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "accelerator:                %0.3f\r\n",
				btty.accel);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "awv", strlen(v[1])) == 0) {
			if (btty.flags & B_TTY_AWV)
				snprintf(pbuf, btty.len, "access withdrawn var on\r\n");
			else
				snprintf(pbuf, btty.len, "access withdrawn var off\r\n");
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "debug", strlen(v[1])) == 0) {
			if (btty.flags & B_TTY_DEBUG)
				snprintf(pbuf, btty.len, "debug (cli):                on\r\n");
			else
				snprintf(pbuf, btty.len, "debug (cli):                off\r\n");
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "save", strlen(v[1])) == 0) {
			snprintf(pbuf, btty.len, "save set not implemented\r\n");
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "savehistory", strlen(v[1])) == 0) {
			if (btty.flags & B_TTY_SAVE_HIST)
				snprintf(pbuf, btty.len, "savehistory:                on\r\n");
			else
				snprintf(pbuf, btty.len, "savehistory:                off\r\n");
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "history", strlen(v[1])) == 0) {
			for (i = btty.hist_c; i > 0; i--)
			{
				if (btty.hist[i][0] != '\0')
				{
					snprintf(pbuf, btty.len, "%i %s\r\n", i, btty.hist[i]);
					n = write(btty.fd[1], pbuf, strlen(pbuf));
				}
			}
			return(B_ERR_OK);
		}
		return(2);
	}

	if (c == 3)
	{
		if (strncmp(v[1], "line", strlen(v[1])) == 0)
		{
			if ((n = atoi(v[2])) < 0)
				return(3);
			else if (n > B_TTY_LINE_LEN)
				return(3);
			else
				btty.len = n;
			snprintf(pbuf, btty.len, "line width:                 %i\r\n",
				btty.len);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "prompttext", strlen(v[1])) == 0) {
			snprintf(btty.promptText, 64, "%s", v[2]);
			snprintf(pbuf, btty.len, "prompttext:                 %s\r\n",
				btty.promptText);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "play", strlen(v[1])) == 0) {
			if (strncmp(v[2], "mode", strlen(v[2])) == 0) {
				btty.flags &= ~B_TTY_MMASK;
				btty.flags |= B_TTY_PLAY;
				snprintf(pbuf, btty.len,
					"Play mode: type ':' or <ESC> to exit\n\r");
				i = write(btty.fd[1], pbuf, strlen(pbuf));
				 return(B_ERR_EARLY);
			}
		} else if (strncmp(v[1], "accelerator", strlen(v[1])) == 0) {
			btty.accel = atof(v[2]);
			snprintf(pbuf, btty.len, "accelerator:                %0.3f\r\n",
				btty.accel);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "panel", strlen(v[1])) == 0) {
			int x = -1;
			if ((x = atoi(v[2])) < 0)
				x = -1;
			if (x >= SYNTHS->win->app->nresources)
				x = -1;
			if (SYNTHS->win->app->resources[x].ndevices == 0)
				x = -1;
			if (SYNTHS->win->app->resources[x].devlocn == NULL)
				x = -1;
			if (x >= 0)
			{
				btty.p = x;
				if (btty.i >= SYNTHS->win->app->resources[x].ndevices)
					btty.i = SYNTHS->win->app->resources[x].ndevices - 1;
			}
			else
				return(3);
			snprintf(pbuf, btty.len, "active panel:               %i\r\n",
				btty.p);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "auv", strlen(v[1])) == 0) {
			btty.flags &= ~B_TTY_AUV;
			if (strcmp("on", v[2]) == 0)
				btty.flags |= B_TTY_AWV;
			if (btty.flags & B_TTY_AWV)
				snprintf(pbuf, btty.len, "access unnamed var          on\r\n");
			else
				snprintf(pbuf, btty.len, "access unnamed var          off\r\n");
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "awv", strlen(v[1])) == 0) {
			btty.flags &= ~B_TTY_AWV;
			if (strcmp("on", v[2]) == 0)
				btty.flags |= B_TTY_AWV;
			if (btty.flags & B_TTY_AWV)
				snprintf(pbuf, btty.len, "access withdrawn var        on\r\n");
			else
				snprintf(pbuf, btty.len, "access withdrawn var        off\r\n");
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "savehistory", strlen(v[1])) == 0) {
			if (strcmp(v[2], "on") == 0)
				btty.flags |= B_TTY_SAVE_HIST;
			else if (strcmp(v[2], "off") == 0)
				btty.flags &= ~B_TTY_SAVE_HIST;
			if (btty.flags & B_TTY_SAVE_HIST)
				snprintf(pbuf, btty.len, "savehistory:                on\r\n");
			else
				snprintf(pbuf, btty.len, "savehistory:                off\r\n");
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		} else if (strncmp(v[1], "history", strlen(v[1])) == 0) {
			if ((btty.hist_c = atoi(v[2])) <= 0)
				btty.hist_c = 50;
			else if (btty.hist_c > 50)
				btty.hist_c = 50;
			return(B_ERR_OK);
		} else if (strncmp(v[1], "noalias", strlen(v[1])) == 0) {
			for (i = 0; commands[i].map != -1; i++)
			{
				if (commands[i].map != B_COM_ALIAS)
					continue;
				if (strcmp(commands[i].name, v[2]) == 0)
				{
					commands[i].name[0] = '\0';
					commands[i].help[0] = '\0';
					commands[i].map = B_COM_FREE;
					commands[i].exec = NULL;
					break;
				}
			}
			return(B_ERR_OK);
		} else if (strncmp(v[1], "debug", strlen(v[1])) == 0) {
			btty.flags &= ~B_TTY_DEBUG;
			if (strcmp(v[2], "on") == 0)
				btty.flags |= B_TTY_DEBUG;
			if (btty.flags & B_TTY_DEBUG)
				snprintf(pbuf, btty.len, "debug (cli):                on\r\n");
			else
				snprintf(pbuf, btty.len, "debug (cli):                off\r\n");
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		}
	}

	if (c > 3)
	{
		if (strncmp(v[1], "play", strlen(v[1])) == 0) {
			print("set play");
			if (c != 4)
				return(B_ERR_PARAM);
			if (strncmp(v[2], "rate", strlen(v[2])) == 0)
			{
				int rate;

				if ((rate = atoi(v[3])) < 0) return(B_ERR_VALUE);
				sequences[btty.sequence.id].rate = rate;
				return(B_ERR_OK);
			}
			if (strncmp(v[2], "transpose", strlen(v[2])) == 0)
			{
				int transpose;

				transpose = atoi(v[3]);
				btty.sequence.transpose = transpose;
				return(B_ERR_OK);
			}
			if (strncmp(v[2], "sequence", strlen(v[2])) == 0)
			{
				int seq;

				print("sequence");
				if ((seq = atoi(v[3])) < 0) return(B_ERR_VALUE);
				if (seq >= B_SEQ_COUNT) return(B_ERR_VALUE);
				if (sequences[seq].count <= 0) return(B_ERR_VALUE);

				btty.sequence.step = -1;
				btty.sequence.count =
					250 * 60 / sequences[btty.sequence.id].rate;

				btty.flags &= ~B_TTY_MMASK;
				btty.flags |= B_TTY_SEQ|B_TTY_RAW;

				return(B_ERR_EARLY);
			}
		} else if ((strncmp(v[1], "history", strlen(v[1])) == 0)
			&& (strncmp(v[2], "comm", strlen(v[2])) == 0))
		{
			char *comm;

			/* rotate history, add this one */
			if ((comm = strstr(cbuf, "comm")) == NULL)
				return(2);
			comm += 5;
			snprintf(btty.hist[1], B_TTY_LINE_LEN, "%s", comm);
			return(B_ERR_OK);
		}
	}

	return(2);
}

static int
execImport(guimain *global, int c, char **v)
{
	int loc;

	if (execHelpCheck(c, v)) return(0);

	loc = SYNTHS->location;

	if (c == 1)
		return(2);

	if (c == 2) {
		loc = SYNTHS->location;
		if ((v[1] == NULL) || (v[1][0] == '\0'))
			return(3);
	}

	print(v[1]);

	if (strncmp("import", v[0], strlen(v[0])) == 0)
	{
		if (c == 3)
			loc = atoi(v[2]);
		bristolMemoryImport(loc, v[1], RESOURCES->name);
		return(0);
	} else if (strncmp("export", v[0], strlen(v[0])) == 0) {
		if (c == 3)
			loc = atoi(v[2]);
		bristolMemoryExport(loc, v[1], RESOURCES->name);
		return(0);
	}

	return(0);
}

static void
execParamChange(guimain *global, float v)
{
	int i;
	char *name;

	if ((paramname(btty.i) == NULL) || (paramname(btty.i)[0] == '\0'))
		name = unknown;
	else
		name = paramname(btty.i);

/*
	if ((SYNTHS->win->app->resources[btty.p].devlocn[btty.i].type == 1)
		|| (SYNTHS->win->app->resources[btty.p].devlocn[btty.i].type == 7))
		brightonChangeParam(SYNTHS, btty.p, btty.i, v);
	else
		brightonChangeParam(SYNTHS, btty.p, btty.i,
			v * RESOURCES->resources[btty.p].devlocn[btty.i].to);
*/
	brightonChangeParam(SYNTHS, btty.p, btty.i, v * DEVICE(btty.i).to);

	if (DEVICE(btty.i).to == 1.0f)
	{
		if (DEVICE(btty.i).type == 1)
			snprintf(pbuf, btty.len, "%02i %-25s: %4.0f\r\n", btty.i,
				name, (1.0 - PDEV(btty.i)->value) * 1000.0f);
		else if (DEVICE(btty.i).type == 2)
			snprintf(pbuf, btty.len, "%02i %-25s: %1.0f\r\n", btty.i,
				name, PDEV(btty.i)->value);
		else
			snprintf(pbuf, btty.len, "%02i %-25s: %4.0f\r\n", btty.i,
				name, PDEV(btty.i)->value * 1000.0f);
	} else {
		if (DEVICE(btty.i).type == 1)
		{
			if (DEVICE(btty.i).flags & BRIGHTON_REVERSE)
				snprintf(pbuf, btty.len, "%02i %-25s: %4.0f\r\n", btty.i,
					name, PDEV(btty.i)->value * DEVICE(btty.i).to);
			else
				snprintf(pbuf, btty.len, "%02i %-25s: %4.0f\r\n", btty.i,
					name, (1.0 - PDEV(btty.i)->value)
						* DEVICE(btty.i).to);
		} else if (DEVICE(btty.i).type == 2) {
			snprintf(pbuf, btty.len, "%02i %-25s: %4.0f\r\n", btty.i,
				name, PDEV(btty.i)->value);
		} else
			snprintf(pbuf, btty.len, "%02i %-25s: %4.0f\r\n", btty.i,
				name, PDEV(btty.i)->value * DEVICE(btty.i).to);
		//	snprintf(pbuf, btty.len, "%02i %-25s: %3.0f\n\r", btty.i,
		//		name, ((brightonDevice *) DEVICE(btty.i).dev)->value);
	}

	i = write(btty.fd[1], pbuf, strlen(pbuf));
}

static int
execBrighton(guimain *global, int c, char **argv)
{
	int p, d, tp, ti;
	float v;

	print("execBrighton");

	if (execHelpCheck(c, argv)) return(0);

	if (c == 2)
	{
		if (strncmp(brightoncom[1].name, argv[1], strlen(argv[1])) == 0)
		{
			snprintf(pbuf, btty.len, "active panel:        %i\r\n", btty.p);
			p = write(btty.fd[1], pbuf, strlen(pbuf));
			return(0);
		}
		return(1);
	}

	if (c == 3)
	{
		if (strncmp(brightoncom[1].name, argv[1], strlen(argv[1])) == 0)
		{
			int x;

			if ((x = atoi(argv[2])) < 0)
				return(3);
			if (btty.p > SYNTHS->win->app->nresources)
				return(3);
			if (SYNTHS->win->app->resources[x].ndevices == 0)
				return(3);
			btty.p = x;
			snprintf(pbuf, btty.len, "active panel:        %i\r\n", btty.p);
			p = write(btty.fd[1], pbuf, strlen(pbuf));
			return(0);
		}
		return(2);
	}

	if (c == 5)
	{
		if (strncmp(brightoncom[2].name, argv[1], strlen(argv[1])) == 0)
		{
			/* Device settings */
			if ((p = atoi(argv[2])) < 0)
				return(B_ERR_PARAM);
			if (p >= SYNTHS->win->app->nresources)
				return(B_ERR_PARAM);
			if (SYNTHS->win->app->resources[p].ndevices <= 0)
				return(B_ERR_PARAM);
			if ((d = atoi(argv[3])) < 0)
				return(B_ERR_PARAM);
			if (d >= SYNTHS->win->app->resources[p].ndevices)
				return(B_ERR_PARAM);

			tp = btty.p;
			ti = btty.i;

			btty.p = p;
			btty.i = d;
	
			if (argv[4][1] == '=')
			{
				if (argv[4][0] == '+')
					v = atof(&argv[4][2]) + PDEV(d)->value;
				else if (argv[4][0] == '-')
					v = PDEV(d)->value - atof(&argv[4][2]);
				else
					return(B_ERR_VALUE);
			} else if ((v = atof(argv[4])) < 0.0f)
				return(B_ERR_VALUE);

			if (v < 0) v = 0.0f;

			execParamChange(global, v);

			btty.p = tp;
			btty.i = ti;

			return(B_ERR_OK);
		}
	}
	return(B_ERR_PARAM);
}

static void
cliParamChange(guimain *global, float v)
{
	int i;
	char *name, crlf[4] = "\r\n", *c1 = crlf, *c2 = "";

	if ((paramname(btty.i) == NULL) || (paramname(btty.i)[0] == '\0'))
		name = unknown;
	else
		name = paramname(btty.i);

	if (v != 0)
	{
		//if (RESOURCES->resources[btty.p].devlocn[btty.i].to != 1.0f)
		if (DEVICE(btty.i).to != 1.0f)
		{
			if (DEVICE(btty.i).type == 1)
			{
				if (DEVICE(btty.i).flags & BRIGHTON_VERTICAL) {
					if (v > 0)
						brightonChangeParam(SYNTHS, btty.p, btty.i,
							PDEV(btty.i)->value * DEVICE(btty.i).to + 1);
					else
						brightonChangeParam(SYNTHS, btty.p, btty.i,
							PDEV(btty.i)->value * DEVICE(btty.i).to - 1);
				} else if (DEVICE(btty.i).flags & BRIGHTON_REVERSE) {
					if (v > 0)
						brightonChangeParam(SYNTHS, btty.p, btty.i,
							PDEV(btty.i)->value * DEVICE(btty.i).to - 1);
					else
						brightonChangeParam(SYNTHS, btty.p, btty.i,
							PDEV(btty.i)->value * DEVICE(btty.i).to + 1);
				} else {
					if (v > 0)
						brightonChangeParam(SYNTHS, btty.p, btty.i,
							(1.0 - PDEV(btty.i)->value) 
							* DEVICE(btty.i).to + 1);
					else
						brightonChangeParam(SYNTHS, btty.p, btty.i,
							(1.0 - PDEV(btty.i)->value) 
							* DEVICE(btty.i).to - 1);
				}
			} else if (DEVICE(btty.i).type == 7) {
				if (v > 0)
					brightonChangeParam(SYNTHS, btty.p, btty.i,
						PDEV(btty.i)->value
						* DEVICE(btty.i).to + 1);
				else
					brightonChangeParam(SYNTHS, btty.p, btty.i,
						PDEV(btty.i)->value 
						* DEVICE(btty.i).to - 1);
			} else if (DEVICE(btty.i).type == 2) {
				if (v > 0)
					brightonChangeParam(SYNTHS, btty.p, btty.i,
						PDEV(btty.i)->value + 1);
				else
					brightonChangeParam(SYNTHS, btty.p, btty.i,
						PDEV(btty.i)->value - 1);
			} else {
				if (v > 0)
					brightonChangeParam(SYNTHS, btty.p, btty.i,
						PDEV(btty.i)->value * DEVICE(btty.i).to + 1);
				else
					brightonChangeParam(SYNTHS, btty.p, btty.i,
						PDEV(btty.i)->value * DEVICE(btty.i).to - 1);
			}
		} else {
			if (DEVICE(btty.i).type == 1)
			{
				if (DEVICE(btty.i).flags & BRIGHTON_VERTICAL)
					brightonChangeParam(SYNTHS, btty.p, btty.i,
						PDEV(btty.i)->value + v);
				else if (DEVICE(btty.i).flags & BRIGHTON_REVERSE)
					brightonChangeParam(SYNTHS, btty.p, btty.i,
						PDEV(btty.i)->value - v);
				else
					brightonChangeParam(SYNTHS, btty.p, btty.i,
						(1.0 - PDEV(btty.i)->value) + v);
			} else
				brightonChangeParam(SYNTHS, btty.p, btty.i,
					PDEV(btty.i)->value + v);
		}
	}

	if (btty.flags & B_TTY_COOKED)
	{
		print("post return");
		c1 = c2; c2 = crlf;
	} else
		print("pre return");

	if (DEVICE(btty.i).to == 1.0f)
	{
		if (DEVICE(btty.i).type == 1)
		{
			if (DEVICE(btty.i).flags & BRIGHTON_VERTICAL)
				snprintf(pbuf, btty.len, "%s%02i %-25s: %4.0f%s", c1, btty.i,
					name, PDEV(btty.i)->value * 1000.0f, c2);
			else
				snprintf(pbuf, btty.len, "%s%02i %-25s: %4.0f%s", c1, btty.i,
					name, (1.0 - PDEV(btty.i)->value) * 1000.0f, c2);
		} else if (DEVICE(btty.i).type == 2)
			snprintf(pbuf, btty.len, "%s%02i %-25s: %4.0f%s", c1, btty.i,
				name, PDEV(btty.i)->value, c2);
		else
			snprintf(pbuf, btty.len, "%s%02i %-25s: %4.0f%s", c1, btty.i,
				name, PDEV(btty.i)->value * 1000.0f, c2);
	} else {
		if (DEVICE(btty.i).type == 1)
		{
			if (DEVICE(btty.i).flags & BRIGHTON_REVERSE)
				snprintf(pbuf, btty.len, "%s%02i %-25s: %4.0f%s", c1, btty.i,
					name, PDEV(btty.i)->value * DEVICE(btty.i).to, c2);
			else
				snprintf(pbuf, btty.len, "%s%02i %-25s: %4.0f%s", c1, btty.i,
					name, (1.0 - PDEV(btty.i)->value)
						* DEVICE(btty.i).to, c2);
		} else if (DEVICE(btty.i).type == 2) {
			snprintf(pbuf, btty.len, "%s%02i %-25s: %4.0f%s", c1, btty.i,
				name, PDEV(btty.i)->value, c2);
		} else
			snprintf(pbuf, btty.len, "%s%02i %-25s: %4.0f%s", c1, btty.i,
				name, PDEV(btty.i)->value * DEVICE(btty.i).to, c2);
		//	snprintf(pbuf, btty.len, "%02i %-25s: %3.0f\n\r", btty.i,
		//		name, ((brightonDevice *) DEVICE(btty.i).dev)->value);
	}

	lp = PDEV(btty.i)->value;

	i = write(btty.fd[1], pbuf, strlen(pbuf));
}

void
brightonCLIinit(guimain *global, int fd[])
{
	btty.fd[0] = fd[0];
	btty.fd[1] = fd[1];
	btty.cycle = 500;

	snprintf(btty.prompt, 64, "nicky");

	btty.global = global;
	btty.flags |= B_TTY_COOKED|B_TTY_INIT;
}

#define B_MATCHES 64

/*
 * Take input line, extract word up to first space and attempt completion.
 * If we get a completion see if there are more words and complete them with 
 * the new comset
 */
int
bttyCookedCompletion(char *line, comSet *coms)
{
	int i, v, count = 0, t, c, first = -1;
	char src[B_TTY_LINE_LEN], dst[B_TTY_LINE_LEN];
	char *matches[B_MATCHES];

	print("Complete");

	if (coms == NULL)
		return(0);

	print(line);

	memset(matches, 0, sizeof(matches));
	memset(src, 0, B_TTY_LINE_LEN);
	memset(dst, 0, B_TTY_LINE_LEN);

	for (i = 0; 1 ; i++)
	{
		if (((src[i] = line[i]) == ' ') || (src[i] == '\0'))
		{
			src[i] = '\0';
			break;
		}
	}

	for (v = 0; coms[v].map != B_COM_LAST; v++)
	{
		if (strncasecmp(coms[v].name, src, strlen(src)) == 0)
		{
			print(coms[v].name);

			matches[count] = brightonmalloc(64);
			snprintf(matches[count], 64, "%s", coms[v].name);

			if (first < 0) first = v;
			if (++count >= B_MATCHES)
				break;
		}
	}

	if (count == 1) {
		/*
		 * We now have one word filled out, see if we can continue
		 */
		snprintf(src, B_TTY_LINE_LEN, "%s", coms[first].name);

		if (line[i] != '\0')
		{
			snprintf(dst, B_TTY_LINE_LEN, "%s", &line[i + 1]);
			print("subsearch");
			print(dst);
			if (bttyCookedCompletion(dst, coms[first].subcom) >= 1)
				snprintf(line, B_TTY_LINE_LEN, "%s %s", src, dst);
			else
				snprintf(line, B_TTY_LINE_LEN, "%s %s", src, &line[i + 1]);
		} else
			snprintf(line, B_TTY_LINE_LEN, "%s ", src);
		//btty.edit_p = strlen(src);
	} else if (count != 0) {
		int y, x, ch, cont = 1;

		/* Many hits - see how we can match them, display the result */
		for (x = 0; cont; x++)
		{
			ch = matches[0][x];
			for (y = 1; y < count; y++)
			{
				if (matches[y][x] == '\0')
					cont = 0;
				if (matches[y][x] != ch)
					cont = 0;
			}
		}
		if (x > 0)
		{
			c = 0;
			ch = ' ';
			/* Show the list of commands we have matched */
			for (y = 0; y < count; y++)
			{
				t = write(btty.fd[1], matches[y], strlen(matches[y]));
				t = write(btty.fd[1], &ch, 1);
				if ((c += strlen(matches[y]) + 1) > btty.len - 16)
				{
					snprintf(pbuf, 4, "\n\r");
					t = write(btty.fd[1], pbuf, 2);
					c = 0;
				}
			}
			/* And an extra return if we need it */
			if (c != 0)
			{
				snprintf(pbuf, 4, "\n\r");
				t = write(btty.fd[1], pbuf, 2);
			}
			/* Then see if we can move the display forward */
			if (x > strlen(line))
			{
				snprintf(line, x, "%s", matches[0]);
				btty.edit_p = strlen(line);
			} 
//			snprintf(pbuf, B_TTY_LINE_LEN, "%i %s\r\n", x, line);
//			t = write(btty.fd[1], pbuf, strlen(pbuf));
		}
	}

	btty.edit_p = strlen(line);

	for (i = 0; matches[i] != NULL; i++)
		brightonfree(matches[i]);

	return(count);
}

/*
 * Search only through the CLI command set
 */
static int
bttyComplete(guimain *global)
{
	int v, first = -1, count = 0, c, t;//, mi = 0;
	char *matches[B_MATCHES];

	memset(matches, 0, sizeof(matches));

	if (btty.flags & B_TTY_DEBUG)
	{
		snprintf(pbuf, btty.len, "\n\rcomplete %s\r", cbuf);
		t = write(btty.fd[1], pbuf, strlen(pbuf));
	}

	if (btty.flags & B_TTY_COOKED) {
		/* Complete then edit */
		if (isdigit(cbuf[0]))
		{
			switch (strlen(cbuf)) {
				case 1:
				case 2:
					v = atoi(cbuf);
					if (btty.hist[v][0] == '\0')
						break;
					snprintf(cbuf, btty.len, "%s", btty.hist[v]);
					btty.edit_p = strlen(cbuf);
					return(1);
				default:
					break;
			}
		}

		/* Complete then edit */
		if ((cbuf[0] == '!') && (isdigit(cbuf[1])))
		{
			switch (strlen(&cbuf[1])) {
				case 1:
				case 2:
					v = atoi(&cbuf[1]);
					if (btty.hist[v][0] == '\0')
						break;
					snprintf(cbuf, btty.len, "%s", btty.hist[v]);
					btty.edit_p = strlen(cbuf);
					return(1);
				default:
					break;
			}
		}

		count = bttyCookedCompletion(cbuf, commands);
		btty.edit_p = strlen(cbuf);
	}

	for (v = 0; ((matches[v] != NULL) && (v < B_MATCHES)); v++)
		brightonfree(matches[v]);
	memset(matches, 0, sizeof(matches));

	if (count != 0)
		return(0);

	c = SYNTHS->win->app->resources[btty.p].ndevices;

	for (v = 0; v != c; v++)
	{
		if (cbuf[0] == '^') {
			if (strncasecmp(paramname(v), &cbuf[1], strlen(cbuf) - 1) == 0)
			{
				if (first < 0) first = v;
				matches[count] = brightonmalloc(64);
				snprintf(matches[count], 64, "%s", paramname(v));
				if (++count >= B_MATCHES)
					break;
			}
		} else if (strcasestr(paramname(v), cbuf) != NULL) {
			if (first < 0) first = v;
			matches[count] = brightonmalloc(64);
			snprintf(matches[count], 64, "%s", paramname(v));
			if (++count >= B_MATCHES)
				break;
		}
	}
	if (count == 1) {
		snprintf(cbuf, btty.len, "%s", paramname(first));
		btty.i = first;
		btty.edit_p = strlen(cbuf);
	} else if (count != 0) {
		int y, x, ch, cont = 1;

		/* Many hits - see how far we can match them, display the result */
		for (x = 0; cont; x++)
		{
			ch = matches[0][x];
			for (y = 1; y < count; y++)
				{
				if (matches[y][x] == '\0')
					cont = 0;
				if (matches[y][x] != ch)
					cont = 0;
			}
		}
		if (x > 0)
		{
			c = 0;
			ch = ' ';
			for (y = 0; y < count; y++)
			{
				t = write(btty.fd[1], matches[y], strlen(matches[y]));
				t = write(btty.fd[1], &ch, 1);
				if ((c += strlen(matches[y]) + 1) > btty.len - 16)
				{
					snprintf(pbuf, 4, "\n\r");
					t = write(btty.fd[1], pbuf, 2);
					c = 0;
				}
			}
			if (c != 0)
			{
				snprintf(pbuf, 4, "\n\r");
				t = write(btty.fd[1], pbuf, 2);
			}
			if (x > strlen(cbuf))
			{
				snprintf(cbuf, x, "%s", matches[0]);
				btty.edit_p = strlen(cbuf);
			}
		}
	}

	for (v = 0; ((matches[v] != NULL) && (v < B_MATCHES)); v++)
		brightonfree(matches[v]);

	return(count);
}

/*
 * Search only through the synth parameters.
 */
static int
bttySearch(guimain *global)
{
	int v, first = -1, count = 0;

//	snprintf(pbuf, btty.len, "bttySearch\n\r");
//	t = write(btty.fd[1], pbuf, strlen(pbuf));

	if (cbuf[0] == '\0')
		return(1);

	if (btty.flags & B_TTY_DEBUG)
	{
		snprintf(pbuf, btty.len, "\n\rsearch %s\r", cbuf);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
	}

	/*
	 * Search through the internal commands and the synth parameters. If only
	 * one match then commplete it, otherwise list them.
	 */
	if (btty.i >= SYNTHS->win->app->resources[btty.p].ndevices)
	{
		btty.i = SYNTHS->win->app->resources[btty.p].ndevices - 1;
		v = 0;
	} else
		v = btty.i + 1;

	for (; v != btty.i; v++)
	{
		if (v >= SYNTHS->win->app->resources[btty.p].ndevices)
			v = -1;
//snprintf(pbuf, btty.len, "%s %s %i %i\n\r", paramname(v), cbuf, btty.i, v);
//t = write(btty.fd[1], pbuf, strlen(pbuf));
		if (paramname(v) == NULL)
			continue;
		if (unnamed(v))
			continue;
		if (cbuf[0] == '^') {
			if (strncasecmp(paramname(v), &cbuf[1], strlen(cbuf) - 1) == 0)
			{
				if (first == -1)
					first = v;
				count++;
				break;
			}
		} else if (strcasestr(paramname(v), cbuf) != NULL) {
			if (first == -1)
				first = v;
			count++;
			break;
		}
	}

	if (first >= 0)
		btty.i = first;

	cliParamChange(global, 0.0f);

	btty.flags &= ~B_TTY_MMASK;
	btty.flags |= B_TTY_RAW;

	btty.edit_p = 0;
	btty.hist_tmp = 0;
	memset(cbuf, 0, B_TTY_LINE_LEN);

	return(count);
}

static int
execDebug(guimain *global, int c, char **v)
{
	int i;

	if (execHelpCheck(c, v)) return(0);

	if (c == 2) {
		if (strncmp("off", v[1], strlen(v[1])) == 0)
			btty.flags &= ~B_TTY_DEBUG;
		else if (strncmp("on", v[1], strlen(v[1])) == 0)
			btty.flags |= B_TTY_DEBUG;
		else
			btty.flags &= ~B_TTY_DEBUG;
		return(0);
	}

	if (c == 3)
	{
		if (strncmp("cli", v[1], strlen(v[1])) == 0)
		{
			if (strncmp("off", v[2], strlen(v[2])) == 0)
				btty.flags &= ~B_TTY_DEBUG;
			else if (strncmp("on", v[2], strlen(v[2])) == 0)
				btty.flags |= B_TTY_DEBUG;
			else
				btty.flags &= ~B_TTY_DEBUG;
			return(0);
		}
		if (strncmp(debugcomm[2].name, v[1], strlen(v[1])) == 0)
		{
			if (strncmp("off", v[2], strlen(v[2])) == 0)
			{
				SYNTHS->flags &= ~REQ_DEBUG_MASK;
				SYNTHS->win->flags &= ~BRIGHTON_DEBUG;
			} else if (strncmp("on", v[2], strlen(v[2])) == 0) {
				SYNTHS->flags |= REQ_DEBUG_MASK;
				SYNTHS->win->flags |= BRIGHTON_DEBUG;
			}
		}
		if (strncmp("engine", v[1], strlen(v[1])) == 0)
		{
			if ((btty.edbg = atoi(v[2])) <= 0)
				btty.edbg = 0;
			if (btty.edbg > 15)
				btty.edbg = 15;

			bristolMidiSendMsg(global->controlfd, SYNTHS->sid,
				BRISTOL_SYSTEM, 0, BRISTOL_REQ_DEBUG|btty.edbg);

			snprintf(pbuf, btty.len, "debug (engine):             %i\r\n",
				btty.edbg);
			i = write(btty.fd[1], pbuf, strlen(pbuf));

			return(0);
		} if (strncmp("midi", v[1], strlen(v[1])) == 0) {
			int n = atoi(v[2]);
			switch (n) {
				default:
				case 0:
					n = 0;
					break;
				case 1:
					break;
				case 2:
					break;
				case 3:
					break;
			}
			btty.mdbg = n;
			if (global->libtest == 0)
				bristolMidiSendNRP(global->controlfd, SYNTHS->midichannel,
		            BRISTOL_NRP_DEBUG, n);
			snprintf(pbuf, btty.len, "MIDI debug:                   %i\r\n",
				btty.mdbg);
			i = write(btty.fd[1], pbuf, strlen(pbuf));
			return(B_ERR_OK);
		}
	}

	if (c == 1)
	{
		if (btty.flags & B_TTY_DEBUG)
			snprintf(pbuf, btty.len, "debug (cli):                on\r\n");
		else
			snprintf(pbuf, btty.len, "debug (cli):                off\r\n");
		i = write(btty.fd[1], pbuf, strlen(pbuf));

		snprintf(pbuf, btty.len, "debug (engine):             %i\r\n", btty.edbg);
		i = write(btty.fd[1], pbuf, strlen(pbuf));

		return(0);
	}

	return(0);
}

static int
execLoad(guimain *global, int c, char **v)
{
	int n = 0, undo = 0;

	if (execHelpCheck(c, v)) return(0);

	print(v[0]);

	if (c >= 2)
	{
		if (strncmp("undo", v[1], strlen(v[1])) == 0)
			undo = 1;
		else if ((n = atoi(v[1])) < 0)
			return(0);
	}

	if ((strncmp(v[0], "load", strlen(v[0])) == 0)
		|| (strncmp(v[0], "read", strlen(v[0])) == 0))
	{
		if (undo) {
			SYNTHS->location = 9999;
			bttyMemLoad(global);
			SYNTHS->location = btty.lastmem;
		} else if (c == 1) {
			/* load current mem */
			btty.lastmem = SYNTHS->location;
			bttyMemLoad(global);
			SYNTHS->location = btty.lastmem;
			return(0);
		} else {
			/* load named memory */
			btty.lastmem = SYNTHS->location;
			SYNTHS->location = n;
			bttyMemLoad(global);
		}
	}

	if ((strncmp(v[0], "save", strlen(v[0])) == 0)
		|| (strncmp(v[0], "write", strlen(v[0])) == 0))
	{
		if (c == 1)
		{
			/* save current mem */
			bttyMemSave(global);
			return(0);
		} else {
			SYNTHS->location = n;
			bttyMemSave(global);
		}
	}

	if (strncmp(v[0], "find", strlen(v[0])) == 0)
	{
		int location = SYNTHS->location + 1;

		if (location > 1023)
			location = 0;

		if ((c == 2) && (strncmp(v[1], "free", strlen(v[1])) == 0))
		{
			/* find free mem */
			while (loadMemory(SYNTHS, RESOURCES->name, 0,
				location, SYNTHS->mem.active, 0, BRISTOL_STAT) == 0)
			{
				if (++location > 1024)
					location = 0;
				if (location == SYNTHS->location)
					break;
			}
			if (location != SYNTHS->location)
			{
				SYNTHS->location = location;
				snprintf(pbuf, btty.len, "free: %i\r\n", SYNTHS->location);
				n = write(btty.fd[1], pbuf, strlen(pbuf));
			} else {
				snprintf(pbuf, btty.len, "no free memories\r\n");
				n = write(btty.fd[1], pbuf, strlen(pbuf));
			}
		}

		if ((c == 2) && ((strncmp(v[1], "load", strlen(v[1])) == 0)
			|| (strncmp(v[1], "read", strlen(v[1])) == 0)))
		{
			/* find mem */
			while (loadMemory(SYNTHS, RESOURCES->name, 0,
				location, SYNTHS->mem.active, 0, BRISTOL_STAT) != 0)
			{
				if (++location > 1024)
					location = 0;
				if (location == SYNTHS->location)
					break;
			}

			SYNTHS->location = location;
			bttyMemLoad(global);
		}

		if (c == 1)
		{
			/* find mem */
			while (loadMemory(SYNTHS, RESOURCES->name, 0,
				location, SYNTHS->mem.active, 0, BRISTOL_STAT) != 0)
			{
				if (++location > 1024)
					location = 0;
				if (location == SYNTHS->location)
					break;
			}
			if (location != SYNTHS->location)
			{
				SYNTHS->location = location;
				snprintf(pbuf, btty.len, "mem: %i\r\n", SYNTHS->location);
				n = write(btty.fd[1], pbuf, strlen(pbuf));
			} else {
				snprintf(pbuf, btty.len, "no memories\r\n");
				n = write(btty.fd[1], pbuf, strlen(pbuf));
			}

			return(0);
		}
	}

	return(0);
}

static int
execHelp(guimain *global, int c, char **v)
{
	int i, n;

	if (execHelpCheck(c, v)) return(0);

	print(v[0]);

	if (c == 1)
	{
		snprintf(pbuf, B_TTY_LINE_LEN, "CLI ':insert' mode command list:\r\n");
		n = write(btty.fd[1], pbuf, strlen(pbuf));

		for (i = 0; commands[i].map != -1; i++)
		{
			if (commands[i].map == B_COM_NOT_USED)
				continue;
			snprintf(pbuf, B_TTY_LINE_LEN, "%12s (:) %s\n\r",
				commands[i].name, commands[i].help);
			if (commands[i].map == B_COM_ALIAS)
				pbuf[14] = 'a';
			else
				pbuf[14] = 'c';
			n = write(btty.fd[1], pbuf, strlen(pbuf));
		}
		return(0);
	}

	for (i = 0; commands[i].map != B_COM_LAST; i++)
	{
		if (commands[i].map == B_COM_NOT_USED)
			continue;
		if (strncmp(commands[i].name, v[1], strlen(v[1])) == 0)
		{
			snprintf(pbuf, B_TTY_LINE_LEN, "%12s: %s\n\r",
				commands[i].name, commands[i].help);
			n = write(btty.fd[1], pbuf, strlen(pbuf));
			break;
		}
	}

	if (commands[i].map == B_COM_LAST)
	{
		snprintf(pbuf, btty.len, "help '%s' not found\n\r", v[1]);
		n = write(btty.fd[1], pbuf, strlen(pbuf));
	}

	return(0);
}
static int
execQuit(guimain *global, int c, char **v)
{
	if ((c == 1) && (strlen(v[0]) == 4) && (strcmp(v[0], "quit") == 0))
		return(B_ERR_EXIT);
	return(B_ERR_PARAM);
}

static int
bttyExecute(guimain *global, int c, char **v)
{
	int i, j, k, n;
	char var[B_TTY_LINE_LEN];
	float value = 0.0f;

	if (btty.flags & B_TTY_DEBUG)
	{
		for (i = 0; i < c; i++)
		{
			snprintf(pbuf, btty.len, "%i: %i: %s\n\r", i, (int) strlen(v[i]), v[i]);
			n = write(btty.fd[1], pbuf, strlen(pbuf));
		}
	}

	for (i = 0; commands[i].map != B_COM_LAST; i++)
	{
		if (commands[i].map == B_COM_NOT_USED)
			continue;
		if (commands[i].map == B_COM_FREE)
			continue;
		if ((strncmp(v[0], commands[i].name, strlen(v[0])) == 0)
			&& (commands[i].exec != NULL))
		{
			n = commands[i].exec(global, c, v);
			if (btty.flags & B_TTY_ALIASING)
				return(n);
			memset(pbuf, 0, B_TTY_LINE_LEN);
			return(n);
		}
	}

	/*
	 * Search for the command in the synth parameters.
	 */
	for (i = 0, j = 0, k = 0, n = -1; j < c; i++)
	{
		switch (v[j][i]) {
			case '\0':
				i = -1;
				if (++j < c)
					var[k++] = ' ';
				else
					var[k++] = '\0';
				break;
			case '=':
				var[k] = '\0';
				print(var);
				value = atof(&v[j][++i]);
				n = 1;
				break;
			case '+':
				if (v[j][i+1] == '+') {
					n = 3;
				} else if (v[j][i+1] == '=') {
					n = 4;
					++i;
					value = atof(&v[j][++i]);
				} else {
					n = 2;
				}
				var[k] = '\0';
				print("found");
				break;
			case '-':
				if (v[j][i+1] == '-') {
					n = 6;
				} else if (v[j][i+1] == '=') {
					n = 7;
					++i;
					value = atof(&v[j][++i]);
				} else {
					n = 5;
				}
				var[k] = '\0';
				print("found");
				break;
			default:
				var[k++] = v[j][i];
				break;
		}
		if (n != -1) break;
	}

	for (i = 0; i < SYNTHS->win->app->resources[btty.p].ndevices; i++)
	{
		if ((paramname(i) == NULL) || (paramname(i)[0] == '\0'))
			continue;
		if (strcmp(var, paramname(i)) == 0)
		{
			print(paramname(i));
			btty.i = i;

			switch (n) {
				case 1: /* Value was set explicity */
					break;
				case 2: /* + */
					value = PDEV(btty.i)->value + 0.01;
					break;
				case 3: /* ++ */
					value = PDEV(btty.i)->value + btty.accel;
					break;
				case 4: /* += */
					value += PDEV(btty.i)->value;
					break;
				case 5: /* - */
					if ((value = PDEV(btty.i)->value - 0.01) < 0.0f)
						value = 0.0f;
					break;
				case 6: /* -- */
					if ((value = PDEV(btty.i)->value - btty.accel) < 0.0f)
						value = 0.0f;
					break;
				case 7: /* -= */
					if ((value = PDEV(btty.i)->value - value) < 0.0f)
						value = 0.0f;
					break;
			}

			if (n > 0)
				execParamChange(global, value);
			else
				cliParamChange(global, 0.0f);
			n = 0;
			return(0);
		}
	}

	memset(pbuf, 0, B_TTY_LINE_LEN);

	return(1);
}

static char resvar[64];

char *
getprintvar(guimain *global, char *var)
{
	int i;

	if (strcmp(var, "panelid") == 0)
	{
		snprintf(resvar, 64, "%s", RESOURCES->resources[btty.p].name);
		return(resvar);
	}
	if (strcmp(var, "algo") == 0)
	{
		snprintf(resvar, 64, "%s", RESOURCES->name);
		return(resvar);
	}
	if (strcmp(var, "channel") == 0)
	{
		snprintf(resvar, 64, "%i", SYNTHS->midichannel + 1);
		return(resvar);
	}
	if (strcmp(var, "memory") == 0)
	{
		snprintf(resvar, 64, "%i", SYNTHS->location);
		return(resvar);
	}

	for (i = 0 ; i < SYNTHS->win->app->resources[btty.p].ndevices; i++)
	{
		if (strcasecmp(paramname(i), var) == 0)
			break;
	}

	if (i >= SYNTHS->win->app->resources[btty.p].ndevices)
	{
		snprintf(resvar, 64, "%s", var);
		return(resvar);
	}

	/*
	 * Found var, take value.
	 */
	if (DEVICE(i).to == 1.0f)
	{
		if (DEVICE(i).type == 2)
			snprintf(resvar, 64, "%1.0f", PDEV(i)->value);
		else
			snprintf(resvar, 64, "%-3.0f", PDEV(i)->value*1000.0f);
	} else
		snprintf(resvar, 64, "%1.0f", PDEV(btty.i)->value);

	return(resvar);
}

/*
 * So how much work to accept a single string:
 * promt='mem %memory% chan %channel%'
 */
char *
printprompt(guimain *global)
{
	int i = 0, j = 0, aliasing = 0;;
	char var[64], *s = btty.promptText, *d = btty.prompt, *alias;

	/*
	 * Format is "text", or "text <variable>" where var is searched in the
	 * synth table.
	 *
	 * We are looking for 'mem %something% chan %something%: '
	 */
	for (i = 0; i < 64; i++)
	{
		if (*s == '\0')
		{
			*d = '\0';
			break;
		}
		if (*s == '%')
		{
			if (aliasing)
			{
				var[j] = '\0';
				alias = getprintvar(global, var);
				for (; *alias != 0;)
					*d++ = *alias++;
				aliasing = 0;
			} else {
				aliasing = 1;
				memset(var, 0, 64);
				j = 0;
			}
			s++;
		} else if (aliasing)
			var[j++] = *s++;
		else
			*d++ = *s++;
	}

	if (btty.prompt[0] == '\0')
	{
		btty.prompt[0] = ':';
		btty.prompt[1] = '\0';
	}

	return(btty.prompt);
}

static int
bttyInterpret(guimain *global, char *tbuf)
{
	int v, i = 0, j = 0, k, quoted = 0;
	char *comm = tbuf, **vargs, zbuf[B_TTY_LINE_LEN];

	while (isspace(*comm))
		comm++;

	/*
	 * This is to 'overlook' cooked commands that start with a colon, they
	 * typicaly come from retyping ':' due to a damaged output stream from 
	 * debuging output.
	 */
	if ((*comm == ':') && (btty.flags & B_TTY_COOKED))
		comm++;

	if (comm[0] == '!')
	{
		if (((i = atoi(&comm[1])) < 0) || (i >= 49))
		{
			btty.hist_tmp = 0;
			btty.edit_p = 0;
			memset(tbuf, 0, B_TTY_LINE_LEN);
			return(0);
		}
		memset(zbuf, 0, B_TTY_LINE_LEN);
		snprintf(zbuf, B_TTY_LINE_LEN, "%s", btty.hist[i]);
		comm = zbuf;
	}

	if ((comm[0] == '\0') || (isalpha(comm[0]) == 0))
	{
		v = write(btty.fd[1], btty.hist[0], strlen(btty.hist[0]));
		snprintf(pbuf, btty.len, "\r:");
		v = write(btty.fd[1], pbuf, strlen(pbuf));

		btty.edit_p = 0;
		memset(cbuf, 0, B_TTY_LINE_LEN);
		memset(btty.hist[0], 0, B_TTY_LINE_LEN);
		return(1);
	}

	if (strncmp("history", comm, strlen(comm)) == 0)
	{
		for (i = btty.hist_c; i > 0; i--)
		{
			if (btty.hist[i][0] == '\0')
				continue;
			snprintf(pbuf, B_TTY_LINE_LEN, "%i %s\n\r", i, btty.hist[i]);
			v = write(btty.fd[1], pbuf, strlen(pbuf));
		}
		snprintf(pbuf, B_TTY_LINE_LEN, ":");
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		btty.edit_p = 0;
		memset(cbuf, 0, B_TTY_LINE_LEN);
		return(0);
	}

	if (((strncmp("quit", comm, 4) == 0) || (strncmp("exit", comm, 4) == 0))
		&& (strlen(comm) == 4))
	{
		memset(pbuf, 0, B_TTY_LINE_LEN);
		snprintf(pbuf, btty.len, "quiting\n\r");
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		brightonCLIcleanup();
		return(-1);
	}

	if (strlen(comm) == 0)
	{
		snprintf(pbuf, B_TTY_LINE_LEN, "\r%s", btty.prompt);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		btty.edit_p = 0;
		memset(cbuf, 0, B_TTY_LINE_LEN);
		return(1);
	}

	if (~btty.flags & B_TTY_RAW)
		bttyManageHistory(comm);

	vargs = brightonmalloc(sizeof(size_t) * B_TTY_ARGC);
	memset(vargs, 0, sizeof(size_t) * B_TTY_ARGC);

	for (k = 0, i = 0, j = 0; k < btty.len; k++)
	{
		/* Get an arg pointer */
		if (vargs[i] == NULL)
		{
			vargs[i] = brightonmalloc(B_TTY_LINE_LEN);
			memset(vargs[i], 0, B_TTY_LINE_LEN);
		}
		/* See if it is the end of the line */
		if (comm[k] == '\0')
		{
			if ((quoted) & (j != 0))
				vargs[i][j - 1] = '"';
			vargs[i][j] = '\0';
			if (j != 0)
				i++;
			break;
		}
		if ((comm[k] == '\'') || (comm[k] == '"'))
		{
			//vargs[i][j++] = comm[k];
			if (quoted)
			{
				quoted = 0;
				vargs[i][j] = '\0';
				if (j == 0)
					break;
				if (++i >= B_TTY_ARGC)
					break;
				j = 0;
			} else
				quoted = 1;
		} else if ((comm[k] == ' ') & (quoted == 0)) {
			vargs[i][j] = '\0';
			if (j == 0)
				break;
			if (++i >= B_TTY_ARGC)
				break;
			j = 0;
		} else
			vargs[i][j++] = comm[k];
	}

	k = bttyExecute(global, i, vargs);

	switch (k) {
		case 0:
			if (btty.flags & B_TTY_DEBUG)
			{
				snprintf(pbuf, btty.len, "success: command executed\n\r");
				v = write(btty.fd[1], pbuf, strlen(pbuf));
			}
			break;
		case B_ERR_NOT_FOUND:
			snprintf(pbuf, btty.len, "failed: command not found\n\r");
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			break;
		case B_ERR_PARAM:
			snprintf(pbuf, btty.len, "failed: parameter error\n\r");
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			break;
		case B_ERR_VALUE:
			snprintf(pbuf, btty.len, "failed: parameter out of range\n\r");
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			break;
		case B_ERR_INV_ARG:
			snprintf(pbuf, btty.len, "failed: invalid argument\n\r");
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			break;
		case B_ERR_EXIT:
			snprintf(pbuf, btty.len, "requested to exit\n\r");
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			break;
		case B_ERR_EARLY:
			break;
		default:
			snprintf(pbuf, btty.len, "failed: unknown error\n\r");
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			break;
	}

	for (i = 0; vargs[i] != NULL; i++)
		brightonfree(vargs[i]);
	brightonfree(vargs);

	snprintf(btty.hist[0], btty.len, "%s", b_blank);
	if (~btty.flags & B_TTY_RAW)
	{
		snprintf(pbuf, btty.len, "\r%s\r", b_blank);
		//snprintf(pbuf, btty.len, "\r:");
		v = write(btty.fd[1], pbuf, strlen(pbuf));
	}

	btty.edit_p = 0;
	memset(cbuf, 0, B_TTY_LINE_LEN);
	memset(btty.hist[0], 0, B_TTY_LINE_LEN);

	return(k);
}

static int
bttyPlayMode(guimain *global, char ch)
{
	int v;

	if (btty.flags & B_TTY_DEBUG)
	{
		snprintf(pbuf, btty.len, "%x\r\n", ch);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
	}

	btty.acycle = btty.pcycle;

	if (ch == 0x1b)
	{
		btty.flags &= ~B_TTY_MMASK;
		btty.flags |= B_TTY_RAW;
		brightonKeyInput(SYNTHS->win, btty.key, 0);
		cliParamChange(global, 0.0f);
		btty.acycle = btty.cycle;
		return(B_ERR_OK);
	} else if (ch == ':') {
		btty.flags &= ~B_TTY_MMASK;
		btty.flags |= B_TTY_COOKED;
		brightonKeyInput(SYNTHS->win, btty.key, 0);
		snprintf(pbuf, btty.len, "%s", btty.prompt);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		btty.acycle = btty.cycle;
		return(B_ERR_OK);
	}

	/* They were the two escape codes, now we have to play keys */
	if (ch != btty.key)
	{
		brightonKeyInput(SYNTHS->win, btty.key, 0);
		brightonKeyInput(SYNTHS->win, ch, 1);
		btty.key = ch;
		btty.flags &= ~B_TTY_CLEAR_KEY;
	}

	return(B_ERR_OK);
}

static int
bttyCookedMode(guimain *global, char ch)
{
	int v, i;

	snprintf(pbuf, btty.len, "\r%s", b_blank);
	v = write(btty.fd[1], pbuf, strlen(pbuf));
	snprintf(pbuf, btty.len, "\r");
	v = write(btty.fd[1], pbuf, strlen(pbuf));

	btty.acycle = btty.cycle;

	/*
	 * Start with escape, this will later become interpretted as it could
	 * also by arrows for history functions.
	 */
	if (btty.flags & B_TTY_RAW_P2) {
		btty.flags &= ~B_TTY_MMASK;
		btty.flags |= B_TTY_COOKED;
		/*
		 * If we get back here with P2 then we have an arrow escape. Start with
		 * up and down arrow for history, then command line editing later.
		 */
		switch (ch) {
			case 0x43: /* Right */
			{
				if (cbuf[btty.edit_p] != '\0')
					btty.edit_p++;
				break;
			}
			case 0x44: /* Left */
			{
				if (--btty.edit_p < 0)
					btty.edit_p = 0;
				//snprintf(tbuf, btty.edit_p, "\r%s", btty.hist[btty.hist_tmp]);
				//v = write(btty.fd[1], tbuf, strlen(tbuf));
				break;
			}
			case 0x41: /* Up */
				if ((btty.hist_tmp < 50)
					&& (btty.hist[btty.hist_tmp + 1] != NULL)
					&& (btty.hist[btty.hist_tmp + 1][0] != '\0'))
				{
					btty.hist_tmp++;
					//memset(cbuf, 0, B_TTY_LINE_LEN);
					//memset(btty.hist[btty.hist_tmp], 0, B_TTY_LINE_LEN);
					snprintf(cbuf, btty.len, "%s", btty.hist[btty.hist_tmp]);
					snprintf(pbuf, btty.len, "%s%s", btty.prompt, cbuf);
					v = write(btty.fd[1], pbuf, strlen(pbuf));
				} else
					snprintf(cbuf, btty.len, "%s", btty.hist[btty.hist_tmp]);
				btty.edit_p = strlen(cbuf);;
				break;
			case 0x42: /* Down */
				if (btty.hist_tmp > 0)
					btty.hist_tmp--;
				snprintf(cbuf, btty.len, "%s", btty.hist[btty.hist_tmp]);
				snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
				btty.edit_p = strlen(cbuf);;
				v = write(btty.fd[1], pbuf, strlen(pbuf));
				break;
		}
	} else if (ch == 0x03) {
		/* ^C kill line
		btty.edit_p = 0;
		btty.hist_tmp = 0;
		btty.flags &= ~B_TTY_MMASK;
		btty.flags |= B_TTY_RAW_P;
		snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
		memset(cbuf, 0, B_TTY_LINE_LEN);
		btty.edit_p = 0;
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		cliParamChange(global, 0.0f);
		return(0);
		*/
		snprintf(pbuf, btty.len, "\r%s%s\n\r", btty.prompt, cbuf);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		memset(cbuf, 0, B_TTY_LINE_LEN);
		btty.edit_p = 0;
		snprintf(pbuf, btty.len, "\r%s%s", printprompt(global), cbuf);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
	} else if ((ch == 0x08) || (ch == 0x7f)) {
		int c;
		/* BS/DEL */
		if (btty.edit_p == 0)
		{
			if (btty.flags & B_TTY_SEARCH)
			{
				btty.flags &= ~B_TTY_MMASK;
				btty.flags |= B_TTY_RAW;
				snprintf(pbuf, btty.len, "\r/");
				v = write(btty.fd[1], pbuf, strlen(pbuf));
				cliParamChange(global, 0.0f);
				return(1);
			}
			//snprintf(pbuf, btty.len, "\r:");
			snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
			//pbuf[1] = btty.delim;
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			return(0);
		}

		btty.edit_p--;

		for (c = btty.edit_p; c < strlen(cbuf); c++)
			cbuf[c] = cbuf[c + 1];

		if (btty.flags & B_TTY_COOKED)
			snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
		else if (btty.flags & B_TTY_SEARCH)
			snprintf(pbuf, btty.len, "\r/%s", cbuf);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		if (strlen(cbuf) == btty.len)
		{
			cbuf[strlen(cbuf) - 1] = '\0';
			btty.edit_p = strlen(cbuf);;
		}
	} else if ((ch == 0x0c) || (ch == 0x12)) {
		/* ^L ^R - redraw */
		snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
		pbuf[1] = btty.delim;
	} else if (ch == 0x09) {
		/* tab */
		bttyComplete(global);
	} else if (ch == 0x01) {
		/* ^A start of line */
		snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
		btty.edit_p = 0;
		v = write(btty.fd[1], pbuf, strlen(pbuf));
	} else if (ch == 0x05) {
		/* ^E end of line */
		snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
		btty.edit_p = strlen(cbuf);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
	} else if (ch == 0x0e) {
		/* Down hist ^n */
		if (btty.hist_tmp > 0)
			btty.hist_tmp--;
		snprintf(cbuf, btty.len, "%s", btty.hist[btty.hist_tmp]);
		snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
		btty.edit_p = strlen(cbuf);;
		v = write(btty.fd[1], pbuf, strlen(pbuf));
	} else if (ch == 0x10) {
		/* Up hist ^p */
		if ((btty.hist_tmp < 50)
			&& (btty.hist[btty.hist_tmp + 1] != NULL)
			&& (btty.hist[btty.hist_tmp + 1][0] != '\0'))
		{
			btty.hist_tmp++;
			//memset(cbuf, 0, B_TTY_LINE_LEN);
			//memset(btty.hist[btty.hist_tmp], 0, B_TTY_LINE_LEN);
			snprintf(cbuf, btty.len, "%s", btty.hist[btty.hist_tmp]);
			snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
			v = write(btty.fd[1], pbuf, strlen(pbuf));
		} else
			snprintf(cbuf, btty.len, "%s", btty.hist[btty.hist_tmp]);
		btty.edit_p = strlen(cbuf);;
	} else if (ch == 0x15) {
		/* ^U kill  line */
		memset(cbuf, 0, B_TTY_LINE_LEN);
		memset(btty.hist[0], 0, B_TTY_LINE_LEN);
		btty.edit_p = 0;
		btty.hist_tmp = 0;
	} else if (ch == 0x17) {
		int t = btty.edit_p - 1;
		int o = btty.edit_p;
		/* ^W kill word - current edit point to start of previous word */
		snprintf(btty.hist[0], btty.len, "%s", cbuf);
		while (cbuf[t] == ' ')
		{
			if (--t == 0)
				break;
		}
		if (strlen(cbuf) != '\0')
		{
			while (t != 0)
			{
				if (cbuf[t] == ' ')
					break;
				t--;
			}
			if (t != 0)
				btty.edit_p = ++t;
			else
				btty.edit_p = 0;
			while ((cbuf[t++] = cbuf[o++]) != '\0')
				;

			if (btty.flags & B_TTY_COOKED)
				snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
			else
				snprintf(pbuf, btty.len, "\r/%s", cbuf);
			v = write(btty.fd[1], pbuf, strlen(pbuf));
		}
	} else if (ch == 0x1b) {
		/* Esc - we should not actually get this */
		btty.flags &= ~B_TTY_MMASK;
		btty.flags |= B_TTY_RAW_P;
		cliParamChange(global, 0.0f);
		return(1);
	} else if ((ch == 0x0a) || (ch == 0x0d)) {
		/*
		 * Enter
		if (cbuf[0] == '\0')
		{
			snprintf(pbuf, btty.len, ":\r");
			pbuf[0] = btty.delim;
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			btty.flags &= ~B_TTY_MMASK;
			btty.flags |= B_TTY_RAW_P;
			cliParamChange(global, 0.0f);
			return(1);
		}
		 */
		if (btty.flags & B_TTY_COOKED)
		{
			int i;
			snprintf(pbuf, btty.len, "\r%s%s\n\r", btty.prompt, cbuf);
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			if ((i = bttyInterpret(global, cbuf)) < 0)
			{
				brightonCLIcleanup();
				return(i);
			}
			if (i != B_ERR_EARLY)
			{
				snprintf(pbuf, btty.len, "\r%s%s", printprompt(global), cbuf);
				v = write(btty.fd[1], pbuf, strlen(pbuf));
			}
			return(i);
		}
		if (btty.flags & B_TTY_SEARCH)
		{
			if (cbuf[0] == '\0')
			{
				btty.flags &= ~B_TTY_MMASK;
				btty.flags |= B_TTY_RAW;
				snprintf(pbuf, btty.len, "\r/");
				v = write(btty.fd[1], pbuf, strlen(pbuf));
				cliParamChange(global, 0.0f);
				return(1);
			}
			snprintf(pbuf, btty.len, "/%s", cbuf);
			v = write(btty.fd[1], pbuf, strlen(pbuf));
			return(bttySearch(global));
		}
	} else {
		/* Insert char at edit point */
		for (i = strlen(cbuf) + 1; i >= btty.edit_p; i--)
			cbuf[i] = cbuf[i - 1];
		cbuf[btty.edit_p++] = ch;
	}

	if (btty.flags & B_TTY_SEARCH)
	{
		snprintf(pbuf, btty.len, "\r/%s", cbuf);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
	} else if (btty.flags & B_TTY_COOKED) {
		if (strlen(cbuf) >= 79)
			return(bttyInterpret(global, cbuf));
		snprintf(pbuf, btty.len, "\r%s%s", btty.prompt, cbuf);
		v = write(btty.fd[1], pbuf, strlen(pbuf));
	}

	/* Cursor positioning for command line editing */
	*pbuf = 0x08;
	for (i = strlen(cbuf); i > btty.edit_p; i--)
		v = write(btty.fd[1], pbuf, 1);

	return(1);
}

int
brightonCLIcheck(guimain *global)
{
	char ch = 0, chmap = B_TTY_A_NULL;
	int i = 0, v = 0, r = 0, s;

	if (btty.flags & B_TTY_INIT)
	{
		struct termios attr;

		sleep(1);

		memset(&attr, 0, sizeof(struct termios));

		tcgetattr(btty.fd[0], &resetattr);
		tcgetattr(btty.fd[0], &attr);
		cfmakeraw(&attr);
		tcsetattr(btty.fd[0], TCSANOW, &attr);

		btty.flags &= ~B_TTY_MMASK;

		if (SYNTHS->flags & REQ_DEBUG_1)
		{
			btty.p = RESOURCES->emulate.panel;
			snprintf(pbuf, btty.len, "panel: %i\r\n", btty.p);
			v = write(btty.fd[1], pbuf, strlen(pbuf));
		}

		btty.flags &= ~B_TTY_INIT;
		btty.flags |= B_TTY_RAW;
		snprintf(pbuf, btty.len, "\n\rHi %s!\n\r", getenv("USER"));
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		snprintf(pbuf, btty.len, "Bristol Command Line Interface.\r\n");
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		snprintf(pbuf, btty.len, "Cursor motion keys will access parameters.\r\n");
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		snprintf(pbuf, btty.len, "Try ':help' and ':set cli' for information");
		v = write(btty.fd[1], pbuf, strlen(pbuf));
		btty.i = 0;
	}

	FD_ZERO(stdioset);
	FD_SET(btty.fd[0], stdioset);
	timeout.tv_sec = 0;

	if ((timeout.tv_usec = btty.acycle * 1000 - 1) < 0)
		timeout.tv_usec = 100000;

	/*
	 * We have three modes here:
	 *	raw: default vi type control
	 *	insert mode: readline style entry
	 *	raw_p pending: seen escape but might be arrow keys, needs second state
	 */
	while ((s = select(2, stdioset, NULL, NULL, &timeout)) != 0)
	{
		r = read(btty.fd[0], &ch, 1);

//		pbuf[0] = ch + 96;
//		pbuf[1] = '\0';
//		print(pbuf);
		printChar(ch);

		if (ch == 0x04)
		{
			brightonCLIcleanup();
			return(-1);
		}

		switch (btty.flags & B_TTY_MMASK) {
			default:
			case B_TTY_RAW:
				if (ch == 0x1b)
				{
					/* Esc */
					btty.flags &= ~B_TTY_MMASK;
					btty.flags |= B_TTY_RAW_ESC;
					return(1);
				}
				if (ch == 0x0c)
				{
					if ((unnamed(btty.i)) && (~btty.flags & B_TTY_AWV))
						break;

					cliParamChange(global, 0.0f);
					return(0);
				}
				break;
				if (ch == 0x0d)
				{
					if (btty.i >= SYNTHS->win->app->resources[btty.p].ndevices)
						btty.i = SYNTHS->win->app->resources[btty.p].ndevices-1;

					if ((unnamed(btty.i)) && (~btty.flags & B_TTY_AWV))
						break;

					cliParamChange(global, 0.0f);
					return(0);
				}
				break;
			case B_TTY_RAW_ESC:
				if (ch == '[')
				{
					btty.flags &= ~B_TTY_MMASK;
					btty.flags |= B_TTY_RAW_ESC2;
					return(1);
				}
				break;
			case B_TTY_RAW_P:
				/* If we get 'O' go to P2 else RAW */
				btty.flags &= ~B_TTY_MMASK;
				if (ch == '[') {
					btty.flags |= B_TTY_RAW_P2;
					return(1);
				}
				btty.flags |= B_TTY_RAW;
				break;
			case B_TTY_RAW_P2:
				if ((ch == 0x41) || (ch == 0x42)
					|| (ch == 0x43) || (ch == 0x44))
					return(bttyCookedMode(global, ch));
				btty.flags &= ~B_TTY_MMASK;
				btty.flags |= B_TTY_RAW;
				break;
			case B_TTY_COOKED:
				if (ch == 0x1b)
				{
					btty.flags &= ~B_TTY_MMASK;
					btty.flags |= B_TTY_RAW_P;
					return(1);
				}
			case B_TTY_SEARCH:
				return(bttyCookedMode(global, ch));
			case B_TTY_PLAY:
				return(bttyPlayMode(global, ch));
		}

		if (btty.flags & B_TTY_RAW_ESC2)
		{
			btty.flags &= ~B_TTY_MMASK;
			btty.flags |= B_TTY_RAW;
			switch (ch) {
				case 0x41:
					ch = 'k';
					break;
				case 0x42:
					ch = 'j';
					break;
				case 0x43:
					ch = 'l';
					break;
				case 0x44:
					ch = 'h';
					break;
				case 0x0d:
				case 0x0a:
					ch = 'l';
					break;
			}
		}
		if (btty.flags & B_TTY_RAW_ESC2)
		{
			switch (ch) {
				case 0x5b:
				case 0x1b:
					btty.flags &= ~B_TTY_MMASK;
					btty.flags |= B_TTY_RAW_P;
					return(i);
			}
		}
/*
		switch (ch) {
			case 0x0d:
				if (++btty.i >= SYNTHS->mem.active)
					btty.i = SYNTHS->mem.active -1;

				if (DEVICE(bbty.i).flags & BRIGHTON_WITHDRAWN)
					break;

				cliParamChange(global, 0.0f);
				return(0);
		};
*/

		for (i = 0; i < B_TTY_ACT_COUNT; i++)
		{
			if (action[i].map == '\0')
				break;
			if (action[i].map == ch)
			{
				chmap = action[i].imap;
				/*
				 * We need to exec the template here, it is not predefined so
				 * is probably an aliased command for accelerated access
				 */
				if (chmap >= B_TTY_A_PREDEF)
				{
					i = chmap;
					print(templates[i].opname);
					snprintf(cbuf, B_TTY_LINE_LEN, "%s", templates[i].opname);
					return(bttyInterpret(global, cbuf));
				}
				break;
			}
		}

		if ((chmap == B_TTY_A_NULL) || (i == B_TTY_ACT_COUNT))
			return(1);

		switch (chmap) {
			case B_TTY_A_M_TOG:
			{
				int cmem;

				if ((SYNTHS->flags & REQ_DEBUG_MASK) > REQ_DEBUG_2)
					snprintf(pbuf, btty.len,
						"excepted Control switch memory: %s\r\n",
						RESOURCES->name);
				v = write(btty.fd[1], pbuf, strlen(pbuf));

				cmem = SYNTHS->cmem;

				if (SYNTHS->loadMemory != NULL)
					SYNTHS->loadMemory(SYNTHS,
						RESOURCES->name, 0,
						SYNTHS->lmem, SYNTHS->mem.active, 0, btty.fl);
				else
					loadMemory(SYNTHS, RESOURCES->name,
						0, SYNTHS->lmem, SYNTHS->mem.active,
						0, btty.fl);
				SYNTHS->lmem = cmem;
				break;
			}
			case B_TTY_A_M_WRITE:
				/* Save */
				SYNTHS->cmem = SYNTHS->lmem;
				if (SYNTHS->saveMemory != NULL)
					SYNTHS->saveMemory(SYNTHS,
						RESOURCES->name, 0,
						SYNTHS->location, 0);
				else
					saveMemory(SYNTHS, RESOURCES->name,
						0, SYNTHS->location, 0);
				snprintf(pbuf, btty.len, "\r\nwrite: %i", SYNTHS->location);
				v = write(btty.fd[1], pbuf, strlen(pbuf));
				break;
			case B_TTY_A_M_READ:
				/* Open */
				SYNTHS->lmem = SYNTHS->cmem;
				if (SYNTHS->loadMemory != NULL)
					SYNTHS->loadMemory(SYNTHS,
						RESOURCES->name,
						0, SYNTHS->location, SYNTHS->mem.active,
						0, btty.fl);
				else
					loadMemory(SYNTHS, RESOURCES->name,
						0, SYNTHS->location, SYNTHS->mem.active,
						0, btty.fl);
				SYNTHS->cmem = SYNTHS->location;
				snprintf(pbuf, btty.len, "read: %i\r\n", SYNTHS->location);
				v = write(btty.fd[1], pbuf, strlen(pbuf));
				break;
			case B_TTY_A_M_DOWN:
				/* Mem down */
				if (--SYNTHS->location < 0)
					SYNTHS->location = 99;
				if (loadMemory(SYNTHS, RESOURCES->name,
						0, SYNTHS->location, SYNTHS->mem.active,
						0, BRISTOL_STAT) < 0)
					snprintf(pbuf, btty.len, "free mem: %i\r\n",
						SYNTHS->location);
				else
					snprintf(pbuf, btty.len, "mem: %i\r\n", SYNTHS->location);
					v = write(btty.fd[1], pbuf, strlen(pbuf));
				break;
			case B_TTY_A_M_UP:
				/* Mem up */
				if (++SYNTHS->location > 99)
					SYNTHS->location = 0;
				if (loadMemory(SYNTHS, RESOURCES->name,
						0, SYNTHS->location, SYNTHS->mem.active,
						0, BRISTOL_STAT) < 0)
					snprintf(pbuf, btty.len, "free mem: %i\r\n",
						SYNTHS->location);
				else
					snprintf(pbuf, btty.len, "mem: %i\r\n", SYNTHS->location);
					v = write(btty.fd[1], pbuf, strlen(pbuf));
				break;
			case B_TTY_A_LEFT:
				if (--btty.i < 0)
				{
					btty.i = 0;
					break;
				}

				if ((paramname(btty.i)[0] == '\0') && (~btty.flags & B_TTY_AWV))
				{
					while (paramname(btty.i)[0] == '\0')
					{
						btty.i--;
						if (btty.i < 0)
						{
							btty.i = 0;
							return(1);
						}
					}
				}

				cliParamChange(global, 0.0f);
				break;
			case B_TTY_A_RIGHT:
				btty.i++;

				if (btty.i >= SYNTHS->win->app->resources[btty.p].ndevices)
				{
					btty.i = SYNTHS->win->app->resources[btty.p].ndevices - 1;
					break;
				}

				//if ((DEVICE(btty.i).flags & BRIGHTON_WITHDRAWN)
				if ((unnamed(btty.i)) && (~btty.flags & B_TTY_AWV))
				{
					int h = btty.i - 1;
					//while (DEVICE(btty.i).flags & BRIGHTON_WITHDRAWN)
					while (paramname(btty.i)[0] == '\0')
					{
						if (++btty.i
							>= SYNTHS->win->app->resources[btty.p].ndevices)
						{
							if ((btty.i = h) < 0)
								btty.i = 0;
							return(1);
						}
					}
				}

				cliParamChange(global, 0.0f);
				break;
			case B_TTY_A_UPDATE:
				/* refresh */
				cliParamChange(global, 0.0f);
				break;
			case B_TTY_A_DECMAX:
				/* Param down */
				cliParamChange(global, -0.1f);
				break;
			case B_TTY_A_DEC:
				/* Param down */
				cliParamChange(global, -btty.accel);
				break;
				/*
				 * Param down
				 */
			case B_TTY_A_DEC1:
				cliParamChange(global, -1.0f/16383);
				break;
			case B_TTY_A_DEC4:
				cliParamChange(global, -4.0f/16383);
				break;
			case B_TTY_A_DECMIN:
				cliParamChange(global, -0.001f);
				break;
			case B_TTY_A_INCMAX:
				/* Param up */
				cliParamChange(global, 0.1f);
				break;
			case B_TTY_A_INC:
				/* Param up */
				cliParamChange(global, btty.accel);
				break;
				/*
				 * Param up
				 */
			case B_TTY_A_INCMIN:
				cliParamChange(global, 0.001f);
				break;
			case B_TTY_A_INC1:
				cliParamChange(global, 1.0f/16383.001f);
				break;
			case B_TTY_A_INC4:
				cliParamChange(global, 4.0f/16383.001f);
				break;
			case B_TTY_A_INSERT:
				seqStop(global);
				btty.flags &= ~(B_TTY_MMASK|B_TTY_SEQ);
				btty.flags |= B_TTY_COOKED;
				btty.edit_p = 0;
				memset(cbuf, 0, B_TTY_LINE_LEN);
				snprintf(pbuf, btty.len, "\n\r%s", printprompt(global));
				btty.delim = ':';
				v = write(btty.fd[1], pbuf, strlen(pbuf));
				break;
			case B_TTY_A_FIND:
				btty.flags &= ~B_TTY_MMASK;
				btty.flags |= B_TTY_SEARCH;
				btty.edit_p = 0;
				memset(cbuf, 0, B_TTY_LINE_LEN);
				snprintf(pbuf, btty.len, "\n\r/");
				btty.delim = '/';
				v = write(btty.fd[1], pbuf, strlen(pbuf));
				break;
			default:
			case B_TTY_A_NULL:
				snprintf(pbuf, btty.len, "unimplemented char 0x%x\r\n", ch);
				if (SYNTHS->flags & REQ_DEBUG_1)
					v = write(btty.fd[1], pbuf, strlen(pbuf));
				break;
		}
		lp = PDEV(btty.i)->value;
	}

	/* if the GUI has changed the value we show here, CLI to match */
	if ((btty.flags & B_TTY_PLAY) && (btty.flags & B_TTY_PLAY_LINE))
	{
		if (btty.flags & B_TTY_CLEAR_KEY)
		{
			brightonKeyInput(SYNTHS->win, btty.key, 0);
			btty.key = 255;
		}

		btty.flags |= B_TTY_CLEAR_KEY;
	}

	if (btty.flags & B_TTY_SEQ)
	{
		if (sequences[btty.sequence.id].count > 0)
		{
			if ((btty.sequence.count -= btty.acycle) <= 0)
			{
				/* Send note off/on */
				print("note off");
				if (btty.sequence.step < 0)
					btty.sequence.step = sequences[btty.sequence.id].count;
				else if (sequences[btty.sequence.id].step[btty.sequence.step]
					> 0)
					bristolMidiSendMsg(global->controlfd, SYNTHS->midichannel,
						BRISTOL_EVENT_KEYOFF, 0,
						sequences[btty.sequence.id].step[btty.sequence.step]
						+ btty.sequence.transpose);

				if (++btty.sequence.step
					>= sequences[btty.sequence.id].count)
					btty.sequence.step = 0;

				print("note on");
				if (sequences[btty.sequence.id].step[btty.sequence.step] > 0)
					bristolMidiSendMsg(global->controlfd, SYNTHS->midichannel,
						BRISTOL_EVENT_KEYON, 0,
						sequences[btty.sequence.id].step[btty.sequence.step]
						+ btty.sequence.transpose);

				btty.sequence.count =
					250 * 60 / sequences[btty.sequence.id].rate;

				snprintf(pbuf, btty.len, "rate %i, count %i(%i), step %i\n\r",
					sequences[0].rate, btty.sequence.count, btty.acycle,
					btty.sequence.step);
				v = write(btty.fd[1], pbuf, strlen(pbuf));
			}
		} else {
			btty.flags &= ~B_TTY_SEQ;
			seqStop(global);
		}
	}

	if ((btty.flags & B_TTY_RAW)
		&& (s == 0) && (r == 0)
		&& (lp != PDEV(btty.i)->value))
	{
		lp = PDEV(btty.i)->value;
		cliParamChange(global, 0.0f);
	}

	if ((r == 0) && (btty.flags & B_TTY_RAW_P))
	{
		btty.flags &= ~B_TTY_MMASK;
		btty.flags |= B_TTY_RAW;
		cliParamChange(global, 0.0f);
	}

	return(r);
}

