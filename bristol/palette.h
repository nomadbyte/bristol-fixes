
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
 * Palette consists of three sections. We have a set of global operators which
 * will affect any given audio stream, and we have a set of algorithms, which
 * will sequence the operators according to any desired method, and produce a
 * synthesiser.
 * Finally we have a set of effects processors for a final output stage. These
 * can potentially be mixed into any given voice, but would then need to be 
 * define in the palette as well as the FX lists.
 */

#ifndef PALETTE_H
#define PALETTE_H

/*
 * Effects operators.
 */
extern bristolOP * leslieinit();

/*
 * Synth operators
 */
extern bristolOP * bristolinit();
extern bristolOP * noiseinit();
extern bristolOP * dcoinit();
extern bristolOP * envinit();
extern bristolOP * dcainit();
extern bristolOP * filterinit();
extern bristolOP * hammondinit();
extern bristolOP * resinit();
extern bristolOP * prophetdcoinit();
extern bristolOP * dxopinit();
extern bristolOP * hpfinit();
extern bristolOP * junodcoinit();
extern bristolOP * chorusinit();
extern bristolOP * vchorusinit();
extern bristolOP * filter2init();
extern bristolOP * expdcoinit();
extern bristolOP * lfoinit();
extern bristolOP * voxdcoinit();
extern bristolOP * sdcoinit();
extern bristolOP * arpdcoinit();
extern bristolOP * ringmodinit();
extern bristolOP * eswitchinit();
extern bristolOP * reverbinit();
extern bristolOP * followerinit();
extern bristolOP * aksdcoinit();
extern bristolOP * aksenvinit();
extern bristolOP * aksreverbinit();
extern bristolOP * aksfilterinit();
extern bristolOP * hammondchorusinit();
extern bristolOP * quantuminit();
extern bristolOP * bit1oscinit();
extern bristolOP * cs80oscinit();
extern bristolOP * cs80envinit();
extern bristolOP * trilogyoscinit();
extern bristolOP * env5stageinit();
extern bristolOP * nroinit();

struct bristolPalette {
	bristolOP * (*initialise)();
} bristolPalette[BRISTOL_SYNTHCOUNT] = {
	{dcoinit},
	{envinit},
	{dcainit},
	{filterinit},
	{noiseinit},
	{hammondinit},
	{resinit},
	{leslieinit},
	{prophetdcoinit}, /* 8 */
	{dxopinit},
	{hpfinit},
	{junodcoinit},
	{chorusinit},
	{vchorusinit},
	{filter2init},
	{expdcoinit},
	{lfoinit}, /* 16 */
	{voxdcoinit},
	{sdcoinit},
	{arpdcoinit},
	{ringmodinit},
	{eswitchinit},
	{reverbinit},
	{followerinit},
	{aksdcoinit}, /* 24 */
	{aksenvinit},
	{aksreverbinit},
	{aksfilterinit},
	{hammondchorusinit},
	{quantuminit}, /* This will be the "quantum" grain generator */
	{bit1oscinit},
	{cs80oscinit},
	{cs80envinit}, /* 32 - now to use the env5stage init */
	{trilogyoscinit},
	{env5stageinit},
	{nroinit},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL}
};

/*
 * Synth algorithms
 */
extern int bristolMMInit();
extern int bristolHammondInit();
extern int bristolProphetInit();
extern int bristolProphet52Init();
extern int bristolDXInit();
extern int bristolRhodesInit();
extern int bristolJunoInit();
extern int bristolExpInit();
extern int bristolHammondB3Init();
extern int bristolVoxInit();
extern int bristolSamplerInit();
extern int bristolMixerInit();
extern int bristolOBXInit();
extern int bristolOBXaInit();
extern int bristolPolyInit();
extern int bristolPoly6Init();
extern int bristolAxxeInit();
extern int bristolOdysseyInit();
extern int bristolMemoryMoogInit();
extern int bristolArp2600Init();
extern int bristolAksInit();
extern int bristolSolinaInit();
extern int bristolRoadrunnerInit();
extern int bristolGranularInit();
extern int bristolMg1Init();
extern int bristolVoxM2Init();
extern int bristolJupiterInit();
extern int bristolBitoneInit();
extern int bristolCs80Init();
extern int bristolPro1Init();
extern int bristolSonic6Init();
extern int bristolTrilogyInit();
extern int bristolTrilogyODCInit();
extern int bristolPoly800Init();
extern int bristolBme700Init();
extern int bristolBassMakerInit();
extern int bristolSidInit();

struct bristolAlgos {
	int (*initialise)(audioMain *, Baudio *);
	char *name;
} bristolAlgos[BRISTOL_SYNTHCOUNT] = {
	{bristolMMInit, "mini"},
	{bristolHammondInit, "hammond"},
	{bristolProphetInit, "prophet"},
	{bristolDXInit, "dx"},
	{bristolJunoInit, "juno"},
	{bristolExpInit, "explorer"},
	{bristolHammondB3Init, "hammondB3"},
	{bristolVoxInit, "vox"},
	{bristolRhodesInit, "dx"},
	{bristolSamplerInit, NULL},
	{bristolMixerInit, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{bristolProphet52Init, "prophet52"}, /* Uses Prophet algorithm with added FX. */
	{bristolOBXInit, "obx"}, /* Used by OB-X and two for OB-Xa */
	{NULL, NULL}, /* THis was for the OBXA? */
	{NULL, NULL},
	{bristolPolyInit, "mono"},
	{bristolPoly6Init, "poly"},
	{bristolAxxeInit, "axxe"},
	{bristolOdysseyInit, "odyssey"},
	{bristolMemoryMoogInit, "memoryMoog"},
	{bristolArp2600Init, "arp2600"},
	{bristolAksInit, "aks"},
	{NULL, NULL}, /* Will be MS-20 */
	{bristolSolinaInit, "solina"},
	{bristolRoadrunnerInit, "roadrunner"},
	{bristolGranularInit, "granular"},
	{bristolMg1Init, "realistic"},
	{bristolVoxM2Init, "voxM2"},
	{bristolJupiterInit, "jupiter8"},
	{bristolBitoneInit, "bitone"},
	{NULL, NULL}, /* Will be Master Keyboard */
	{bristolCs80Init, "cs80"},
	{bristolPro1Init, "pro1"},
	{bristolExpInit, "explorer"},
	{bristolSonic6Init, "sonic6"},
	{bristolTrilogyInit, "trilogy"},
	{bristolTrilogyODCInit, "trilogyODC"},
	{NULL, "stratus"}, /* Is the trilogy */
	{bristolPoly800Init, "poly800"},
	{bristolBme700Init, "bme700"},
	{bristolBassMakerInit, "bassmaker"},
	{bristolSidInit, "sid"},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL}
};

void
newPalette()
{
	/*
	 * This is defined in fatboy headers, here we only have a null call
	 */
}

struct bristolEffects {
	bristolOP * (*initialise)(audioMain *, Baudio *);
} bristolEffects[BRISTOL_SYNTHCOUNT] = {
	{leslieinit},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL}
};

#endif

