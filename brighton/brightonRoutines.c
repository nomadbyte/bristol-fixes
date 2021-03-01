
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
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>

#include "bristol.h"
#include "brightoninternals.h"
#include "brightonMini.h"

extern guimain global;

extern void brightonMidiInput(bristolMidiMsg *, guimain *);

struct sockaddr address;

/*
 * Free any memory we have have used up, close the link to the control socket
 * potentially sending a disconnect request.
 */
void
cleanupBristolQuietly()
{
	/*
	 * This comes from a sigpipe only. This means we have lost our socket to
	 * to the engine OR our socket to the X server, which is a bit of a bummer
	 * since in one case we want to send a message to the engine but if it
	 * is this socket that failed we will then get a SIGPIPE again.
	 *
	 * Put the signal back on default handling and then call the normal
	 * cleanup routines. If this was the engine socket then we will then die
	 * reasonably gracefully.....
	global.controlfd = -1;
	cleanupBristol();
	 */
	signal(SIGPIPE, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	cleanupBristol();
}

void
cleanupBristol()
{
	bristolMidiMsg msg;

	close(0);

	if (global.synths->win != 0)
		global.synths->win->flags |= BRIGHTON_EXITING;

	if (global.synths->flags & REQ_DEBUG_MASK)
		printf("cleanupBrighton()\n");

	if (global.libtest)
	{
		global.flags |= REQ_EXIT;
		return;
	}

	if (global.controlfd < 0)
	{
		global.flags |= REQ_EXIT;
		return;
	}

	/*
	 * We don't need to clean out every synth, we need to cleanout the second
	 * manual if it is configured, then the controlfd
	bristolMidiSendNRP(global.controlfd, global.synths->sid,
		BRISTOL_NRP_MIDI_GO, 0);
	 */

	if ((global.manualfd != 0) && (global.manualfd != global.controlfd))
	{
		bristolMidiSendMsg(global.manualfd, global.synths->sid2,
			127, 0, BRISTOL_EXIT_ALGO);
		if (global.synths->flags & REQ_DEBUG_MASK)
			printf("Secondary layer removed\n");
		/* OK, so sleep is lax, we used to wait for a message */
		bristolMidiRead(global.controlfd, &msg);
	}

	/* The engine has to restart MIDI processing after the exit */
	bristolMidiSendNRP(global.controlfd, global.synths->sid,
		BRISTOL_NRP_MIDI_GO, 0);

	bristolMidiSendMsg(global.controlfd, global.synths->sid,
		127, 0, BRISTOL_EXIT_ALGO);
	if (global.synths->flags & REQ_DEBUG_MASK)
		printf("Primary layer removed\n");
	/* We used to wait for a message however it is of no interest now */
	//bristolMidiRead(global.controlfd, &msg);

	usleep(250000);

	if ((global.manualfd != 0) && (global.manualfd != global.controlfd))
		bristolMidiClose(global.manualfd);
	bristolMidiClose(global.controlfd);

	printf("brighton exiting\n");

	//BAutoRepeat(global.synths->win->display, 1);

	//sleep(1);
	global.flags |= REQ_EXIT;

	//exit(0);
}

int
destroySynth(brightonWindow* win)
{
	guiSynth *synth = findSynth(global.synths, win);
	bristolMidiMsg msg;

	printf("destroySynth(%p): %i\n", win, synth->sid);

	/*
	 * Since we registered two synths, we now need to remove the upper
	 * manual.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
		BRISTOL_EXIT_ALGO);

	if (bristolMidiRead(global.controlfd, &msg) < 0)
		printf("socket closed\n");

	return(0);
}

void
displayPanel(guiSynth *synth, char *text, int value, int panel, int index)
{
	brightonEvent event;
	char display[32];

	memset(&event, 0, sizeof(brightonEvent));

	sprintf(display, "%s", text);
	event.type = BRIGHTON_MEM;
	event.m = (void *) &display[0];
	brightonParamChange(synth->win, panel, index, &event);
}

int
displayPanelText(guiSynth *synth, char *text, int value, int panel, int index)
{
	brightonEvent event;
	char display[32];

	memset(&event, 0, sizeof(brightonEvent));

	sprintf(display, "%s: %i", text, value);
	event.type = BRIGHTON_MEM;
	event.m = (void *) &display[0];
	brightonParamChange(synth->win, panel, index, &event);

	return(0);
}

int
displayText(guiSynth *synth, char *text, int value, int index)
{
	brightonEvent event;
	char display[32];

	memset(&event, 0, sizeof(brightonEvent));

	sprintf(display, "%s: %i", text, value);
	event.type = BRIGHTON_MEM;
	event.m = (void *) &display[0];
	brightonParamChange(synth->win, synth->panel, index, &event);

	return(0);
}

guiSynth *
findSynth(guiSynth *synth, brightonWindow* win)
{
/*printf("findSynth(%x, %x)\n", synth, reference); */
	if (synth == 0)
		return(0);

	if (synth->win == win)
		return(synth);

	return(findSynth(synth->next, win));
}

static char *myHome = NULL;

char *
oldgetBristolCache()
{
	/*
	 * See if we have a valid private directory, if we do not find one then 
	 * create it.
	 */
	if (myHome == NULL)
	{
		struct stat statbuf; 
		char *envcache;
		char path[1024];

		myHome = (char *) calloc(1024, 1);

		/*
		 * See if we have an env configured if so see if it exists, if not
		 * create a tree.
		 *
		 * If this fails then use $HOME/.bristol and create the tree.
		 *
		 * If these fail then fall back to the factory tree.
		 */

		if ((envcache = getenv("BRISTOL_CACHE")) != NULL)
		{
			/*
			 * See if we can access the cache.
			 */
			if (stat(envcache, &statbuf) == 0) {
				/*
				 * If we have the cache, is it formatted and do we have 
				 * permissions? There are several cases:
				 *	We own it and have rw permissions.
				 *	Somebody else owns it but we have write permissions
				 *	We are in the group and have write permissions.
				 * I think we should only take the first option since otherwise
				 * we can create memories that other users may not be able to
				 * access due to our umask.
				 */
				sprintf(path, "%s/memory", envcache);

				if (stat(path, &statbuf) == 0) {
					if (((statbuf.st_uid != getuid())
						|| ((statbuf.st_mode & 0600) != 0600)))
						envcache = NULL;
					else
						strcpy(myHome, envcache);
				} else {
					if (mkdir(path, 0755) != 0)
						envcache = NULL;
					else {
						sprintf(path, "%s/memory/profiles", envcache);
						mkdir(path, 0755);
						strcpy(myHome, envcache);
					}
				}
			} else {
				/*
				 * So, we have an env variable but cannot access it. See if we
				 * can create it.
				 */
				if (mkdir(envcache, 0755) != 0)
					envcache = NULL;
				else {
					sprintf(path, "%s/memory", envcache);
					mkdir(path, 0755);
					sprintf(path, "%s/memory/profiles", envcache);
					mkdir(path, 0755);

					strcpy(myHome, envcache);
				}
			}
		}

		/*
		 * If the cache is still null it means either we do not have an
		 * environment variable or it is not accessible.
		 */
		if (envcache == NULL)
		{
			/* This obviously won't happen often, but anyway: */
			if ((envcache = getenv("HOME")) == NULL)
			{
				if (envcache == NULL)
					envcache = global.home;
			} else {
				/*
				 * Use home directory. If we do not have a tree then create it
				 * again.
				 */
				sprintf(path, "%s/.bristol", envcache);

				sprintf(myHome, "%s", path);

				if (stat(path, &statbuf) != 0) {
					mkdir(path, 0755);
					sprintf(path, "%s/.bristol/memory", envcache);
					mkdir(path, 0755);
					sprintf(path, "%s/.bristol/memory/profiles", envcache);
					mkdir(path, 0755);
				}
			}
		}
	}

	return(myHome);
}

extern char *getBristolCache(char *);

int
getMemoryLocation(char *algo, char *name, int location, int perms)
{
	char path[256], *ppath, *ext = "mem";
	int fd;

	/*
	 * This should look for a private copy, and if not found then look for
	 * a public copy.
	 */
	if ((name != 0) && (name[0] == '/'))
		sprintf(path, "%s", name);
	else {
		struct stat statbuf; 

		/*
		 * So this should get us our memory location. We need to look and see
		 * if we have a directory for this emulation and create it if not.
		 */
		ppath = getBristolCache(algo);
		sprintf(path, "%s/memory/%s", ppath, algo);

		if (stat(path, &statbuf) != 0)
		{
			/*
			 * If we cannot stat the private location then we really ought to 
			 * look and see if .bristol exists. If so then we should create a
			 * tree to include memory/algo and return it
			 */
			mkdir(path, 0755);
		}

		if (name != NULL)
			ext = name;

		sprintf(path, "%s/memory/%s/%s%i.%s", ppath, algo, algo, location, ext);
	}

	if ((fd = open(path, perms, 0600)) < 0)
	{
		//printf("%s\n", path);
		sprintf(path, "%s/memory/%s/%s%i.%s",
			global.home, algo, algo, location, ext);
		return(open(path, perms, 0660));
	}

	//printf("%s\n", path);

	return(fd);
}

#define RECOPY_MAX_FILE 1024 * 1024

static void
bsmCopy(char *src, char *dst)
{
	char buffer[RECOPY_MAX_FILE];
	int srcfd, dstfd, count;

	srcfd = open(src, O_RDONLY);

	if ((count = read(srcfd, buffer, RECOPY_MAX_FILE)) > 0)
	{
		dstfd = open(dst, O_WRONLY|O_TRUNC|O_CREAT, 0644);
		count = write(dstfd, buffer, count);
		close(dstfd);
	}

	close(srcfd);
}

/*
 * The next two will copy files into and out of the bristol cache. Read is only
 * really used on system startup if a session file has been given by Jack SM but
 * it could be more general. Write will copy cache files out to wherever they 
 * have been requested.
 */
int
bristolMemoryExport(int source, char *dst, char *algo)
{
	char srcpath[1024];
	char fpath[1024];

	snprintf(srcpath, 1024, "%s/memory/%s/%s%i.mem",
		getBristolCache(algo), algo, algo, source);
	bsmCopy(srcpath, dst);

	printf("Export %s to %s\n", srcpath, dst);

	snprintf(srcpath, 1024, "%s/memory/%s/%s%i.seq",
		getBristolCache(algo), algo, algo, source);
	snprintf(fpath, 1024, "%s.seq", dst);
	bsmCopy(srcpath, fpath);

	snprintf(fpath, 1024, "%s.prof", dst);
	brightonWriteConfiguration(
		global.synths->win,
		algo,
		global.synths->midichannel,
		fpath);

	return(0);
}

int
bristolMemoryImport(int dest, char *src, char *algo)
{
	char dstpath[1024];
	char fpath[1024];

	snprintf(dstpath, 1024, "%s/memory/%s/%s%i.mem",
		getBristolCache(algo), algo, algo, dest);
	bsmCopy(src, dstpath);

	printf("Import %s to %s\n", src, dstpath);

	snprintf(fpath, 1024, "%s.seq", src);
	snprintf(dstpath, 1024, "%s/memory/%s/%s%i.seq",
		getBristolCache(algo), algo, algo, dest);
	bsmCopy(fpath, dstpath);

	snprintf(fpath, 1024, "%s.prof", src);
	brightonReadConfiguration(
		global.synths->win,
		global.synths->win->template,
		global.synths->midichannel,
		algo,
		fpath);
	//brightonReadConfiguration(synth->win, algo, synth->midichannel, fpath);

	return(0);
}

static int
checkSeqMem(arpeggiatorMemory *mem)
{
	if ((mem->s_max < 0)
		|| (mem->s_max > BRISTOL_SEQ_MAX)
		|| (mem->c_count < 0)
		|| (mem->c_count > BRISTOL_CHORD_MAX))
		return(-1);
	
	return(0);
}

/*
 * Just to be aware if you reading this, the GUI does not sequence the engine
 * or play chords. The calls below tell the engine to go into learning mode 
 * and then we tell it what keys it should store. The engine then sequences 
 * and chords.
 */
int
fillSequencer(guiSynth *synth, arpeggiatorMemory *mem, int flags)
{
	int sid = synth->sid;
	int i;

	if (flags & BRISTOL_NOCALLS)
		return(0);

	if (checkSeqMem(mem) < 0)
	{
		printf("Sequencer sizing problem: %f %p\n", mem->s_max, mem->chord);
		return(-1);
	}

	if (flags & BRISTOL_SID2)
		sid = synth->sid2;

	if ((synth->flags & REQ_DEBUG_MASK) > REQ_MIDI_DEBUG2)
		printf("Sending Arpeggiation to engine: %i events\n", (int) mem->s_max);

	printf("fill sequence %i\n", (int) mem->s_max);

	if (mem->s_max > 0)
	{
		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 1);

		printf("send arpeggiation: %i\n", (int) mem->s_max);

		for (i = 0; i < mem->s_max; i++)
		{
			printf("arpeggio step %i is %i\n", i, (int) mem->sequence[i]);
			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_KEY, (int) mem->sequence[i]);
		}
	}

	bristolMidiSendMsg(global.controlfd, sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);

	if ((synth->flags & REQ_DEBUG_MASK) > REQ_MIDI_DEBUG2)
		printf("Sending Chord to engine: %i events\n", (int) mem->c_count);

	if (mem->c_count > 0)
	{
		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 1);

		for (i = 0; i < mem->c_count; i++)
		{
			printf("chord %i is %i\n", i, (int) mem->chord[i]);
			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_KEY, (int) mem->chord[i]);
		}
	}

	bristolMidiSendMsg(global.controlfd, sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 0);

	return(0);
}

