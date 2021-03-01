
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
 *	Marks for global/external variables
 */

#ifndef globals_complete

#define	export			/* export variable from module */
#define import	extern		/* import variable into module */

/*
 *	Various constants and types
 */
#define NAME_LENGTH 32
#define GUI_DEBUG_EXT (0x2 << 20)

/* frequently used characters */

#define NUL	'\0'
#define TAB	'\t'
#define NL	'\n'
#define CR	'\r'
#define BS	'\b'
#define SP	' '

/* misc macros */

#define fl fflush(stdout)

#ifdef CTRL
#undef CTRL
#endif
#define CTRL(c)	(c&037)

#ifdef SIGNAL_HANDLERS_ARE_VOID
typedef void sig_type;
#else
typedef int sig_type;
#endif

/*
 *	Some systems don't define these in <sys/stat.h>
 */

#ifndef S_IFMT
#define	S_IFMT	0170000			/* type of file */
#define S_IFDIR	0040000			/* directory */
#define S_IFREG	0100000			/* regular */
#endif

#ifndef O_RDONLY
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#endif

/*
 * The following are for the audio net tap
 */
#define SLAB_INPUT_TAP 1
#define SLAB_OUTPUT_TAP 2

/* define types of own functions */
char *mk_file_name(), *home_relative();
char *date_time(), *user_name();
char *copy_str();

time_t	file_exist(), m_time();

#define	OPEN_READ	0	/* open for reading */
#define	OPEN_UPDATE	1	/* open/create for update */
#define	OPEN_CREATE	2	/* create/truncate for write */
#define	OPEN_APPEND	3	/* open for append */

#define	DONT_CREATE	0x40	/* return if file does not exist */
#define	MUST_EXIST	0x80	/* fatal error if cannot open */
#define	OPEN_UNLINK	0x100	/* unlink after open (not OPEN_UPDATE) */


/*
 *	Other external definitions
 *
 *	NOTICE: the distinction between pointers and arrays is important
 *		here (they are global variables - not function arguments)
 */
#ifndef SERVICE
#define SERVICE "slab"
#endif

extern int

    s_hangup,	/* hangup signal */
    s_keyboard,	/* keyboard signal */
    s_pipe,	/* broken pipe */
    s_redraw,	/* continue signal after stop */
    is_master;

/*
unsigned short
    user_id,
    group_id;
*/

#define BUFSIZE 1024
/*
 * Several definitions for the socket handling, plus a few addr's.
 */

#define local_socket_name	"pwmgr_sock"
#define MAXCONNECTIONS		10
#define TIMEOUT				600 /* seconds for select timeout */

#define	CleanClose	0
#define	DirtyClose	1

/* int process_id; */

extern int errno;
#define globals_complete
#endif
