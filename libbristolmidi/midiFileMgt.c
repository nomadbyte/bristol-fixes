
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

/*
 * This started out primarily to load arrays of values, they being used for
 * whatever purposes required by the caller. The midi interface uses them to
 * define the gain parameters for velocity and any other control settings.
 * The Hammond preacher algorithm uses them to load the the tonewheel settings,
 * not really a midi function however the code is reasonably general.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <pthread.h>

#include "bristolmidi.h"
#include "bristolmidiapi.h"
//#include "bristolmidiapidata.h"
#include "bristolmidieventnames.h"

#define NO_INTERPOLATE 0x01

static char *myHome = NULL;
static char tmppath[BUFSZE];

pthread_t lthread;

void
resetBristolCache(char *dir)
{
	if (dir == NULL)
		return;

	setenv("BRISTOL_CACHE", dir, 1);

	if (myHome == NULL)
		return;

	free(myHome);
	myHome = NULL;
}

/*
 * Search the file path given below to look for the desired file. Return the
 * base to that file: the bristol base irrespective of where we find the file
 * in our tree search. For two of the locations we should consider building 
 * a subtree infrastructure to allow the user to generate their own file 
 * copies.
 *
 * There are a couple of issues due to the tree structure. This might be a 
 * memory request or a profile request. One needs a valid tree the other needs
 * a valid file.
 */
char *
getBristolCache(char *file)
{
	struct stat statbuf;
	char *envcache;

	/*
	 * Find the file specified in the request. Use the search order given
	 * below. Create a local cache in either $BRISTOL_CACHE or $HOME if its
	 * tree does not exist since this will be used for our private memories.
	 *
	 * Search in the following order:
	 *
	 * 	$BRISTOL_CACHE/memory/profiles/<file> - return BRISTOL_CACHE
	 * 	$BRISTOL_CACHE/memory/<file> - return BRISTOL_CACHE
	 *	$HOME/.bristol/memory/profiles/<file> - return $HOME/.bristol
	 * 	$HOME/.bristol/memory/<file> - return $HOME/.bristol
	 *	$BRISTOL/memory/<file> - these should never fail
	 *	$BRISTOL/memory/profiles - return $BRISTOL
	 */
	if (myHome == NULL)
		myHome = (char *) calloc(BUFSZE, 1);
	else
		return(myHome);

	/*
	 * See if we have an env configured:
	 *	if so see if it exists, create a tree if necessary.
	 */
	if ((envcache = getenv("BRISTOL_CACHE")) != NULL)
	{
		sprintf(tmppath, "%s/memory/profiles/%s", envcache, file);
		if (stat(tmppath, &statbuf) == 0)
		{
			/*
			 * Path exists.
			 */
			sprintf(myHome, "%s", envcache);
			/* printf("opened BRISTOL_CACHE: %s\n", tmppath); */
			return(myHome);
		} else {
			/*
			 * Ok, path was desired but not available, build the tree if we
			 * can and then carry on searching.
			 */
			sprintf(tmppath, "%s", envcache);
			mkdir(tmppath, 0755);
			sprintf(tmppath, "%s/memory", envcache);
			mkdir(tmppath, 0755);
			sprintf(tmppath, "%s/memory/profiles", envcache);
			mkdir(tmppath, 0755);
			//printf("making %s/memory\n", envcache);
		}

		sprintf(tmppath, "%s/memory/%s", envcache, file);
		if (stat(tmppath, &statbuf) == 0)
		{
			/*
			 * Path exists.
			 */
			sprintf(myHome, "%s", envcache);
			/* printf("opened BRISTOL_CACHE: %s\n", tmppath); */
			return(myHome);
		}

		//printf("1 create cache %s\n", tmppath);
		mkdir(tmppath, 0755);

		return(tmppath);
	}

	/*
	 * So, no cache or the file did not exist. See about our default cache
	 * $HOME/.bristol
	 */
	if ((envcache = getenv("HOME")) != NULL)
	{
		sprintf(tmppath, "%s/.bristol/memory/profiles/%s", envcache, file);
		if (stat(tmppath, &statbuf) == 0)
		{
			/*
			 * Path exists.
			 */
			sprintf(myHome, "%s/.bristol", envcache);
			/* printf("opened private profile cache: %s\n", tmppath); */
			return(myHome);
		} else {
			/*
			 * Ok, path was desired but not available, build the tree if we
			 * can and then carry on searching.
			 */
			sprintf(tmppath, "%s", envcache);
			//printf("2 create cache %s\n", tmppath);
			mkdir(tmppath, 0755);
			sprintf(tmppath, "%s/memory", envcache);
			mkdir(tmppath, 0755);
			sprintf(tmppath, "%s/memory/profiles", envcache);
			mkdir(tmppath, 0755);
		}

		sprintf(tmppath, "%s/.bristol/memory/%s", envcache, file);
		if (stat(tmppath, &statbuf) == 0)
		{
			/*
			 * Path exists.
			 */
			sprintf(myHome, "%s/.bristol", envcache);
			/* printf("opened private memory cache: %s\n", tmppath); */
			return(myHome);
		}
	}

	if ((envcache = getenv("BRISTOL")) != NULL)
	{
		sprintf(tmppath, "%s/memory/profiles/%s", envcache, file);
		if (stat(tmppath, &statbuf) == 0)
		{
			sprintf(myHome, "%s", envcache);
			/* printf("opened factory cache: %s\n", tmppath); */
			return(myHome);
		}

		sprintf(tmppath, "%s/memory/%s", envcache, file);
		if (stat(tmppath, &statbuf) == 0)
		{
			sprintf(myHome, "%s", envcache);
			/* printf("opened factory cache: %s\n", tmppath); */
			return(myHome);
		}
	}

	return(myHome);
}