int
loadSequence(memory *seq, char *algo, int location, int flags)
{
	int fd;
	memory tmem;

	if (seq->param == NULL) {
		seq->param = brightonmalloc(sizeof(arpeggiatorMemory));
		bzero(seq->param, sizeof(arpeggiatorMemory));
		seq->count = sizeof(arpeggiatorMemory) / sizeof(float);
		seq->active = seq->count;
		seq->vers = 0;
		sprintf(&seq->algo[0], "bristol sequencer");
	}

	if ((fd = getMemoryLocation(algo, "seq", location, O_RDONLY)) < 0)
		return(-1);

	if (read(fd, &tmem.algo[0], 32) < 0)
	{
		printf("read failed on params\n");
		close(fd);
		return(-1);
	}
	if (read(fd, &tmem.name[0], 32) < 0)
	{
		printf("read failed on name\n");
		close(fd);
		return(-1);
	}
	if (read(fd, &tmem.count, 4 * sizeof(short)) < 0)
	{
		printf("read failed opts\n");
		close(fd);
		return(-1);
	}

	if (tmem.vers != 0)
	{
		/*
		 * Future revisions to the memory structures should put hooks in here
		 */
		printf("read failed: vers %i no supported\n", tmem.vers);
		close(fd);
		return(-1);
	}

	if (strcmp(&tmem.algo[0], "bristol sequencer") != 0)
	{
		if (tmem.algo[0] == '\0')
			printf("Attempt to read (noname) into sequencer\n");
		else
			printf("Attempt to read %s into sequencer\n", &tmem.algo[0]);

		if ((flags & BRISTOL_SEQFORCE) == 0)
		{
			close(fd);
			return(-1);
		}
		sprintf(&seq->algo[0], "%s", algo);
	} else {
		sprintf(&seq->algo[0], "%s", &tmem.algo[0]);
		sprintf(&seq->name[0], "%s", &tmem.name[0]);
	}

	if (seq->count != tmem.count)
		printf("Sequence structure size mismatched %i vs %i\n",
			seq->count, tmem.count);

	if (tmem.count > seq->count)
		tmem.count = seq->count;

	seq->vers = tmem.vers;

	if (read(fd, &seq->param[0], tmem.count * sizeof(float)) < 0)
		printf("read failed\n");

	close(fd);

printf("loaded sequence\n");
	return(0);
}

