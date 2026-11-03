/*
    wdatutil.c - Code to generate wavedata for bandlimited waveforms.

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ladspa.h>
#include "common.h"
#include "wavedata.h"
#include "wdatutil.h"

#ifdef __cplusplus
extern "C" {
#endif

void
generate_sine (LADSPA_Data * samples,
               unsigned long sample_count);
void
generate_sawtooth (LADSPA_Data * samples,
                   unsigned long sample_count,
                   unsigned long harmonics,
                   float gibbs_comp);
void
generate_square (LADSPA_Data * samples,
                 unsigned long sample_count,
                 unsigned long harmonics,
                 float gibbs_comp);
void
generate_parabola (LADSPA_Data * samples,
                   unsigned long sample_count,
                   unsigned long harmonics,
                   float gibbs_comp);

#ifdef __cplusplus
} /* extern "C" { */
#endif

char *wave_names[] = {
	"Saw",
	"Square",
	"Parabola"
};

char *wave_descriptions[] = {
	"Sawtooth Wave",
	"Square Wave",
	"Parabola Wave"
};

unsigned long wave_first_harmonics[] = {
	1,
	1,
	1
};

unsigned long wave_harmonic_intervals[] = {
	1,
	2,
	1
};

Wavedata *
wavedata_new (unsigned long sample_rate)
{
	Wavedata * w;

	w = (Wavedata *) malloc (sizeof (Wavedata));

	if (!w)
		return 0;

	w->data_handle = 0;
	w->table_count = 0;
	w->tables = 0;
	w->lookup = 0;
	w->lookup_max = 0;
	w->sample_rate = (LADSPA_Data) sample_rate;
	w->nyquist = w->sample_rate * 0.5f;

	return w;
}

void
wavedata_cleanup (Wavedata * w)
{
	unsigned long ti;
	Wavetable * t;

	for (ti = 0; ti < w->table_count; ti++)
	{
		t = w->tables[ti];
		if (t)
		{
			if (t->samples_hf)
				free (t->samples_hf);

			if (t->samples_lf)
				free (t->samples_lf);

			free (t);
		}
	}

	free (w);
}

int
wavedata_add_table (Wavedata * w,
                    unsigned long sample_count,
                    unsigned long harmonics)
{
	Wavetable ** tables;
	Wavetable * t;
	size_t bytes;

	t = (Wavetable *) malloc (sizeof (Wavetable));

	if (!t)
		return -1;

/* Extra 3 samples for interpolation */
	bytes = (sample_count + 3) * sizeof (LADSPA_Data);

	t->samples_lf = (LADSPA_Data *) malloc (bytes);

	if (!t->samples_lf)
	{
		free (t);
		return -1;
	}

	t->samples_hf = (LADSPA_Data *) malloc (bytes);

	if (!t->samples_hf)
	{
		free (t->samples_lf);
		free (t);
		return -1;
	}

	bytes = (w->table_count + 1) * sizeof (Wavetable *);
	if (w->table_count == 0)
		tables = (Wavetable **) malloc (bytes);
	else
		tables = (Wavetable **) realloc (w->tables, bytes);

	if (!tables)
	{
		free (t);
		return -1;
	}

	t->sample_count = sample_count;
	t->harmonics = harmonics;

	if (w->lookup_max < harmonics)
		w->lookup_max = harmonics;

	tables[w->table_count] = t;
	w->tables = tables;
	w->table_count++;

	return 0;
}

