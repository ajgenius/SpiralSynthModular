/*
	wavegen.c - A program to generate c header files containing pre-calculated
				wavedata.

	Copyright (C) 2002  Mike Rawes

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <ladspa.h>
#include "wdatutil.h"
#include "wavedata.h"
#include "common.h"

void
usage(void)
{
	int i;

	fprintf (stderr, "\n");
	fprintf (stderr, "Generate bandlimited wavedata and write as c header file\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "Usage: wavegen -w <Wavename> -r <Sample Rate> -f <Note> -s <Note Step>\n");
	fprintf (stderr, "               -m <Samples> [-o <Output Filename>] [-p <Prefix>]\n");
	fprintf (stderr, "               [-g <Factor>] [-q] [-t] [-h]\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "    -w OR --wave       Name of wave to generate (case insensitive)\n");
	fprintf (stderr, "    -r OR --rate       Intended playback rate in Samples/Second\n");
	fprintf (stderr, "    -f OR --first      First MIDI note to generate table for\n");
	fprintf (stderr, "    -s OR --step       Number of MIDI notes to skip for next table\n");
	fprintf (stderr, "    -m OR --min        Minimum table size in samples\n");
	fprintf (stderr, "    -o OR --output     Output Filename, name of file to output\n");
	fprintf (stderr, "                        If not given, output is to stdout\n");
	fprintf (stderr, "    -p OR --prefix     Prefix for declarations in header\n");
	fprintf (stderr, "    -g OR --gibbs      Compensate for Gibbs' effect\n");
	fprintf (stderr, "    -q OR --quiet      Surpress stderr output\n");
	fprintf (stderr, "    -t OR --test       Don't actually generate data\n");
	fprintf (stderr, "    -h OR --help       Print this text and exit\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "Supported waves:\n");

	for (i = 0; i < WAVE_TYPE_COUNT; i++)
		fprintf (stderr, "    %s (%s)\n", wave_names[i], wave_descriptions[i]);

	fprintf (stderr, "\n");
	fprintf (stderr, "Gibbs' Effect\n");
	fprintf (stderr, "    Gibbs' effect causes overshoot in waves generated from finite\n");
	fprintf (stderr, "    Fourier Series. Compensation can be applied, which will result in\n");
	fprintf (stderr, "    a waveform that sounds slightly less bright.\n");
	fprintf (stderr, "    Use the --gibbs option to set degree of compensatation, from 0.0\n");
	fprintf (stderr, "    (no compensation) to 1.0 (full compensation)\n");
	fprintf (stderr, "\n");
}

/*
* Create bandlimited wavedata header files for various
* waveforms
*/

int
main (int argc,
      char **argv)
{
	int option_index;
	int opt;
	const char * options = "w:r:f:s:m:o:p:g:qth";
	struct option long_options[] =
	{
		{ "wave", 1, 0, 'w' },
		{ "rate", 1, 0, 'r' },
		{ "first", 1, 0, 'f' },
		{ "step", 1, 0, 's' },
		{ "min", 1, 0, 'm' },
		{ "output", 1, 0, 'o' },
		{ "prefix", 0, 0, 'p' },
		{ "gibbs", 1, 0, 'g' },
		{ "quiet", 0, 0, 'q' },
		{ "test", 0, 0, 't' },
		{ "help", 0, 0, 'h' },
		{0, 0, 0, 0}
	};
	int wavetype = -1;
	long sample_rate = -1;
	long first_note = -1;
	long note_step = -1;
	long min_table_size = -1;
	char *filename = NULL;
	FILE *file;
	char *prefix = NULL;
	float gibbs = 0.0f;
	int quiet = 0;
	int test = 0;

	Wavedata * w;
	LADSPA_Data freq;
	unsigned long sample_count;
	unsigned long max_harmonic_hf;
	unsigned long max_harmonic_lf;
	unsigned long i;

	size_t strcmplen;
	size_t len1;
	size_t len2;

/* Parse arguments */
	if (argc == 1)
	{
		usage ();
		exit (-1);
	}

	opterr = 0;
	while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != -1)
	{
		switch (opt)
		{
		case 'w':
			for (i = 0; i < WAVE_TYPE_COUNT; i++)
			{
				len1 = strlen (optarg);
				len2 = strlen (wave_names[i]);
				strcmplen = len1 < len2 ? len1 : len2;

				if (!strncasecmp (optarg, wave_names[i], strcmplen))
					wavetype = i;
			}
			if (wavetype == -1)
			{
				fprintf (stderr, "Unrecognised option for Wave: %s\n", optarg);
				exit (-1);
			}
			break;
		case 'r':
			sample_rate = (long) atoi (optarg);
			break;
		case 'f':
			first_note = (long) atoi (optarg);
			break;
		case 's':
			note_step = (long) atoi (optarg);
			break;
		case 'm':
			min_table_size = (long) atoi (optarg);
			break;
		case 'o':
			filename = strdup (optarg);
			break;
		case 'p':
			prefix = strdup (optarg);
			break;
		case 'g':
			gibbs = atof (optarg);
			break;
		case 'q':
			quiet = -1;
			break;
		case 't':
			test = -1;
			break;
		case 'h':
			usage ();
			exit (0);
			break;
		default:
			usage ();
			exit (-1);
		}
	}