int
loadMemory(guiSynth *synth, char *algo, char *name, int location,
int active, int skip, int flags)
{
	brightonEvent event;
	int i, fd, panel = 0, index = 0;
	memory tmem;

	memset(&event, 0, sizeof(brightonEvent));

/*printf("loadmemory %i: %s %x\n", location, synth->resources->name, flags); */
	if (active == 0)
		active = synth->mem.active;

	synth->lmem = synth->cmem;
	synth->cmem = location;

	if (flags & BRISTOL_SEQLOAD) {
		if (BRISTOL_SID2) {
			if (loadSequence(&synth->seq2, algo, location, flags) >= 0)
				/*
				 * These should now have given us a half valid arpeggioMemory
				 * structure, pass it off to some code to do a sanity check
				 * and then send the keys to the engine
				 */
				fillSequencer(synth, (arpeggiatorMemory *) synth->seq2.param,
					flags);
			else
				printf("sequencer memory not found\n");
		} else {
			if (loadSequence(&synth->seq1, algo, location, flags) >= 0)
				fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param,
					flags);
			else
				printf("sequencer memory not found\n");
		}
	}

	location += synth->mbi;

	if (location >= 0)
	{
		if ((fd = getMemoryLocation(algo, name, location, O_RDONLY)) < 0)
			return(-1);

		if (flags & BRISTOL_STAT)
		{
			close(fd);
			return(0);
		}

		if (read(fd, &tmem.algo[0], 32) < 0)
		{
			printf("read failed on params\n");
			close(fd);
			return(-1);
		}
		if (read(fd, &tmem.name[0], 32) < 0)
		{
			printf("read failed on name\n");
			close(fd);
			return(-1);
		}
		if (read(fd, &tmem.count, 4 * sizeof(short)) < 0)
		{
			printf("read failed opts\n");
			close(fd);
			return(-1);
		}

		if (tmem.vers != 0)
		{
			/*
			 * Future revisions to the memory structures should put hooks in
			 * here
			 */
			printf("read failed: vers %i no supported\n", synth->mem.vers);
			close(fd);
			return(-1);
		}

		if (strcmp(&tmem.algo[0], algo) != 0)
		{
			if ((flags & BRISTOL_FORCE) == 0)
			{
				if (tmem.algo[0] == '\0')
					printf("Attempt to read (noname) into %s algorithm\n",
						algo);
				else
					printf("Attempt to read %s into %s algo\n", &tmem.algo[0],
						algo);

				close(fd);
				return(-1);
			}
			sprintf(&synth->mem.algo[0], "%s", algo);
		} else {
			sprintf(&synth->mem.algo[0], "%s", &tmem.algo[0]);
			sprintf(&synth->mem.name[0], "%s", &tmem.name[0]);
		}
		synth->mem.count = tmem.count;
		synth->mem.vers = tmem.vers;

		if (((synth->flags & REQ_DEBUG_MASK) > REQ_MIDI_DEBUG2)
			&& (synth->mem.active != tmem.active))
			printf("Active count changed. Overriding\n");
/*	synth->mem.active = tmem.active; */

		if (read(fd, &synth->mem.param[skip], active * sizeof(float)) < 0)
			printf("read failed\n");

		close(fd);

		if (flags & BRISTOL_NOCALLS)
			return(0);
	} else
		memset(synth->mem.param, 0, active * sizeof(float));

	synth->flags |= MEM_LOADING;
	/*
	 * We now have to call the GUI to configure all these values. The GUI
	 * will then call us back with the parameters to send to the synth.
	 */
	for (i = 0; i < active; i++)
	{
		event.type = BRIGHTON_FLOAT;
		event.value = synth->mem.param[i];
		/*
		 * We need to find which panel hosts this memory index. If we only
		 * have one panel, this is easy, it has all the controls. If we have
		 * multiple then we need to go through them all.
		 */
		if (synth->resources->nresources == 1)
			brightonParamChange(synth->win, synth->panel, i, &event);
		else {
			index = i;

			for (panel = 0; panel < synth->resources->nresources; panel++)
			{
				if ((index - synth->resources->resources[panel].ndevices) < 0)
				{
					/*
					 * If this is a text panel, skip it.
					 */
					if (synth->resources->resources[panel].devlocn[index].device
						== 3)
						continue;
					/*
					 * The next is odd, and could break a couple of the synth
					 * memories, but if the value that is in the newly loaded
					 * memory is outside of the configured range then we are
					 * not going to set the value.
					 */
					if ((synth->resources->resources[panel].devlocn[index].from
						< synth->resources->resources[panel].devlocn[index].to)
						&&
						((synth->resources->resources[panel].devlocn[index].from
							> event.value) ||
						(synth->resources->resources[panel].devlocn[index].to
							< event.value)))
						continue;
					brightonParamChange(synth->win, panel, index, &event);
					break;
				} else
					index -= synth->resources->resources[panel].ndevices;
			}
		}
	}

	synth->flags &= ~MEM_LOADING;

	return(0);
}

