
/*
 *  Diverse SLab audio routines.
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
 * This will contain 3 called routines, from the audio daemon, one to open the
 * mastering file, one to write the header, and one to close the file.
 *
 * openMaster()
 *	The calling rouine will pass the file type. Expects an answer of:
 *		-1: failure - forget mastering.
 *		Anything else - do mastering with output to that FD number.
 *
 * writeMaster()
 *	Calling routine passes type of file, and a buffer of data. Called party
 *	must format data as necessary.
 *
 * closeMaster()
 *	The calling rouine will pass the file type, and the number of bytes written.
 */
#include <slabrevisions.h>
#include <slabaudiodev.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef MASTER_WAV
/***** I stole this from Topy Shepard                         *****/
/***** He stole the rest of this file from 'vplay'.           *****/
/***** Thanks to: Michael Beck - beck@informatik.hu-berlin.de *****/
       
/* Definitions for Microsoft WAVE format */

#define BUFFSIZE	0x10000
#define RIFF		0x46464952	
#define WAVE		0x45564157
#define FMT			0x20746d66
#define DATA		0x61746164
#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

/* it's in chunks like .voc and AMIGA iff, but my source say there
   are in only in this combination, so I combined them in one header;
   it works on all WAVE-file I have
*/

typedef struct {
	u_long	main_chunk;	/* 'RIFF' */
	u_long	length;		/* filelen */
	u_long	chunk_type;	/* 'WAVE' */

	u_long	sub_chunk;	/* 'fmt ' */
	u_long	sc_len;		/* length of sub_chunk, =16 */
	u_short	format;		/* should be 1 for PCM-code */
	u_short	modus;		/* 1 Mono, 2 Stereo */
	u_long	sample_fq;	/* frequence of sample */
	u_long	byte_p_sec;
	u_short	byte_p_spl;	/* samplesize; 1 or 2 bytes */
	u_short	bit_p_spl;	/* 8, 12 or 16 bit */ 

	u_long	data_chunk;	/* 'data' */
	u_long	data_length;/* samplecount */
} WaveHeader;

WaveHeader header;
#endif

#ifdef MASTER_MP3ONLINE
int pipeFD[2];
int pid = -1;
int status;
#endif

static int d;

#ifdef MASTERING
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

static void writeWaveHdr(duplexDev *, int, int);
static void cdrFormat();
static void cdrPad();

/*
 * Note that the count parameter is the current songlength. This is only needed
 * for MP3ONLINE, since we need to put some value in the waveHeader before we
 * start piping the output.
 */
int
openMaster(duplexDev *audioDev, int type, char *fileName, int count)
{
	int fd;

	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("openMaster(%i, %s, %i)\n", type, fileName, count);

#ifdef MASTER_WAV
#ifdef MASTER_MP3ONLINE
	if (pid != -1)
	{
		/* kill(pid, SIGINT); */
		waitpid(pid, &status, WNOHANG);
		pid = -1;
	}
	/*
	 * BASICALLY THIS DOES NOT WORK, NOT SURE WHY. FOR FUTURE STUDY.....
	 *
	 * Online MP3 will open a pipe to bladeenc and work dynamically. This is
	 * a bit of a CPU hog, so the normal MP3 encoding will use output to a WAV
	 * file, and then bladeenc is called separately by the GUI once the output
	 * file is closed.
	 */
	if (type & MASTER_MP3ONLINE)
	{
		/*
		 * A bit ugly, but we are going to open a pipe, and fork/exec bladeenc.
		 * This is going into a separate library when it finally works.
		 *
		 * The library may use socketpair(2) instead.
		 */
		if (socketpair(PF_UNIX, SOCK_STREAM, PF_UNSPEC, pipeFD) < 0)
		{
			printf("mp3 online: socketpair() error\n");
			return(-1);
		} else {
			if ((pid = vfork()) == 0)
			{
				close(pipeFD[1]);
				dup2(pipeFD[0], 0);
				execlp("bladeenc", "bladeenc", /* "-quiet", */ "stdin",
					fileName, (char *) NULL);
				printf("Could not find bladeenc\n");
				_exit(0);
			} else {
				close(pipeFD[0]);
				fd = pipeFD[1];
			}
		}
	} else
#endif
#endif
		fd = open(fileName, O_CREAT|O_WRONLY|O_TRUNC, 0644);

	switch(type & MASTERING_MASK)
	{
#ifdef MASTER_WAV
#ifdef MASTER_MP3ONLINE
		case MASTER_MP3ONLINE:
			writeWaveHdr(audioDev, fd, count);
			break;
#endif
#ifdef MASTER_MP3
		case MASTER_MP3:
#endif
		case MASTER_WAV:
			/*
			 * We write a zero length now, and when we close the file we put
			 * the actual length in there.
			 */
			writeWaveHdr(audioDev, fd, 0);
			break;
#endif
#ifdef MASTER_CDR
		case MASTER_CDR:
#endif
		default:
			/*
			 * No header.
			 */
			break;
	}
	return(fd);
}

