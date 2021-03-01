
/*
 *  Diverse Bristol audio routines.
 *  Copyright (c) by Nick Copeland <nickycopeland@hotmail.com> 1996,2012
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *	 it under the terms of the GNU General Public License as published by
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <pthread.h>

#include "brighton.h"
#include "brightonMini.h"
#include "bristolmidi.h"
#include "brightonhelp.h"

#include "brightonKeyboards.h"

#include "brightonVImages.h"

extern brightonApp miniApp;
extern brightonApp prophetApp;
extern brightonApp hammondApp;
extern brightonApp junoApp;
extern brightonApp dxApp;
extern brightonApp explorerApp;
extern brightonApp mixApp;
extern brightonApp hammondB3App;
extern brightonApp voxApp;
extern brightonApp rhodesApp;
extern brightonApp rhodesBassApp;
extern brightonApp pro10App;
extern brightonApp prophet52App;
extern brightonApp obxApp;
extern brightonApp obxaApp;
extern brightonApp polyApp;
extern brightonApp poly6App;
extern brightonApp axxeApp;
extern brightonApp odysseyApp;
extern brightonApp memMoogApp;
extern brightonApp arp2600App;
extern brightonApp sAksApp;
extern brightonApp ms20App;
extern brightonApp solinaApp;
extern brightonApp roadrunnerApp;
extern brightonApp granularApp;
extern brightonApp realisticApp;
extern brightonApp voxM2App;
extern brightonApp jupiterApp;
extern brightonApp bitoneApp;
extern brightonApp bit99App;
extern brightonApp bit100App;
extern brightonApp masterApp;
extern brightonApp cs80App;
extern brightonApp pro1App;
extern brightonApp voyagerApp;
extern brightonApp sonic6App;
extern brightonApp trilogyApp;
extern brightonApp stratusApp;
extern brightonApp poly800App;
extern brightonApp bme700App;
extern brightonApp bmApp;
extern brightonApp sidApp;
extern brightonApp sid2App;

extern int brightonCLIcheck(guimain *);
extern int brightonCLIinit(guimain *, int *);

#define BRIGHTON_BIT99APP (BRISTOL_SYNTHCOUNT + 1)

char *bristolhome = NULL;

guimain global;

brightonApp *synthesisers[BRISTOL_SYNTHCOUNT];
#include "brightonreadme.h"

static char *defname = "bristol";

void *eventMgr();

static int readMe = 0;
static int gmc = 0;
static int opacity = 60;
static int gs = 1;
static int nar = 0;
static int azoom = 0;
static int quality = 6;
static int deswidth = -1;
static float scale = 1.0;
static float antialias = 0.0;
static int aliastype = 0;
static int library = 1;
static int dcTimeout = 500000;
static int sampleRate = 44100;
static int sampleCount = 256;
static int mwt = 50000;
static int activeSense = 2000;
static int activeSensePeriod = 15000;
static int bwflags = BRIGHTON_POST_WINDOW;
static int rlflags = BRIGHTON_SET_RAISE|BRIGHTON_SET_LOWER;
static int nrpcc = BRIGHTON_NRP_COUNT;
static int cli = 0;
static int cli_fd[2];

static char defaultcdev[16];

extern int vuInterval;
void printBrightonHelp(int);
void printBrightonReadme();

extern int brightonMidiInput(bristolMidiMsg *);
extern void brightonControlKeyInput(brightonWindow *, int, int);

/*
 * Some of the emulations really benefit from a bit of detune, this is the
 * value they will use unless another has been requested.
 */
#define NON_DEF_DETUNE 100

static int asc, emStart = 0, midiHandle = -1;

volatile sig_atomic_t ladiRequest = 0;

static void
savehandler()
{
	if (global.synths->flags & LADI_ENABLE && global.synths->ladimode)
		ladiRequest = 1;
	else
		ladiRequest = 0;
}

static void
loadhandler()
{
	if (global.synths->flags & LADI_ENABLE && global.synths->ladimode)
		ladiRequest = 2;
	else
		ladiRequest = 0;
}

static void
printSummaryText()
{
	int i;

	for (i = 0 ; i < BRISTOL_SYNTHCOUNT; i++)
	{
		if ((synthesisers[i] != NULL)
			&& (synthesisers[i]->name != NULL)
			&& (synthesisers[i]->name[0] != '\0'))
			printf("%s ", synthesisers[i]->name);
	}
	printf("\n");
}

static void
brightonFindEmulation(int argc, char **argv)
{
	int argCount, i, found = -1;

	for (argCount = 1; argCount < argc; argCount++)
	{
		if (argv[argCount][0] != '-')
			continue;

		for (i = 0 ; i < BRISTOL_SYNTHCOUNT; i++)
		{
			if (synthesisers[i] == NULL)
				continue;

			/* Check for a few aliases */
			if (strcmp("-2600", argv[argCount]) == 0)
				found = global.synths->synthtype = BRISTOL_2600;
			if (strcmp("-mg1", argv[argCount]) == 0)
				found = global.synths->synthtype = BRISTOL_REALISTIC;
			if (strcmp("-bme700", argv[argCount]) == 0)
				found = global.synths->synthtype = BRISTOL_BME700;
			if (strcmp("-pro10", argv[argCount]) == 0)
				found = global.synths->synthtype = BRISTOL_PROPHET10;
			if (strcmp("-pro52", argv[argCount]) == 0)
				found = global.synths->synthtype = BRISTOL_PRO52;
			if ((strcmp("-poly6", argv[argCount]) == 0)
				|| (strcmp("-polysix", argv[argCount]) == 0))
				found = global.synths->synthtype = BRISTOL_POLY6;
			if ((strcmp("-memmoog", argv[argCount]) == 0)
				|| (strcmp("-memorymoog", argv[argCount]) == 0))
				found = global.synths->synthtype = BRISTOL_MEMMOOG;

			if ((strcmp("-emulate", argv[argCount]) == 0)
				&& (argCount < (argc - 1)))
			{
				if (strcmp(synthesisers[i]->name, argv[argCount + 1]) == 0)
					found = global.synths->synthtype = i;
			} else {
				if (strcmp(synthesisers[i]->name, &argv[argCount][1]) == 0)
					found = global.synths->synthtype = i;
			}
		}
	}

	/*
	 * Global.synths->synthtype will now either be the last emulator found or
	 * will be the usual default (b3). Stuff this back into i, that is nicely
	 * confusing but makes the lines shorter. Hm.
	 */
	i = global.synths->synthtype;

	if (found > 0)
		printf("%s emulation defaults:\n", synthesisers[i]->name);
	else {
		printf("emulation defaults:\n");
	}

	if ((global.synths->voices = synthesisers[i]->emulate.voices) < 0)
	{
		printf("    -voices  %i\n", BRISTOL_VOICECOUNT);
		global.synths->voices = BRISTOL_VOICECOUNT;
	} else
		printf("    -voices  %i\n",synthesisers[i]->emulate.voices);

	if (global.synths->voices == 1)
	{
		printf("    -retrig\n");
		printf("    -lvel\n");
		printf("    -hnp\n");
		printf("    -wwf\n");
		global.synths->lwf = 2;
		global.synths->legatovelocity = 1;
		global.synths->notepref = BRIGHTON_HNP;
		global.synths->notetrig = 1;
	}

	if (i == BRISTOL_2600)
		printf("    -multi   4\n");
	printf("    -detune  %i\n", synthesisers[i]->emulate.detune);
	printf("    -gain    %i\n",	synthesisers[i]->emulate.gain);
	printf("    -pwd     %i\n",	synthesisers[i]->emulate.pwd);
	printf("    -glide   %i\n",	synthesisers[i]->emulate.glide);
	printf("    -curve   %i\n",	synthesisers[i]->emulate.velocity);

	global.synths->detune = synthesisers[i]->emulate.detune;
	global.synths->gain = synthesisers[i]->emulate.gain * 16;
	global.synths->pwd = synthesisers[i]->emulate.pwd;
	global.synths->glide = synthesisers[i]->emulate.glide;
	global.synths->velocity = synthesisers[i]->emulate.velocity;
	if (synthesisers[i]->emulate.opacity != 0)
		printf("    -opacity %i\n",
			opacity = synthesisers[i]->emulate.opacity);
}

/*
 * Need to make this multithreaded?
 */