int
saveSequence(guiSynth *synth, char *algo, int location, int flags)
{
	memory *seq = &synth->seq1;
	int fd;

	if (flags & BRISTOL_SID2)
		seq = &synth->seq2;

	/*
	 * Try and save our sequencer information.
	 */
	if (seq->param != NULL)
	{
		if ((fd = getMemoryLocation(
			algo, "seq", location, O_WRONLY|O_CREAT|O_TRUNC)) < 0)
			return(-1);

		if (checkSeqMem((arpeggiatorMemory *) seq->param) >= 0) {
printf("Saving %i steps\n", (int) ((arpeggiatorMemory *) seq->param)->s_max);
			if (write(fd, seq,
				sizeof(struct Memory) - sizeof(float *)) < 0)
				printf("seq write failed 1\n");
			if (write(fd, seq->param, sizeof(arpeggiatorMemory)) < 0)
				printf("seq write failed 2\n");
		} else {
			printf("problem saving sequencer memory\n");
			close(fd);
			return(-1);
		}

		close(fd);
	}
	return(0);
}

int
saveMemory(guiSynth *synth, char *algo, char *name, int location, int skip)
{
	int fd;

	if (location < 0)
		return(0);

	location += synth->mbi;

	if ((fd = getMemoryLocation(algo, name, location, O_WRONLY|O_CREAT|O_TRUNC))
		< 0)
		return(-1);

	if (write(fd, &synth->mem, sizeof(struct Memory) - sizeof(float *)) < 0)
		printf("mem write failed 1\n");
	if (write(fd, &synth->mem.param[skip], synth->mem.active * sizeof(float))
		< 0)
		printf("mem write failed 2\n");

	/*
	 * Save global profile with controller registrations. No filename is given
	 * so that the called code will take the cache copy.
	 */
	brightonWriteConfiguration(synth->win, algo, synth->midichannel, NULL);

	close(fd);

	return(0);
}

