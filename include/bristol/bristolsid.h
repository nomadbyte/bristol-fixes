
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
 * Emulator code for the C64 SID audio chip. All the register numbers and bit
 * masks are defined here, the sid.c code will accept the register addressess,
 * parse the register bitmaps into some internal tables and then allow IO to
 * generate all the signals. The SID chip was partly digital (OSC/Env/noise)
 * and partly analogue (filter). This is emulated as integer code and floating
 * point code respectively.
 */

#ifndef _B_SID_DEFS
#define _B_SID_DEFS

#define B_SID_VOICE_1	0
#define B_SID_VOICE_2	1
#define B_SID_VOICE_3	2

#define B_SID_COUNT			32

/* Registers */
#define B_SID_REGISTERS		32
#define B_SID_V1_FREQ_LO	0x00
#define B_SID_V1_FREQ_HI	0x01
#define B_SID_V1_PW_LO		0x02
#define B_SID_V1_PW_HI		0x03
#define B_SID_V1_CONTROL	0x04
#define B_SID_V1_ATT_DEC	0x05
#define B_SID_V1_SUS_REL	0x06

#define B_SID_V2_FREQ_LO	0x07
#define B_SID_V2_FREQ_HI	0x08
#define B_SID_V2_PW_LO		0x09
#define B_SID_V2_PW_HI		0x0a
#define B_SID_V2_CONTROL	0x0b
#define B_SID_V2_ATT_DEC	0x0c
#define B_SID_V2_SUS_REL	0x0d

#define B_SID_V3_FREQ_LO	0x0e
#define B_SID_V3_FREQ_HI	0x0f
#define B_SID_V3_PW_LO		0x10
#define B_SID_V3_PW_HI		0x11
#define B_SID_V3_CONTROL	0x12
#define B_SID_V3_ATT_DEC	0x13
#define B_SID_V3_SUS_REL	0x14

#define B_SID_FILT_LO		0x15
#define B_SID_FILT_HI		0x16
#define B_SID_FILT_RES_F	0x17
#define B_SID_FILT_M_VOL	0x18

/*
 * X/Y Pots can be reassigned as the chip emulator has no support for them
 * For now voice-1 and voice-2 digital outputs (osc via env) will appear on
 * these taps.
 */
#define B_SID_X_ANALOGUE	0x19
#define B_SID_Y_ANALOGUE	0x1a

#define B_SID_OSC_3_OUT		0x1b
#define B_SID_ENV_3_OUT		0x1c

/*
 * This register was not defined as a part of SID. Control is needed to emulate
 * like the reset line and give filter type and oscillator mixing options.
 */
#define B_SID_CONTROL		0x1d

/* Bits in different registers - Voice */
#define B_SID_V_NOISE		0x80
#define B_SID_V_SQUARE		0x40
#define B_SID_V_RAMP		0x20
#define B_SID_V_TRI			0x10
#define B_SID_V_TEST		0x08
#define B_SID_V_RINGMOD		0x04
#define B_SID_V_SYNC		0x02
#define B_SID_V_GATE		0x01

/*
 * Bits in different registers - B_SID_FILT_RES_F
 */
#define B_SID_F_RESMASK		0xf0
#define B_SID_F_MIX_EXT		0x08
#define B_SID_F_MIX_V_3		0x04
#define B_SID_F_MIX_V_2		0x02
#define B_SID_F_MIX_V_1		0x01

/*
 * Bits in different registers - B_SID_FILT_M_VOL
 */
#define B_SID_F_3_OFF		0x80
#define B_SID_F_HP			0x40
#define B_SID_F_BP			0x20
#define B_SID_F_LP			0x10
#define B_SID_F_VOLMASK		0x0f

/* Bits in different registers - bristol control */
#define B_SID_C_RESET		0x80
#define B_SID_C_LPF			0x40
#define B_SID_C_MULTI_V		0x20 /* Mix waveforms rather than 'AND' them */
#define B_SID_C_DEBUG		0x10
#define B_SID_C_DEBUG_D		0x10 /* An alias, digital vs analogue debug */ 
#define B_SID_C_DEBUG_A		0x08
#define B_SID_C_PRINT		0x04

/*
 * B_SID diverse analogue (ie, float) functions - emulate the package 
 * characteristics such as oscillator leakage, S/N ratio and analogue IO
 * lines
 */
#define B_SID_IO			10
#define B_SID_INIT			0x00 /* INIT, takes samplerate, returns ID */ 
#define B_SID_DESTROY		0x01
#define B_SID_ANALOGUE_IO	0x02
#define B_SID_GAIN			0x03
#define B_SID_DETUNE		0x04 /* Depth of phase detuning with Osc mixing */ 
#define B_SID_SN_RATIO		0x05 /* Filter noise */
#define B_SID_SN_LEAKAGE	0x06 /* Envelope leakage */
#define B_SID_DC_BIAS		0x07 /* Output DC signal bias */
#define B_SID_OBERHEIM		0x08 /* Feed forword level of filter poles */
#define B_SID_CLOCKRATE		0x09 /* Nominal chip clock MHz */

/*
 * These are frequency table multipliers to convert the phase offsets to and
 * from actual frequencies. The following can be used to define the Oscillator
 * phase accumulator for a given Frequency, hence we can define at least a 
 * midi key to phase accumulation mapping table. This calculation is based on
 * the nominal 1Mhz clock from the original.
 *
 *	Fout = Fn * Fclk/16777216
 *	Fout = Fn * 0.059604645
 *	Fphase = Fout / 0.059604645
 *
 * Note: the C64 had ntsc and pal version with different CPU clock speeds. The
 * divider function need to match the frequency tables.
 */
#define B_SID_FREQ_DIV_PAL 0.059604645
#define B_SID_FREQ_DIV_NTSC 0.06097
#define B_SID_FREQ_DIV B_SID_FREQ_DIV_NTSC // From the reference manual

/* Evaluate 1/B_SID_FREQ_DIV since mult is faster than div */
#define B_SID_FREQ_MULT_PAL 16.777216
#define B_SID_FREQ_MULT_NTSC 16.401508939
#define B_SID_FREQ_MULT B_SID_FREQ_MULT_NTSC

/* The NTSC version has been tested to be correct. PAL not yet */
#define B_SID_MASTER_CLOCK_PAL 985000
#define B_SID_MASTER_CLOCK_NTSC 1023000
#define B_SID_MASTER_CLOCK B_SID_MASTER_CLOCK_NTSC

/*
 * This is to scale the 12 bit counter back to 0..1 float
 */
#define B_S_F_SCALER 0.000488520

extern unsigned short freqmap[128];

/*
 * Finally, these are the two single access methods to the chip emulation, one
 * will write a value to one of its registers, the other will set some emulator
 * equivalent analogue parameters in the floating point domain. 
 *
 * Details of the register settings are in the C64 programmer manual excepting
 * the control register which is specific to the emulator.
 */

int sid_register(int, unsigned char, unsigned char); /* ID, register, value */

float sid_IO(int, int, float); /* ID, command, param */

#endif /* _B_SID_DEFS */

