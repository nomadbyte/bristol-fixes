
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

/*#define DEBUG */

#include <sys/time.h>
#include <time.h>
#include <string.h>

#include <bristolmidi.h>
#include <bristolmidieventnames.h>
#include "bristolmessages.h"

extern bristolMidiMain bmidi;

/* We should put this into a structure */
static int NonOrRegistered = 0, NonOrRegisteredValue = 0;

void
bristolMidiPrintGM2(bristolMidiMsg *msg)
{
	printf("GM2: base %i from %i/%i\n",
		msg->GM2.c_id, msg->GM2.c_id_coarse, msg->GM2.c_id_fine);
	printf("	%i/%i = %i %f\n",
		msg->GM2.coarse, msg->GM2.fine, msg->GM2.intvalue, msg->GM2.value);
}

void
bristolMsgPrint(bristolMsg *msg)
{
	printf("ID  0x%x%x%x%x\n", msg->SysID, msg->L, msg->a, msg->b);
	printf("    msgLen  %i\n", msg->msgLen);
	printf("    msgType %i\n", msg->msgType);
	printf("    channel %i\n", msg->channel);
	printf("    from    %i\n", msg->from);
	printf("    op      %i\n", msg->operator);
	printf("    cont    %i\n", msg->controller);
	printf("    MSB     %i\n", msg->valueMSB);
	printf("    LSB     %i\n", msg->valueLSB);
	printf("    value   %i/%x\n",
		msg->valueLSB + (msg->valueMSB<<7),
		msg->valueLSB + (msg->valueMSB<<7));
}

void
bristolMidiPrint(bristolMidiMsg *msg)
{
	int command = ((msg->command & MIDI_COMMAND_MASK) & ~MIDI_STATUS_MASK) >> 4;

	switch (msg->command & MIDI_COMMAND_MASK) {
		case MIDI_SYSTEM:
			printf("system");
			if ((msg->params.bristol.SysID == ((bmidi.SysID >> 24) & 0x0ff))
				&& (msg->params.bristol.L == ((bmidi.SysID >> 16) & 0x0ff))
				&& (msg->params.bristol.a == ((bmidi.SysID >> 8) & 0x0ff))
				&& (msg->params.bristol.b == (bmidi.SysID & 0x0ff)))
			{
				printf(" bristol (%i)\n", msg->sequence);
				bristolMsgPrint(&msg->params.bristol);
			} else
				printf("\n");
			break;
		case MIDI_NOTE_ON:
			printf("%s (%i) ch %i: %i, velocity %i\n", eventNames[command],
				msg->sequence,
				msg->channel, msg->params.key.key,  msg->params.key.velocity);
			break;
		case MIDI_NOTE_OFF:
			printf("%s (%i) ch %i: %i, velocity %i\n", eventNames[command],
				msg->sequence,
				msg->channel, msg->params.key.key,  msg->params.key.velocity);
			break;
		case MIDI_POLY_PRESS:
			printf("%s (%i) ch %i: key %i, pressure %i\n", eventNames[command],
				msg->sequence, msg->channel,
				msg->params.pressure.key,  msg->params.pressure.pressure);
			break;
		case MIDI_CONTROL:
			if (controllerName[msg->params.controller.c_id] == NULL)
				printf("%s (%i) ch %i: c_id %i, c_val %i\n",
					eventNames[command],
					msg->sequence,
					msg->channel,
					msg->params.controller.c_id,
					msg->params.controller.c_val);
			else
				printf("%s (%i) ch %i: %s, value %i\n", eventNames[command],
					msg->sequence,
					msg->channel,
					controllerName[msg->params.controller.c_id],
					msg->params.controller.c_val);
			break;
		case MIDI_PROGRAM:
			printf("%s (%i) ch %i: p_id %i\n", eventNames[command],
				msg->sequence,
				msg->channel, msg->params.program.p_id);
			break;
		case MIDI_CHAN_PRESS:
			printf("%s (%i) ch %i: pressure %i\n", eventNames[command],
				msg->sequence,
				msg->channel, msg->params.channelpress.pressure);
			break;
		case MIDI_PITCHWHEEL:
			printf("%s (%i) ch %i: msb %i, lsb %i\n", eventNames[command],
				msg->sequence,
				msg->channel, msg->params.pitch.msb, msg->params.pitch.lsb);
			break;
	}
}