void
wavedata_generate_tables (Wavedata * w,
                          Wavetype wavetype,
                          float gibbs_comp)
{
	Wavetable * t;
	LADSPA_Data * samples_lf;
	LADSPA_Data * samples_hf;
	unsigned long h_lf;
	unsigned long h_hf;
	unsigned long s;
	unsigned long i;

	for (i = 0; i < w->table_count; i++)
	{
		t = w->tables[i];

		h_lf = t->harmonics;

		if (i < w->table_count - 1)
			h_hf = w->tables[i+1]->harmonics;
		else
			h_hf = 1;

		samples_lf = t->samples_lf;
		samples_hf = t->samples_hf;
		samples_lf++;
		samples_hf++;

		switch (wavetype)
		{
			case SAW:
				generate_sawtooth (samples_lf, t->sample_count, h_lf, gibbs_comp);
				generate_sawtooth (samples_hf, t->sample_count, h_hf, gibbs_comp);
				break;
			case SQUARE:
				generate_square (samples_lf, t->sample_count, h_lf, gibbs_comp);
				generate_square (samples_hf, t->sample_count, h_hf, gibbs_comp);
				break;
			case PARABOLA:
				generate_parabola (samples_lf, t->sample_count, h_lf, gibbs_comp);
				generate_parabola (samples_hf, t->sample_count, h_hf, gibbs_comp);
				break;
		}

	/* Basic denormalization */
		for (s = 0; s < t->sample_count; s++)
			samples_lf[s] = FABSF (samples_lf[s]) < SMALLEST_FLOAT ? 0.0 : samples_lf[s];

		samples_lf--;
		samples_lf[0] = samples_lf[t->sample_count];
		samples_lf[t->sample_count + 1] = samples_hf[1];
		samples_lf[t->sample_count + 2] = samples_hf[2];

		for (s = 0; s < t->sample_count; s++)
			samples_hf[s] = FABSF (samples_hf[s]) < SMALLEST_FLOAT ? 0.0 : samples_hf[s];

		samples_hf--;
		samples_hf[0] = samples_hf[t->sample_count];
		samples_hf[t->sample_count + 1] = samples_hf[1];
		samples_hf[t->sample_count + 2] = samples_hf[2];
	}
}