int
bristolGetMap(char *file, char *match, float *points, int count, int flags)
{
	int i, n = 0, mapped = 0;
	FILE *fd;
	float from, delta;

	/*
	 * Open and read configuration. Should consider seaching
	 * $HOME/.bristol/memory and $BRISTOL_DB/memory.
	 */
	sprintf(tmppath, "%s/memory/profiles/%s", getBristolCache("profiles"), file);
/*printf("		%s\n", tmppath);*/

	/*
	 * If we can open the file then clean the array and start again. If not
	 * then see if we have a factory copy available.
	 */
	if ((fd = fopen(&tmppath[0], "r")) == NULL)
	{
		sprintf(tmppath, "%s/memory/profiles/%s", getenv("BRISTOL"), file);
/*printf("attempt	%s\n", tmppath); */
		fd = fopen(&tmppath[0], "r");
	}

	if (fd != NULL)
	{
		char param[256];

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
				{
					points[wheel] = value;
					mapped++;
					//printf("mapped %i to %0.2f\n", wheel, value);
				}
			}
		}
	} else
		return(0);

	fclose(fd);

	if (flags & NO_INTERPOLATE)
		return(mapped);

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
if (mapped)
{
	i = 0;
	printf("%s\n", match);
	printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
		points[i], points[i+1], points[i+2], points[i+3], 
		points[i+4], points[i+5], points[i+6], points[i+7]);
	if ((i += 8) < 128) 
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
}
*/

	return(mapped);
}

int
bristolGetFreqMap(char *file, char *match, fTab *freqs, int count, int flags,
int sr)
{
	float points[128];
	int mapped;

	if ((mapped = bristolGetMap(file, match, points, count, flags)) > 0)
	{
		int i;

		for (i = 0; i < mapped; i++)
		{
			freqs[i].freq = points[i];
			freqs[i].step = 1024.0 * points[i] / sr;
		}

		printf("%i frequency mappings: %f %f, %f %f\n", mapped,
			points[0], points[127], freqs[0].step, freqs[127].step);
	}

	return(mapped);
}

/*
 * This could also go into the library so the engine could use the same code?
 *
 * We want to go through the midi controller mapping file for this synth and
 * search for directives for value maps. The names are taken from the midi
 * header file and we want to add a few others for preconfigured value tables.
 */
static void
invertMap(u_char map[128])
{
	int j;

	for (j = 0; j < 128; j++)
		map[j] = (u_char) (127 - map[j]);
}

static void
logMap(u_char map[128])
{
	int i;
	float scaler = 127 / logf(128.0);

	/*
	 * We are going to take a logarithmic scale such that values go from 0 to
	 * 127
	 */
    for (i = 1; i < 129; i++)
		map[i - 1] = (u_char) (logf((float) i) * scaler);
}

static void
exponentialMap(u_char map[128])
{
	int i;

	/*
	 * We are going to take exponential scale
	 */
    for (i = 0; i < 128; i++)
		map[i] = (u_char) (i * i / 127.0);
}