static void
connectengine(guimain *global)
{
	int flags;

	flags = BRISTOL_CONN_TCP|BRISTOL_DUPLEX|BRISTOL_CONN_NBLOCK|
		(global->flags & BRISTOL_CONN_FORCE);

	if ((global->controlfd = bristolMidiOpen(global->host,
		flags, global->port, -1, brightonMidiInput, global)) < 0)
	{
		printf("opening link to engine: %i\n", global->port);
		if ((global->controlfd = bristolMidiOpen(global->host,
			flags, global->port, -1, brightonMidiInput, global)) < 0)
			cleanupBristol();
	} else
		printf("%s already active (%i)\n", BRISTOL_ENGINE, global->controlfd);
}

int
initConnection(guimain *global, guiSynth *synth)
{
	bristolMidiMsg msg;
	int sid = -1;

	connectengine(global);

	//printf("initConnection()\n");
	/*
	 * send a hello, with a voice count, then request starting of the
	 * synthtype. All of these will require an ACK, and we should wait here
	 * and read that ack before proceeding with each next step.
	 */
	bristolMidiSendMsg(global->controlfd, synth->midichannel,
/*		127, 0, BRISTOL_HELLO|BRISTOL_VOICES|synth->voices); */
		127, 0, BRISTOL_HELLO|synth->voices);

	if (synth->sid > 0)
		synth->sid2 = global->controlfd + 1;

	if (bristolMidiRead(global->controlfd, &msg) != BRISTOL_OK)
	{
		printf("unacknowledged request on %i\n", global->controlfd);
		bristolMidiPrint(&msg);
		sleep(1);
		global->flags |= REQ_EXIT;
		return(-1);
	}

	/*
	 * The returned value is the connection ID that we need to use for all
	 * further system conversations.
	 */
	sid = (msg.params.bristol.valueMSB << 7) + msg.params.bristol.valueLSB;

	if (synth->flags & REQ_MIDI_DEBUG2)
	{
		printf("sid is %i (%i, %i)\n", sid,
			msg.params.bristol.valueMSB, msg.params.bristol.valueLSB);

		bristolMidiPrint(&msg);
	}

	bristolMidiSendMsg(global->controlfd, sid,
		127, 0, BRISTOL_MIDICHANNEL|synth->midichannel);

	if (bristolMidiRead(global->controlfd, &msg) != BRISTOL_OK)
	{
		bristolMidiSendMsg(global->controlfd, sid,
			127, 0, BRISTOL_MIDICHANNEL|synth->midichannel);
		if (bristolMidiRead(global->controlfd, &msg) != BRISTOL_OK)
		{
			printf("unacknowledged request on %i\n", sid);
			bristolMidiPrint(&msg);
			sleep(1);
			global->flags |= REQ_EXIT;
			return(-1);
		}
	}

	bristolMidiSendMsg(global->controlfd, sid,
		127, 0, BRISTOL_REQSTART);
	if (bristolMidiRead(global->controlfd, &msg) != BRISTOL_OK)
	{
		printf("unacknowledged request on %i\n", sid);
		sleep(1);
		global->flags |= REQ_EXIT;
		return(-1);
	}

/*printf("	initialising one connection of type %i\n", synth->synthtype); */
	bristolMidiSendMsg(global->controlfd, sid,
		127, 0, BRISTOL_INIT_ALGO|synth->synthtype);
	if (bristolMidiRead(global->controlfd, &msg) != BRISTOL_OK)
	{
		printf("unacknowledged request on %i\n", sid);
		sleep(1);
		global->flags |= REQ_EXIT;
		return(-1);
	}

	return(sid);
}

