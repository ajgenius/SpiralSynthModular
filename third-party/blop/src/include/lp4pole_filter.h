/*
    lp4pole_filter.h - Header for lp4pole_filter struct, and functions
                       to run instance.

    Copyright (C) 2003  Mike Rawes

    Originally originally appeared in CSound as Timo Tossavainen's (sp?)
    implementation from the Stilson/Smith CCRMA paper.

    See http://musicdsp.org/archive.php?classid=3#26

    Originally appeared in the arts softsynth by Stefan Westerfeld:
    http://www.arts-project.org/

    First ported to LADSPA by Reiner Klenk (pdq808[at]t-online.de)

    Tuning and stability issues (output NaN) and additional audio-rate
    variant added by Mike Rawes (mike_rawes[at]yahoo.co.uk)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef blop_lp4pole_filter_h
#define blop_lp4pole_filter_h

#include <ladspa.h>
#include "common.h"

typedef struct {
	LADSPA_Data f;
	LADSPA_Data coeff;
	LADSPA_Data fb;
	LADSPA_Data in1;
	LADSPA_Data in2;
	LADSPA_Data in3;
	LADSPA_Data in4;
	LADSPA_Data inv_nyquist;
	LADSPA_Data out1;
	LADSPA_Data out2;
	LADSPA_Data out3;
	LADSPA_Data out4;
	LADSPA_Data max_abs_in;
} LP4PoleFilter;

/*****************************************************************************
 * Description: Allocate a new LP4PoleFilter instance
 *
 *   Arguments: sample_rate         Intended playback (DAC) rate
 *
 *     Returns: Allocated LP4PoleFilter instance
 *****************************************************************************/
LP4PoleFilter *
lp4pole_new (unsigned long sample_rate);

/*****************************************************************************
 * Description: Cleanup an existing LP4PoleFilter instance
 *
 *   Arguments: lpf                 Pointer to LP4PoleFilter instance
 *                                   allocated with initFilter
 *****************************************************************************/
void
lp4pole_cleanup (LP4PoleFilter * lpf);

/*****************************************************************************
 * Description: Initialise filter
 *
 *   Arguments: lpf                 Pointer to LP4PoleFilter instance
 *                                   allocated with initFilter
 *****************************************************************************/
void
lp4pole_init (LP4PoleFilter * lpf);

/*****************************************************************************
 * Set up filter coefficients for given LP4Pole instance
 *
 *   Arguments: lpf                 Pointer to LP4PoleFilter instance
 *              cutoff              Cutoff frequency in Hz
 *              resonance           Resonance [Min=0.0, Max=4.0]
 *****************************************************************************/
static inline void
lp4pole_set_params (LP4PoleFilter *lpf,
                    LADSPA_Data cutoff,
                    LADSPA_Data resonance)
{
	LADSPA_Data fsqd;
	LADSPA_Data tuning;

/* Normalise cutoff and find tuning - Magic numbers found empirically :) */
	lpf->f = cutoff * lpf->inv_nyquist;
	tuning = f_clip (3.13f - (lpf->f * 4.24703592f), 1.56503274f, 3.13f);

/* Clip to bounds */
	lpf->f = f_clip (lpf->f * tuning, lpf->inv_nyquist, 1.16f);

	fsqd = lpf->f * lpf->f;
	lpf->coeff = fsqd * fsqd * 0.35013f;

	lpf->fb = f_clip (resonance, -1.3f, 4.0f) * (1.0f - 0.15f * fsqd);

	lpf->f = 1.0f - lpf->f;
}

/*****************************************************************************
 * Description: Run given LP4PoleFilter instance for a single sample
 *
 *   Arguments: lpf                 Pointer to LP4PoleFilter instance
 *              in                  Input sample
 *
 *     Returns: Filtered sample
 *****************************************************************************/
static inline LADSPA_Data
lp4pole_run (LP4PoleFilter * lpf,
             LADSPA_Data in)
{
	LADSPA_Data abs_in = fabsf (16.0f * in); /* ~24dB unclipped headroom */

	lpf->max_abs_in = f_max (lpf->max_abs_in, abs_in);

	in -= lpf->out4 * lpf->fb;
	in *= lpf->coeff;

	lpf->out1 = in        + 0.3f * lpf->in1 + lpf->f * lpf->out1;  /* Pole 1 */
	lpf->in1  = in;
	lpf->out2 = lpf->out1 + 0.3f * lpf->in2 + lpf->f * lpf->out2;  /* Pole 2 */
	lpf->in2  = lpf->out1;
	lpf->out3 = lpf->out2 + 0.3f * lpf->in3 + lpf->f * lpf->out3;  /* Pole 3 */
	lpf->in3  = lpf->out2;
	lpf->out4 = lpf->out3 + 0.3f * lpf->in4 + lpf->f * lpf->out4;  /* Pole 4 */
	lpf->in4  = lpf->out3;

/* Simple hard clip to prevent NaN */
	lpf->out4 = f_clip (lpf->out4, -lpf->max_abs_in, lpf->max_abs_in);

	lpf->max_abs_in *= 0.999f;

	return lpf->out4;
}

#endif /* blop_lp4pole_filter_h */
