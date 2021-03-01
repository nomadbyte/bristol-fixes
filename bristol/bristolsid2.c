
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

/*#define DEBUG */

#include <math.h>

#include "bristol.h"
#include "bristolmm.h"
#include "bristolsid.h"
#include "bristolsid2.h"

static void sid2AssignVoice(sid2mods *, int, int);
static void sid2ClearVoices(sid2mods *);

static void
sid2adsr(sid2mods *smods, int id, unsigned char reg, unsigned char off, int v)
{
	switch (reg) {
		case _B_ATTACK:
			smods->sid2reg[id][off + B_SID_V1_ATT_DEC] =
				(smods->sid2reg[id][off + B_SID_V1_ATT_DEC] & 0x0f)
					| ((v << 4) & 0x0f0);
			sid_register(smods->sid2id[id], off + B_SID_V1_ATT_DEC,
				smods->sid2reg[id][off + B_SID_V1_ATT_DEC]);
			break;
		case _B_DECAY:
			smods->sid2reg[id][off + B_SID_V1_ATT_DEC] =
				(smods->sid2reg[id][off + B_SID_V1_ATT_DEC] & 0xf0)
					| (v & 0x0f);
			sid_register(smods->sid2id[id], off + B_SID_V1_ATT_DEC,
				smods->sid2reg[id][off + B_SID_V1_ATT_DEC]);
			break;
		case _B_SUSTAIN:
			smods->sid2reg[id][off + B_SID_V1_SUS_REL] =
				(smods->sid2reg[id][off + B_SID_V1_SUS_REL] & 0x0f)
					| ((v << 4) & 0x0f0);
			sid_register(smods->sid2id[id], off + B_SID_V1_SUS_REL,
				smods->sid2reg[id][off + B_SID_V1_SUS_REL]);
			break;
		case _B_RELEASE:
			smods->sid2reg[id][off + B_SID_V1_SUS_REL] =
				(smods->sid2reg[id][off + B_SID_V1_SUS_REL] & 0xf0)
					| (v & 0x0f);
			sid_register(smods->sid2id[id], off + B_SID_V1_SUS_REL,
				smods->sid2reg[id][off + B_SID_V1_SUS_REL]);
			break;
	}
}

static void
sid2cutoff(sid2mods *smods, int id, int v)
{
	smods->sid2reg[id][B_SID_FILT_LO] = v & 0x07;
	smods->sid2reg[id][B_SID_FILT_HI] = v >> 3;

	sid_register(smods->sid2id[id],
		B_SID_FILT_LO, smods->sid2reg[id][B_SID_FILT_LO]);
	sid_register(smods->sid2id[id],
		B_SID_FILT_HI, smods->sid2reg[id][B_SID_FILT_HI]);
}

static void
sid2res(sid2mods *smods, int id, int v)
{
	smods->sid2reg[id][B_SID_FILT_RES_F] = ((v << 4) & 0x0f0)
		| (smods->sid2reg[id][B_SID_FILT_RES_F] & 0x0f);

	sid_register(smods->sid2id[id],
		B_SID_FILT_RES_F, smods->sid2reg[id][B_SID_FILT_RES_F]);
}

static void
sid2volume(sid2mods *smods, int id, int v)
{
	smods->sid2reg[id][B_SID_FILT_M_VOL] = (v & 0x0f)
		| (smods->sid2reg[id][B_SID_FILT_M_VOL] & 0xf0);

	sid_register(smods->sid2id[id],
		B_SID_FILT_M_VOL, smods->sid2reg[id][B_SID_FILT_M_VOL]);
}

static void
sid2pw(sid2mods *smods, int id, unsigned char reg, int v)
{
	smods->sid2reg[id][reg] = v & 0xff;
	smods->sid2reg[id][reg + 1] = (v >> 8) & 0x0f;

	sid_register(smods->sid2id[id], reg, smods->sid2reg[id][reg]);
	sid_register(smods->sid2id[id], reg + 1, smods->sid2reg[id][reg + 1]);

	smods->pw[id] = v & 0x0fff;
}

static void
sid2flag(sid2mods *smods, int id, unsigned char reg, unsigned char flag, int v)
{
	if (v == 0)
		smods->sid2reg[id][reg] &= ~flag;
	else
		smods->sid2reg[id][reg] |= flag;

	sid_register(smods->sid2id[id], reg, smods->sid2reg[id][reg]);
}

static void
sid2dt(sid2mods *smods)
{
	if (smods->sid2reg[AUD_SID][B_SID_CONTROL]
		& (B_SID_C_DEBUG_D|B_SID_C_DEBUG_A))
	{
		smods->sid2reg[AUD_SID][B_SID_CONTROL] &=
			~(B_SID_C_DEBUG_D|B_SID_C_DEBUG_A);
		smods->sid2reg[MOD_SID][B_SID_CONTROL] &=
			~(B_SID_C_DEBUG_D|B_SID_C_DEBUG_A);
	} else {
		smods->sid2reg[AUD_SID][B_SID_CONTROL]
			|= (B_SID_C_DEBUG_D|B_SID_C_DEBUG_A);
		smods->sid2reg[MOD_SID][B_SID_CONTROL]
			|= (B_SID_C_DEBUG_D|B_SID_C_DEBUG_A);
	}

	sid_register(smods->sid2id[AUD_SID], B_SID_CONTROL,
		smods->sid2reg[AUD_SID][B_SID_CONTROL]);
	sid_register(smods->sid2id[MOD_SID], B_SID_CONTROL,
		smods->sid2reg[MOD_SID][B_SID_CONTROL]);
}