static void
parabolaMap(u_char map[128])
{
	int i;
	float scaler = 127.0 / (64.0 * 64.0);

	/*
	 * We are going to take a parabolic scale. Not sure if it will be use....
	 */
    for (i = 0; i < 128; i++)
		map[i] = (u_char) (((float) ((i - 64) * (i - 64))) * scaler);
}

void
bristolMidiValueMappingTable(u_char valuemap[128][128], int midimap[128], char *synth)
{
	float tmap[128];
	char name[256];
	int i, j;

	sprintf(name, "%s.mcm", synth);

	for (i = 0; i < 128; i++)
		for (j = 0; j < 128; j++)
			valuemap[i][j] = j;

	/*
	 * We first want to look for predefined curves:
	 *
	 *	linear: <c_id> <0|1>
	 *	inverseLinear: <c_id> <0|1>
	 *	log: <c_id> <0|1>
	 *	inverseLog: <c_id> <0|1>
	 *	parabolaMap, inverseParabola
	 *	exponential, inverseExponential
	 *
	 * These should also add some extra curves for velocity mappings that
	 * are also going to be required. These will build non-linear curves from
	 * 0 to 127 over the 127 available values.
	 */
	if (bristolGetMap(name, "controllerMap", &tmap[0], 128, NO_INTERPOLATE) > 0)
	{
		for (i = 0; i < 128; i++)
		{
			if (tmap[i] >= 0.0)
				midimap[i] = (int) tmap[i];
			else
				midimap[i] = i;
		}
		bzero(tmap, sizeof(tmap));
	} else {
		for (i = 0; i < 128; i++)
			midimap[i] = i;
	}

	if (bristolGetMap(name, "inverseLinear", &tmap[0], 128, NO_INTERPOLATE) > 0)
	{
		/* We have some requirements to map inverse linear */
		for (i = 0; i < 128; i++)
		{
			if (tmap[i] >= 1.0)
				invertMap(&valuemap[i][0]);
		}
		bzero(tmap, sizeof(tmap));
	}

	if (bristolGetMap(name, "log", &tmap[0], 128, NO_INTERPOLATE) > 0)
	{
		for (i = 0; i < 128; i++)
		{
			if (tmap[i] >= 1.0)
				logMap(&valuemap[i][0]);
		}
		bzero(tmap, sizeof(tmap));
	}
	if (bristolGetMap(name, "inverseLog", &tmap[0], 128, NO_INTERPOLATE) > 0)
	{
		for (i = 0; i < 128; i++)
		{
			if (tmap[i] >= 1.0)
			{
				logMap(&valuemap[i][0]);
				invertMap(&valuemap[i][0]);
			}
		}
		bzero(tmap, sizeof(tmap));
	}

	if (bristolGetMap(name, "exponential", &tmap[0], 128, NO_INTERPOLATE) > 0)
	{
		for (i = 0; i < 128; i++)
		{
			if (tmap[i] >= 1.0)
				exponentialMap(&valuemap[i][0]);
		}
		bzero(tmap, sizeof(tmap));
	}
	if (bristolGetMap(name, "inverseExponential", &tmap[0], 128, NO_INTERPOLATE) > 0)
	{
		for (i = 0; i < 128; i++)
		{
			if (tmap[i] >= 1.0)
			{
				exponentialMap(&valuemap[i][0]);
				invertMap(&valuemap[i][0]);
			}
		}
		bzero(tmap, sizeof(tmap));
	}

	if (bristolGetMap(name, "parabola", &tmap[0], 128, NO_INTERPOLATE) > 0)
	{
		for (i = 0; i < 128; i++)
		{
			if (tmap[i] >= 1.0)
				parabolaMap(&valuemap[i][0]);
		}
		bzero(tmap, sizeof(tmap));
	}
	if (bristolGetMap(name, "inverseParabola", &tmap[0], 128, NO_INTERPOLATE) > 0)
	{
		for (i = 0; i < 128; i++)
		{
			if (tmap[i] >= 1.0)
			{
				parabolaMap(&valuemap[i][0]);
				invertMap(&valuemap[i][0]);
			}
		}
		bzero(tmap, sizeof(tmap));
	}

	/*
	 * Then converge the known controllers for any specific profiles configured
	 * by the user to override both the default linear curves and any other
	 * predefined curves.
	 */
	for (i = 0; i < 128; i++) {
		if (controllerName[i] == NULL)
			continue;

		if (bristolGetMap(name, controllerName[i], &tmap[0], 128, 0) > 0)
		{
			/*
			 * If something was mapped for this controller then see about
			 * converging the values
			 */
			for (j = 0; j < 128; j++)
				if ((tmap[j] >= 0) && (tmap[j] < 128))
					valuemap[i][j] = (u_char) tmap[j];
			bzero(tmap, sizeof(tmap));
		}
	}
}