/* Check basic arguments */
	if (wavetype == -1)
	{
		if (!quiet) fprintf (stderr, "No wavetype specified.\n");
		exit (-1);
	}

	if (sample_rate == -1)
	{
		if (!quiet) fprintf (stderr, "No sample rate specified.\n");
		exit (-1);
	}

	if (first_note == -1)
	{
		if (!quiet) fprintf (stderr, "No first note specified.\n");
		exit (-1);
	}

	if (note_step == -1)
	{
		if (!quiet) fprintf (stderr, "No note step specified.\n");
		exit (-1);
	}

	if (min_table_size == -1)
	{
		if (!quiet) fprintf (stderr, "No minimum table size specified.\n");
		exit (-1);
	}

	if (gibbs < 0.0f || gibbs > 1.0f)
	{
		if (!quiet)
		{
			fprintf (stderr, "Gibbs compensation clamped to [0.0, 1.0]\n");
			fprintf (stderr, "    Supplied value: %.2f\n", gibbs);
		}
		gibbs = gibbs < 0.0f ? 0.0f : gibbs;
		gibbs = gibbs > 1.0f ? 1.0f : gibbs;
		if (!quiet)
			fprintf (stderr, "    Clamped to: %.2f\n", gibbs);
	}

	if (note_step < 1)
	{
		if (!quiet) fprintf (stderr, "Using minimum note step of 1\n");
		note_step = 1;
	}

/* Get file to write to */
	if (!filename)
		file = stdout;
	else
		file = fopen (filename, "w");

	w = wavedata_new (sample_rate);

	if (!w)
	{
		if (!quiet)
			fprintf (stderr, "Unable to create wavedata\n");

		exit (-1);
	}

	freq = FREQ_FROM_NOTE (first_note);
	max_harmonic_lf = HARM_FROM_FREQ (freq, sample_rate);
	max_harmonic_hf = max_harmonic_lf;

	for (i = 0; max_harmonic_hf > MIN_HARM (wavetype); i += note_step)
	{
		freq = FREQ_FROM_NOTE (first_note + i + note_step);
		max_harmonic_hf = HARM_FROM_FREQ (freq, sample_rate);

		max_harmonic_hf = ACTUAL_HARM (max_harmonic_hf, wavetype);
		max_harmonic_lf = ACTUAL_HARM (max_harmonic_lf, wavetype);

		while (max_harmonic_lf == max_harmonic_hf)
		{
			i += note_step;
			freq = FREQ_FROM_NOTE (first_note + i + note_step);
			max_harmonic_hf = HARM_FROM_FREQ (freq, sample_rate);
			max_harmonic_hf = ACTUAL_HARM (max_harmonic_hf, wavetype);
		}

		if (max_harmonic_lf > MIN_EXTRA_HARM (wavetype))
		{
			sample_count = max_harmonic_lf * 2;
			sample_count = sample_count < min_table_size ? min_table_size : sample_count;

			if (wavedata_add_table (w, sample_count, max_harmonic_lf))
			{
				if (!quiet)
					fprintf (stderr, "Could not add wavetable to wavedata\n");

				wavedata_cleanup (w);
				exit (-1);
			}
		}
		max_harmonic_lf = max_harmonic_hf;
	}

	if (!quiet)
	{
		fprintf (stderr, "\n");
		fprintf (stderr, "Generating %s wave\n", wave_names[wavetype]);
		fprintf (stderr, "    Sample Rate: %ld\n", sample_rate);
		if (gibbs > 0.0f)
			fprintf(stderr, "    Gibbs' compensation factor: %+.2f\n\n", gibbs);
	}

	wavedata_generate_tables (w, wavetype, gibbs);

	if (!test)
	{
		if (wavedata_write (w, file, prefix))
		{
			if (!quiet)
				fprintf (stderr, "Could not write to file %s!\n\n", filename);
		}
		else
		{
			if (!quiet)
				fprintf (stderr, "Written to file %s\n\n", filename);
		}
	}

	if (filename)
		free (filename);

	if (prefix)
		free (prefix);

	wavedata_cleanup (w);

	return 0;
}