/*
 * This will take the specified messages and recode the GM2 portion for things
 * such as coarse/fine controllers. It will also take the values of the coded
 * values and turn them into a floating point value.
 *
 * This stuff is painful for several reasons - some GM2 values denote coarse
 * and fine, others denote semitone and cents, etc, some are integer rather
 * than floats. It might be better to call this later if parsing is required.
 *
 * We also need to have a midi controller mapping system. If a controller is
 * remapped then we should consider it to be coarse only?
 *
 * The engine has a version of this code with different parameterisation. It 
 * is in midihandlers and we should pull it out into the library.
 */
void
bristolMidiToGM2(int GM2values[128], int midimap[128], u_char valuemap[128][128], bristolMidiMsg *msg)
{
	if (msg->command != MIDI_CONTROL)
	{
		/*
		 * This may be counter productive?
		 */
		msg->GM2.c_id = -1;
		msg->GM2.value = 0;
		return;
	}

	/*
	 * Apply the mappings already if any are defined. This is just a controller
	 * mapping however we also want to apply a value mapping this early. It
	 * is something to the effect of the following however this table needs to
	 * be initialised
	 */
	if (valuemap != NULL)
		msg->params.controller.c_val = valuemap[msg->params.controller.c_id]
			[msg->params.controller.c_val];
	if (midimap != NULL)
		msg->params.controller.c_id = midimap[msg->params.controller.c_id];

	/*
	 * Keep a copy of the controller setting for later use.
	 */
	GM2values[msg->params.controller.c_id] = msg->params.controller.c_val;

	/*
	 * This are true for all the controllers, but the later ones may have
	 * specific modifications
	 */
	msg->GM2.c_id = msg->params.controller.c_id;
	msg->GM2.value = ((float) msg->params.controller.c_val) / 127.0f;
	msg->GM2.intvalue = msg->params.controller.c_val;
	msg->GM2.coarse = msg->params.controller.c_val;
	msg->GM2.fine = 0;

	/*
	 * These are coarse of the course/fine controls
	 */
	if (msg->params.controller.c_id < 14)
	{
		msg->GM2.coarse = msg->params.controller.c_val;
		msg->GM2.fine = GM2values[msg->params.controller.c_id + 32];
		msg->GM2.intvalue = (msg->GM2.coarse << 7) + msg->GM2.fine;
		msg->GM2.value = ((float) msg->GM2.intvalue) / 16383.0f;
		return;
	}

	if (msg->params.controller.c_id < 32)
		return;

	/*
	 * Fine controls for the first 14 coarse/fine controllers.
	 */
	if (msg->params.controller.c_id < 46)
	{
/*		msg->GM2.c_id -= 32; */
		msg->GM2.fine = msg->params.controller.c_val;
		msg->GM2.coarse = GM2values[msg->params.controller.c_id - 32];
		msg->GM2.intvalue = (msg->GM2.coarse << 7) + msg->GM2.fine;
		msg->GM2.value = ((float) msg->GM2.intvalue) / 16383.0f;
		/*
		 * We should change the controller to be NRP/RP and the value to 
		 * reflect the associated controller setting:
		 *
		 * c_id = N/RP
		 * find = N/RP value
		 * value/intvalue = that controller  value.
		 *
		 * In other words, never deliver a c_id 38.
		 */
		if (msg->params.controller.c_id == 38) {
			msg->GM2.c_id = NonOrRegistered;
			msg->GM2.coarse = NonOrRegisteredValue;
		}
		return;
	}

	if (msg->params.controller.c_id < 80)
		return;

	/*
	 * These are 4 buttons.
	if (msg->params.controller.c_id < 84)
	{
		if (msg->GM2.intvalue > 63)
			msg->GM2.value = 1.0;
		else
			msg->GM2.value = 0.0;
		return;
	}
	 */

	if (msg->params.controller.c_id < 96)
		return;

	/*
	 * These are 2 data entry buttons.
	if (msg->params.controller.c_id < 98)
	{
		if (msg->GM2.intvalue > 63)
			msg->GM2.value = 1.0;
		else
			msg->GM2.value = 0.0;
		return;
	}
	 */

	/*
	 * Non Registered Parameter Numbers.
	 */
	if ((msg->params.controller.c_id == 98)
		|| (msg->params.controller.c_id == 99))
	{
		msg->GM2.c_id = 99;
		msg->GM2.fine = GM2values[98];
		msg->GM2.coarse = GM2values[99];
		msg->GM2.intvalue = (msg->GM2.coarse << 7) + msg->GM2.fine;
		msg->GM2.value = ((float) msg->GM2.intvalue) / 16383.0f;
		NonOrRegistered = MIDI_GM_NRP;
		NonOrRegisteredValue = msg->GM2.intvalue;
		return;
	}

	/*
	 * Registered Parameter Numbers.
	 */
	if ((msg->params.controller.c_id == 100)
		|| (msg->params.controller.c_id == 101))
	{
		msg->GM2.c_id = 101;
		msg->GM2.fine = GM2values[100];
		msg->GM2.coarse = GM2values[101];
		msg->GM2.intvalue = (msg->GM2.coarse << 7) + msg->GM2.fine;
		msg->GM2.value = ((float) msg->GM2.intvalue) / 16383.0f;
		NonOrRegistered = MIDI_GM_RP;
		NonOrRegisteredValue = msg->GM2.intvalue;
		return;
	}

	/*
	 * local Keyboard
	if (msg->params.controller.c_id == 122)
	{
		if (msg->GM2.intvalue > 63)
			msg->GM2.value = 1.0;
		else
			msg->GM2.value = 0.0;
		return;
	}
	 */
}