int
sid2Controller(Baudio *baudio, u_char operator, u_char controller, float value)
{
	int ivalue = value * CONTROLLER_RANGE;
	sid2mods *smods = ((sid2mods *) baudio->mixlocals);

#ifdef DEBUG
	printf("bristolSid2Control(%i, %i, %f)\n", operator, controller, value);
#endif

	if (operator != 126)
		return(0);

	if (baudio->detune != smods->detune) {
		sid_IO(smods->sid2id[AUD_SID], B_SID_DETUNE, baudio->detune * 1000);
		smods->detune = baudio->detune;
	}

	switch (controller) {
		case 0:
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 1:
			if (smods->modrouting & S_MOD_PITCH)
			{
				baudio->gtune = 1.0
					+ (baudio->note_diff - 1)
					* (value * 2 - 1);

				buildCurrentTable(baudio, baudio->gtune);
				alterAllNotes(baudio);
			}
			break;
		case 2:
			sid2flag(smods, AUD_SID, B_SID_CONTROL, B_SID_C_MULTI_V, ivalue);
			break;
		case 3:
			sid2volume(smods, AUD_SID, ivalue);
			break;
		case 4:
			sid2dt(smods);
			break;
		case 5:
			/* Key assignment mode */
			smods->keymode = ivalue;
			sid2ClearVoices(smods);
			break;
		case 6:
			/*
			 * Arpeggiator speed and modes:
			 * Want to go from something ridiculously small to max 250ms, this
			 * is the number of samples between frequency steps, triggers, 
			 * waveform scan, etc.
			 */
			smods->arpegspeed = 800 + value * value * baudio->samplerate / 4;
			break;
		case 7:
			/* Retrig */
			if (ivalue == 0)
				smods->flags &= ~B_SID2_ARPEG_TRIG;
			else
				smods->flags |= B_SID2_ARPEG_TRIG;
			break;
		case 8:
			/* Wave scanning */
			if (ivalue == 0)
				smods->flags &= ~B_SID2_ARPEG_WAVE;
			else
				smods->flags |= B_SID2_ARPEG_WAVE;
			break;

		/*
		 * The rest are operator values from the GUI with their respective
		 * values. We have to convert them into the flags or sizes and then
		 * apply them to the associated SID.
		 *
		 * Voice 1
		 */
		case 10:
			sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_NOISE, ivalue);
			break;
		case 11:
			sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_TRI, ivalue);
			break;
		case 12:
			sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_RAMP, ivalue);
			break;
		case 13:
			sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_SQUARE, ivalue);
			break;
		case 14:
			/* PW */
			sid2pw(smods, AUD_SID, B_SID_V1_PW_LO, ivalue >> 2);
			break;
		case 15:
			/*
			 * Tune - emulator parameter. This is currently a semitone which 
			 * will work but means we need to make transpose into 'n' notes, 
			 * not just a couple of octaves.
			 */
			smods->tune[B_SID_VOICE_1] = 1.0
				+ (baudio->note_diff - 1) * (value * 2 - 1);
			if (smods->ckey[B_SID_VOICE_1] != 0)
				sid2AssignVoice(smods, B_SID_VOICE_1, 0);
			break;
		case 16:
			/*
			 * Transpose - emulator parameter. This is octave transpose but
			 * we should make this into 24 semitones
			 */
			smods->transpose[B_SID_VOICE_1] = powf(baudio->note_diff, ivalue);
			if (smods->ckey[B_SID_VOICE_1] != 0)
				sid2AssignVoice(smods, B_SID_VOICE_1, 0);
			break;
		case 17:
			/* Glide - emulator parameter */
			smods->glide[B_SID_VOICE_1] =
				value * value * value * 5 * baudio->samplerate / 16;
			break;
		case 18:
			sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_RINGMOD, ivalue);
			break;
		case 19:
			sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_SYNC, ivalue);
			break;
		case 20:
			sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_TEST, ivalue);
			break;
		case 21:
			sid2flag(smods, AUD_SID, B_SID_FILT_RES_F, B_SID_F_MIX_V_1, ivalue);
			break;

		case 22:
			sid2adsr(smods, AUD_SID, _B_ATTACK, _V1_OFFSET, ivalue);
			break;
		case 23:
			sid2adsr(smods, AUD_SID, _B_DECAY, _V1_OFFSET, ivalue);
			break;
		case 24:
			sid2adsr(smods, AUD_SID, _B_SUSTAIN, _V1_OFFSET, ivalue);
			break;
		case 25:
			sid2adsr(smods, AUD_SID, _B_RELEASE, _V1_OFFSET, ivalue);
			break;

		/*
		 * The rest are operator values from the GUI with their respective
		 * values. We have to convert them into the flags or sizes and then
		 * apply them to the associated SID.
		 *
		 * Voice 2
		 */
		case 30:
			sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_NOISE, ivalue);
			break;
		case 31:
			sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_TRI, ivalue);
			break;
		case 32:
			sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_RAMP, ivalue);
			break;
		case 33:
			sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_SQUARE, ivalue);
			break;
		case 34:
			/* PW */
			sid2pw(smods, AUD_SID, B_SID_V2_PW_LO, ivalue >> 2);
			break;
		case 35:
			/* Tune - emulator parameter */
			smods->tune[B_SID_VOICE_2] = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);
			if (smods->ckey[B_SID_VOICE_2] != 0)
				sid2AssignVoice(smods, B_SID_VOICE_2, 0);
			break;
		case 36:
			/* Transpose - emulator parameter */
			smods->transpose[B_SID_VOICE_2] = powf(baudio->note_diff, ivalue);
			if (smods->ckey[B_SID_VOICE_2] != 0)
				sid2AssignVoice(smods, B_SID_VOICE_2, 0);
			break;
		case 37:
			/* Glide - emulator parameter */
			smods->glide[B_SID_VOICE_2] =
				value * value * value * 5 * baudio->samplerate / 16;
			break;
		case 38:
			sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_RINGMOD, ivalue);
			break;
		case 39:
			sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_SYNC, ivalue);
			break;
		case 40:
			sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_TEST, ivalue);
			break;
		case 41:
			sid2flag(smods, AUD_SID, B_SID_FILT_RES_F, B_SID_F_MIX_V_2, ivalue);
			break;
		case 42:
			sid2adsr(smods, AUD_SID, _B_ATTACK, _V2_OFFSET, ivalue);
			break;
		case 43:
			sid2adsr(smods, AUD_SID, _B_DECAY, _V2_OFFSET, ivalue);
			break;
		case 44:
			sid2adsr(smods, AUD_SID, _B_SUSTAIN, _V2_OFFSET, ivalue);
			break;
		case 45:
			sid2adsr(smods, AUD_SID, _B_RELEASE, _V2_OFFSET, ivalue);
			break;

		/*
		 * The rest are operator values from the GUI with their respective
		 * values. We have to convert them into the flags or sizes and then
		 * apply them to the associated SID.
		 *
		 * Voice 3
		 */
		case 50:
			sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_NOISE, ivalue);
			break;
		case 51:
			sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_TRI, ivalue);
			break;
		case 52:
			sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_RAMP, ivalue);
			break;
		case 53:
			sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_SQUARE, ivalue);
			break;
		case 54:
			/* PW */
			sid2pw(smods, AUD_SID, B_SID_V3_PW_LO, ivalue >> 2);
			break;
		case 55:
			/* Tune - emulator parameter */
			smods->tune[B_SID_VOICE_3] = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);
			if (smods->ckey[B_SID_VOICE_3] != 0)
				sid2AssignVoice(smods, B_SID_VOICE_3, 0);
			break;
		case 56:
			/* Transpose - emulator parameter */
			smods->transpose[B_SID_VOICE_3] = powf(baudio->note_diff, ivalue);
			if (smods->ckey[B_SID_VOICE_3] != 0)
				sid2AssignVoice(smods, B_SID_VOICE_3, 0);
			break;
		case 57:
			/* Glide - emulator parameter */
			smods->glide[B_SID_VOICE_3] =
				value * value * value * 5 * baudio->samplerate / 16;
			break;
		case 58:
			sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_RINGMOD, ivalue);
			break;
		case 59:
			sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_SYNC, ivalue);
			break;
		case 60:
			/*
			 * This should not use TEST, it should really OFF the voice
			sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_TEST, ivalue);
			 */
			sid2flag(smods, AUD_SID, B_SID_FILT_M_VOL, B_SID_F_3_OFF, ivalue);
			break;
		case 61:
			sid2flag(smods, AUD_SID, B_SID_FILT_RES_F, B_SID_F_MIX_V_3, ivalue);
			break;
		case 62:
			sid2adsr(smods, AUD_SID, _B_ATTACK, _V3_OFFSET, ivalue);
			break;
		case 63:
			sid2adsr(smods, AUD_SID, _B_DECAY, _V3_OFFSET, ivalue);
			break;
		case 64:
			sid2adsr(smods, AUD_SID, _B_SUSTAIN, _V3_OFFSET, ivalue);
			break;
		case 65:
			sid2adsr(smods, AUD_SID, _B_RELEASE, _V3_OFFSET, ivalue);
			break;

		/* Mod waveform */
		case 66:
			/*
			 * This looks counter intuitive however if we mod with noise then
			 * we want to generate a square wave that will be used to gate the
			 * sample and hold on the noise that comes out of the MOD_SID
			 * analogue IO
			 *
			 * The GUI is responsable for ensuring the square wave is correctly
			 * selected.
			 */
			sid2flag(smods, MOD_SID, B_SID_V3_CONTROL, B_SID_V_SQUARE, ivalue);
			if (value == 0)
				smods->modrouting &= ~S_MOD_NOISE;
			else
				smods->modrouting |= S_MOD_NOISE;
//printf("%i %i\n", controller, ivalue);
			break;
		case 67:
			sid2flag(smods, MOD_SID, B_SID_V3_CONTROL, B_SID_V_TRI, ivalue);
			if (value != 0)
				smods->modrouting &= ~S_MOD_NOISE;
//printf("%i %i\n", controller, ivalue);
			break;
		case 68:
			sid2flag(smods, MOD_SID, B_SID_V3_CONTROL, B_SID_V_RAMP, ivalue);
			if (value != 0)
				smods->modrouting &= ~S_MOD_NOISE;
//printf("%i %i\n", controller, ivalue);
			break;
		case 69:
			sid2flag(smods, MOD_SID, B_SID_V3_CONTROL, B_SID_V_SQUARE, ivalue);
			if (value != 0)
				smods->modrouting &= ~S_MOD_NOISE;
