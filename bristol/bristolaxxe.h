
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

#define A_LFO_MULTI 0x0001
#define A_LFO_TRILL 0x0002

typedef struct axxeMods {
	unsigned int flags;

	void *lfolocals;

	float o_f_sine;
	float o_f_square;
	float o_f_sh;
	float o_f_adsr;

	float o_pwm_sine;
	float o_pwm_adsr;

	float m_noise;
	float m_ramp;
	float m_square;

	float f_sine;
	float f_adsr;

	float a_gain;
	float a_adsr;

	int lfoAuto;
	int lfoAutoCount;
} axxemods;