char databytes[1024];

static void
buildOneMsg(unsigned char p1, unsigned char p2, int dev, bristolMidiMsg *msg)
{
#ifdef DEBUG
	printf("buildOneMsg(%x, %x, %i), comm %x\n", p1, p2, dev,
		bmidi.dev[dev].lastcommand);
#endif

	gettimeofday(&(msg->timestamp), 0);

	/*
	 * We have enough message capacity, put information into the message buffer
	 * and return.
	 */
	msg->command = bmidi.dev[dev].lastcommand;
	msg->channel = bmidi.dev[dev].lastchan;

	msg->sequence = bmidi.dev[dev].sequence++;

	if (p1 != 0xff)
	{
		msg->params.key.key = p1;
		msg->params.key.velocity = p2;
		if (bmidi.dev[dev].flags & BRISTOL_CONN_JACK)
			msg->params.key.flags = BRISTOL_KF_JACK;
		else if (bmidi.dev[dev].flags & BRISTOL_CONN_TCP)
			msg->params.key.flags = BRISTOL_KF_TCP;
		else
			msg->params.key.flags = BRISTOL_KF_RAW;
	}

	switch (msg->command) {
		case MIDI_SYSTEM:
			if (msg->params.bristol.msgType >= 8)
			{
//				printf("T2 data: %s\n", databytes);
				msg->params.bristolt2.data = databytes;
			}
			break;
		case MIDI_NOTE_OFF:
		case MIDI_NOTE_ON:
		case MIDI_POLY_PRESS:
		case MIDI_CONTROL:
		case MIDI_PITCHWHEEL:
			msg->params.bristol.msgLen = 3;
			break;
		case MIDI_PROGRAM:
		case MIDI_CHAN_PRESS:
			msg->params.bristol.msgLen = 2;
			break;
	}
}