int
writeMaster(duplexDev *audioDev, int type, int fd, void *buffer, int size)
{
	int d;

	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("writeMaster(%i, %i, %p, %i)\n", type, fd, buffer, size);

	switch(type & MASTERING_MASK)
	{
#ifdef MASTER_CDR
		case MASTER_CDR:
			/*
			 * We need to byteswap first. Oops, this should cater for differnt
			 * output data types.
			 */
			cdrFormat(buffer, size >> 2);

			d = write(fd, buffer, size);
			break;
#endif
#ifdef MASTER_WAV
#ifdef MASTER_MP3
#ifdef MASTER_MP3ONLINE
		case MASTER_MP3ONLINE:
/*			printf("MP3 Online write to fd %i\n", fd); */
#endif
		case MASTER_MP3:
#endif
		case MASTER_WAV:
#endif
		default:
			/*
			 * We don't need to byteswap first.
			 */
			d = write(fd, buffer, size);
			break;
	}
	return(0);
}

void
closeMaster(duplexDev *audioDev, int fd, int type, int count)
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("closeMaster(%i, %i, %i)\n", type, fd, count);

	switch(type & MASTERING_MASK)
	{
#ifdef MASTER_CDR
		case MASTER_CDR:
			/*
			 * We need to pad the file to frag boundry.
			 */
			cdrPad(fd, count);
			break;
#endif
#ifdef MASTER_WAV
#ifdef MASTER_MP3ONLINE
		case MASTER_MP3ONLINE:
			/*
			 * We cannot kick the child yet, since it may not be finnished.
			 */
			break;
#endif
#ifdef MASTER_MP3
		case MASTER_MP3:
#endif
		case MASTER_WAV:
			/*
			 * We don't need to byteswap first.
			 */
			writeWaveHdr(audioDev, fd, count);
			break;
#endif
		default:
			/*
			 * We don't need to byteswap first.
			 */
			break;
	}
	close(fd);
}

#ifdef MASTERING
void
static cdrFormat(buffer, count)
register int count;
register char *buffer;
{
#if defined(__i386) || defined(__i486) || defined(__i586) || defined(__i686) \
	|| defined(__pentium) || defined(__pentiumpro)

	register short tmp;

#ifdef DEBUG
	printf("cdrFormat(%x, %i)\n", buffer, count);
#endif

	/*
	 * This needs to convert the Linux based short into CD-DA format.
	 * Not going to bother with porting issues just yet, but we should add
	 * "ENDIAN" flags for compilation.
	 *
	 * cdrecord expects MSBLeft, LSBLeft, MSBRight, LSBRight.
	 *
	 * Swap each byte pair.
	 */
	for (;count > 0; count--)
	{
		tmp = *(buffer + 1);
		*(buffer + 1) = *buffer;
		*buffer = tmp;
		buffer += 2;

		tmp = *(buffer + 1);
		*(buffer + 1) = *buffer;
		*buffer = tmp;
		buffer += 2;
	}
#endif /* cpu type */
}

/*
 * Check the number of samples written to the output file, and pad this out
 * to match the CD-R sector size.
 */
static void
cdrPad(int fd, int size)
{
	int outCount;
	short extend[2];

#ifdef DEBUG
	printf("cdrPad(%i, %i)\n", fd, size);
#endif

#ifdef CD_R_OUTPUT
	/*
	 * This is the number of bytes, converted into samples, modulo sectorsize.
	 */
	if ((outCount = CD_R_OUTPUT - (size % CD_R_OUTPUT)) == CD_R_OUTPUT)
		return;
#endif
	extend[0] = 0;

#ifdef DEBUG
	printf("cdrPadding with %i bytes\n", outCount);
#endif

	while (outCount > 0) {
		d = write(fd, extend, 1);
		outCount-=1;
	}
}
#endif

#ifdef MASTERING_WAV
/*
 * This was taken from SLabIO, with many thanks to Toby Shepard. He in turn
 * took it from Michael Beck - beck@informatik.hu-berlin.de - thanks Michael.
 * Minor changes for audioDev support. Funny how code goes around.....
 */
static void
writeWaveHdr(duplexDev *audioDev, int fd, int count)
{
	if (audioDev->cflags & SLAB_AUDIODBG)
		printf("writeWavHdr(%i, %i, %i): %i, %i\n", audioDev->devID, fd, count,
			audioDev->channels, audioDev->writeSampleRate);

	if (audioDev->channels == 0)
		audioDev->channels = 2;

	lseek(fd, (long) 0, SEEK_SET);

	header.main_chunk	= RIFF;
	header.length		= 36 + count + 8; /* HUH? header is 36 + data + chunk */
	header.chunk_type	= WAVE;

	header.sub_chunk	= FMT;
	header.sc_len		= 16;
	header.format		= PCM_CODE;
	header.modus		= audioDev->channels;
	header.sample_fq	= audioDev->writeSampleRate;
	header.byte_p_sec	= header.modus * header.sample_fq;
	header.byte_p_spl	= 2;
	header.bit_p_spl	= 16;

	header.data_chunk	= DATA;
	header.data_length	= count / header.modus / header.byte_p_spl;

	d = write(fd, &header, 44);
}
#endif

#endif /* MASTERING over whole file */