int
wavedata_write (Wavedata * w,
                FILE * wdat_fp,
                char * data_name)
{
	Wavetable * t = 0;
	unsigned long table_count;
	unsigned long i;
	unsigned long j;
	unsigned long s;
	int column;
/*
 * Extra table at end
 */
	table_count = w->table_count + 1;

	fprintf (wdat_fp, "#include <ladspa.h>\n");
	fprintf (wdat_fp, "#include <stdio.h>\n");
	fprintf (wdat_fp, "#include \"wavedata.h\"\n");
	fprintf (wdat_fp, "\n");
/*
 * Fixed data and tables
 */
 	fprintf (wdat_fp, "unsigned long ref_count = 0;\n");
	fprintf (wdat_fp, "unsigned long first_sample_rate = 0;\n");
	fprintf (wdat_fp, "unsigned long table_count = %ld;\n", table_count);
	fprintf (wdat_fp, "Wavetable tables[%ld];\n", table_count);
	fprintf (wdat_fp, "Wavetable * ptables[%ld];\n", table_count);
	fprintf (wdat_fp, "unsigned long lookup[%ld];\n", w->lookup_max + 1);
	fprintf (wdat_fp, "unsigned long lookup_max = %ld;\n", w->lookup_max);
	fprintf (wdat_fp, "\n");
/*
 * Sample data
 * Each table has an extra 3 samples for interpolation
 */
	for (i = 0; i < w->table_count; i++)
	{
		t = w->tables[i];

		fprintf(wdat_fp, "static LADSPA_Data samples_lf_%ld[%ld] = {\n", i, t->sample_count + 3);

		column = 0;
		for (s = 0; s < t->sample_count + 3 - 1; s++, column++)
		{
			if (column == 5)
			{
				fprintf (wdat_fp, "\n");
				column = 0;
			}
			fprintf (wdat_fp, "%+.8ef,", t->samples_lf[s]);
		}

		if (column == 5)
			fprintf (wdat_fp, "\n");

		fprintf (wdat_fp, "%+.8ef\n", t->samples_lf[s]);
		fprintf (wdat_fp, "};\n");
		fprintf (wdat_fp, "\n");

		fprintf(wdat_fp, "static LADSPA_Data samples_hf_%ld[%ld] = {\n", i, t->sample_count + 3);

		column = 0;
		for (s = 0; s < t->sample_count + 3 - 1; s++, column++)
		{
			if (column == 5)
			{
				fprintf (wdat_fp, "\n");
				column = 0;
			}
			fprintf (wdat_fp, "%+.8ef,", t->samples_hf[s]);
		}

		if (column == 5)
			fprintf (wdat_fp, "\n");

		fprintf (wdat_fp, "%+.8ef\n", t->samples_hf[s]);
		fprintf (wdat_fp, "};\n");
		fprintf (wdat_fp, "\n");
	}

	fprintf (wdat_fp, "LADSPA_Data samples_zero[%ld];\n", t->sample_count + 3);
	fprintf (wdat_fp, "\n");
/*
 * Function to get Wavedata - the sample rate is needed to calculate
 * frequencies and related things
 */
	fprintf (wdat_fp, "int\n");
	fprintf (wdat_fp, "blop_get_%s (Wavedata * w, unsigned long sample_rate)\n", data_name);
	fprintf (wdat_fp, "{\n");
	fprintf (wdat_fp, "\tWavetable * t;\n");
	fprintf (wdat_fp, "\tunsigned long ti;\n");
	fprintf (wdat_fp, "\n");
/*
 * Sample rate must be > 0
 */
	fprintf (wdat_fp, "\tif (sample_rate == 0)\n");
	fprintf (wdat_fp, "\t\treturn -1;\n");
	fprintf (wdat_fp, "\n");
/*
 * First time call - set up all sample rate dependent data
 */
	fprintf (wdat_fp, "\tif (first_sample_rate == 0)\n");
	fprintf (wdat_fp, "\t{\n");
	fprintf (wdat_fp, "\t\tfirst_sample_rate = sample_rate;\n");
	fprintf (wdat_fp, "\t\tw->sample_rate = (LADSPA_Data) sample_rate;\n");
	fprintf (wdat_fp, "\t\tw->nyquist = w->sample_rate / 2.0f;\n");
	fprintf (wdat_fp, "\t\tw->table_count = table_count;\n");
	fprintf (wdat_fp, "\t\tw->tables = ptables;\n");
	fprintf (wdat_fp, "\t\tw->lookup = lookup;\n");
	fprintf (wdat_fp, "\t\tw->lookup_max = lookup_max;\n");
	fprintf (wdat_fp, "\n");
	fprintf (wdat_fp, "\t\tfor (ti = 1; ti < table_count - 1; ti++)\n");
	fprintf (wdat_fp, "\t\t{\n");
	fprintf (wdat_fp, "\t\t\tt = ptables[ti];\n");
	fprintf (wdat_fp, "\t\t\tt->min_frequency = w->nyquist / (LADSPA_Data) (ptables[ti - 1]->harmonics);\n");
	fprintf (wdat_fp, "\t\t\tt->max_frequency = w->nyquist / (LADSPA_Data) (t->harmonics);\n");
	fprintf (wdat_fp, "\t\t}\n");
	fprintf (wdat_fp, "\n");
	fprintf (wdat_fp, "\t\tt = w->tables[0];\n");
	fprintf (wdat_fp, "\t\tt->min_frequency = 0.0f;\n");
	fprintf (wdat_fp, "\t\tt->max_frequency = ptables[1]->min_frequency;\n");
	fprintf (wdat_fp, "\n");
	fprintf (wdat_fp, "\t\tt = ptables[table_count - 1];\n");
	fprintf (wdat_fp, "\t\tt->min_frequency = ptables[w->table_count - 2]->max_frequency;\n");
	fprintf (wdat_fp, "\t\tt->max_frequency = w->nyquist;\n");
	fprintf (wdat_fp, "\t\n");
	fprintf (wdat_fp, "\t\tfor (ti = 0; ti < w->table_count; ti++)\n");
	fprintf (wdat_fp, "\t\t{\n");
	fprintf (wdat_fp, "\t\t\tt = w->tables[ti];\n");
	fprintf (wdat_fp, "\t\t\tt->phase_scale_factor = (LADSPA_Data) (t->sample_count) / w->sample_rate;\n");
	fprintf (wdat_fp, "\t\t\tt->range_scale_factor = 1.0f / (t->max_frequency - t->min_frequency);\n");
	fprintf (wdat_fp, "\t\t}\n");
	fprintf (wdat_fp, "\n");
	fprintf (wdat_fp, "\t\treturn 0;\n");
	fprintf (wdat_fp, "\t}\n");
/*
 * Already called at least once, so just set up wavedata
 */
	fprintf (wdat_fp, "\telse if (sample_rate == first_sample_rate)\n");
	fprintf (wdat_fp, "\t{\n");
	fprintf (wdat_fp, "\t\tw->sample_rate = (LADSPA_Data) sample_rate;\n");
	fprintf (wdat_fp, "\t\tw->nyquist = w->sample_rate / 2.0f;\n");
	fprintf (wdat_fp, "\t\tw->table_count = table_count;\n");
	fprintf (wdat_fp, "\t\tw->tables = ptables;\n");
	fprintf (wdat_fp, "\t\tw->lookup = lookup;\n");
	fprintf (wdat_fp, "\t\tw->lookup_max = lookup_max;\n");
	fprintf (wdat_fp, "\n");
	fprintf (wdat_fp, "\t\treturn 0;\n");
	fprintf (wdat_fp, "\t}\n");
/*
 * Sample rate does not match, so fail
 *
 * NOTE: This means multiple sample rates are not supported
 *       This should not present any problems
 */
	fprintf (wdat_fp, "\telse\n");
	fprintf (wdat_fp, "\t{\n");
	fprintf (wdat_fp, "\t\treturn -1;\n");
	fprintf (wdat_fp, "\t}\n");
	fprintf (wdat_fp, "}\n");
	fprintf (wdat_fp, "\n");
/*
 * _init()
 * Assemble tables and lookup
 */
	fprintf (wdat_fp, "void\n");
	fprintf (wdat_fp, "_init (void)\n");
	fprintf (wdat_fp, "{\n");
	fprintf (wdat_fp, "\tunsigned long max_harmonic;\n");
	fprintf (wdat_fp, "\tunsigned long ti;\n");
	fprintf (wdat_fp, "\tunsigned long li;\n");
	fprintf (wdat_fp, "\tunsigned long s;\n");
	fprintf (wdat_fp, "\n");

	for (i = 0; i < w->table_count; i++)
	{
		t = w->tables[i];

		fprintf (wdat_fp, "\ttables[%ld].sample_count = %ld;\n", i, t->sample_count);
		fprintf (wdat_fp, "\ttables[%ld].samples_lf = samples_lf_%ld;\n", i, i);
		fprintf (wdat_fp, "\ttables[%ld].samples_hf = samples_hf_%ld;\n", i, i);
		fprintf (wdat_fp, "\ttables[%ld].harmonics = %ld;\n", i, t->harmonics);
		fprintf (wdat_fp, "\n");
	}
/*
 * Last table - uses same sample data as previous table for lf data,
 * and zeroes for hf data
 */
	i = w->table_count - 1;
	j = i + 1;
	t = w->tables[i];
/*
 * Zero silent samples
 */
	fprintf (wdat_fp, "\tfor (s = 0; s < %ld; s++)\n", t->sample_count + 3);
	fprintf (wdat_fp, "\t\tsamples_zero[s] = 0.0f;\n");
	fprintf (wdat_fp, "\n");

	fprintf (wdat_fp, "\ttables[%ld].sample_count = %ld;\n", j, t->sample_count);
	fprintf (wdat_fp, "\ttables[%ld].samples_lf = samples_hf_%ld;\n", j, i);
	fprintf (wdat_fp, "\ttables[%ld].samples_hf = samples_zero;\n", j);
	fprintf (wdat_fp, "\ttables[%ld].harmonics = 1;\n", j);
	fprintf (wdat_fp, "\n");
/*
 * Get pointers to each wavetable and put them in the pointer array
 */
	fprintf (wdat_fp, "\tfor (ti = 0; ti < table_count; ti++)\n");
	fprintf (wdat_fp, "\t\tptables[ti] = &tables[ti];\n");
	fprintf (wdat_fp, "\n");
/*
 * Shift all sample offsets forward by one sample
 * !!! NO! Don't! 
	fprintf (wdat_fp, "\tfor (ti = 0; ti < table_count; ti++)\n");
	fprintf (wdat_fp, "\t{\n");
	fprintf (wdat_fp, "\t\tptables[ti]->samples_lf++;\n");
	fprintf (wdat_fp, "\t\tptables[ti]->samples_hf++;\n");
	fprintf (wdat_fp, "\t}\n");
	fprintf (wdat_fp, "\n");
 */
/*
 * Table lookup vector indexed by harmonic
 * Add lookup data to vector
 */
	fprintf (wdat_fp, "\tli = 0;");
	fprintf (wdat_fp, "\n");
	fprintf (wdat_fp, "\tfor (ti = table_count - 1; ti > 0; ti--)\n");
	fprintf (wdat_fp, "\t{\n");
	fprintf (wdat_fp, "\t\tmax_harmonic = ptables[ti]->harmonics;\n");
	fprintf (wdat_fp, "\n");
	fprintf (wdat_fp, "\t\tfor ( ; li <= max_harmonic; li++)\n");
	fprintf (wdat_fp, "\t\t\tlookup[li] = ti;\n");
	fprintf (wdat_fp, "\t}\n");
	fprintf (wdat_fp, "\n");
	fprintf (wdat_fp, "\tfor ( ; li <= lookup_max; li++)\n");
	fprintf (wdat_fp, "\t\tlookup[li] = 0;\n");
	fprintf (wdat_fp, "}\n");

	return 0;
}

