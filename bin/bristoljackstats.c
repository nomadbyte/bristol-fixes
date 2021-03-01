
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
 * This file was taken out of the engine and moved to the audio library where
 * it correctly lies however it incorporates a few flags from bristol.h header
 * file that should really be removed to keep them independent. That is for
 * later study.
 */

/*#define DEBUG */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#ifdef _BRISTOL_JACK
#include <jack/jack.h>

jack_client_t *handle;

static int samplecount, samplerate;

static char *regname = "bgetstats";

static int
bristolJackTest()
{
	if ((handle = jack_client_open(regname, 0, NULL)) == 0)
		return(-1);

	samplerate = jack_get_sample_rate(handle);
	samplecount = jack_get_buffer_size(handle);

	jack_client_close(handle);

	return(0);
}

/*
 * This should go out as the first release with Jack. After that the interface
 * will change - at the moment Jack subsumes Bristol, and this is the wrong
 * way around. The audiomain structure is buried inside the jack structure,
 * but I would prefer the contrary. In addition, with it the contrary then
 * it would be easier to integrate alternative distribution drivers (DSSI).
 */
#endif /* _BRISTOL_JACK */
int
main(int argc, char *argv[])
{
	int outfd, nullfd;
	char lbuf[80];

#ifdef _BRISTOL_JACK /* _BRISTOL_JACK */
//	printf("%s: connect to jackd to find samplerate and period size\n",
//		argv[0]);

	/* I don't want all the output from jack, redirect it */
	outfd = dup(STDOUT_FILENO);
	nullfd = open("/dev/null", O_WRONLY);
	dup2(nullfd, STDOUT_FILENO);
	dup2(nullfd, STDERR_FILENO);

	if (bristolJackTest() != 0)
		return(-1);

	snprintf(lbuf, 80, "JACKSTATS: %i %i\n", samplerate, samplecount);

	nullfd = write(outfd, lbuf, strlen(lbuf));

	return(0);
#else
	printf("This should connect to jack but it does not appear to be compiled\n");
	return(-2);
#endif /* _BRISTOL_JACK */

}

