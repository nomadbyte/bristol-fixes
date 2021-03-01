
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

#ifndef BRISTOL_MESSAGES_H
#define BRISTOL_MESSAGES_H

#include <sys/types.h>

#define BRISTOL_M_TYPE2_LEN

#define MSG_TYPE_CONTROL 1
#define MSG_TYPE_SYSTEM 2
#define MSG_TYPE_PARAM 4
#define MSG_TYPE_SESSION 8 // Type-2 messages

/*
 * These will be wrapped in s MIDI sysex, and sent down the control link.
 * We will either require ALL codes are 7 bits only, or will put in an 
 * encoder.
 */
typedef struct BristolMsg {
	u_char SysID; /* How about "83"? hex 'S' - or find a free one from net */
	u_char L; /* hex 76 - some more bytes of ID! */
	u_char a; /* hex 97 */
	u_char b; /* hex 98 */
	u_char msgLen;
	u_char msgType;
	u_char channel; /* Multitimbral, or SLab track */
	u_char from; /* Source channel for return messages */
	u_char operator; /* Operator on voice */
	u_char controller; /* Controller on operator */
	u_char valueLSB;
	u_char valueMSB;
} bristolMsg;

/*
 * These are the type 2 message operations
 */
#define BRISTOL_MT2_WRITE	1
#define BRISTOL_MT2_READ	2

typedef struct BristolMsgType2 {
	u_char SysID; /* How about "83"? hex 'S' - or find a free one from net */
	u_char L; /* hex 76 - some more bytes of ID! */
	u_char a; /* hex 97 */
	u_char b; /* hex 98 */
	u_char msgLen;
	u_char msgType;
	u_char channel; /* Multitimbral, or SLab track */
	u_char from; /* Source channel for return messages */
	int operation;
	char *data;
} bristolMsgType2;

#endif /* BRISTOL_MESSAGES_H */