//printf("%i %i\n", controller, ivalue);
			break;

		/* Filter */
		case 70:
			sid2flag(smods, AUD_SID, B_SID_FILT_M_VOL, B_SID_F_HP, ivalue);
			break;
		case 71:
			sid2flag(smods, AUD_SID, B_SID_FILT_M_VOL, B_SID_F_BP, ivalue);
			break;
		case 72:
			sid2flag(smods, AUD_SID, B_SID_FILT_M_VOL, B_SID_F_LP, ivalue);
			break;
		case 73:
			sid2cutoff(smods, AUD_SID, ivalue >> 3 > 2047? 2047:ivalue>>3);
			break;
		case 74:
			sid2res(smods, AUD_SID, ivalue > 15? 15: ivalue);
			break;
		case 75:
			sid_IO(smods->sid2id[AUD_SID], B_SID_OBERHEIM, value);
			break;
		case 76:
			sid2flag(smods, AUD_SID, B_SID_CONTROL, B_SID_C_LPF, ivalue);
			break;

		/* LFO mod rate and level */
		case 77:
			/*
			 * Need to generate phase incrementor for 0.1 to 10 Hz, note here
			 * that the resolution at these low frequencies is not great.
			 *
			 * Phase for 0.1Hz is 0.1 * B_SID_FREQ_MULT = 1.6777216
			 * Phase for 10Hz is 10 * B_SID_FREQ_MULT = 167.77216
			 */
			smods->sid2reg[MOD_SID][B_SID_V3_FREQ_HI] =
				((int) (1.6777216 + value * value * 167.77216)) >> 8;
			smods->sid2reg[MOD_SID][B_SID_V3_FREQ_LO] =
				((int) (1.6777216 + value * value * 167.77216)) & 0xff;

			sid_register(smods->sid2id[MOD_SID], B_SID_V3_FREQ_HI,
				smods->sid2reg[MOD_SID][B_SID_V3_FREQ_HI]);
			sid_register(smods->sid2id[MOD_SID], B_SID_V3_FREQ_LO,
				smods->sid2reg[MOD_SID][B_SID_V3_FREQ_LO]);
			break;
		case 78:
			smods->lfogain = value;
			break;

		/*
		 * 14 mod routing selections
		 *
		 * This is wrong, it results in large number of if statements. We could
		 * apply mod routing constantly and manipulate pointers to affect what
		 * goes where. Not sure if this would help much though since if we do
		 * constant mod routing we also have to apply all the sid2reg() requests.
		 */
		case 79:
			if (value == 0)
				smods->modrouting &= ~S_MOD_LFO_V1_PW;
			else
				smods->modrouting |= S_MOD_LFO_V1_PW;
			break;
		case 80:
			if (value == 0)
				smods->modrouting &= ~S_MOD_LFO_V2_PW;
			else
				smods->modrouting |= S_MOD_LFO_V2_PW;
			break;
		case 81:
			if (value == 0)
				smods->modrouting &= ~S_MOD_LFO_V3_PW;
			else
				smods->modrouting |= S_MOD_LFO_V3_PW;
			break;
		case 82:
			if (value == 0)
				smods->modrouting &= ~S_MOD_LFO_V1_FREQ;
			else
				smods->modrouting |= S_MOD_LFO_V1_FREQ;
			break;
		case 83:
			if (value == 0)
				smods->modrouting &= ~S_MOD_LFO_V2_FREQ;
			else
				smods->modrouting |= S_MOD_LFO_V2_FREQ;
			break;
		case 84:
			if (value == 0)
				smods->modrouting &= ~S_MOD_LFO_V3_FREQ;
			else
				smods->modrouting |= S_MOD_LFO_V3_FREQ;
			break;
		case 85:
			if (value == 0)
				smods->modrouting &= ~S_MOD_LFO_FILTER;
			else
				smods->modrouting |= S_MOD_LFO_FILTER;
			break;
		case 86:
			if (value == 0)
				smods->modrouting &= ~S_MOD_ENV_V1_PW;
			else
				smods->modrouting |= S_MOD_ENV_V1_PW;
			break;
		case 87:
			if (value == 0)
				smods->modrouting &= ~S_MOD_ENV_V2_PW;
			else
				smods->modrouting |= S_MOD_ENV_V2_PW;
			break;
		case 88:
			if (value == 0)
				smods->modrouting &= ~S_MOD_ENV_V3_PW;
			else
				smods->modrouting |= S_MOD_ENV_V3_PW;
			break;
		case 89:
			if (value == 0)
				smods->modrouting &= ~S_MOD_ENV_V1_FREQ;
			else
				smods->modrouting |= S_MOD_ENV_V1_FREQ;
			break;
		case 90:
			if (value == 0)
				smods->modrouting &= ~S_MOD_ENV_V2_FREQ;
			else
				smods->modrouting |= S_MOD_ENV_V2_FREQ;
			break;
		case 91:
			if (value == 0)
				smods->modrouting &= ~S_MOD_ENV_V3_FREQ;
			else
				smods->modrouting |= S_MOD_ENV_V3_FREQ;
			break;
		case 92:
			if (value == 0)
				smods->modrouting &= ~S_MOD_ENV_FILTER;
			else
				smods->modrouting |= S_MOD_ENV_FILTER;
			break;

		case 93:
			sid2adsr(smods, MOD_SID, _B_ATTACK, _V3_OFFSET, ivalue);
			break;
		case 94:
			sid2adsr(smods, MOD_SID, _B_DECAY, _V3_OFFSET, ivalue);
			break;
		case 95:
			sid2adsr(smods, MOD_SID, _B_SUSTAIN, _V3_OFFSET, ivalue);
			break;
		case 96:
			sid2adsr(smods, MOD_SID, _B_RELEASE, _V3_OFFSET, ivalue);
			break;
		case 97:
			/* Env modulation gain */
			smods->envgain = value;
			break;
		case 98:
			/* Mod to LFO depth */
			if (value == 0)
				smods->modrouting &= ~S_MOD_LFO_DEPTH;
			else
				smods->modrouting |= S_MOD_LFO_DEPTH;
			break;
		case 99:
			/* Mod to ENV depth */
			if (value == 0)
				smods->modrouting &= ~S_MOD_ENV_DEPTH;
			else
				smods->modrouting |= S_MOD_ENV_DEPTH;
			break;
		case 100:
			/* Mod to pitch depth */
			if (value == 0)
				smods->modrouting &= ~S_MOD_PITCH;
			else
				smods->modrouting |= S_MOD_PITCH;
			break;
		case 101:
			/* Touch to LFO rate */
			if (value == 0)
				smods->modrouting &= ~S_MOD_T_LFO_RATE;
			else
				smods->modrouting |= S_MOD_T_LFO_RATE;
			break;
		case 102:
			/* Touch to LFO depth */
			if (value == 0)
				smods->modrouting &= ~S_MOD_T_LFO_DEPTH;
			else
				smods->modrouting |= S_MOD_T_LFO_DEPTH;
			break;
		case 103:
			/* Touch to ENV depth */
			if (value == 0)
				smods->modrouting &= ~S_MOD_T_ENV_DEPTH;
			else
				smods->modrouting |= S_MOD_T_ENV_DEPTH;
			break;

	}
	return(0);
}

/*
 * This is needed primarily on changing keymode, otherwise the algorithms can
 * leave undesirable flags, settings, etc.
 */
static void
sid2ClearVoices(sid2mods *smods)
{
	smods->flags &= ~(B_SID2_E_GATE_V1|B_SID2_E_GATE_V2|B_SID2_E_GATE_V3);

	sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_GATE, 0);
	sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_GATE, 0);
	sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_GATE, 0);

	sid2flag(smods, MOD_SID, B_SID_V3_CONTROL, B_SID_V_GATE, 0);

	smods->ckey[B_SID_VOICE_1] = smods->ckey[B_SID_VOICE_2]
		= smods->ckey[B_SID_VOICE_3] = 0;

	/*
	 * There is also the localmap however that is supposed to be a reflection
	 * of the baudio keymap to detect key events so we should leave it alone.
	 */
	smods->ccount = 0;
}

static void
sid2AssignVoice(sid2mods *smods, int voice, int flags)
{
	unsigned int nf, modf, offset = 0;

	switch (voice) {
		case B_SID_VOICE_1:
			if (flags & B_SID2_TRIG)
				smods->flags |= B_SID2_E_GATE_V1;
			offset = _V1_OFFSET;
			break;
		case B_SID_VOICE_2:
			if (flags & B_SID2_TRIG)
				smods->flags |= B_SID2_E_GATE_V2;
			offset = _V2_OFFSET;
			break;
		case B_SID_VOICE_3:
			if (flags & B_SID2_TRIG)
				smods->flags |= B_SID2_E_GATE_V3;
			offset = _V3_OFFSET;
			break;
	}

	if (flags & B_SID2_REL)
	{
		sid2flag(smods, AUD_SID, offset + B_SID_V1_CONTROL, B_SID_V_GATE, 0);
		return;
	}

//	if (smods->ckey[voice] == 0)
//		return;

	sid2flag(smods, AUD_SID, offset + B_SID_V1_CONTROL, B_SID_V_GATE, 1);

	nf = freqmap[smods->ckey[voice]];

	modf = nf * (smods->tune[voice] * smods->transpose[voice]);
	while (modf > 65535)
		modf = modf >> 1;

	//printf("%i -> %i from %f %f (%i) [%i]\n", nf, modf, smods->tune[voice],
	//smods->transpose[voice], smods->ckey[voice], freqmap[smods->ckey[voice]]);

	if ((smods->glide[voice] == 0) || (flags & B_SID2_RENEW))
	{
		sid_register(smods->sid2id[AUD_SID], offset+B_SID_V1_FREQ_LO,
			modf & 0x0ff);
		sid_register(smods->sid2id[AUD_SID], offset+B_SID_V1_FREQ_HI,
			modf >> 8);

		smods->sid2reg[AUD_SID][offset + B_SID_V1_FREQ_LO] = modf & 0x0ff;
		smods->sid2reg[AUD_SID][offset + B_SID_V1_FREQ_HI] = (modf >> 8) & 0x0ff;

		smods->dfreq[voice] = smods->cfreq[voice] = modf;
		smods->gliderate[voice] = 1.0;
	} else {
		/* Do not change the current frequence of the voice, just the target */
		smods->dfreq[voice] = modf;
		if (smods->cfreq[voice] == 0)
			smods->cfreq[voice] = smods->dfreq[voice];

		smods->gliderate[voice] =
			powf(M_E, logf(smods->dfreq[voice]/smods->cfreq[voice])
				/ (smods->glide[voice]));
	}
}

/*
 * If we have less than 3 voices assigned just look for the next free voice.
 * If not look for the middle note.
 *
 *	flag voice as occupying this midi note, 
 *	flag the midi note as known,
 */