static int
parseCommand(unsigned char comm, int dev)
{
#ifdef int
	printf("parseCommand(%x, %i)\n", comm, dev);
#endif

	/*
	 * We have a new command, save any interesting device state infomation.
	 */
	bmidi.dev[dev].lastchan = comm & MIDI_CHAN_MASK;
	bmidi.dev[dev].lastcommand = comm & MIDI_COMMAND_MASK;

	/*
	 * Set up how many more bytes we need.
	 */
	switch (bmidi.dev[dev].lastcommand) {
		case MIDI_SYSTEM:
			if (comm == MIDI_EOS) 
			{
				/*
				if (bmidi.dev[dev].sysex.count != sizeof(bristolMsg))
					printf("Was bad sysex message (wrong length)\n");
				else
					printf("Was right length message: %x\n",
						bmidi.dev[dev].sysex.count);
				*/
				bmidi.dev[dev].sysex.count = 0;
				bmidi.dev[dev].lastcommand = 0;
			}
			if (comm == MIDI_SYSEX) 
			{
				/*printf("SYSEX checks\n"); */
				bmidi.dev[dev].sysex.count = 0;
				return(1);
			}
			break;
		case MIDI_PROGRAM:
		case MIDI_CHAN_PRESS:
			bmidi.dev[dev].lastcommstate =
				BRISTOL_CHANSTATE_WAIT_1;
			break;
		case MIDI_NOTE_ON:
		case MIDI_NOTE_OFF:
		case MIDI_POLY_PRESS:
		case MIDI_PITCHWHEEL:
			bmidi.dev[dev].lastcommstate =
				BRISTOL_CHANSTATE_WAIT_2;
			break;
		default:
			break;
	}

	return(0);
}