void
generate_sawtooth (LADSPA_Data * samples,
                   unsigned long sample_count,
                   unsigned long harmonics,
                   float gibbs_comp)
{
	double phase_scale = 2.0 * M_PI / (double) sample_count;
	LADSPA_Data scale = 2.0f / M_PI;
	unsigned long i;
	unsigned long h;
	double mhf;
	double hf;
	double k;
	double m;
	double phase;
	double partial;

	if (gibbs_comp > 0.0f)
	{
	/* Degree of Gibbs Effect compensation */
		mhf = (double) harmonics;
		k = M_PI * (double) gibbs_comp / mhf;

		for (i = 0; i < sample_count; i++)
			samples[i] = 0.0f;

		for (h = 1; h <= harmonics; h++)
		{
			hf = (double) h;

		/* Gibbs Effect compensation - Hamming window */
		/* Modified slightly for smoother fade at highest frequencies */
			m = 0.54 + 0.46 * cos ((hf - 0.5 / mhf) * k);

			for (i = 0; i < sample_count; i++)
			{
				phase = (double) i * phase_scale;
				partial = (m / hf) * sin (phase * hf);
				samples[i] += (LADSPA_Data) partial;
			}
		}

		for (i = 0; i < sample_count; i++)
			samples[i] *= scale;
	}
	else
	{
	/* Allow overshoot */
		for (i = 0; i < sample_count; i++)
		{
			phase = (double) i * phase_scale;
			samples[i] = 0.0f;

			for (h = 1; h <= harmonics; h++)
			{
				hf = (double) h;
				partial = (1.0 / hf) * sin (phase * hf);
				samples[i] += (LADSPA_Data) partial;
			}
			samples[i] *= scale;
		}
	}
}