static void
sid2Poly1NoteLogic(Baudio *baudio, sid2mods *smods)
{
	int i, low = B_SID_VOICE_1, middle = B_SID_VOICE_2, high = B_SID_VOICE_3;

	/*
	 * rather trivial voice sort. There is a special case where if a key has
	 * value '0' it should be called 'middle', ie, be the next assigned note.
	 */
	if (smods->ckey[low] > smods->ckey[middle]) {
		i = low;
		low = middle;
		middle = i;
	}
	if (smods->ckey[middle] > smods->ckey[high]) {
		i = middle;
		middle = high;
		high = i;
	}
	if (smods->ckey[low] > smods->ckey[middle]) {
		i = low;
		low = middle;
		middle = i;
	}

	if (smods->ckey[low] == 0)
	{
		i = middle;
		middle = low;
		low = middle;
	}

	/*
	 * This will be poly. We cannot use the KEYON flags since we are in 
	 * mono mode but probably also want to actually see other notes being
	 * pressed, our monophonic note logic supresses events for notes outsid2e
	 * of the preference however they are still kept in the note mapping
	 * table, we need to look at that.
	 *
	 * The matching is pretty trivial seeing as we only have three voices
	 * to track so it is not a great overhead: for each key see if it was 
	 * already known. If not, apply the 'middle' voice to it.
	 */
	for (i = 1; i < 128; i++)
	{
		/*
		 * There are 3 cases:
		 *
		 * 	a. We do not know this key - assign it.
		 * 	b. We do know this key, its assigned - do nothing.
		 * 	c. We do know this key, its gone. Free voice.
		 *
		 * If a voice gets reassigned we have to clear out previous references
		 * to it.
		 */
		if ((baudio->notemap.key[i] > 0)
			&& (smods->localmap[0][i] != i))
		{
			/* Not a known note - new */
			if (smods->ccount < 3)
			{
				/* Find a voice, apply this note */
				if (smods->ckey[B_SID_VOICE_1] == 0) {
					smods->ckey[B_SID_VOICE_1] = i;
					smods->lvel = smods->velocity[B_SID_VOICE_1]
						= baudio->notemap.velocity[i] / 127.0;
					sid2AssignVoice(smods, B_SID_VOICE_1, B_SID2_TRIG);
				} else if (smods->ckey[B_SID_VOICE_2] == 0) {
					smods->ckey[B_SID_VOICE_2] = i;
					smods->lvel = smods->velocity[B_SID_VOICE_2]
						= baudio->notemap.velocity[i] / 127.0;
					sid2AssignVoice(smods, B_SID_VOICE_2, B_SID2_TRIG);
				} else if (smods->ckey[B_SID_VOICE_3] == 0) {
					smods->ckey[B_SID_VOICE_3] = i;
					smods->lvel = smods->velocity[B_SID_VOICE_3]
						= baudio->notemap.velocity[i] / 127.0;
					sid2AssignVoice(smods, B_SID_VOICE_3, B_SID2_TRIG);
				}

				smods->ccount++;
			} else {
				/*
				 * Assign the middle key.
				 */
				smods->ckey[middle] = i;
				smods->lvel = smods->velocity[middle]
					= baudio->notemap.velocity[i] / 127.0;
				sid2AssignVoice(smods, middle, B_SID2_TRIG);
			}

			smods->localmap[0][i] = i;
		} else if ((baudio->notemap.key[i] <= 0)
			&& (smods->localmap[0][i] == i)) {
			int j, fv = -1;

			/* Was a known key, note off if allocated */
			if (smods->ckey[B_SID_VOICE_1] == i) {
				fv = B_SID_VOICE_1;
				sid2AssignVoice(smods, B_SID_VOICE_1, B_SID2_REL);
				smods->ckey[B_SID_VOICE_1] = 0;
				smods->ccount--;
			} else if (smods->ckey[B_SID_VOICE_2] == i) {
				fv = B_SID_VOICE_2;
				sid2AssignVoice(smods, B_SID_VOICE_2, B_SID2_REL);
				smods->ckey[B_SID_VOICE_2] = 0;
				smods->ccount--;
			} else if (smods->ckey[B_SID_VOICE_3] == i) {
				fv = B_SID_VOICE_3;
				sid2AssignVoice(smods, B_SID_VOICE_3, B_SID2_REL);
				smods->ckey[B_SID_VOICE_3] = 0;
				smods->ccount--;
			}

			smods->localmap[0][i] = 0;

			if (smods->ccount < 0)
				smods->ccount = 0;

			/*
			 * We should also consid2er what could be done if ccount was three
			 * and falls such that we could reassign a key back to another note
			 * that was pre-empted. We can only really do this in Poly-1 where
			 * all the voices have the same sounds. What we want to do is try
			 * to ensure that if we have keys pressed that we attempt to use
			 * the voices and for now we will do that irrespective of the mode
			 * since three voices is pretty spartan.
			 */
			if (fv >= B_SID_VOICE_1)
				for (j = 128; j > 0; j--)
				{
					if ((smods->localmap[0][j] > 0)
						&& (smods->ckey[B_SID_VOICE_1] != j)
						&& (smods->ckey[B_SID_VOICE_2] != j)
						&& (smods->ckey[B_SID_VOICE_3] != j))
					{
						//printf("Reapply note %i\n", j);
						smods->ckey[fv] = j;
						smods->velocity[middle]
							= baudio->notemap.velocity[j] / 127.0;
						/* Request new frequency, no trigger */
						sid2AssignVoice(smods, fv, B_SID2_RENEW);
						smods->localmap[0][j] = j;
						smods->ccount++;
					}
				}
		}
	}
}

/*
 * Take a fixed split point, below this point stuff the arpeggiation table, 
 * above this point use duophonic note preference - we should consid2er always
 * having the voices active if there is a note pressed, acting as a monophonic
 * with two voices where there is only one note.
 */
static void
sid2Poly3NoteLogic(Baudio *baudio, sid2mods *smods)
{
	int i, ac = 0;
	//int occ = 0;

	bzero(smods->arpegtable, B_SID2_ARPEG_MAX * sizeof(unsigned short));

	/* 52 is middle-e on the GUI */
	for (i = 127; i >= 52; i--)
	{
		if (baudio->notemap.key[i] != 0)
			smods->arpegtable[ac++] = i;
		smods->localmap[0][i] = baudio->notemap.key[i] == 0? 0:i;
	}

	ac = 0;

	for (i = 1; i < 52; i++)
	{
		/*
		 * There are 3 cases:
		 *
		 * 	a. We do not know this key:
		 * 		Assign to a voice
		 * 	b. We do know this key, its assigned - do nothing.
		 * 	c. We do know this key, its gone:
		 * 		Free voice if it was Voice-1 or Voice-3
		 */
		if (baudio->notemap.key[i] > 0)
		{
			if (smods->localmap[0][i] != i)
			{
				int voice = B_SID_VOICE_3;

				/*
				 * Not a known note - new , Check V1 and V3
				 *	if V1 == 0, take it
				 *	if not is V3 == 0, take it
				 *	if not take highest of V1, V3
				 */
				if ((smods->ckey[B_SID_VOICE_1] == 0) ||
					((smods->ckey[B_SID_VOICE_3] != 0) &&
					(smods->ckey[B_SID_VOICE_1] > smods->ckey[B_SID_VOICE_3])))
					voice = B_SID_VOICE_1;

				smods->ckey[voice] = i;
				smods->lvel = smods->velocity[voice]
					= baudio->notemap.velocity[i] / 127.0;
				sid2AssignVoice(smods, voice, B_SID2_TRIG);
				smods->ccount++;
				smods->localmap[0][i] = i;
			}

			smods->localmap[0][i] = i;
		} else
			if ((baudio->notemap.key[i] <= 0) && (smods->localmap[0][i] == i))
		{
			/* Was a known key? note off only if allocated */
			if (smods->ckey[B_SID_VOICE_1] == i)
			{
				sid2AssignVoice(smods, B_SID_VOICE_1, B_SID2_REL);
				smods->ckey[B_SID_VOICE_1] = 0;
				smods->ccount--;
				//occ++;
			} else if (smods->ckey[B_SID_VOICE_3] == i) {
				sid2AssignVoice(smods, B_SID_VOICE_3, B_SID2_REL);
				smods->ckey[B_SID_VOICE_3] = 0;
				smods->ccount--;
				//occ++;
			}

			smods->localmap[0][i] = 0;

			if (smods->ccount < 0)
				smods->ccount = 0;
		}
	}

/*
	if (occ)
	{
		printf("low %i, high %i\narpeg:",
			smods->ckey[B_SID_VOICE_1], smods->ckey[B_SID_VOICE_3]);
		for (ac = 0; smods->arpegtable[ac] != 0; ac++)
			printf(" %i,", smods->arpegtable[ac]);
		printf("\n");
	}
*/
}

/*
 * Take a fixed split point, below this point stuff the arpeggiation table, 
 * above this point use duophonic note preference - we should consid2er always
 * having the voices active if there is a note pressed, acting as a monophonic
 * with two voices where there is only one note.
 */