static int ztime = 0, uztime, std_out, f_out = -1, bsyslog = 0;
FILE *logInput = NULL;

/*
 * This will close stdin, stdout and stderr, then open some files for debug
 * output. First an attempt will be made to open files in /var/log/<process>.log
 * then ~/.bristol/log/<process>.log (with O_TRUNC in this case) and creating
 * the directory if necessary. Failing that /dev/null.
 *
 * We should look to pthread() a logging process that takes a pipe() to the main
 * threads then prints every message that comes from the pipd()ed stdout/stderr
 * to a date formatted message to the log file on new fd:
 *
 *	create pipe.
 *	close 0, 1, 2.
 *	createthread
 *		parent
 *			dup2() pipe write fd onto 1, 2
 *			closes pipe write fd
 *		child
 *			opens logfile
 *			dup2() pipe read fd onto 0
 *			closes pipe read fd
 *			reads from pipe - gets()? May just scan for \n and add date
 *			formats to log fd
 */
void
logthread(char *process)
{
	char filename[BUFSZE], *inputtext = filename, outputtext[BUFSZE], ttext[BUFSZE];
	time_t timep;
	struct tm *tm;
	struct timeval tv;
	int len;

	sprintf(filename, "/var/log/%s.log", process);

	/* See about /var/log */
	if ((f_out = open(filename, O_CREAT|O_WRONLY|O_APPEND, 0644)) < 0)
	{
		/* see about ~/.bristol/log */
		sprintf(filename, "%s/.bristol/log/%s.log", getenv("HOME"), process);

		if ((f_out = open(filename, O_TRUNC|O_CREAT|O_WRONLY, 0644)) < 0)
		{
			sprintf(filename, "%s/.bristol/log", getenv("HOME"));
			mkdir(filename, 0755);

			sprintf(filename, "%s/.bristol/log/%s.log", getenv("HOME"),	
				process);

			if ((f_out = open(filename, O_TRUNC|O_CREAT|O_WRONLY, 0644)) < 0)
				/* All failed, open /dev/null */
				f_out = open("/dev/null", O_WRONLY);
		}
	}

	gettimeofday(&tv, NULL);
	/*
	 * The use of ztime is just to pull the seconds back to when the app
	 * was started.
	 */
	ztime = (int) tv.tv_sec;
	uztime = (int) tv.tv_usec;

	/*
	 * At this point we should pretty print the current date into the
	 * file so that we can correlated different invocations - bristol
	 * and brighton now have separate log files and if in /var/log they
	 * may keep track of several invocations.
	 */
//	while ((c = read(0, &inputtext[i], 1)) >= 0) {
	/*
	while (1) {
		c = read(0, &inputtext[i], 1);
		if (c == 0)
		{
			usleep(100000);
			continue;
		}
		if (inputtext[i++] != '\n')
			continue;
		inputtext[i-1] = '\0';
		i = 0;
	*/

	while (fgets(inputtext, BUFSZE, logInput) > 0)
	{
		if (((len = strlen(inputtext)) > 0)
			&& (inputtext[len - 1] != '\n'))
			sprintf(inputtext, "(suppressed excess message %i bytes)", len);
		else
			inputtext[len - 1] = '\0';

		gettimeofday(&tv, NULL);

		if (bsyslog)
		{
			if (std_out > 0)
			{
				//fsync(std_out);
				close(std_out);
				std_out = -1;
			}
			if (f_out > 0)
			{
				//fsync(f_out);
				close(f_out);
				f_out = -1;
			}

			sprintf(outputtext, "[%05.6f] %s\n",
				(float) (((int) tv.tv_usec) < uztime?
					(int) tv.tv_sec - ztime - 1:
					(int) tv.tv_sec - ztime)
					+ (float) (((int) tv.tv_usec) < uztime?
						((int) tv.tv_usec) + (1000000 - uztime):
						(((int) tv.tv_usec) - uztime)) / 1000000,
				inputtext);
			syslog(LOG_USER|LOG_INFO, "%s", outputtext);
			continue;
		}

		time(&timep);
		tm = localtime(&timep);
		// day month year: %d %m %y
		// %b %e
		// The one taken is similar to syslog format:
		strftime(ttext, BUFSZE, "%b %e %T", tm);

		sprintf(outputtext, "%s %-8s [%05.6f] %s\n",
			ttext, process,
			(float) (((int) tv.tv_usec) < uztime?
				(int) tv.tv_sec - ztime - 1:
				(int) tv.tv_sec - ztime)
				+ (float) (((int) tv.tv_usec) < uztime?
					((int) tv.tv_usec) + (1000000 - uztime):
					(((int) tv.tv_usec) - uztime)) / 1000000,
			inputtext);

		if (std_out < 0)
			continue;

		if (write(std_out, outputtext, strlen(outputtext)) < 0)
			pthread_exit(0);
		fsync(std_out);
	}

	/*
	sprintf(outputtext, "%s %-8s [%05.6f] %s\n",
		ttext, process,
		(float) (((int) tv.tv_usec) < uztime?
			(int) tv.tv_sec - ztime - 1:
			(int) tv.tv_sec - ztime)
			+ (float) (((int) tv.tv_usec) < uztime?
				((int) tv.tv_usec) + (1000000 - uztime):
				(((int) tv.tv_usec) - uztime)) / 1000000,
		"terminating log thread");
	write(std_out, outputtext, strlen(outputtext));

	 * We are not going to redirect the input stream for now. If we end up
	 * going for real daemon processing this needs to be cleared up in separate
	 * code that detaches the controlling terminal, and a few other loose ends.
	tmpfd = open("/dev/null", O_RDONLY);
	dup2(tmpfd, STDIN_FILENO);
	close(tmpfd);
	 */

	if (bsyslog)
		closelog();
	else {
		close(std_out);
	}

	pthread_exit(0);
}