void
generate_square (LADSPA_Data * samples,
                 unsigned long sample_count,
                 unsigned long harmonics,
                 float gibbs_comp)
{
	double phase_scale = 2.0 * M_PI / (double) sample_count;
	LADSPA_Data scale = 4.0f / M_PI;
	unsigned long i;
	unsigned long h;
	double mhf;
	double hf;
	double k;
	double m;
	double phase;
	double partial;

	if (gibbs_comp > 0.0f)
	{
	/* Degree of Gibbs Effect compensation */
		mhf = (double) harmonics;
		k = M_PI * (double) gibbs_comp / mhf;

		for (i = 0; i < sample_count; i++) 
			samples[i] = 0.0f;

		for (h = 1; h <= harmonics; h += 2) 
		{
			hf = (double) h;

		/* Gibbs Effect compensation - Hamming window */
		/* Modified slightly for smoother fade at highest frequencies */
			m = 0.54 + 0.46 * cos((hf - 0.5 / pow (mhf, 2.2)) * k);

			for (i = 0; i < sample_count; i++) 
			{
				phase = (double) i * phase_scale;
				partial = (m / hf) * sin (phase * hf);
				samples[i] += (LADSPA_Data) partial;
			}
		}

		for (i = 0; i < sample_count; i++) 
			samples[i] *= scale;
	} 
	else 
	{
	/* Allow overshoot */
		for (i = 0; i < sample_count; i++)
		{
			phase = (double) i * phase_scale;
			samples[i] = 0.0f;

			for (h = 1; h <= harmonics; h += 2)
			{
				hf = (double) h;
				partial = (1.0 / hf) * sin (phase * hf);
				samples[i] += (LADSPA_Data) partial;
			}
			samples[i] *= scale;
		}
	}
}

