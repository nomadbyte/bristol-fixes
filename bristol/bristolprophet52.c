
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

#include "bristol.h"
#include "bristolmm.h"
#include "bristolprophet.h"

extern int operateOneProphet(audioMain *, Baudio *, bristolVoice *, float *);
extern int prophetController(Baudio *, u_char, u_char, float);
extern int operateProphetPreops(audioMain *, Baudio *, bristolVoice *, float *);
extern int bristolProphetDestroy(audioMain *, Baudio *);

extern float *freqbuf;
extern float *osc3buf;
extern float *wmodbuf;
extern float *pmodbuf;
extern float *adsrbuf;
extern float *filtbuf;
extern float *noisebuf;
extern float *oscbbuf;
extern float *oscabuf;
extern float *scratchbuf;

int
bristolProphet52Init(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one prophet\n");
	baudio->soundCount = 8; /* Number of operators in this voice (MM) */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	initSoundAlgo(8, 0, baudio, audiomain, baudio->sound);
	initSoundAlgo(8, 1, baudio, audiomain, baudio->sound);
	initSoundAlgo(16, 2, baudio, audiomain, baudio->sound);
	/* An ADSR */
	initSoundAlgo(1, 3, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(3, 4, baudio, audiomain, baudio->sound);
	/* Another ADSR */
	initSoundAlgo(1, 5, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2, 6, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4, 7, baudio, audiomain, baudio->sound);

	baudio->param = prophetController;
	baudio->destroy = bristolProphetDestroy;
	baudio->operate = operateOneProphet;
	baudio->preops = operateProphetPreops;

	/*
	 * Put in a vibrachorus on our effects list.
	 */
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);

	if (freqbuf == 0)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratchbuf == 0)
		scratchbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (pmodbuf == 0)
		pmodbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == 0)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == 0)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscbbuf == 0)
		oscbbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscabuf == 0)
		oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(pmods));
	((pmods *) baudio->mixlocals)->pan = 0.0;
//	baudio->mixflags |= BRISTOL_STEREO;
	baudio->mixflags |= P_UNISON;
	return(0);
}