int
bristolMidiRawToMsg(unsigned char *buff, int count, int index, int dev,
	bristolMidiMsg *msg)
{
	int parsed = 0;

	msg->command = -1;
	msg->offset = 0;

	/*
	 * Attempt to parse a raw message buffer. If we can resolve a complete
	 * message then return the number we have parsed.
	 */
	if (count <= 0)
		return(0);

#ifdef DEBUG
	printf("bristolMidiRawToMsg(%x, %i, %i, %i) [%x]\n",
		(size_t) buff, count, index, dev, buff[index]);
#endif

	/*
	 * Although we know that we have buffered data, we do not know if we are
	 * being given complete messages by the raw interface - it could be byte by
	 * byte, or chunks of a large sysex. Parse the data byte by byte, and see
	 * if we can put together complete messages.
	 *
	 * Check out our current command in operation on this device:
	 */
	bmidi.dev[dev].sysex.count = 0;
	while (parsed < count)
	{
		/*
		 * If this is a status byte, find out what we cn do with it. Otherwise
		 * look for data commands.
		if ((bmidi.dev[dev].lastcommand != MIDI_SYSTEM) &&
			(buff[index] & MIDI_STATUS_MASK))
		 */
		if (buff[index] & MIDI_STATUS_MASK) {
			parseCommand(buff[index++], dev);
			return(1);
		} else {
			switch (bmidi.dev[dev].lastcommand)
			{
				/*
				 * Looking for one more byte. We have it, since we made it here.
				 */
				case MIDI_PROGRAM:
				case MIDI_CHAN_PRESS:
					buildOneMsg(buff[index], -1, dev, msg);
					return(parsed + 1);
				/*
				 * Looking for two more bytes, if they are there.
				 */
				case MIDI_NOTE_ON:
				case MIDI_CONTROL:
				case MIDI_NOTE_OFF:
				case MIDI_POLY_PRESS:
				case MIDI_PITCHWHEEL:
					/*
					 * if we do not have enough bytes, return.
					 */
					if ((count - parsed) < 2)
						return(0);

					/*
					 * Otherwise, go get the command, checking for buffer wrap.
					 * We also need to make sure that the next spare byte is not
					 * a status byte - ie, that we have not binned a byte in the
					 * midi cable.
					 */
					if ((index + 1) == BRISTOL_MIDI_BUFSIZE)
					{
						if (buff[0] & MIDI_STATUS_MASK)
							break;
						else
							buildOneMsg(buff[index], buff[0], dev, msg);
					} else {
						if (buff[index+1] & MIDI_STATUS_MASK)
							break;
						else
							buildOneMsg(buff[index], buff[index+1], dev, msg);
					}

					return(parsed + 2);
				case MIDI_SYSTEM:
					/*
					 * Sysex management requires we read bytes until we get
					 * a status byte, and it should minimally be blocking on 
					 * this device. Assume, since we have been sent this, that
					 * it is going to be a bristolMidiMsg, and look for the 
					 * first few bytes. If it is SLab we are OK, otherwise we
					 * have an issue, and should read to next status byte then
					 * dump the data.
					 *
					 * Looks bad? Turn the bm message into an array, and index
					 * it depending on which byte we have. Could do with some
					 * error handling!
					 */
					if (bmidi.dev[dev].sysex.count < 12)
						((char *) &msg->params.bristol)
							[bmidi.dev[dev].sysex.count] = buff[index];
						bmidi.dev[dev].sysex.count++;

					if (bmidi.dev[dev].sysex.count == 4)
					{
						/*
						 * If this is not a SLab message, then set the
						 * current state to unknown.
						 */
						if ((msg->params.bristol.SysID
							!= ((bmidi.SysID >> 24) & 0x07f))
							|| (msg->params.bristol.L
								!= ((bmidi.SysID >> 16) & 0x07f))
							|| (msg->params.bristol.a
								!= ((bmidi.SysID >> 8) & 0x07f))
							|| (msg->params.bristol.b
								!= (bmidi.SysID & 0x07f)))
						{
							int rv = bmidi.dev[dev].sysex.count;

							printf("unrecognised SYSEX ID %x %x %x %x (%i)\n",
								msg->params.bristol.SysID,
								msg->params.bristol.L,
								msg->params.bristol.a,
								msg->params.bristol.b,
								index
							);

							bmidi.dev[dev].lastcommand = 0;
							bmidi.dev[dev].sysex.count = 0;
							return(rv);
						}
							//else printf("SLab SYSEX type\n");
					}

					if (bmidi.dev[dev].sysex.count == sizeof(bristolMsg))
					{
						/* Might be type 2 messages */
						if (msg->params.bristol.msgType > 7)
						{
							//printf("Received a type 2 message\n");
							if ((index+=1) >= BRISTOL_MIDI_BUFSIZE)
								index = 0;
							parsed++;
							continue;
						}

						if (bmidi.dev[dev].sysex.count
							== msg->params.bristol.msgLen)
						{
							// printf("Received a SLab SYSEX message\n");
							buildOneMsg(0xff, 0xff, dev, msg);
							bmidi.dev[dev].lastcommand = 0;
							return(msg->params.bristol.msgLen);
						} else {
							int rv = bmidi.dev[dev].sysex.count;
							// printf("unknown SYSEX message\n");
							bmidi.dev[dev].lastcommand = 0;
							bmidi.dev[dev].sysex.count = 0;
							return(rv);
						}
					}

					if (bmidi.dev[dev].sysex.count >= sizeof(bristolMsg))
					{
						if ((msg->params.bristol.msgLen >= 1023)
							|| (msg->params.bristol.msgType < 8))
						{
							int rv = bmidi.dev[dev].sysex.count;
							printf("bad SLab SYSEX message length\n");
							bmidi.dev[dev].lastcommand = 0;
							bmidi.dev[dev].sysex.count = 0;
							return(rv);
						}

						databytes[bmidi.dev[dev].sysex.count - 1
							- sizeof(bristolMsg)] = buff[index];

						//printf("inserted %x, %s\n", buff[index], databytes);

						if (bmidi.dev[dev].sysex.count
							>= msg->params.bristol.msgLen)
						{
							/* This is for strings but works generally */
							databytes[bmidi.dev[dev].sysex.count
								- sizeof(bristolMsg)] = '\0';
							buildOneMsg(0xff, 0xff, dev, msg);
							bmidi.dev[dev].lastcommand = 0;
							return(msg->params.bristol.msgLen);
						}
					}
					break;
				default:
					/*
					 * Looking for a recognised command:
					if (buff[index] & MIDI_STATUS_MASK)
						parseCommand(buff[index], dev);
					 */
					index++;

					return(1);
			}
		}

		/*
		 * Look after our indices
		 */
		if ((index+=1) >= BRISTOL_MIDI_BUFSIZE)
			index = 0;

		parsed++;
	}

	return(0);
}

