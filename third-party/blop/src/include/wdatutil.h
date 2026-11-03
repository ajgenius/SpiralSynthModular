/*
    wdatutil.h - Code to generate wavedata dl containing pre-calculated
                 wavetables.

    Copyright (C) 2003  Mike Rawes

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

#ifndef blop_wdatutil_h
#define blop_wdatutil_h

#include <stdio.h>
#include <ladspa.h>
#include "math_func.h"
#include "wavedata.h"

#define WAVE_TYPE_COUNT         3

extern char *wave_names[];
extern char *wave_descriptions[];
extern unsigned long wave_first_harmonics[];
extern unsigned long wave_harmonic_intervals[];

/* Get actual maximum harmonic from given harmonic, h, and wavetype, w */
#define ACTUAL_HARM(h,w) h - (h - wave_first_harmonics[w]) % wave_harmonic_intervals[w]
/* Get minimum harmonic content in given wavetype, w */
#define MIN_HARM(w) wave_first_harmonics[w]
/* Get minimum extra harmonic content possible in given wavetype, w */
#define MIN_EXTRA_HARM(w) wave_harmonic_intervals[w]
/* Get frequency from MIDI note, n */
#define FREQ_FROM_NOTE(n) 6.875f * POWF (2.0f, (float)(n + 3) / 12.0f)
/* Get max harmonic from given frequency, f, at sample rate, r */
#define HARM_FROM_FREQ(f,r) (unsigned long)((float)r / f / 2.0f)

/*
 * A single wavetable will have a range of pitches at which their samples
 * may be played back.
 *
 * The upper bound is determined by the maximum harmonic present in the
 * waveform - above this frequency, the higher harmonics will alias.
 *
 * The lower bound is chosen to be the higher bound of the previous wavetable
 * (or a pre-defined limit if there is no such table).
 */

typedef enum
{
	SAW,
	SQUARE,
	PARABOLA
} Wavetype;

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *  Description: Allocate new wavedata struct
 *
 *    Arguments: sample_rate       Sample rate to use when generating data
 *
 *      Returns: Pointer to wavedata on success
 *               NULL (0) on failure
 *
 *        Notes: No wavetables are allocated. Use wavedata_add_table
 ******************************************************************************/
Wavedata *
wavedata_new (unsigned long sample_rate);

/*******************************************************************************
 *  Description: Destroy allocated wavedata and any tables
 *
 *    Arguments: w      Wavedata struct to cleanup
 ******************************************************************************/
void
wavedata_cleanup (Wavedata * w);

/*******************************************************************************
 *     Description: Add new wavetable information to wavedata file object
 *
 *       Arguments: w                   Wavedata to add table to
 *                  sample_count        Number of samples in wavetable
 *                  harmonics           Maximum harmonics present in table
 *
 *         Returns:  0 on success
 *                  -1 otherwise
 ******************************************************************************/
int
wavedata_add_table (Wavedata * w,
                    unsigned long sample_count,
                    unsigned long harmonics);

/*******************************************************************************
 *     Description: Initialise all wavetables in wavedata with a waveform
 *                  generated from Fourier series.
 *
 *       Arguments: w               Wavedata to generate data for
 *                  wavetype        Wavetype to generate
 *                  gibbs_comp      Compensation for Gibbs' effect:
 *                                    0.0: none (wave will overshoot)
 *                                    1.0: full (wave will not overshoot)
 *
 *           Notes: Compensation for Gibbs' Effect will reduce the degree
 *                   of overshoot and ripple at the transitions. A value of 1.0
 *                   will pretty much eliminate it.
 ******************************************************************************/
void
wavedata_generate_tables (Wavedata * w,
                          Wavetype wavetype,
                          float gibbs_comp);

/*******************************************************************************
 *     Description: Write wavedata as a c header file
 *
 *       Arguments: w           Wavedata to write
 *                  wdat_fp     Pointer to output file
 *                  prefix      Prefix to use in declarations. If this is null
 *                               declarations are prefixed with 'wdat'.
 *
 *         Returns:  0 on success
 *                  -1 otherwise
 ******************************************************************************/
int
wavedata_write (Wavedata * w,
                FILE * wdat_fp,
                char * prefix);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* blop_wdatutil_h */