void
generate_parabola (LADSPA_Data * samples,
                   unsigned long sample_count,
                   unsigned long harmonics,
                   float gibbs_comp)
{
	double phase_scale = 2.0 * M_PI / (double) sample_count;
	LADSPA_Data scale = 2.0f / (M_PI * M_PI);
	unsigned long i;
	unsigned long h;
	double mhf;
	double hf;
	double k;
	double m;
	double phase;
	double partial;
    double sign;

	if (gibbs_comp > 0.0f)
	{
	/* Degree of Gibbs Effect compensation */
		mhf = (double) harmonics;
		k = M_PI * (double) gibbs_comp / mhf;

		for (i = 0; i < sample_count; i++)
			samples[i] = 0.0f;

		sign = -1.0;

		for (h = 1; h <= harmonics; h++) 
		{
			hf = (double) h;

		/* Gibbs Effect compensation - Hamming window */
		/* Modified slightly for smoother fade at highest frequencies */
			m = 0.54 + 0.46 * cos ((hf - 0.5 / mhf) * k);

			for (i = 0; i < sample_count; i++) 
			{
				phase = (double) i * phase_scale;
				partial = (sign * 4.0 / (hf * hf)) * cos (phase * hf);
				samples[i] += (LADSPA_Data) partial;
			}
			sign = -sign;
		}

		for (i = 0; i < sample_count; i++) 
			samples[i] *= scale;
	} 
	else 
	{
	/* Allow overshoot */
		for (i = 0; i < sample_count; i++) 
		{
			phase = (double) i * phase_scale;
			samples[i] = 0.0f;
			sign = -1.0;

			for (h = 1; h <= harmonics; h++) 
			{
				hf = (double) h;
				partial = (sign * 4.0 / (hf * hf)) * cos (phase * hf);
				samples[i] += (LADSPA_Data) partial;
				sign = -sign;
			}
			samples[i] *= scale;
		}
	}
}
