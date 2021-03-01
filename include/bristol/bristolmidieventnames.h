
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
 * The events (eee) are encoded in the first nibble of byte 1 along with the
 * channel number (cccc) in the low order nibble:
 * 	1 eee cccc
 */
static char eventNames[8][32] = {
	"midiNoteOff",			/* 1000cccc: +k/v */
	"midiNoteOn",			/* 1001cccc: +k/v */
	"midiPolyPressure",		/* 1010cccc: +k/p */
	"midiControl",			/* 1011cccc: +c/v */
	"midiProgram",			/* 1100cccc: +p */
	"midiChannelPressure",	/* 1101cccc: +p */
	"midiPitchWheel",		/* 1110cccc: +P/p */
	"midiSystem"			/* 1111cccc: Diverse sizes for common/sysex */
};

/*
 * Of the system messages there is no channel identifier, rather a command of
 * some type. The following are interesting however the same functionality has
 * kind of been built into the SLab messages explicitly:
 *
 *  F0 - SYSEX start
 * 	F6 - tune
 *  F7 - SYSEX end
 * 	F8 - MTC (tbd)
 * 	FE - ActiveSense
 * 	FF - reset
 */

/*
 * These are the control names encoded as 7 bits in the second byte of any
 * related midi message, the value is encoded as 7 bits in the 3rd byte. For
 * some controls there are two messages for high and low order bytes for 14
 * bit resolution however the sequencing of the two messages is a little
 * ambiguous.
 */
static char *controllerName[128] = {
	"BankSelectCoarse",
	"ModulationWheelCoarse",
	"BreathcontrollerCoarse",
	NULL,
	"FootPedalCoarse",
	"PortamentoTimeCoarse",
	"DataEntryCoarse",
	"VolumeCoarse",
	"BalanceCoarse",
	NULL,
	"PanpositionCoarse",
	"ExpressionCoarse",
	"EffectControl1Coarse",
	"EffectControl2Coarse",
	NULL,
	NULL,
	"GeneralPurposeSlider1",
	"GeneralPurposeSlider2",
	"GeneralPurposeSlider3",
	"GeneralPurposeSlider4",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"BankSelectFine",
	"ModulationWheelFine",
	"BreathcontrollerFine",
	NULL,
	"FootPedalFine",
	"PortamentoTimeFine",
	"DataEntryFine",
	"VolumeFine",
	"BalanceFine",
	NULL,
	"PanpositionFine",
	"ExpressionFine",
	"EffectControl1Fine",
	"EffectControl2Fine",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"HoldPedal",
	"Portamento",
	"SustenutoPedal",
	"SoftPedal",
	"LegatoPedal",
	"Hold2Pedal",
	"SoundVariation",
	"SoundTimbre",
	"SoundReleaseTime",
	"SoundAttackTime",
	"SoundBrightness",
	"SoundControl6",
	"SoundControl7",
	"SoundControl8",
	"SoundControl9",
	"SoundControl10",
	"GeneralPurposeButton1",
	"GeneralPurposeButton2",
	"GeneralPurposeButton3",
	"GeneralPurposeButton4",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"EffectsLevel",
	"TremuloLevel",
	"ChorusLevel",
	"CelesteLevel",
	"PhaserLevel",
	"DataButtonincrement",
	"DataButtondecrement",
	"Non-registeredParameter",
	"Non-registeredParameter",
	"RegisteredParameter",
	"RegisteredParameter",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"AllSoundOff",
	"AllControllersOff",
	"LocalKeyboard",
	"AllNotesOff",
	"OmniModeOff",
	"OmniModeOn",
	"MonoOperation",
	"PolyOperation",
};