static char procname[64];
static struct timeval tv;
static int lcons = 0;

pthread_t
bristolOpenStdio(int mode)
{
	int pipefds[2];
	int count = 40;

	if (lcons)
		return(0);

	switch (mode) {
		case BRISTOL_LOG_TERMINATE:
			if (lthread != 0)
				pthread_cancel(lthread);
			return(0);
		case BRISTOL_LOG_DISYNTHEGRATE:
			sprintf(procname, "%s", "disynthegrate");
			break;
		case BRISTOL_LOG_BRISTOL:
			sprintf(procname, "%s", "bristol");
			break;
		case BRISTOL_LOG_BRIGHTON:
			sprintf(procname, "%s", "brighton");
			break;
		case BRISTOL_LOG_DAEMON:
			close(std_out);
			std_out = f_out;
			/*
			 * We need this to log into the file rather than stdtio
			 */
			printf("\nstarting file logging [@%i.%i]\n",
				(int) tv.tv_sec, (int) tv.tv_usec);

			return(0);
		case BRISTOL_LOG_SYSLOG:
			openlog(procname, LOG_CONS|LOG_NDELAY|LOG_NOWAIT, LOG_USER);

			bsyslog = 1;

			return(0);
		case BRISTOL_LOG_CONSOLE:
			lcons = 1;
			return(0);
	}

	gettimeofday(&tv, NULL);
	printf("starting logging thread [@%i.%i]\n",
		(int) tv.tv_sec, (int) tv.tv_usec);

	if (pipe(pipefds) < 0)
		return(0);

	fcntl(pipefds[0], F_SETFL, O_RDONLY);
	fcntl(pipefds[1], F_SETFL, O_WRONLY|O_NDELAY);

	std_out = dup(STDOUT_FILENO);

	dup2(pipefds[0], STDIN_FILENO);
	dup2(pipefds[1], STDERR_FILENO);

	if ((logInput = fdopen(STDIN_FILENO, "r")) == NULL)
		printf("Could not fdopen() log fd\n");

	dup2(pipefds[1], STDOUT_FILENO);
	close(pipefds[0]);
	close(pipefds[1]);

	pthread_create(&lthread, NULL, (void *) logthread, &procname);

	while (f_out < 0)
	{
		usleep(100000);
		if (--count < 0)
			break;
	}

	printf("starting console logging [@%i.%i]\n",
		(int) tv.tv_sec, (int) tv.tv_usec);

	return(lthread);
}