static void
sid2Poly4NoteLogic(Baudio *baudio, sid2mods *smods)
{
	int i, ac = 0;
	//int occ = 0;

	bzero(smods->arpegtable, B_SID2_ARPEG_MAX * sizeof(unsigned short));

	/* 52 is middle-e on the GUI */
	for (i = 1; i < 52; i++)
	{
		if (baudio->notemap.key[i] != 0)
			smods->arpegtable[ac++] = i;
		smods->localmap[0][i] = baudio->notemap.key[i] == 0? 0:i;
	}

	ac = 0;

	for (i = 127; i >= 52; i--)
	{
		/*
		 * There are 3 cases:
		 *
		 * 	a. We do not know this key:
		 * 		Assign to a voice
		 * 	b. We do know this key, its assigned - do nothing.
		 * 	c. We do know this key, its gone:
		 * 		Free voice if it was Voice-1 or Voice-3
		 */
		if (baudio->notemap.key[i] > 0)
		{
			if (smods->localmap[0][i] != i)
			{
				int voice = B_SID_VOICE_3;

				/*
				 * Not a known note - new , Check V1 and V3
				 *	if V1 == 0, take it
				 *	if not is V3 == 0, take it
				 *	if not take lowest of V1, V3
				 */
				if ((smods->ckey[B_SID_VOICE_1] == 0) ||
					((smods->ckey[B_SID_VOICE_3] != 0) && 
					(smods->ckey[B_SID_VOICE_1] < smods->ckey[B_SID_VOICE_3])))
					voice = B_SID_VOICE_1;

				smods->ckey[voice] = i;
				smods->lvel = smods->velocity[voice]
					= baudio->notemap.velocity[i] / 127.0;
				sid2AssignVoice(smods, voice, B_SID2_TRIG);
				smods->ccount++;
				smods->localmap[0][i] = i;
			}

			smods->localmap[0][i] = i;
		} else if ((baudio->notemap.key[i] <= 0)
			&& (smods->localmap[0][i] == i))
		{
			/* Was a known key? note off if allocated */
			if (smods->ckey[B_SID_VOICE_1] == i)
			{
				sid2AssignVoice(smods, B_SID_VOICE_1, B_SID2_REL);
				smods->ckey[B_SID_VOICE_1] = 0;
				smods->ccount--;
				//occ++;
			} else if (smods->ckey[B_SID_VOICE_3] == i) {
				sid2AssignVoice(smods, B_SID_VOICE_3, B_SID2_REL);
				smods->ckey[B_SID_VOICE_3] = 0;
				smods->ccount--;
				//occ++;
			}

			smods->localmap[0][i] = 0;

			if (smods->ccount < 0)
				smods->ccount = 0;
		}
	}

/*
	if (occ)
	{
		printf("low %i, high %i\narpeg:",
			smods->ckey[B_SID_VOICE_1], smods->ckey[B_SID_VOICE_3]);
		for (ac = 0; smods->arpegtable[ac] != 0; ac++)
			printf(" %i,", smods->arpegtable[ac]);
		printf("\n");
	}
*/
}

static int
sid2CheckModTrigger(sid2mods *smods)
{
	if ((smods->flags & (B_SID2_E_GATE_V1|B_SID2_E_GATE_V2|B_SID2_E_GATE_V3)) == 0)
		return(0);

	switch (smods->keymode) {
		default:
		case B_S_KEYMODE_MONO:
		case B_S_KEYMODE_POLY1:
			/* All notes are the same - trigger */
			if (smods->flags
				& (B_SID2_E_GATE_V1|B_SID2_E_GATE_V2|B_SID2_E_GATE_V3))
				return(1);
			break;
		case B_S_KEYMODE_POLY2:
			/*
			 * Check if Envmod affects this voice - mod routing, all voices
			 *
			 *	Do we go to filter and mod goes to filter? Trig
			 *	Does mod go to this voice? Trig
			 */
			if ((smods->flags & B_SID2_E_GATE_V1) &&
				/* Filter match */
				(((smods->sid2reg[AUD_SID][B_SID_FILT_RES_F] & B_SID_F_MIX_V_1)
					&& (smods->modrouting & S_MOD_ENV_FILTER))
				|| /* Mod me match */
					(smods->modrouting & (S_MOD_ENV_V1_PW|S_MOD_ENV_V1_FREQ))))
				return(1);
			if ((smods->flags & B_SID2_E_GATE_V2) &&
				/* Filter match */
				(((smods->sid2reg[AUD_SID][B_SID_FILT_RES_F] & B_SID_F_MIX_V_2)
					&& (smods->modrouting & S_MOD_ENV_FILTER))
				|| /* Mod me match */
					(smods->modrouting & (S_MOD_ENV_V2_PW|S_MOD_ENV_V2_FREQ))))
				return(1);
			if ((smods->flags & B_SID2_E_GATE_V3) &&
				/* Filter match */
				(((smods->sid2reg[AUD_SID][B_SID_FILT_RES_F] & B_SID_F_MIX_V_3)
					&& (smods->modrouting & S_MOD_ENV_FILTER))
				|| /* Mod me match */
					(smods->modrouting & (S_MOD_ENV_V3_PW|S_MOD_ENV_V3_FREQ))))
				return(1);
			break;
		case B_S_KEYMODE_POLY3: /* Arpeg-1 */
		case B_S_KEYMODE_POLY4: /* Arpeg-2 */
			/* Check if Envmod affects this voice - mod routing, voice-1/3 */
			if ((smods->flags & B_SID2_E_GATE_V1) &&
				/* Filter match */
				(((smods->sid2reg[AUD_SID][B_SID_FILT_RES_F] & B_SID_F_MIX_V_1)
					&& (smods->modrouting & S_MOD_ENV_FILTER))
				|| /* Mod me match */
					(smods->modrouting & (S_MOD_ENV_V1_PW|S_MOD_ENV_V1_FREQ))))
				return(1);
			if ((smods->flags & B_SID2_E_GATE_V3) &&
				/* Filter match */
				(((smods->sid2reg[AUD_SID][B_SID_FILT_RES_F] & B_SID_F_MIX_V_3)
					&& (smods->modrouting & S_MOD_ENV_FILTER))
				|| /* Mod me match */
					(smods->modrouting & (S_MOD_ENV_V3_PW|S_MOD_ENV_V3_FREQ))))
				return(1);
			break;
	}

	return(0);
}

/*
 * We have monophonic voice assignment, ie, this should only be called once
 * although that can be overridden with options. We are not too concerned with
 * the midi note logic giving us its idea of which note we should be playing,
 * we have 'n' voices, 3 to start with, and some note assignement logic:
 *
 * 	Mono: assign all voices to the suggested key
 * 	Poly-1: all voices will have the same sound, 3 voices. GUI to enforce this.
 * 	Poly-2: each voice can have its own sound, 3 voices.
 * 	Poly-3: two lead voices, rest of notes have rapid arpeggiation
 *
 * We need to keep a mapping of midi note to voice allocation and also of voice
 * to midi note for tuning so there will be a few numbers hanging around in
 * different tables. If a voice gets a new frequency then it should also be 
 * given a GATE off/on to trigger the env. Poly note logic in 3 voice mode
 * should not take the extreme notes, it should take middle ones.
 *
 * The code should be written with multiple SID in mind. The first release had
 * two SID: one for 3 audio voices and another for mods. It would be quite 
 * easy to have generalised this to 5 voices with one for mod and the two 
 * filter under commmon control. This should be consid2ered for the poly modes,
 * mono is probably already enough with 3 voices and up to 9 oscillators.
 */