int
main(int argc, char **argv)
{
	int argCount = 1, i, j, logtype = BRISTOL_LOG_BRIGHTON;
	pthread_t emgrThread, logthread;
	char appname[65], *devname = defname;
	int reqW = 0, reqH = 0, reqX = 0, reqY = 0;

	signal(SIGINT, cleanupBristol);
	signal(SIGPIPE, cleanupBristolQuietly);
	signal(SIGHUP, cleanupBristol);
	signal(SIGUSR1, savehandler);
	signal(SIGUSR2, loadhandler);

	global.home = getenv("BRISTOL");

	global.synths = (guiSynth *) brightonmalloc(sizeof(guiSynth));

	global.controlfd = -1;
	global.voices = BRISTOL_VOICECOUNT;
	global.synths->voices = BRISTOL_VOICECOUNT;
	global.synths->pwd = 2;
	global.synths->gain = 32;
	global.synths->ladimem = 1024;
	global.synths->synthtype = BRISTOL_HAMMONDB3;
	global.synths->velocity = 510;  /* linear tracking */
	global.synths->glide = 5;  /* maximum glide delay - up to 30s */
	global.synths->detune = 40;
	global.synths->lowkey = 0;
	global.synths->highkey = 127;
	global.synths->lwf = 0; //3;
	global.synths->mbi = 0;
	global.synths->notepref = BRIGHTON_POLYPHONIC;
	global.synths->notetrig = 0;
	global.synths->win = NULL;
	global.synths->flags =
		REQ_FORWARD|REQ_LOCAL_FORWARD|REQ_REMOTE_FORWARD|NO_LATCHING_KEYS;
	global.port = 5028;
	snprintf(defaultcdev, 16, "localhost");
	global.host = defaultcdev;

	memset(synthesisers, 0, sizeof(synthesisers));

	synthesisers[BRISTOL_MINI] = &miniApp;
	synthesisers[BRISTOL_PROPHET] = &prophetApp;
	synthesisers[BRISTOL_PRO52] = &prophet52App;
	synthesisers[BRISTOL_HAMMOND] = &hammondB3App;
	synthesisers[BRISTOL_JUNO] = &junoApp;
	synthesisers[BRISTOL_DX] = &dxApp;
	synthesisers[BRISTOL_EXPLORER] = &explorerApp;
	synthesisers[BRISTOL_HAMMONDB3] = &hammondB3App;
	synthesisers[BRISTOL_VOX] = &voxApp;
	synthesisers[BRISTOL_RHODES] = &rhodesApp;
	synthesisers[BRISTOL_RHODES_BASS] = &rhodesBassApp;
	synthesisers[BRISTOL_PROPHET10] = &pro10App;
	synthesisers[BRISTOL_MIXER] = &mixApp;
	synthesisers[BRISTOL_OBX] = &obxApp;
	synthesisers[BRISTOL_OBXA] = &obxaApp;
	synthesisers[BRISTOL_POLY] = &polyApp;
	synthesisers[BRISTOL_POLY6] = &poly6App;
	synthesisers[BRISTOL_AXXE] = &axxeApp;
	synthesisers[BRISTOL_ODYSSEY] = &odysseyApp;
	synthesisers[BRISTOL_MEMMOOG] = &memMoogApp;
	synthesisers[BRISTOL_2600] = &arp2600App;
	synthesisers[BRISTOL_SAKS] = &sAksApp;
	synthesisers[BRISTOL_MS20] = &ms20App;
	synthesisers[BRISTOL_SOLINA] = &solinaApp;
	synthesisers[BRISTOL_ROADRUNNER] = &roadrunnerApp;
	synthesisers[BRISTOL_GRANULAR] = &granularApp;
	synthesisers[BRISTOL_REALISTIC] = &realisticApp;
	synthesisers[BRISTOL_VOXM2] = &voxM2App;
	synthesisers[BRISTOL_JUPITER8] = &jupiterApp;
	synthesisers[BRISTOL_BIT_ONE] = &bitoneApp;
	synthesisers[BRISTOL_MASTER] = &masterApp;
	synthesisers[BRISTOL_CS80] = &cs80App;
	synthesisers[BRISTOL_PRO1] = &pro1App;
	synthesisers[BRISTOL_VOYAGER] = &voyagerApp;
	synthesisers[BRISTOL_SONIC6] = &sonic6App;
	synthesisers[BRISTOL_TRILOGY] = &trilogyApp;
	synthesisers[BRISTOL_STRATUS] = &stratusApp;
	synthesisers[BRISTOL_POLY800] = &poly800App;
	synthesisers[BRISTOL_BME700] = &bme700App;
	synthesisers[BRISTOL_BASSMAKER] = &bmApp;
	synthesisers[BRISTOL_SID_M1] = &sidApp;
	synthesisers[BRISTOL_SID_M2] = &sid2App;

	readme[BRISTOL_PRO52] = readme[BRISTOL_PROPHET];
	readme[BRISTOL_VOYAGER] = readme[BRISTOL_EXPLORER];

	if (argc == 1)
	{
		printf("You probably prefer to use the startBristol script.\n");
		exit(0);
	}

	if (strcmp(argv[1], "-console") == 0)
		logtype = BRISTOL_LOG_CONSOLE;

	if (strcmp(argv[1], "-readme") == 0)
	{
		readMe = 1;
		logtype = BRISTOL_LOG_CONSOLE;
		if (argc == 2)
		{
			printBrightonReadme();
			exit(0);
		}
	}

	if (((strcmp(argv[argCount], "-V") == 0)
		|| (strcmp(argv[argCount], "-version") == 0))
		&& (strlen(argv[argCount]) == 2))
	{
		printf("Version %s\n", VERSION);
		exit(0);
	}

	if ((argc > 1) && (strcmp(argv[1], "--summary") == 0))
	{
		printSummaryText();
		exit(0);
	}
	if ((argc > 1) && (strcmp(argv[1], "-summary") == 0))
	{
		printf("%s\n", summarytext);
		exit(0);
	}

	if (argc < 2) {
		printf("You probably prefer to use the startBristol script.\n");
		exit(1);
	}

	if ((argc == 2) 
		&& ((strcmp(argv[argCount], "--help") == 0)
		|| (strcmp(argv[argCount], "-help") == 0)
		|| (strcmp(argv[argCount], "-h") == 0)
		|| (strcmp(argv[argCount], "--h") == 0)))
	{
#ifdef BRISTOL_BUILD
		if (BRISTOL_BUILD == 0)
			printf("bristol %i.%i.%i: ",
				BRISTOL_MAJOR, BRISTOL_MINOR, BRISTOL_PATCH);
		else
			printf("bristol %i.%i.%i-%i: ",
				BRISTOL_MAJOR, BRISTOL_MINOR, BRISTOL_PATCH, BRISTOL_BUILD);
#else
			printf("bristol %s\n", VERSION);
#endif
		printf("%s", helptext);
		exit(0);
	}

	if (readMe == 0)
		printf("%s", gplnotice);

	global.synths->location = 0;

	if ((getenv("BRISTOL_LOG_CONSOLE") != NULL)
		&& (strcmp(getenv("BRISTOL_LOG_CONSOLE"), "true") == 0))
		logtype = BRISTOL_LOG_CONSOLE;

	cli_fd[0] = 0;//dup(0);
	cli_fd[1] = 1;//dup(1);

	logthread = bristolOpenStdio(logtype);

	/* Set up a default SYSEX ID, may get changed later */
	bristolMidiOption(0, BRISTOL_NRP_SYSID_H, 0x534C);
	bristolMidiOption(0, BRISTOL_NRP_SYSID_L, 0x6162);

	brightonFindEmulation(argc, argv);

	/*
	 * close our standard input, create a pipe which will never be used.
	 */
	while (argc > argCount)
	{
		if ((strcmp(argv[argCount], "-log") == 0)
			&& (logtype != BRISTOL_LOG_CONSOLE))
			bristolOpenStdio(BRISTOL_LOG_DAEMON);

		if ((strcmp(argv[argCount], "-syslog") == 0)
			&& (logtype != BRISTOL_LOG_CONSOLE))
			bristolOpenStdio(BRISTOL_LOG_SYSLOG);

		if ((strcmp(argv[argCount], "-V") == 0)
			&& (strlen(argv[argCount]) == 2))
		{
#ifdef BRISTOL_BUILD
			if (BRISTOL_BUILD == 0)
				printf("brighton version %i.%i.%i\n",
					BRISTOL_MAJOR, BRISTOL_MINOR, BRISTOL_PATCH);
			else
				printf("brighton version %i.%i.%i-%i\n",
					BRISTOL_MAJOR, BRISTOL_MINOR, BRISTOL_PATCH, BRISTOL_BUILD);
#endif
			exit(0);
		}

		if (strcmp(argv[argCount], "-libtest") == 0)
			global.libtest = 1 - global.libtest;

		if (strcmp(argv[argCount], "-pixmap") == 0)
			library = 0;

		if ((strcmp(argv[argCount], "-synth") == 0)
			&& (argCount < argc))
		{
			int i;

			for (i = 0 ; i < BRISTOL_SYNTHCOUNT; i++)
			{
				if (synthesisers[i] == NULL)
					continue;

				if (strcmp(synthesisers[i]->name, argv[argCount + 1]) == 0)
				{
					printf("found %s at %i\n", synthesisers[i]->name, i);
					global.synths->synthtype = i;
					break;
				}
			}

			if (i == BRISTOL_SYNTHCOUNT) {
				printf("Could not find synth named \"%s\"\n",
					argv[argCount + 1]);
				exit(-1);
			} else {
				/*
				 * We found the emulation, now we should put in the default
				 * from its application definition.
				 */
				printf("emulation defaults:\n");
				printf("	-voices  %i\n", synthesisers[i]->emulate.voices);
				printf("	-detune: %i\n", synthesisers[i]->emulate.detune);
				printf("	-gain:   %i\n",	synthesisers[i]->emulate.gain);
				printf("	-pwd:    %i\n",	synthesisers[i]->emulate.pwd);
				printf("	-glide:  %i\n",	synthesisers[i]->emulate.glide);
				printf("	-curve:  %i\n",	synthesisers[i]->emulate.velocity);

				if ((global.synths->voices = synthesisers[i]->emulate.voices)
					< 0)
					global.synths->voices = BRISTOL_VOICECOUNT;
				global.synths->detune = synthesisers[i]->emulate.detune;
				global.synths->gain = synthesisers[i]->emulate.gain * 128;
				global.synths->pwd = synthesisers[i]->emulate.pwd;
				global.synths->glide = synthesisers[i]->emulate.glide;
				global.synths->velocity = synthesisers[i]->emulate.velocity;
				if (synthesisers[i]->emulate.opacity != 0)
					printf("    -opacity:%i\n",
						opacity = synthesisers[i]->emulate.opacity);
				if (global.synths->voices == 1)
					global.synths->notepref = BRIGHTON_HNP;
				devname = argv[argCount + 1];
			}
			argCount++;
			continue;
		}

		if (((strcmp(argv[argCount], "-geometry") == 0)
			|| (strcmp(argv[argCount], "-geom") == 0))
			&& (argCount < argc))
		{
			int i = 0;
			char *yp;

			argCount++;

			/*
			 * We want WxH+x+y
			 * If we only have +-x+-y then don't apply w.h
			 * If we have H+x+y then use H as a width and use it to scale
			 * the image to a given bitmap. If we have W+H then automatically
			 * set -ar.
			 */

			/*
			 * Start looking for width
			 */
			while ((argv[argCount][i] != '-')
				&& (argv[argCount][i] != '+')
				&& (argv[argCount][i] != '\0')
				&& (argv[argCount][i] != 'x'))
			{
				reqW = reqW * 10 + argv[argCount][i] - 48;
				i++;
			}

			if (argv[argCount][i] == 'x')
				i++;

			/* Look for height */
			while ((argv[argCount][i] != '-')
				&& (argv[argCount][i] != '+')
				&& (argv[argCount][i] != '\0')
				&& (argv[argCount][i] != 'x'))
			{
				reqH = reqH * 10 + argv[argCount][i] - 48;
				i++;
			}

			/* Take an X if it is available */
			if (argv[argCount][i] != '\0')
				reqX = atoi(&argv[argCount][i]);
			if ((reqX == 0) && (argv[argCount][i] == '-'))
				reqX = -1;

			if ((yp = strchr(&argv[argCount][++i], '+')))
				reqY = atoi(yp);
			else if ((yp = strchr(&argv[argCount][++i], '-')))
				reqY = atoi(yp);
			if ((reqY == 0) && (argv[argCount][i] == '-'))
				reqY = -1;
		}

		if (strcmp(argv[argCount], "-iconify") == 0)
			bwflags &= ~BRIGHTON_POST_WINDOW;

		if ((strcmp(argv[argCount], "-quality") == 0) && (argCount < argc))
		{
			if ((quality = atoi(argv[argCount + 1])) < 2)
				quality = 2;
			else if (quality > 8)
				quality = 8;
			argCount++;
			continue;
		}

		if ((strcmp(argv[argCount], "-aliastype") == 0) && (argCount < argc))
		{
			/*
			 * I want this to take a text value and if not recognised then
			 * an integer. Text will be 'texture' or 'all'.
			 */
			if (strcmp(argv[argCount + 1], "texture") == 0) {
				aliastype = 1;
				argCount++;
				if (antialias == 0.0)
					antialias = 0.3;
				continue;
			} else if (strcmp(argv[argCount + 1], "pre") == 0) {
				aliastype = 5;
				argCount++;
				if (antialias == 0.0)
					antialias = 0.3;
				continue;
			} else if (strcmp(argv[argCount + 1], "all") == 0) {
				aliastype = 3;
				argCount++;
				if (antialias == 0.0)
					antialias = 0.3;
				continue;
			} else if ((aliastype = atoi(argv[argCount + 1])) < 0) {
				aliastype = 0;
				continue;
			} else if (aliastype > 4) {
				aliastype = 0;
				continue;
			}
			argCount++;
			continue;
		}

		if ((strcmp(argv[argCount], "-antialias") == 0) && (argCount < argc))
		{
			if ((antialias = (float) atoi(argv[argCount + 1])) < 0)
				antialias = 0.0;
			else if (antialias > 99)
				antialias = 0.01;
			else
				antialias = 1.0 - antialias / 100.0;

			if (aliastype == 0)
				aliastype = 1;

			argCount++;
			continue;
		}

		if ((strcmp(argv[argCount], "-jsmuuid") == 0) && (argCount < argc))
		{
			global.synths->ladimem += atoi(argv[argCount + 1]);
			argCount++;
			continue;
		}

		/*
		 * LADI level 1 options:
		 */
		if ((strcmp(argv[argCount], "-ladi") == 0) && (argCount < argc))
		{
			global.synths->flags |= LADI_ENABLE;

			if (strcmp(argv[argCount + 1], "both") == 0)
				global.synths->ladimode = 1;
			else if (strcmp(argv[argCount + 1], "bristol") == 0)
				global.synths->ladimode = 0;
			else if (strcmp(argv[argCount + 1], "brighton") == 0)
				global.synths->ladimode = 1;
			else {
				if ((global.synths->ladimem = atoi(argv[argCount + 1])) <= 0)
				{
					global.synths->ladimem = 1024;
					global.synths->ladiStateFile = argv[argCount + 1];
				}
			}

			/*
			 * We also need to force the startup to open this memory location
			 * as well, that can get overridden if there is a -load option 
			 * anywhere later on the command line.
			 */
			global.synths->location = global.synths->ladimem;

			global.synths->mbi = (global.synths->location / 1000) * 1000;
			global.synths->location = global.synths->location % 1000;

			argCount++;
			continue;
		}

		if ((strcmp(argv[argCount], "-audiodev") == 0) && (argCount < argc))
		{
			argCount++;
			devname = argv[argCount];
		}
		if ((strcmp(argv[argCount], "-register") == 0)
			&& (argCount < argc)
			&& (argv[argCount + 1][0] != '-'))
		{
			argCount++;
			devname = argv[argCount];
		}

		if ((strcmp(argv[argCount], "-lowkey") == 0) && (argCount < argc))
		{
			argCount++;
			global.synths->lowkey = atoi(argv[argCount]);
		}

		if ((strcmp(argv[argCount], "-highkey") == 0) && (argCount < argc))
		{
			argCount++;
			global.synths->highkey = atoi(argv[argCount]);
		}

		if (strcmp(argv[argCount], "-rate") == 0)
		{
			if ((sampleRate = atoi(argv[argCount + 1])) <= 0)
				sampleRate = 44100;

			argCount++;
			continue;
		}

		if (strcmp(argv[argCount], "-count") == 0)
		{
			if ((sampleCount = atoi(argv[argCount + 1])) <= 0)
				sampleCount = 256;

			argCount++;

			continue;
		}

		if (((strcmp(argv[argCount], "-activesense") == 0)
			|| (strcmp(argv[argCount], "-as") == 0))
			&& (argCount < argc))
		{
			activeSense = atoi(argv[argCount + 1]);

			if ((activeSense != 0) && (activeSense < 50))
				activeSense = 50;

			/*
			 * This is now set to the default timer to pass to the engine
			 * It is defaulted to three times the update rate. We should do
			 * a quick check to ensure this is larger than the sample period
			 * count by a little bit but we can only do that later.
			 */
			if (activeSense > 5000)
				activeSense = 5000;

			if ((activeSensePeriod = activeSense * 3) > 16000)
				activeSensePeriod = 16000;

			argCount++;
		}

		if ((strcmp(argv[argCount], "-ast") == 0)
			&& (argCount < argc))
		{
			if ((activeSensePeriod = atoi(argv[argCount + 1])) < 50)
				activeSensePeriod = 50;

			if (activeSensePeriod < activeSense * 2)
				activeSensePeriod = activeSense * 3;

			if (activeSensePeriod > 16000)
				activeSensePeriod = 16000;

			/*
			 * This is now set to the default timer to pass to the engine
			 * It is defaulted to three times the update rate.
			activeSensePeriod = activeSensePeriod * sampleRate / 1000;
			 */

			argCount++;
		}

		if (strcmp(argv[argCount], "-gmc") == 0)
			gmc = 1;

		if ((strcmp(argv[argCount], "-mct") == 0)
			&& (argCount < argc))
		{
			if ((mwt = atoi(argv[argCount + 1]) * 1000) < 0)
				mwt = 50000;
			argCount++;
		}

		if ((strcmp(argv[argCount], "-dct") == 0)
			&& (argCount < argc))
		{
			if ((dcTimeout = atoi(argv[argCount + 1]) * 1000) < 10000)
				dcTimeout = 250000;

			argCount++;
		}

		if ((strcmp(argv[argCount], "-opacity") == 0) && (argCount < argc))
		{
			if ((opacity = atoi(argv[argCount + 1])) < 20)
				opacity = 20;
			else if (opacity > 100)
				opacity = 100;
			argCount++;
			continue;
		}

		if ((strcmp(argv[argCount], "-width") == 0) && (argCount < argc))
		{
			deswidth = atoi(argv[argCount + 1]);
			argCount++;
		}

		if ((strcmp(argv[argCount], "-scale") == 0) && (argCount < argc))
		{
			if ((strcmp(argv[argCount + 1], "fs") == 0)
				|| (strcmp(argv[argCount + 1], "fullscreen") == 0))
			{
				aliastype = 5;
				scale = 100.0;
				argCount++;
				continue;
			}
			if ((scale = atof(argv[argCount + 1])) > 0)
			{
				if ((scale > 1.1) || (scale < 0.9))
					aliastype = 5;
				else
					aliastype = 0;
				argCount++;
				continue;
			}
			scale = 1.0;
		}

		if (strcmp(argv[argCount], "-raise") == 0)
			rlflags &= ~BRIGHTON_SET_RAISE;
		if (strcmp(argv[argCount], "-lower") == 0)
			rlflags &= ~BRIGHTON_SET_LOWER;

		if ((strcmp(argv[argCount], "-rotaryUD") == 0)
			|| (strcmp(argv[argCount], "-rud") == 0))
			rlflags |= BRIGHTON_ROTARY_UD;

		if (strcmp(argv[argCount], "-autozoom") == 0)
		{
			azoom = 1;
			if (deswidth < 1)
				deswidth = 100;
		}

		if ((strcmp(argv[argCount], "-ar") == 0) ||
			(strcmp(argv[argCount], "-nar") == 0) ||
			(strcmp(argv[argCount], "-aspect") == 0))
			nar = 1;

		if (((strcmp(argv[argCount], "-gs") == 0) ||
			(strcmp(argv[argCount], "-grayscale") == 0) ||
			(strcmp(argv[argCount], "-greyscale") == 0)) && (argCount < argc))
		{
			gs = atoi(argv[argCount + 1]);
			argCount++;
		}

		if ((strcmp(argv[argCount], "-detune") == 0) && (argCount < argc))
		{
			if (argCount < argc)
				global.synths->detune = atoi(argv[argCount + 1]);
			argCount++;
		}

		if ((strcmp(argv[argCount], "-gain") == 0) && (argCount < argc))
		{
			if (argCount < argc)
				global.synths->gain = atoi(argv[argCount + 1]) * 128;
			argCount++;
		}

		if ((strcmp(argv[argCount], "-pwd") == 0) && (argCount < argc))
		{
			if (argCount < argc)
				global.synths->pwd = atoi(argv[argCount + 1]);
			argCount++;
		}
/* Need to add in PitchWheel Depth (done), Find and Coarse tuning*/

		if (strcmp(argv[argCount], "-neutral") == 0)
			global.synths->location = -1;

		if ((strcmp(argv[argCount], "-load") == 0) && (argCount < argc))
		{
			if (argCount < argc)
				global.synths->location = atoi(argv[argCount + 1]);

			global.synths->mbi = (global.synths->location / 1000) * 1000;
			global.synths->location = global.synths->location % 1000;

			argCount++;
		}

		/* SYSEX System ID */
		if ((strcmp(argv[argCount], "-sysid") == 0) && (argCount < argc))
		{
			unsigned int sysid = 0x534C6162, o = argCount + 1;

			/*
			 * We will get 0x534C6162 for example
			 */
			if ((argv[o][0] == '0') || (argv[o][1] == 'x')
				|| (strlen(argv[o]) == 10))
			{
				sscanf(argv[o], "0x%x", &sysid);
				sysid &= 0x7f7f7f7f;
				bristolMidiOption(0, BRISTOL_NRP_SYSID_H, sysid >> 16);
				bristolMidiOption(0, BRISTOL_NRP_SYSID_L, sysid & 0x0000ffff);
				printf("fixing sysex system id at 0x%x\n", sysid);
			}
		}

		if ((strcmp(argv[argCount], "-nrpcc") == 0) && (argCount < argc))
		{
			nrpcc = atoi(argv[argCount + 1]);
			printf("Reserving %i NRP controllers\n", nrpcc);
			argCount++;
		}

		/* Master Bank Index */
		if ((strcmp(argv[argCount], "-mbi") == 0) && (argCount < argc))
		{
			int mbi;

			if (argCount < argc)
				mbi = atoi(argv[argCount + 1]) * 1000;

			if ((global.synths->mbi != 0) && (global.synths->mbi != mbi))
				printf("Master bank index and loaded memory conflict (%i/%i)\n",
					mbi/1000, global.synths->mbi/1000);

			global.synths->mbi = mbi;

			argCount++;
		}

		if ((strcmp(argv[argCount], "-glide") == 0) && (argCount < argc))
		{
			if (argCount < argc)
				global.synths->glide = atoi(argv[argCount + 1]);
			if (global.synths->glide <= 0)
				global.synths->glide = 30;
			if (global.synths->glide > 30)
				global.synths->glide = 30;
			argCount++;
		}

		if ((strcmp(argv[argCount], "-voices") == 0) && (argCount < argc))
		{
			if ((atoi(argv[argCount + 1]) > 0)
				&& ((global.synths->voices = atoi(argv[argCount + 1])) == 1)
				&& (global.synths->notepref == BRIGHTON_POLYPHONIC))
				global.synths->notepref = BRIGHTON_HNP;
			else if (atoi(argv[argCount + 1]) == 0)
				global.synths->voices = BRISTOL_VOICECOUNT;
			argCount++;
		}

		if (strcmp(argv[argCount], "-mono") == 0)
		{
			global.synths->voices = 1;
			global.synths->notepref = BRIGHTON_HNP;
		}

		if (strcmp(argv[argCount], "-localforward") == 0)
			global.synths->flags &= ~REQ_LOCAL_FORWARD;
		if (strcmp(argv[argCount], "-remoteforward") == 0)
			global.synths->flags &= ~REQ_REMOTE_FORWARD;
		if (strcmp(argv[argCount], "-forward") == 0)
			global.synths->flags
				&= ~(REQ_FORWARD|REQ_LOCAL_FORWARD|REQ_REMOTE_FORWARD);

		if (strcmp(argv[argCount], "-mididbg2") == 0)
			global.synths->flags |= REQ_MIDI_DEBUG2;
		if ((strcmp(argv[argCount], "-mididbg") == 0)
			|| (strcmp(argv[argCount], "-mididbg1") == 0))
			global.synths->flags |= REQ_MIDI_DEBUG;

		if (strcmp(argv[argCount], "-debug") == 0)
		{
			if ((argCount < argc) && (argv[argCount + 1][0] != '-'))
			{
				int i;

				if ((i = atoi(argv[argCount + 1])) > 15)
					global.synths->flags |= REQ_DEBUG_MASK;
				else {
					global.synths->flags &= ~REQ_DEBUG_MASK;
					global.synths->flags |= (i << 12);
				}
			} else {
				/*
				 * If we only have -debug and no value then increment the debug 
				 * level.
				 */
				global.synths->flags = (global.synths->flags & ~REQ_DEBUG_MASK)
					| (((global.synths->flags & REQ_DEBUG_MASK) + REQ_DEBUG_1)
						& REQ_DEBUG_MASK);
			}

			if (global.synths->flags & REQ_DEBUG_MASK)
				printf("debuging level set to %i\n",
					(global.synths->flags&REQ_DEBUG_MASK)>>12);
		}

		if (strcmp(argv[argCount], "-gnrp") == 0)
			global.synths->flags ^= GUI_NRP;
		if (strcmp(argv[argCount], "-enrp") == 0)
			global.synths->flags ^= MIDI_NRP;
		if (strcmp(argv[argCount], "-nrp") == 0)
			global.synths->flags |= MIDI_NRP|GUI_NRP;

		if ((strcmp(argv[argCount], "-channel") == 0) && (argCount < argc))
		{
			if (argCount < argc)
				global.synths->midichannel = atoi(argv[argCount + 1]) - 1;
			argCount++;
		}

		if (((strcmp(argv[argCount], "-velocity") == 0)
			|| (strcmp(argv[argCount], "-curve") == 0)
			|| (strcmp(argv[argCount], "-mvc") == 0))
			&& (argCount < argc))
		{
			if (argCount < argc)
				global.synths->velocity = atoi(argv[argCount + 1]);
			argCount++;
		}

		if (strcmp(argv[argCount], "-tracking") == 0)
		{
			global.synths->flags |= NO_KEYTRACK;
			argCount++;
		}
		if (strcmp(argv[argCount], "-keytoggle") == 0)
			global.synths->flags &= ~NO_LATCHING_KEYS;

		if (strcmp(argv[argCount], "-port") == 0)
		{
			if ((argCount < (argc - 1)) && (argv[argCount + 1] != NULL))
				global.port = atoi(argv[argCount + 1]);
			argCount++;
		}

		if (strcmp(argv[argCount], "-host") == 0)
			snprintf(defaultcdev, 16, "%s", argv[++argCount]);

		if (strcmp(argv[argCount], "-window") == 0)
			synthesisers[global.synths->synthtype]->flags ^= BRIGHTON_WINDOW;

		if (strcmp(argv[argCount], "-cli") == 0)
		{
			cli = 1;
			brightonCLIinit(&global, cli_fd);
			//synthesisers[global.synths->synthtype]->flags ^= BRIGHTON_WINDOW;
		}

		if (strcmp(argv[argCount], "-nwf") == 0)
			global.synths->lwf = 0;
		if (strcmp(argv[argCount], "-lwf") == 0)
			global.synths->lwf = 1;
		if (strcmp(argv[argCount], "-lwf2") == 0)
			global.synths->lwf = 2;
		if (strcmp(argv[argCount], "-wwf") == 0)
			global.synths->lwf = 2;
		if (strcmp(argv[argCount], "-hwf") == 0)
			global.synths->lwf = 3;

		if (strcmp(argv[argCount], "-lnp") == 0)
			global.synths->notepref = BRIGHTON_LNP;
		if (strcmp(argv[argCount], "-hnp") == 0)
			global.synths->notepref = BRIGHTON_HNP;
		if (strcmp(argv[argCount], "-nnp") == 0)
			global.synths->notepref = BRIGHTON_POLYPHONIC;
		if (strcmp(argv[argCount], "-retrig") == 0)
			global.synths->notetrig = 1 - global.synths->notetrig;
		if (strcmp(argv[argCount], "-lvel") == 0)
			global.synths->legatovelocity = 1 - global.synths->legatovelocity;

		/*
		 * And finally all the different synths
		 */
		if ((strcmp(argv[argCount], "-mini") == 0)
			||(strcmp(argv[argCount], "-minimoog") == 0))
			global.synths->synthtype = BRISTOL_MINI;

		if (strcmp(argv[argCount], "-hammond") == 0)
			global.synths->synthtype = BRISTOL_HAMMONDB3;

		if (strcmp(argv[argCount], "-prophet10") == 0)
			global.synths->synthtype = BRISTOL_PROPHET10;

		if ((strcmp(argv[argCount], "-pro1") == 0)
			|| (strcmp(argv[argCount], "-proone") == 0)
			|| (strcmp(argv[argCount], "-prophet1") == 0))
		{
			global.synths->synthtype = BRISTOL_PRO1;
			opacity = 60;
		}

		if ((strcmp(argv[argCount], "-pro52") == 0)
			|| (strcmp(argv[argCount], "-prophet52") == 0))
			global.synths->synthtype = BRISTOL_PRO52;

		if ((strcmp(argv[argCount], "-prophet") == 0)
			|| (strcmp(argv[argCount], "-sequential") == 0)
			|| (strcmp(argv[argCount], "-sequentialcircuits") == 0)
			|| (strcmp(argv[argCount], "-prophet5") == 0))
			global.synths->synthtype = BRISTOL_PROPHET;

		if (strcmp(argv[argCount], "-prophet10") == 0)
			global.synths->synthtype = BRISTOL_PROPHET10;

		if (strcmp(argv[argCount], "-pro5") == 0)
			global.synths->synthtype = BRISTOL_PROPHET;

		if (strcmp(argv[argCount], "-pro10") == 0)
		{
			if (global.synths->detune == 0)
				global.synths->detune = NON_DEF_DETUNE;
			global.synths->synthtype = BRISTOL_PROPHET10;
		}

		if (strcmp(argv[argCount], "-dx") == 0)
			global.synths->synthtype = BRISTOL_DX;

		if (strcmp(argv[argCount], "-juno") == 0)
			global.synths->synthtype = BRISTOL_JUNO;

		if (strcmp(argv[argCount], "-cs80") == 0)
		{
			global.synths->synthtype = BRISTOL_CS80;
			printf("The CS80 is not an operational emulator\n");
			global.libtest = 1;
		}

		if ((strcmp(argv[argCount], "-jupiter8") == 0) ||
			(strcmp(argv[argCount], "-uranus") == 0) ||
			(strcmp(argv[argCount], "-jupiter") == 0))
		{
			if (global.synths->voices == BRISTOL_VOICECOUNT)
				global.synths->voices = 8; /* Two 4 voice Jupiters */
			if (global.synths->detune == 0)
				global.synths->detune = NON_DEF_DETUNE;
			global.synths->synthtype = BRISTOL_JUPITER8;
			opacity = 40;
		}

		if (strcmp(argv[argCount], "-sampler") == 0)
		{
			global.synths->synthtype = BRISTOL_SAMPLER;
			printf("The sampler is not an operational emulator\n");
			global.libtest = 1;
		}

		if (strcmp(argv[argCount], "-bristol") == 0)
			global.synths->synthtype = BRISTOL_BRISTOL;

		if (strcmp(argv[argCount], "-voyager") == 0)
			global.synths->synthtype = BRISTOL_EXPLORER;

		if (strcmp(argv[argCount], "-sonic6") == 0)
			global.synths->synthtype = BRISTOL_SONIC6;

		if (strcmp(argv[argCount], "-explorer") == 0)
			global.synths->synthtype = BRISTOL_EXPLORER;
		if (strcmp(argv[argCount], "-voyager") == 0)
			global.synths->synthtype = BRISTOL_VOYAGER;

		if (strcmp(argv[argCount], "-mixer") == 0)
		{
			global.libtest = 1;
			printf("The bristol mixer is not an operational emulator\n");
			global.synths->synthtype = BRISTOL_MIXER;
		}

		if (strcmp(argv[argCount], "-rhodesbass") == 0)
			global.synths->synthtype = BRISTOL_RHODES_BASS;
		else if (strcmp(argv[argCount], "-rhodes") == 0)
			global.synths->synthtype = BRISTOL_RHODES;

		if (strcmp(argv[argCount], "-vox") == 0)
			global.synths->synthtype = BRISTOL_VOX;

		if ((strcmp(argv[argCount], "-voxm2") == 0)
			|| (strcmp(argv[argCount], "-voxM2") == 0)
			|| (strcmp(argv[argCount], "-vox300") == 0))
			global.synths->synthtype = BRISTOL_VOXM2;

		if (strcmp(argv[argCount], "-ddd") == 0)
			global.synths->synthtype = BRISTOL_DDD;

		if (strcmp(argv[argCount], "-b3") == 0)
			global.synths->synthtype = BRISTOL_HAMMONDB3;

		if (strcmp(argv[argCount], "-obxa") == 0)
		{
			if (global.synths->detune == 0)
				global.synths->detune = NON_DEF_DETUNE;
			global.synths->synthtype = BRISTOL_OBXA;
			if (opacity == 40)
				opacity = 60;
		} else if (strcmp(argv[argCount], "-obx") == 0) {
			if (global.synths->detune == 0)
				global.synths->detune = NON_DEF_DETUNE;
			global.synths->synthtype = BRISTOL_OBX;
		}

		if (strcmp(argv[argCount], "-moog") == 0)
			global.synths->synthtype = BRISTOL_MINI;

		if (strcmp(argv[argCount], "-arp") == 0)
			global.synths->synthtype = BRISTOL_2600;

		if (strcmp(argv[argCount], "-roland") == 0)
			global.synths->synthtype = BRISTOL_JUNO;

		if (strcmp(argv[argCount], "-oberheim") == 0)
			global.synths->synthtype = BRISTOL_OBXA;

		if ((strcmp(argv[argCount], "-polysix") == 0)
			|| (strcmp(argv[argCount], "-korg") == 0)
			|| (strcmp(argv[argCount], "-poly6") == 0)
			|| (strcmp(argv[argCount], "-poly") == 0))
			global.synths->synthtype = BRISTOL_POLY6;

		if (strcmp(argv[argCount], "-monopoly") == 0)
			global.synths->synthtype = BRISTOL_POLY;

		if (strcmp(argv[argCount], "-poly800") == 0)
		{
			global.synths->synthtype = BRISTOL_POLY800;
			global.synths->voices = 8;
		}

		if (strcmp(argv[argCount], "-ms20") == 0)
		{
			global.synths->synthtype = BRISTOL_MS20;
			printf("The ms20 is not an operational emulator\n");
			global.libtest = 1;
		}

		if (strcmp(argv[argCount], "-bme700") == 0)
		{
			global.synths->synthtype = BRISTOL_BME700;
			global.synths->voices = 1;
			global.synths->notepref = BRIGHTON_HNP;
		}

		if ((strcmp(argv[argCount], "-bassmaker") == 0)
			|| (strcmp(argv[argCount], "-bm") == 0))
		{
			global.synths->synthtype = BRISTOL_BASSMAKER;
			global.synths->voices = 1;
		}

		if ((strcmp(argv[argCount], "-sid") == 0)
			|| (strcmp(argv[argCount], "-sidney") == 0))
		{
			global.synths->synthtype = BRISTOL_SID_M1;
			global.synths->voices = 1;
			global.synths->notepref = BRIGHTON_LNP;
			global.synths->notetrig = 1;
			antialias = 0.5;
			aliastype = 1;
			opacity = 50;
		}

		if ((strcmp(argv[argCount], "-sid2") == 0)
			|| (strcmp(argv[argCount], "-melbourne") == 0)
			|| (strcmp(argv[argCount], "-canberra") == 0)
			|| (strcmp(argv[argCount], "-perth") == 0)
			|| (strcmp(argv[argCount], "-resid") == 0)
			|| (strcmp(argv[argCount], "-asid") == 0)
			|| (strcmp(argv[argCount], "-acid") == 0))
		{
			global.synths->synthtype = BRISTOL_SID_M2;
			global.synths->voices = 1;
			global.synths->notepref = BRIGHTON_HNP;
			global.synths->notetrig = 1;
			antialias = 0.5;
			aliastype = 1;
			opacity = 50;
		}

		if (strcmp(argv[argCount], "-arp2600") == 0)
			global.synths->synthtype = BRISTOL_2600;
		if (strcmp(argv[argCount], "-2600") == 0)
			global.synths->synthtype = BRISTOL_2600;

		if (strcmp(argv[argCount], "-axxe") == 0)
			global.synths->synthtype = BRISTOL_AXXE;

		if (strcmp(argv[argCount], "-odyssey") == 0)
			global.synths->synthtype = BRISTOL_ODYSSEY;

		if (strcmp(argv[argCount], "-solina") == 0)
			global.synths->synthtype = BRISTOL_SOLINA;

		if (strcmp(argv[argCount], "-roadrunner") == 0)
			global.synths->synthtype = BRISTOL_ROADRUNNER;

		if ((strcmp(argv[argCount], "-memory") == 0)
			|| (strcmp(argv[argCount], "-memmoog") == 0)
			|| (strcmp(argv[argCount], "-memorymoog") == 0))
			global.synths->synthtype = BRISTOL_MEMMOOG;

		if ((strcmp(argv[argCount], "-mas") == 0)
			|| (strcmp(argv[argCount], "-ems") == 0)
			|| (strcmp(argv[argCount], "-aks") == 0))
		{
			global.synths->synthtype = BRISTOL_SAKS;
			printf("AKS is not an operational emulator\n");
			global.libtest = 1;
		}

		if ((strcmp(argv[argCount], "-granular") == 0)
			|| (strcmp(argv[argCount], "-quantum") == 0))
		{
			global.synths->synthtype = BRISTOL_GRANULAR;
			printf("The granular is not an operational emulator\n");
			global.libtest = 1;
		}

		if ((strcmp(argv[argCount], "-realistic") == 0)
			|| (strcmp(argv[argCount], "-mg1") == 0))
			global.synths->synthtype = BRISTOL_REALISTIC;

#define BIT1_ACTIVE 200
		/*
		 * Also add some hacks for a bit-99, same algorithm but different
		 * GUI template
		 */
		if ((strcmp(argv[argCount], "-bitone") == 0)
			|| (strcmp(argv[argCount], "-crumar") == 0)
			|| (strcmp(argv[argCount], "-bit01") == 0)
			|| (strcmp(argv[argCount], "-bit1") == 0))
		{
			int i;

			if (global.synths->detune == 0)
				global.synths->detune = NON_DEF_DETUNE;
			global.synths->synthtype = BRISTOL_BIT_ONE;

			for (i = 68; i < 77; i++)
				bit99App.resources[0].devlocn[i].flags |= BRIGHTON_WITHDRAWN;

			if (opacity == 40)
				opacity = 60;

			bit99App.resources[0].devlocn[48].flags |= BRIGHTON_WITHDRAWN;
			/*
			 * Hide the stereo button
			 */
			bit99App.resources[0].devlocn[BIT1_ACTIVE + 2].flags
				|= BRIGHTON_WITHDRAWN;
			bit99App.resources[0].devlocn[BIT1_ACTIVE + 2].x = 50;
			bit99App.resources[0].devlocn[BIT1_ACTIVE + 2].y = 50;
			/* And add the unison button */
			bit99App.resources[0].devlocn[BIT1_ACTIVE + 38].flags
				&= ~BRIGHTON_WITHDRAWN;
		}

		/*
		 * The bit-99 hack, same algorithm but different GUI template
		 */
		if (strcmp(argv[argCount], "-bit99") == 0)
		{
			synthesisers[BRISTOL_BIT_ONE] = &bit99App;
			global.synths->synthtype = BRISTOL_BIT_ONE;
			if (global.synths->detune == 0)
				global.synths->detune = NON_DEF_DETUNE;

			if (opacity == 40)
				opacity = 60;
			/*
			 * Enable a few extra parameters.
			bit99App.resources[0].devlocn[77].flags &= ~BRIGHTON_WITHDRAWN;
			bit99App.resources[0].devlocn[78].flags &= ~BRIGHTON_WITHDRAWN;
			bit99App.resources[0].devlocn[79].flags &= ~BRIGHTON_WITHDRAWN;
			bit99App.resources[0].devlocn[80].flags &= ~BRIGHTON_WITHDRAWN;
			 */
			bit99App.resources[0].devlocn[12].flags &= ~BRIGHTON_WITHDRAWN;
			bit99App.resources[0].devlocn[BIT1_ACTIVE - 1].flags
				&= ~BRIGHTON_WITHDRAWN;
			/* Show the stereo button */
			bit99App.resources[0].devlocn[BIT1_ACTIVE + 2].flags
				&= ~BRIGHTON_WITHDRAWN;
			/* And remove the unison button */
			bit99App.resources[0].devlocn[BIT1_ACTIVE + 38].flags
				|= BRIGHTON_WITHDRAWN;
		}

		/*
		 * The bit-99 black hack, same algorithm but different GUI template
		 */
		if ((strcmp(argv[argCount], "-bit99m2") == 0)
			|| (strcmp(argv[argCount], "-bit100") == 0))
		{
			synthesisers[BRISTOL_BIT_ONE] = &bit100App;
			global.synths->synthtype = BRISTOL_BIT_ONE;
			if (global.synths->detune == 0)
				global.synths->detune = NON_DEF_DETUNE;

			if (opacity == 40)
				opacity = 60;
			/*
			 * Enable a few extra parameters.
			 */
			bit100App.resources[0].devlocn[88].flags &= ~BRIGHTON_WITHDRAWN;
//			bit100App.resources[0].devlocn[74].flags |= BRIGHTON_WITHDRAWN;
//			bit100App.resources[0].devlocn[75].flags |= BRIGHTON_WITHDRAWN;
//			bit100App.resources[0].devlocn[76].flags |= BRIGHTON_WITHDRAWN;
			bit100App.resources[0].devlocn[78].flags &= ~BRIGHTON_WITHDRAWN;
			bit100App.resources[0].devlocn[79].flags &= ~BRIGHTON_WITHDRAWN;
			bit100App.resources[0].devlocn[80].flags &= ~BRIGHTON_WITHDRAWN;
			bit100App.resources[0].devlocn[12].flags &= ~BRIGHTON_WITHDRAWN;
			bit100App.resources[0].devlocn[199].flags &= ~BRIGHTON_WITHDRAWN;
			/* Show the stereo button */
			bit100App.resources[0].devlocn[BIT1_ACTIVE + 2].flags
				&= ~BRIGHTON_WITHDRAWN;
			/* And remove the unison button */
			bit100App.resources[0].devlocn[BIT1_ACTIVE + 38].flags
				|= BRIGHTON_WITHDRAWN;
			bit100App.resources[0].devlocn[BIT1_ACTIVE - 5].flags
				|= BRIGHTON_CHECKBUTTON;
		}

		if (strcmp(argv[argCount], "-trilogy") == 0)
		{
			global.synths->synthtype = BRISTOL_TRILOGY;
			global.synths->detune = 150;
			opacity = 60;
		}

		if (strcmp(argv[argCount], "-stratus") == 0)
		{
			global.synths->synthtype = BRISTOL_STRATUS;
			global.synths->detune = 150;
			opacity = 60;
		}

		argCount++;
	}

	if (strncmp("unix:", defaultcdev, 5) == 0)
	{
		if (strlen(defaultcdev) == 5)
			snprintf(defaultcdev, 16, "unix:%i", global.port);
		else
			global.port = atoi(&defaultcdev[5]);
	}

#ifndef BRIGHTON_HAS_X11
	/* Reverse the window logic */
	synthesisers[global.synths->synthtype]->flags ^= BRIGHTON_WINDOW;
#endif

	if (reqW)
	{
		/* Width is a scaler */
		float scale;

		scale = ((float) reqW) / synthesisers[global.synths->synthtype]->width;

		/* Have been given some width value */
		if (reqH)
		{
			nar = 1;
			synthesisers[global.synths->synthtype]->width = reqW;
			synthesisers[global.synths->synthtype]->height = reqH;
		} else {
			synthesisers[global.synths->synthtype]->width = reqW;
			synthesisers[global.synths->synthtype]->height *= scale;
		}

		if ((scale > 1.1) || (scale < 0.9))
			aliastype = 5;
		else
			aliastype = 0;
	}

	{
		char statfile[1024];
		struct stat statres;

		sprintf(statfile, "%s/%s.gz", getenv("BRISTOL"),
			synthesisers[global.synths->synthtype]->resources[0].image);

		if ((synthesisers[global.synths->synthtype]->resources[0].image != 0) &&
			(stat(statfile, &statres) != 0))
		{
			sprintf(statfile, "%s/%s", getenv("BRISTOL"),
				synthesisers[global.synths->synthtype]->resources[0].image);

			if (stat(statfile, &statres) != 0)
			{
				printf("unknown emulator\n");
				sleep(1);
				exit(-1);
			}
		}
	}

	if (readMe) {
		printBrightonHelp(global.synths->synthtype);
		exit(0);
	}

	printf("brighton version %s\n", VERSION);

	sleep(1);

	printf("  %s", argv[0]);
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			printf("\n    %s", argv[i]);
		else
			printf(" %s", argv[i]);
	}
	printf("\n");

	/*
	 * Hm, this is a joke really. The Tandy, Realistic, whatever, is an old
	 * synth and this mimics random slider changes. Should not really do it
	 * but its fun: the original used to lose its white caps. Once the gui is
	 * created its more work to change the image.
	 */
	if (global.synths->synthtype == BRISTOL_REALISTIC)
	{
		int cont, i;
		struct timeval now;

		gettimeofday(&now, NULL);

		srand(now.tv_usec);

		for (i = 0; i < 6; i++)
		{
			cont = rand() & 0x01f;
			if (synthesisers[BRISTOL_REALISTIC]->resources[0].devlocn[cont].device
				== 1)
			{
				synthesisers[BRISTOL_REALISTIC]->resources[0].devlocn[cont].image
					= "bitmaps/knobs/knob8.xpm";
			}
		}
	}

	if (synthesisers[global.synths->synthtype] == 0)
		exit(0);

	global.synths->resources = synthesisers[global.synths->synthtype];

	synthesisers[global.synths->synthtype]->width *= scale;
	synthesisers[global.synths->synthtype]->height *= scale;

	/*
	 * Finally go and let the event manager handle our interface. Going to 
	 * create a separate GUI thread, which will allow us to handle things like
	 * MIDI events from the engine, timed operations, etc, from there.
	 */
	if (pthread_create(&emgrThread, NULL, eventMgr, &global) != 0)
		printf("Could not create GUI thread\n");

	/*
	 * We should not detach the thread, we should wait for it later before exit
	if (pthread_detach(thread) != 0)
		printf("Could not detach GUI thread\n");
	 */

	/*
	 * win is actually set by the configuration routines, but we use it 
	 * here anyway. The configuration options are a structure, they were a
	 * list of parameters however that became unwieldy.
	 */
	if ((global.synths->flags & REQ_DEBUG_MASK) >= (REQ_DEBUG_4|REQ_DEBUG_1))
		aliastype |= BRIGHTON_LIB_DEBUG;
	synthesisers[global.synths->synthtype]->flags |= bwflags;

	/*
	 * We play around with the names here so that the window title gets filled
	 * with more information than just the emulator name. This enhancement was
	 * added for JSM but could be any Jack installation. If multiple instances
	 * of the same emulator are started then it is not clear which is which:
	 * both have the same title bar but different names in Jack. What we do here
	 * is add the Jack registration name to the title bar.
	 */
	snprintf(appname, 64, "%s (%s)",
		synthesisers[global.synths->synthtype]->name, devname);
	devname = synthesisers[global.synths->synthtype]->name;
	synthesisers[global.synths->synthtype]->name = appname;
	if ((global.synths->win =
		brightonInterface(synthesisers[global.synths->synthtype],
			quality, library, aliastype, antialias, gs, reqX, reqY)) == NULL)
	{
		printf("could not create window\n");
		exit(-1);
	}
	synthesisers[global.synths->synthtype]->name = devname;
	global.synths->resources->name = devname;

	/*
	 * We now need to decide which memory to load. There are a few choices.
	 * If we had a LADI declaration it will have been copied to both ladimem
	 * and to the emulator memory. If they are still the same then use it.
	 *
	 * If the emulator location has change then we should look and see if the
	 * LADI memory actually exists, if so, use it (overwrite the emulator
	 * memory location again). If it does not exist we will then continue to 
	 * access the emulator memory and it becomes the template for the eventual
	 * LADI state information.
	 */
	if ((global.synths->flags & LADI_ENABLE) &&
		(global.synths->ladimem
			!= (global.synths->location + global.synths->mbi)))
	{
		int memHold = global.synths->location + global.synths->mbi;

		global.synths->mbi = 0;
		global.synths->location = global.synths->ladimem;

		/* Location has changed. Stat the ladimem */
		if (loadMemory(global.synths, global.synths->resources->name, 0,
			global.synths->ladimem,
			global.synths->mem.active, 0, BRISTOL_STAT) < 0)
		{
			/* No LADI mem */
			global.synths->location = memHold;

			global.synths->mbi = (global.synths->location / 1000) * 1000;
			global.synths->location = global.synths->location % 1000;

			printf("did not find LADI memory %i, using %i\n",
				global.synths->ladimem, memHold);
		} else
			printf("using LADI state memory %i\n", global.synths->ladimem);
	}

	if (global.flags & REQ_EXIT)
	{
		printf("early termination logging thread (%i)\n", global.controlfd);

		bristolOpenStdio(BRISTOL_LOG_TERMINATE);

		if ((logthread != 0) && (pthread_join(logthread, NULL) != 0))
			return(1);

		exit(global.controlfd < 0? 1: 0);
	}

	global.synths->win->dcTimeout = dcTimeout;

	global.synths->win->flags |= rlflags;

	if (azoom) {
		global.synths->win->flags |= BRIGHTON_AUTOZOOM;

		if (deswidth >= 100)
		{
			global.synths->win->minw = deswidth;
			global.synths->win->minh =
				deswidth * global.synths->win->template->height
				/ global.synths->win->template->width;
			global.synths->win->maxw =
				synthesisers[global.synths->synthtype]->width * scale;
			global.synths->win->maxh =
				synthesisers[global.synths->synthtype]->height * scale;
		} else {
			global.synths->win->minw 
				= global.synths->win->maxw 
				= global.synths->win->minh 
				= global.synths->win->maxh = 0;
		}
	}
	if (nar)
		global.synths->win->flags |= BRIGHTON_NO_ASPECT;

	brightonOpacity(global.synths->win, ((float) opacity) / 100.0f);

	/*
	 * These should be synth specific?
	 */
	for (i = 0; i < 128; i++)
	{
		global.synths->win->midimap[i] = i;

		for (j = 0; j < 128; j++)
			global.synths->win->valuemap[i][j] = j;
	}

	printf("user r %i/%i, e %i/%i\n", getuid(), getgid(), geteuid(), getegid());

	global.synths->win->nrpcount = nrpcc;
	global.synths->win->nrpcontrol =
		malloc(sizeof(brightonNRPcontrol) * nrpcc);
	memset(global.synths->win->nrpcontrol, 0,
		sizeof(brightonNRPcontrol) * nrpcc);

	brightonReadConfiguration(global.synths->win,
		synthesisers[global.synths->synthtype],
		global.synths->midichannel,
		synthesisers[global.synths->synthtype]->name,
		NULL);

	if (bristolMidiSendMsg(global.controlfd, global.synths->sid,
		BRISTOL_ACTIVE_SENSE, 0, activeSense == 0? 0:16383) != 0)
	{
		printf("Active sense transmit failure\n");
		sleep(1);
		exit(1);
	} if (global.synths->flags & REQ_MIDI_DEBUG)
		printf("Sent Active sense\n");

	if ((global.synths->sid2 > 0) &&
		(bristolMidiSendMsg(global.manualfd, global.synths->sid2,
			BRISTOL_ACTIVE_SENSE, 0, activeSense == 0? 0:16383) != 0))
	{
		printf("Active sense transmit failure\n");
		sleep(1);
		exit(1);
	} if (global.synths->flags & REQ_MIDI_DEBUG)
		printf("Sent Active sense on second channel\n");

	emStart = 1;

	if (global.libtest != 1)
	{
		if (global.synths->flags & REQ_FORWARD)
			bristolMidiOption(0, BRISTOL_NRP_FORWARD, 1);

		if (global.synths->flags & NO_KEYTRACK)
			bristolMidiOption(0, BRISTOL_NRP_MIDI_GO, 0);
		else
			bristolMidiOption(0, BRISTOL_NRP_MIDI_GO, 1);

		/*
		 * The Hammond needs a note event on the upper manual to kick the
		 * gearbox into action.
		 */
		if (global.synths->synthtype == BRISTOL_HAMMONDB3)
		{
			sleep(1);
			bristolMidiSendMsg(global.controlfd, global.synths->midichannel,
				BRISTOL_EVENT_KEYON, 0, 10 + global.synths->transpose);
			usleep(50000);
			bristolMidiSendMsg(global.controlfd, global.synths->midichannel,
				BRISTOL_EVENT_KEYOFF, 0, 10 + global.synths->transpose);
		}
	}

	/* if we are CLI, somebody needs to force this to happen */
	if (synthesisers[global.synths->synthtype]->flags & BRIGHTON_WINDOW)
		brightonWorldChanged(global.synths->win, 500, 500);

	/*
	if (cli == 0)
	{
		close(cli_fd[0]);
		close(cli_fd[1]);
	}
	*/

	while (~global.flags & REQ_EXIT)
	{
		/*
		 * Do whatever we want. Will turn into a wait routine on the MIDI
		 * channel. This could be merged with the UI heartbeat code.
		 */
		if (cli) {
			if (brightonCLIcheck(&global) < 0)
				cleanupBristol();
		}
		/*
		 * This was never operational anyway:
		if (vuInterval != 0) {
			usleep(vuInterval);
			if (global.synths->flags & OPERATIONAL)
				doAlarm();
		} else if (cli == 0)
		 */
		else
			sleep(1);
	}

	if (pthread_join(emgrThread, NULL) != 0)
		printf("brighton event thread exit error\n");
	else
		printf("brighton event thread exited\n");

	/* Now that the event manager thread has exit, put key repeat back on */
	BAutoRepeat(global.synths->win->display, 1);

	printf("terminating logging thread (%i)\n", global.controlfd);

	if (global.libtest == 0)
		sleep(1);

	bristolOpenStdio(BRISTOL_LOG_TERMINATE);

	if ((logthread != 0) && (pthread_join(logthread, NULL) != 0))
		return(1);

	exit(global.controlfd < 0? 1: 0);
}