int
operateOneSid2(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int vcount = 0, i, tmp3;
	sid2mods *smods = ((sid2mods *) baudio->mixlocals);
	float env3, osc3, pitch = 1.0;

	/*printf("operateOneSid2(%i, %x, %x)\n", voice->index, audiomain, baudio);*/

	for (i = 0; i < 127; i++)
		if (baudio->notemap.key[i] != 0)
			vcount++;

	if (smods->keymode == B_S_KEYMODE_MONO)
	{
		if (vcount == 0) {
			if (smods->ckey[B_SID_VOICE_1] > 0) {
				sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_GATE, 0);
				sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_GATE, 0);
				sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_GATE, 0);
				sid2flag(smods, MOD_SID, B_SID_V3_CONTROL, B_SID_V_GATE, 0);
			}
			smods->ckey[B_SID_VOICE_1] = smods->ckey[B_SID_VOICE_2]
				= smods->ckey[B_SID_VOICE_3] = 0;
			smods->lvel
				= smods->velocity[B_SID_VOICE_1]
				= smods->velocity[B_SID_VOICE_2]
				= smods->velocity[B_SID_VOICE_3]
					= baudio->notemap.velocity[voice->key.key] / 127.0;
		} else if (voice->key.key != smods->ckey[B_SID_VOICE_1]) {
			smods->ckey[B_SID_VOICE_1]
				= smods->ckey[B_SID_VOICE_2]
				= smods->ckey[B_SID_VOICE_3]
					= voice->key.key;
			smods->localmap[AUD_SID][voice->key.key] = voice->key.key;

			/*
			 * Reassigned key, take frequency tables, tune and transpose them,
			 * apply them to the voices and finally request a flap of the GATE.
			 */
			sid2AssignVoice(smods, B_SID_VOICE_1, B_SID2_NEW|B_SID2_TRIG);
			sid2AssignVoice(smods, B_SID_VOICE_2, B_SID2_NEW|B_SID2_TRIG);
			sid2AssignVoice(smods, B_SID_VOICE_3, B_SID2_NEW|B_SID2_TRIG);

			/* Take the velocity last, sid2Assign can take oldest value */
			smods->lvel
				= smods->velocity[B_SID_VOICE_1]
				= smods->velocity[B_SID_VOICE_2]
				= smods->velocity[B_SID_VOICE_3]
					= baudio->notemap.velocity[voice->key.key] / 127.0;
		}
	} else if ((smods->keymode == B_S_KEYMODE_POLY1)
		|| (smods->keymode == B_S_KEYMODE_POLY2))
		/* Polyphonic voice allocation, no arpeggiation */
		sid2Poly1NoteLogic(baudio, smods);
	else if (smods->keymode == B_S_KEYMODE_POLY3)
		/* High split */
		sid2Poly3NoteLogic(baudio, smods);
	else
		/* Low Split */
		sid2Poly4NoteLogic(baudio, smods);

	if (smods->modrouting & S_MOD_T_LFO_RATE)
	{
		tmp3 = ((smods->sid2reg[MOD_SID][B_SID_V3_FREQ_HI] << 8)
			+ smods->sid2reg[MOD_SID][B_SID_V3_FREQ_LO]) * smods->lvel;

		//printf("%i %f %f\n", tmp3, smods->lvel, smods->velocity[0]);

		sid_register(smods->sid2id[MOD_SID], B_SID_V3_FREQ_HI, tmp3 >> 8);
		sid_register(smods->sid2id[MOD_SID], B_SID_V3_FREQ_LO, tmp3 & 0x0ff);
	}

	/*
	 * A majority of the mods can be applied intermittently, ie, we should
	 * optimise the code to do 16 samples at a time and take mods every 16 
	 * samples. The optimisation is actually minimal since the main overhead
	 * is in the chip anyway but this does allow the code to insert the 
	 * arpeggiation easily as that will be based on multiples of 16 samples.
	 *
	 * The net gain for going from 1 to 16 samples was about 15%.
	 */
	for (i = 0; i < audiomain->samplecount; i+=16)
	{
		/* 
		 * We retrigger the MOD env only on specific keys: Note that we do not
		 * trigger the MOD env on arpeggiation steps.
		 */
		if (sid2CheckModTrigger(smods))
		{
			sid2flag(smods, MOD_SID, B_SID_V3_CONTROL, B_SID_V_GATE, 0);
			smods->modout =
				sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
			sid2flag(smods, MOD_SID, B_SID_V3_CONTROL, B_SID_V_GATE, 1);
		} else
			smods->modout =
				sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);

		/*
		 * If the selected waveform is noise then we need to only take new osc3
		 * every 'number' of samples, that will be defined by the arpeggiation
		 * rate. It is non-trivial to take it from the MOD_SID.
		 */
		smods->sid2reg[MOD_SID][B_SID_OSC_3_OUT] = osc3
			= sid_register(smods->sid2id[MOD_SID], B_SID_OSC_3_OUT, 0);
		smods->sid2reg[MOD_SID][B_SID_ENV_3_OUT] = env3
			= sid_register(smods->sid2id[MOD_SID], B_SID_ENV_3_OUT, 0);

		if (smods->modrouting & S_MOD_NOISE) {
			if (((smods->noisegate == 0) && (osc3 > 0))
				|| ((smods->noisegate != 0) && (osc3 == 0)))
			{
				/* The scaling is empirical */
				if ((smods->noisesample = smods->modout * 255) > 255)
					smods->noisesample = 255;
			}
			smods->noisegate = osc3;
			osc3 = smods->noisesample;
		}

		/*
		 * Apply glide, mods, arpeggiate. The values extracted from the 
		 * registers are the MSB 8bit of osc and the whole of env. We have to
		 * scale this to apply it to osc, pw and filt.
		 *
		 * Test this with osc to PW and env to filter. We should also look at 
		 * using plain bit shifting which implies limited depth of mods though,
		 * to 3 or 4 bit settings? Maybe just take mults for now.
		 *
		 * Osc to PW:
		 * 	8 bit to 12 bit
		 *
		 * There are some minor optimisations possible to this code.
		 */
		if (smods->modrouting & (S_MOD_LFO_V1_PW|S_MOD_ENV_V1_PW))
		{
			/*
			 * This will change with glide: current frequency should be held
			 * as a float (same value, different format) and we will modulate
			 * that value, later applying it to the registers.
			 */
			tmp3 = smods->sid2reg[AUD_SID][B_SID_V1_PW_LO]
				+ ((smods->sid2reg[AUD_SID][B_SID_V1_PW_HI] & 0x0f) << 8);

			if (smods->modrouting & S_MOD_LFO_V1_PW)
				tmp3 += osc3 * smods->lfogain * 15.0
					* (smods->modrouting & S_MOD_LFO_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_LFO_DEPTH?
						smods->velocity[B_SID_VOICE_1]:1);

			if (smods->modrouting & S_MOD_ENV_V1_PW)
				tmp3 += env3 * smods->envgain * 15.0
					* (smods->modrouting & S_MOD_ENV_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_ENV_DEPTH?
						smods->velocity[B_SID_VOICE_1]:1);

			if (tmp3 > 4095)
				tmp3 = 4095;

			sid_register(smods->sid2id[AUD_SID], B_SID_V1_PW_LO,
				tmp3 & 0x0ff);
			sid_register(smods->sid2id[AUD_SID], B_SID_V1_PW_HI,
				(tmp3 >> 8) & 0x0f);
		}

		if (smods->modrouting & (S_MOD_LFO_V2_PW|S_MOD_ENV_V2_PW))
		{
			tmp3 = smods->sid2reg[AUD_SID][B_SID_V2_PW_LO]
				+ ((smods->sid2reg[AUD_SID][B_SID_V2_PW_HI] & 0x0f) << 8);

			if (smods->modrouting & S_MOD_LFO_V2_PW)
				tmp3 += osc3 * smods->lfogain * 15.0
					* (smods->modrouting & S_MOD_LFO_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_LFO_DEPTH?
						smods->velocity[B_SID_VOICE_2]:1);

			if (smods->modrouting & S_MOD_ENV_V2_PW)
				tmp3 += env3 * smods->envgain * 15.0
					* (smods->modrouting & S_MOD_ENV_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_ENV_DEPTH?
						smods->velocity[B_SID_VOICE_2]:1);

			if (tmp3 > 4095)
				tmp3 = 4095;

			sid_register(smods->sid2id[AUD_SID], B_SID_V2_PW_LO,
				tmp3 & 0x0ff);
			sid_register(smods->sid2id[AUD_SID], B_SID_V2_PW_HI,
				(tmp3 >> 8) & 0x0f);
		}

		if (smods->modrouting & (S_MOD_LFO_V3_PW|S_MOD_ENV_V3_PW))
		{
			tmp3 = smods->sid2reg[AUD_SID][B_SID_V3_PW_LO]
				+ ((smods->sid2reg[AUD_SID][B_SID_V3_PW_HI] & 0x0f) << 8);

			if (smods->modrouting & S_MOD_LFO_V3_PW)
				tmp3 += osc3 * smods->lfogain * 15.0
					* (smods->modrouting & S_MOD_LFO_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_LFO_DEPTH?
						smods->velocity[B_SID_VOICE_3]:1);

			if (smods->modrouting & S_MOD_ENV_V3_PW)
				tmp3 += env3 * smods->envgain * 15
					* (smods->modrouting & S_MOD_ENV_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_ENV_DEPTH?
						smods->velocity[B_SID_VOICE_3]:1);

			if (tmp3 > 4095)
				tmp3 = 4095;

			sid_register(smods->sid2id[AUD_SID], B_SID_V3_PW_LO,
				tmp3 & 0x0ff);
			sid_register(smods->sid2id[AUD_SID], B_SID_V3_PW_HI,
				(tmp3 >> 8) & 0x0f);
		}

		if (smods->modrouting & S_MOD_PITCH)
		{
			if ((pitch = (baudio->contcontroller[1] - 0.5) * 2) != 0)
			{
				pitch = powf(baudio->note_diff, pitch * 2);

				tmp3 = smods->cfreq[B_SID_VOICE_1];
				if ((tmp3 *= pitch) > 65535)
					tmp3 = 65535;
				sid_register(smods->sid2id[AUD_SID], B_SID_V1_FREQ_LO,
					smods->sid2reg[AUD_SID][B_SID_V1_FREQ_LO] = tmp3 & 0x0ff);
				sid_register(smods->sid2id[AUD_SID], B_SID_V1_FREQ_HI,
					smods->sid2reg[AUD_SID][B_SID_V1_FREQ_HI] = (tmp3 >> 8));

				tmp3 = smods->cfreq[B_SID_VOICE_2];
				if ((tmp3 *= pitch) > 65535)
					tmp3 = 65535;
				sid_register(smods->sid2id[AUD_SID], B_SID_V2_FREQ_LO,
					smods->sid2reg[AUD_SID][B_SID_V2_FREQ_LO] = tmp3 & 0x0ff);
				sid_register(smods->sid2id[AUD_SID], B_SID_V2_FREQ_HI,
					smods->sid2reg[AUD_SID][B_SID_V2_FREQ_HI] = (tmp3 >> 8));

				tmp3 = smods->cfreq[B_SID_VOICE_3];
				if ((tmp3 *= pitch) > 65535)
					tmp3 = 65535;
				sid_register(smods->sid2id[AUD_SID], B_SID_V3_FREQ_LO,
					smods->sid2reg[AUD_SID][B_SID_V3_FREQ_LO] = tmp3 & 0x0ff);
				sid_register(smods->sid2id[AUD_SID], B_SID_V3_FREQ_HI,
					smods->sid2reg[AUD_SID][B_SID_V3_FREQ_HI] = (tmp3 >> 8));
			}
		}

		/*
		 * Mods to freq. These may be a little strong, they will cover the whole
		 * frequency range at full throw which means little control at lower
		 * values, consequently we make it a power curve.
		 *
		 * These are 8 bit to 16 bit at full throw.
		 */
		if (smods->modrouting & (S_MOD_LFO_V1_FREQ|S_MOD_ENV_V1_FREQ))
		{
			tmp3 = smods->sid2reg[AUD_SID][B_SID_V1_FREQ_LO]
				+ (smods->sid2reg[AUD_SID][B_SID_V1_FREQ_HI] << 8);

			if (smods->modrouting & S_MOD_LFO_V1_FREQ)
				tmp3 *= (1.0 + osc3 * smods->lfogain * smods->lfogain * 0.015625
					* (smods->modrouting & S_MOD_LFO_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_LFO_DEPTH?
						smods->velocity[B_SID_VOICE_1]:1));

			if (smods->modrouting & S_MOD_ENV_V1_FREQ)
				tmp3 *= (1.0 + env3 * smods->envgain * smods->envgain * 0.015625
					* (smods->modrouting & S_MOD_ENV_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_ENV_DEPTH?
						smods->velocity[B_SID_VOICE_1]:1));

			if (tmp3 > 65535)
				tmp3 = 65535;

			sid_register(smods->sid2id[AUD_SID], B_SID_V1_FREQ_LO,
				tmp3 & 0x0ff);
			sid_register(smods->sid2id[AUD_SID], B_SID_V1_FREQ_HI,
				(tmp3 >> 8) & 0x0ff);
		}
		if (smods->modrouting & (S_MOD_LFO_V2_FREQ|S_MOD_ENV_V2_FREQ))
		{
			tmp3 = smods->sid2reg[AUD_SID][B_SID_V2_FREQ_LO]
				+ (smods->sid2reg[AUD_SID][B_SID_V2_FREQ_HI] << 8);

			if (smods->modrouting & S_MOD_LFO_V2_FREQ)
				tmp3 *= (1.0 + osc3 * smods->lfogain * smods->lfogain * 0.015625
					* (smods->modrouting & S_MOD_LFO_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_LFO_DEPTH?
						smods->velocity[B_SID_VOICE_2]:1));

			if (smods->modrouting & S_MOD_ENV_V2_FREQ)
				tmp3 *= (1.0 + env3 * smods->envgain * smods->envgain * 0.015625
					* (smods->modrouting & S_MOD_ENV_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_ENV_DEPTH?
						smods->velocity[B_SID_VOICE_2]:1));

			if (tmp3 > 65535)
				tmp3 = 65535;

			sid_register(smods->sid2id[AUD_SID], B_SID_V2_FREQ_LO,
				tmp3 & 0x0ff);
			sid_register(smods->sid2id[AUD_SID], B_SID_V2_FREQ_HI,
				(tmp3 >> 8) & 0x0ff);
		}
		if (smods->modrouting & (S_MOD_LFO_V3_FREQ|S_MOD_ENV_V3_FREQ))
		{
			tmp3 = smods->sid2reg[AUD_SID][B_SID_V3_FREQ_LO]
				+ (smods->sid2reg[AUD_SID][B_SID_V3_FREQ_HI] << 8);

			if (smods->modrouting & S_MOD_LFO_V3_FREQ)
				tmp3 *= (1.0 + osc3 * smods->lfogain * smods->lfogain * 0.015625
					* (smods->modrouting & S_MOD_LFO_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_LFO_DEPTH?
						smods->velocity[B_SID_VOICE_3]:1));

			if (smods->modrouting & S_MOD_ENV_V3_FREQ)
				tmp3 *= (1.0 + env3 * smods->envgain * smods->envgain * 0.015625
					* (smods->modrouting & S_MOD_ENV_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_ENV_DEPTH?
						smods->velocity[B_SID_VOICE_3]:1));

			if (tmp3 > 65535)
				tmp3 = 65535;

			sid_register(smods->sid2id[AUD_SID], B_SID_V3_FREQ_LO,
				tmp3 & 0x0ff);
			sid_register(smods->sid2id[AUD_SID], B_SID_V3_FREQ_HI,
				(tmp3 >> 8) & 0x0ff);
		}

		/*
		 * Mods to filter:
		 *	8 bit to 11 bit
		 */
		if (smods->modrouting & (S_MOD_LFO_FILTER|S_MOD_ENV_FILTER))
		{
			tmp3 = (smods->sid2reg[AUD_SID][B_SID_FILT_LO] & 0x07)
				+ (smods->sid2reg[AUD_SID][B_SID_FILT_HI] << 3);

			if (smods->modrouting & S_MOD_LFO_FILTER)
				tmp3 += osc3 * smods->lfogain * smods->lfogain * 8.0
					* (smods->modrouting & S_MOD_LFO_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_LFO_DEPTH? smods->lvel:1);

			if (smods->modrouting & S_MOD_ENV_FILTER)
				tmp3 += env3 * smods->envgain * smods->envgain * 8.0
					* (smods->modrouting & S_MOD_ENV_DEPTH?
						baudio->contcontroller[1]:1)
					* (smods->modrouting & S_MOD_T_ENV_DEPTH? smods->lvel:1);

			if (tmp3 > 2047)
				tmp3 = 2047;

			sid_register(smods->sid2id[AUD_SID], B_SID_FILT_LO,
				tmp3 & 0x07);
			sid_register(smods->sid2id[AUD_SID], B_SID_FILT_HI,
				(tmp3 >> 3) & 0xff);
		}

		/*
		 * Check if any voices need GATEing:
		 */
		if (smods->flags & B_SID2_E_GATE_V1)
			sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_GATE, 0);
		if (smods->flags & B_SID2_E_GATE_V2)
			sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_GATE, 0);
		if (smods->flags & B_SID2_E_GATE_V3)
			sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_GATE, 0);

		/*
		 * This is the main generator however due to mods this may only be a 
		 * small part of the overall CPU load. It can be optimised by doing
		 * 'n' samples here however then we would also need to bury a similar
		 * operation into the MOD_SID as well.
		 */
		baudio->leftbuf[i] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;

		/*
		 * Clear any gating flags. This looks late by one sample but this is
		 * needed for the regular retrigger operations, we only have 3 voices.
		 */
		if (vcount > 0) {
			if (smods->flags & B_SID2_E_GATE_V1)
				sid2flag(smods, AUD_SID, B_SID_V1_CONTROL, B_SID_V_GATE, 1);
			smods->flags &= ~B_SID2_E_GATE_V1;
			if (smods->flags & B_SID2_E_GATE_V2)
				sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_GATE, 1);
			smods->flags &= ~B_SID2_E_GATE_V2;
			if (smods->flags & B_SID2_E_GATE_V3)
				sid2flag(smods, AUD_SID, B_SID_V3_CONTROL, B_SID_V_GATE, 1);
			smods->flags &= ~B_SID2_E_GATE_V3;
		} else
			smods->flags &= ~(B_SID2_E_GATE_V1|B_SID2_E_GATE_V2|B_SID2_E_GATE_V3);

		baudio->leftbuf[i + 1] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 2] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 3] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 4] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 5] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 6] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 7] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 8] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 9] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 10] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 11] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 12] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 13] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 14] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;
		baudio->leftbuf[i + 15] =
			sid_IO(smods->sid2id[AUD_SID], B_SID_ANALOGUE_IO, 0.0) * 32767.0;

		/* And clock the MOD_SID forward too */
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);
		sid_IO(smods->sid2id[MOD_SID], B_SID_ANALOGUE_IO, 0.0);

		/* Check for glide */
		if (smods->cfreq[B_SID_VOICE_1] != smods->dfreq[B_SID_VOICE_1])
		{
			int ifreq;

			if (smods->gliderate[B_SID_VOICE_1] == 0)
				smods->cfreq[B_SID_VOICE_1] = smods->dfreq[B_SID_VOICE_1];
			else {
				if ((smods->gliderate[B_SID_VOICE_1] > 1.0)
					&& ((smods->cfreq[B_SID_VOICE_1]
						*= smods->gliderate[B_SID_VOICE_1])
						> smods->dfreq[B_SID_VOICE_1]))
					smods->cfreq[B_SID_VOICE_1] = smods->dfreq[B_SID_VOICE_1];
				else if ((smods->gliderate[B_SID_VOICE_1] < 1.0)
					&& ((smods->cfreq[B_SID_VOICE_1]
						*= smods->gliderate[B_SID_VOICE_1])
						< smods->dfreq[B_SID_VOICE_1]))
					smods->cfreq[B_SID_VOICE_1] = smods->dfreq[B_SID_VOICE_1];
			}

			ifreq = smods->cfreq[B_SID_VOICE_1];

			sid_register(smods->sid2id[AUD_SID], B_SID_V1_FREQ_LO,
				ifreq & 0x0ff);
			sid_register(smods->sid2id[AUD_SID], B_SID_V1_FREQ_HI,
				ifreq >> 8);

			smods->sid2reg[AUD_SID][B_SID_V1_FREQ_LO] = ifreq & 0x0ff;
			smods->sid2reg[AUD_SID][B_SID_V1_FREQ_HI] = (ifreq >> 8) & 0x0ff;
		}
		if (smods->cfreq[B_SID_VOICE_2] != smods->dfreq[B_SID_VOICE_2])
		{
			int ifreq;

			if (smods->gliderate[B_SID_VOICE_2] == 0)
				smods->cfreq[B_SID_VOICE_2] = smods->dfreq[B_SID_VOICE_2];
			else {
				if ((smods->gliderate[B_SID_VOICE_2] > 1.0)
					&& ((smods->cfreq[B_SID_VOICE_2]
						*= smods->gliderate[B_SID_VOICE_2])
						> smods->dfreq[B_SID_VOICE_2]))
					smods->cfreq[B_SID_VOICE_2] = smods->dfreq[B_SID_VOICE_2];
				else if ((smods->gliderate[B_SID_VOICE_2] < 1.0)
					&& ((smods->cfreq[B_SID_VOICE_2]
						*= smods->gliderate[B_SID_VOICE_2])
						< smods->dfreq[B_SID_VOICE_2]))
					smods->cfreq[B_SID_VOICE_2] = smods->dfreq[B_SID_VOICE_2];
			}

			ifreq = smods->cfreq[B_SID_VOICE_2];

			sid_register(smods->sid2id[AUD_SID], B_SID_V2_FREQ_LO,
				ifreq & 0x0ff);
			sid_register(smods->sid2id[AUD_SID], B_SID_V2_FREQ_HI,
				ifreq >> 8);

			smods->sid2reg[AUD_SID][B_SID_V2_FREQ_LO] = ifreq & 0x0ff;
			smods->sid2reg[AUD_SID][B_SID_V2_FREQ_HI] = (ifreq >> 8) & 0x0ff;
		}
		if (smods->cfreq[B_SID_VOICE_3] != smods->dfreq[B_SID_VOICE_3])
		{
			int ifreq;

			if (smods->gliderate[B_SID_VOICE_3] == 0)
				smods->cfreq[B_SID_VOICE_3] = smods->dfreq[B_SID_VOICE_3];
			else {
				if ((smods->gliderate[B_SID_VOICE_3] > 1.0)
					&& ((smods->cfreq[B_SID_VOICE_3]
						*= smods->gliderate[B_SID_VOICE_3])
						> smods->dfreq[B_SID_VOICE_3]))
					smods->cfreq[B_SID_VOICE_3] = smods->dfreq[B_SID_VOICE_3];
				else if ((smods->gliderate[B_SID_VOICE_3] < 1.0)
					&& ((smods->cfreq[B_SID_VOICE_3]
						*= smods->gliderate[B_SID_VOICE_3])
						< smods->dfreq[B_SID_VOICE_3]))
					smods->cfreq[B_SID_VOICE_3] = smods->dfreq[B_SID_VOICE_3];
			}

			ifreq = smods->cfreq[B_SID_VOICE_3];

			sid_register(smods->sid2id[AUD_SID], B_SID_V3_FREQ_LO,
				ifreq & 0x0ff);
			sid_register(smods->sid2id[AUD_SID], B_SID_V3_FREQ_HI,
				ifreq >> 8);

			smods->sid2reg[AUD_SID][B_SID_V3_FREQ_LO] = ifreq & 0x0ff;
			smods->sid2reg[AUD_SID][B_SID_V3_FREQ_HI] = (ifreq >> 8) & 0x0ff;
		}

		/* Check for arpeggiation */
		if (smods->keymode < B_S_KEYMODE_POLY3)
			continue;

		if ((smods->arpegcount -= 16) < 0)
		{
			smods->arpegcount = smods->arpegspeed;

			if ((++smods->arpegcurrent >= B_SID2_ARPEG_MAX)
				|| (smods->arpegtable[smods->arpegcurrent] == 0))
			{
				if (smods->arpegtable[smods->arpegcurrent = 0] == 0)
				{
					/* If the first entry is 0 then turn off the arpeg voice */
					sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, B_SID_V_GATE, 0);
					continue;
				}
			}

			/*
			 * Otherwise we have a new frequency, perhaps also wave and trigger
			 */
			if (smods->flags & B_SID2_ARPEG_TRIG)
				smods->flags |= B_SID2_E_GATE_V2;
			if (smods->flags & B_SID2_ARPEG_WAVE)
			{
				if (smods->arpegwave == 0)
					smods->arpegwave = B_SID_V_TRI;

				sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, smods->arpegwave, 0);

				if ((smods->arpegwave <<= 1) > B_SID_V_SQUARE)
					smods->arpegwave = B_SID_V_TRI;

				sid2flag(smods, AUD_SID, B_SID_V2_CONTROL, smods->arpegwave, 1);
			}
			smods->ckey[B_SID_VOICE_2] = smods->arpegtable[smods->arpegcurrent];
			sid2AssignVoice(smods, B_SID_VOICE_2, B_SID2_NEW);