void *
eventMgr()
{
	bristolMidiMsg msg;
	int i = 50, r;
	int midiFD, cFD;

	/* This will send activesense as soon as the interface initialises */
	asc = activeSense * 4;

	printf("starting event management thread\n");

	if (global.libtest == 0)
	{
		while (global.controlfd < 0)
		{
			usleep(100000);

			if (i-- < 0)
			{
				global.flags |= REQ_EXIT;
				pthread_exit(0);
			}
		}

		/*
		 * Here we should open an ALSA SEQ interface. We want to get an index
		 * back and pass that to the event handler for selection. This should
		 * not really be a SEQ interface, it should depend on the flags given
		 * to the GUI.
		 */
		if (gmc)
		{
			if ((midiHandle = bristolMidiOpen("brighton",
				BRISTOL_CONN_SEQ|BRISTOL_RDONLY,
				-1, -1, brightonMidiInput, &global)) < 0)
				printf("Error opening midi device %s\n", "0.0");
		}
	}

	while (global.synths->win == NULL)
	{
		usleep(100000);
		printf("waiting for window creation\n");
	}

	midiFD = bristolGetMidiFD(midiHandle);
	cFD = bristolGetMidiFD(global.controlfd);

	printf("opened GUI midi handles: %i, %i\n", midiFD, cFD);

/*
	if (global.libtest != 1)
	{
		if (global.synths->flags & REQ_FORWARD)
			bristolMidiOption(0, BRISTOL_NRP_FORWARD, 1);

		if (global.synths->flags & NO_KEYTRACK)
			bristolMidiOption(0, BRISTOL_NRP_MIDI_GO, 0);
		else
			bristolMidiOption(0, BRISTOL_NRP_MIDI_GO, 1);

		bristolMidiSendMsg(global.controlfd, global.synths->midichannel,
			BRISTOL_EVENT_KEYON, 0, 10 + global.synths->transpose);
		bristolMidiSendMsg(global.controlfd, global.synths->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, 10 + global.synths->transpose);
	}
*/

	while ((global.flags & REQ_EXIT) == 0) {
		if (emStart)
			i = brightonEventMgr();
		else
			i = 0;

		/*
		 * This will now become a select on the ALSA SEQ socket looking for
		 * MIDI events. Not certain how they will be linked into the GUI at
		 * the moment. For now this is just a sleep until the ALSA SEQ interface
		 * registration code has been finalised.
		 *
		 * What we will be looking for are events on a MIDI channel, we will
		 * look for that MIDI channel in our window lists. We want to respond
		 * to key events and reflect that in the GUI optionally, but not send
		 * the key events since the engine will handle all those. We also want
		 * to look for controller values and have some configurable method to
		 * link those to our controls. Now this linkage will be done via the
		 * GUI, preferably, with the <Control><Middle Mouse><MIDI CC # motion>.
		 * Since the GUI handles this then we can dispatch events to another
		 * module that does the linkage. Need to be able to save and retrieve
		 * configurations - <Control><Middle> will go to this module, and all
		 * MIDI controller events as well, and it will make the linkage and
		 * dispatch the events.
		 *
		 * We should try and have a vkeydb type X event keymap.
		 *
		 * With certain intense events the activeSensing can fail, most notably
		 * with window resizing. Since 0.30.8 only a single window configure
		 * event is handled in any one pass of the event list to reduce this
		 * effect.
		 *
		 * This would perform better if X would give me its socket descriptors
		 * for a select operation. We should look at some point in putting
		 * this into a separate thread and using a semaphore since the 'busy'
		 * waiting is ugly.
		 *
		 * We should also look into separating the MIDI and GUI threads more,
		 * it would require a bit of internal signalling to prevent both wanting
		 * X Server access at the same time.
		 */
		 /*
		  * CHANGE TO READ ON EVERY OPENED HANDLE
		bristolMidiDevRead(cFD, &msg);
		bristolMidiDevRead(2, &msg);
		  */
		while ((r = bristolMidiDevRead(midiFD, &msg)) > 0)
			;

		if (r == BRISTOL_MIDI_CHANNEL)
		{
			if ((global.synths->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_4)
				printf("Read failed on Midi FD\n");
			global.flags |= REQ_EXIT;

			pthread_exit(0);
		}

		while ((r = bristolMidiTCPRead(&msg)) > 0)
			;

		if ((r < 0) && (global.libtest == 0))
		{
			if ((global.synths->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_4)
				printf("Read failed on TCP fd (no devices?)\n");
			global.flags |= REQ_EXIT;

			pthread_exit(0);
		}

		if (i == 0)
			usleep(mwt);

		/*
		 * We should have some 'tack' in here where we call a routine in the
		 * library that will execute any timed events that have been requested,
		 * this will cover things like flashing lights, VU metering. It will
		 * also be used to cover the midi sequencer.
		 *
		 * We should also attempt to recover lost time in graphical processing
		 * by changing mwt into a target sleep period by getting the current
		 * time and looking at the ms delta.
		 */
		brightonFastTimer(0, 0, 0, BRIGHTON_FT_CLOCK, mwt / 1000);

		if ((activeSense > 0) && ((asc -= mwt / 1000) < 0) && (emStart))
		{
			asc = activeSense;

			/*
			 * Hm, this is wrong, we should scan the whole synths list but for
			 * now we actually need to send one for each emulation, dual manual
			 * and all that.
			 * The check on send status is not really necessary, we will 
			 * probably end up seeing the SIGPIPE handler have us exit if the
			 * socket closes at the remote end.
			 */
			if (bristolMidiSendMsg(global.controlfd, global.synths->sid,
				BRISTOL_ACTIVE_SENSE, 0, activeSensePeriod) != 0)
			{
				printf("Active sense transmit failure\n");
				sleep(1);
				exit(1);
			}
			if ((global.synths->sid2 > 0) &&
				(bristolMidiSendMsg(global.manualfd, global.synths->sid2,
					BRISTOL_ACTIVE_SENSE, 0, activeSensePeriod) != 0))
			{
				printf("Active sense transmit failure\n");
				sleep(1);
				exit(1);
			}

			if ((ladiRequest == 1) && global.synths->flags & LADI_ENABLE) {
				int memHold = global.synths->cmem;

				ladiRequest = 0;

				printf("LADI save state signalled\n");

				global.synths->cmem = global.synths->ladimem;
				brightonControlKeyInput(global.synths->win, 's', 0);

				global.synths->cmem = memHold;
			}

			if ((ladiRequest == 2) && global.synths->flags & LADI_ENABLE) {
				int memHold = global.synths->cmem;

				ladiRequest = 0;

				printf("LADI load state signalled\n");

				global.synths->cmem = global.synths->ladimem;
				brightonControlKeyInput(global.synths->win, 'l', 0);

				global.synths->cmem = memHold;
			}

			/*
			 * We are going to piggyback the slow event timer onto this code.
			 * It means the slow events will be related to the activeSensing
			 * but if anybody has issues due to changing AS or its timers we
			 * can review it at that time.
			 */
			brightonSlowTimer(0, 0, BRIGHTON_ST_CLOCK);
		}
	}

	printf("brighton event manager thread exiting\n");

	global.flags |= REQ_EXIT;

	pthread_exit(0);
}

void
cleanout(void *id)
{
	if (id)
		brightonRemoveInterface(id);
	cleanupBristol();
	exit(4);
}

void
clearout(int result)
{
	/*
	 * This is not the right way to do things. We should request an exit and
	 * let all the processes do their work. The issue here is that this can
	 * get called very early in the window creation process which causes other
	 * issues if we try to close down nicely. A better option here would be
	 * to assert the result is '0'. That way we get a nice core memory dump
	 * to debug under other circumstances.
	 */
	printf("clearing out via early exit\n");
	exit(result);
}

void
printBrightonHelp(int synth)
{
	if ((synth >= BRISTOL_SYNTHCOUNT) || (readme[synth] == NULL))
		return;

	printf("\n%s\n", readme[synth]);
}

void
printBrightonReadme()
{
	int i;

	printf("\n%s\n\n\n", readmeheader);
	for (i = 0; i < BRISTOL_SYNTHCOUNT; i++)
	{
		if ((synthesisers[i] != NULL) && (readme[i] != NULL))
			printf("%s\n\n\n", readme[i]);
	}
	printf("\n%s\n", readmetrailer);
	printf("%s", helptext);
}