//			printf("Arpeg %x %x, %i[%i] %x\n", smods->flags, smods->arpegwave,
//				smods->arpegcurrent, smods->arpegtable[smods->arpegcurrent],
//				smods->arpegwave);
		}
	}

	return(0);
}

int
static bristolSid2Destroy(audioMain *audiomain, Baudio *baudio)
{
	printf("removing one sid2\n");

	sid_IO( ((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID], B_SID_DESTROY, 0);
	sid_IO( ((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID], B_SID_DESTROY, 0);

	((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID] = -1;
	((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID] = -1;

	return(0);
}

int
bristolSid2Init(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising sid2 driver\n");

	baudio->soundCount = 1; /* Number of operators in this voice */
	baudio->voicecount = 1;

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * We should not really need any operators for this emulation, we are going
	 * to invoke two SID chips, one for audio and one for modulation and drive
	 * all the audio through them. We are going to dump an amp onto the list
	 * for eventual signal normalisation
	 */
	initSoundAlgo(B_DCA,	0, baudio, audiomain, baudio->sound);

	baudio->param = sid2Controller;
	baudio->destroy = bristolSid2Destroy;
	baudio->operate = operateOneSid2;

	/*
	 * Put nothing on our effects list.
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);
	 */

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(sid2mods));

	((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID] =
		sid_IO(-1, B_SID_INIT, audiomain->samplerate);
	((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID] =
		sid_IO(-1, B_SID_INIT, audiomain->samplerate);

	/*
	 * Add debugging for now for both the digital and analogue interfaces.
	 * It should be a GUI option however that will be in a later release.
	 *
	 * Remove this debug as we are pulling values in and out out during every
	 * sample now
	sid_register(((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID],
		B_SID_CONTROL, B_SID_C_DEBUG_D|B_SID_C_DEBUG_A);
	sid_register(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_CONTROL, B_SID_C_DEBUG_D|B_SID_C_DEBUG_A);

	((sid2mods *) baudio->mixlocals)->sid2reg[AUD_SID][B_SID_CONTROL] =
		B_SID_C_DEBUG_D|B_SID_C_DEBUG_A;
	 */
	((sid2mods *) baudio->mixlocals)->sid2reg[AUD_SID][B_SID_FILT_M_VOL] = 0x0f;

	/*
	 * We now have two SID. The first one will provide audio and will be totally
	 * driven from the GUI that has access to all parameters excepting debug.
	 * The second one will be for mods and only a small subset of its features
	 * will be driven by the GUI, the rest need to be configured here:
	 *
	 * The first two voices will be halted with the TEST bit.
	 * All filter will be disabled.
	 */
	sid_register(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_V2_CONTROL, B_SID_V_TEST);

	/*
	 * Kick the modulation SID into action:
	 *
	 * 	gate voice-3 to gen wave (GUI will select form).
	 *
	 *	No audio output of voice-3
	 *	Noise output of voice-1
	 *	Voice-1 bypass filter
	 *	Mute Voice-2
	 *
	 * What we are going to do is take noise as the MOD_SID output. If noise
	 * is selected as the MOD then the LFO will gate the selection of a noise
	 * sample - S&H.
	 */
	sid_register(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_V3_CONTROL, B_SID_V_GATE|B_SID_V_SQUARE);

	sid_register(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_V1_ATT_DEC, 0x07);
	sid_register(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_V1_SUS_REL, 0xff);
	sid_register(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_V1_CONTROL, B_SID_V_GATE|B_SID_V_NOISE);
	sid_register(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_FILT_RES_F, 0);

	((sid2mods *) baudio->mixlocals)->sid2reg[MOD_SID][B_SID_FILT_M_VOL]
		= B_SID_F_3_OFF|0x0f;
	sid_register(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_FILT_M_VOL, B_SID_F_3_OFF|0x0e);

	/*
	 * Set up some analogue parameters for the audio SID. These should really
	 * be in the GUI however that will probably not be the first release and
	 * when it is done there is still no harm in setting these up here anyway.
	 */
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID],
		B_SID_SN_LEAKAGE, 0.1);
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID],
		B_SID_SN_RATIO, 0.06);
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID],
		B_SID_DETUNE, 40); /* Cents */
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID],
		B_SID_DC_BIAS, 0.3);
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID],
		B_SID_OBERHEIM, 0.3);
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[AUD_SID],
		B_SID_DETUNE, baudio->detune * 1000);

	/* The MOD_SID is going to be noisy */
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_SN_LEAKAGE, 0.0);
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_SN_RATIO, 0.9);
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_DETUNE, 0);
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_DC_BIAS, 0.5);
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_OBERHEIM, 0.0);
	sid_IO(((sid2mods *) baudio->mixlocals)->sid2id[MOD_SID],
		B_SID_GAIN, 5.0);

	return(0);
}

