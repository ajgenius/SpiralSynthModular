/*
    quantiser.so.c - A LADSPA plugin to quantise an input to a set
                     of fixed values

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

#include <stdio.h>
#include <stdlib.h>
#include <ladspa.h>
#include "math_func.h"
#include "common.h"
#include "gettext.h"

#ifdef ENABLE_NLS
#include <locale.h>
#define G_(s) gettext(s)
#else
#define G_(s) (s)
#endif
#define G_NOP(s) s

/* These are defined in the compiler flags - see Makefile.am
 * This code is used to create the three quantisers with
 * 20, 50 and 100 steps
#define QUANTISER_BASE_ID                   2027
#define QUANTISER_MAX_INPUTS                20
 */

#define QUANTISER_VARIANT_COUNT             1

#define QUANTISER_RANGE_MIN                 0
#define QUANTISER_RANGE_MAX                 1
#define QUANTISER_MATCH_RANGE               2
#define QUANTISER_MODE                      3
#define QUANTISER_COUNT                     4
#define QUANTISER_VALUE_START               5
#define QUANTISER_INPUT                     (QUANTISER_MAX_INPUTS + 5)
#define QUANTISER_OUTPUT                    (QUANTISER_MAX_INPUTS + 6)
#define QUANTISER_OUTPUT_CHANGED            (QUANTISER_MAX_INPUTS + 7)

LADSPA_Descriptor ** quantiser_descriptors = 0;

typedef struct {
	LADSPA_Data * min;
	LADSPA_Data * max;
	LADSPA_Data * match_range;
	LADSPA_Data * mode;
	LADSPA_Data * count;
	LADSPA_Data * values[QUANTISER_MAX_INPUTS];
	LADSPA_Data * input;
	LADSPA_Data * output_changed;
	LADSPA_Data * output;
	LADSPA_Data   svalues[QUANTISER_MAX_INPUTS+2];
	LADSPA_Data   temp[QUANTISER_MAX_INPUTS+2];
	LADSPA_Data   last_found;
} Quantiser;

/*
 * f <= m <= l
 */
static inline void
merge (LADSPA_Data * v,
       int f,
       int m,
       int l,
       LADSPA_Data * temp)
{
	int f1 = f;
	int l1 = m;
	int f2 = m+1;
	int l2 = l;
	int i = f1;

	while ((f1 <= l1) && (f2 <= l2))
	{
		if (v[f1] < v[f2])
		{
			temp[i] = v[f1];
			f1++;
		}
		else
		{
			temp[i] = v[f2];
			f2++;
		}
		i++;
	}
	while (f1 <= l1)
	{
		temp[i] = v[f1];
		f1++;
		i++;
	}
	while (f2 <= l2)
	{
		temp[i] = v[f2];
		f2++;
		i++;
	}
	for (i = f; i <= l; i++)
		v[i] = temp[i];
}
/*
 * Recursive Merge Sort
 * Sort elements in unsorted vector v
 */
static inline void
msort (LADSPA_Data * v,
       int f,
       int l,
       LADSPA_Data * temp)
{
	int m;

	if (f < l)
	{
		m = (f + l) / 2;
		msort (v, f, m, temp);
		msort (v, m+1, l, temp);
		merge (v, f, m, l, temp);
	}
}
/*
 * Binary search for nearest match to sought value in
 * ordered vector v of given size
 */
static inline int
fuzzy_bsearch (LADSPA_Data * v,
               int size,
               LADSPA_Data sought)
{
	int f = 0;
	int l = size - 1;
	int m = ((l - f) / 2);

	while ((l - f) > 1)
	{
		if (v[m] < sought)
			f = (l - f) / 2 + f;
		else
			l = (l - f) / 2 + f;

		m = ((l - f) / 2 + f);
	}

	if (sought < v[m])
	{
		if (m > 0)
		{
			if (FABSF (v[m] - sought) > FABSF (v[m - 1] - sought))
				m--;
		}
	}
	else if (m < size - 1)
	{
		if (FABSF (v[m] - sought) > FABSF (v[m + 1] - sought))
			m++;
	}

	return m;
}

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < 1)
		return quantiser_descriptors[index];

	return 0;
}

void
cleanupQuantiser (LADSPA_Handle instance)
{
	free (instance);
}

void
connectPortQuantiser (LADSPA_Handle instance,
                      unsigned long port,
                      LADSPA_Data * data)
{
	Quantiser * plugin = (Quantiser *) instance;

	switch (port)
	{
	case QUANTISER_RANGE_MIN:
		plugin->min = data;
		break;
	case QUANTISER_RANGE_MAX:
		plugin->max = data;
		break;
	case QUANTISER_MATCH_RANGE:
		plugin->match_range = data;
		break;
	case QUANTISER_MODE:
		plugin->mode = data;
		break;
	case QUANTISER_COUNT:
		plugin->count = data;
		break;
	case QUANTISER_INPUT:
		plugin->input = data;
		break;
	case QUANTISER_OUTPUT:
		plugin->output = data;
		break;
	case QUANTISER_OUTPUT_CHANGED:
		plugin->output_changed = data;
		break;
	default:
		if (port >= QUANTISER_VALUE_START && port  < QUANTISER_OUTPUT)
			plugin->values[port - QUANTISER_VALUE_START] = data;
		break;
	}
}

LADSPA_Handle
instantiateQuantiser (const LADSPA_Descriptor * descriptor,
                      unsigned long sample_rate)
{
	Quantiser * plugin = (Quantiser *) malloc (sizeof (Quantiser));

	return (LADSPA_Handle) plugin;
}

#if 0
void
runQuantiser_audio (LADSPA_Handle instance,
                    unsigned long sample_count)
{
	Quantiser * plugin = (Quantiser *) instance;

/* Range Min (LADSPA_Data value) */
	LADSPA_Data min = * (plugin->min);

/* Range Max (LADSPA_Data value) */
	LADSPA_Data max = * (plugin->max);

/* Match Range (LADSPA_Data value) */
	LADSPA_Data match_range = * (plugin->match_range);

/* Mode (LADSPA_Data value) */
	LADSPA_Data mode = * (plugin->mode);

/* Count (LADSPA_Data value) */
	LADSPA_Data count = * (plugin->count);

/* Input (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * input = plugin->input;

/* Values */
	LADSPA_Data * values[QUANTISER_MAX_INPUTS];

/* Output (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * output = plugin->output;

/* Output Changed (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * output_changed = plugin->output_changed;

	unsigned long s;

	for (s = 0; s < sample_count; s++) {
		output[s] = input[s];
	}
}
#endif

void
runQuantiser_control (LADSPA_Handle instance,
                      unsigned long sample_count)
{
	Quantiser * plugin = (Quantiser *) instance;

/* Range Min (LADSPA_Data value) */
	LADSPA_Data min = * (plugin->min);

/* Range Max (LADSPA_Data value) */
	LADSPA_Data max = * (plugin->max);

/* Match Range (LADSPA_Data value) */
	LADSPA_Data match_range = FABSF (* (plugin->match_range));

/* Mode (LADSPA_Data value) */
	LADSPA_Data mode = * (plugin->mode);

/* Count (LADSPA_Data value) */
	LADSPA_Data count = * (plugin->count);

/* Input (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * input = plugin->input;

/* Output (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * output = plugin->output;

/* Output Changed (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * output_changed = plugin->output_changed;

/* Instance Data */
	LADSPA_Data * temp = plugin->temp;
	LADSPA_Data * values = plugin->svalues;
	LADSPA_Data last_found = plugin->last_found;

	int md = LRINTF (mode);
	int n = LRINTF (count);
	int i;
	LADSPA_Data in;
	LADSPA_Data out_changed;
	LADSPA_Data range;
	LADSPA_Data offset;
	LADSPA_Data found;
	int found_index = 0;
	unsigned long s;

/* Clip count if out of range */
	n = n < 1 ? 1 : (n > QUANTISER_MAX_INPUTS ? QUANTISER_MAX_INPUTS : n);

/* Swap min and max if wrong way around */
	if (min > max)
	{
		range = min;
		min = max;
		max = range;
	}
	range = max - min;

/* Copy and sort required values */
	for (i = 0; i < n; i++)
		values[i + 1] = * (plugin->values[i]);

	msort (values, 1, n, temp);

/* Add wrapped extremes */
	values[0] = values[n] - range;
	values[n+1] = values[1] + range;

	if (md < 1)
	{
	/* Extend mode */
		for (s = 0; s < sample_count; s++)
		{
			in = input[s];

			if (range > 0.0f)
			{
				if ((in < min) || (in > max))
				{
					offset = FLOORF ((in - max) / range) + 1.0f;
					offset *= range;
					in -= offset;

				/* Quantise */
					found_index = fuzzy_bsearch (values, n + 2, in);

				/* Wrapped */
					if (found_index == 0)
					{
						found_index = n;
						offset -= range;
					}
					else if (found_index == n + 1)
					{
						found_index = 1;
						offset += range;
					}

					found = values[found_index];

				/* Allow near misses */
					if (match_range > 0.0f)
					{
						if (in < (found - match_range))
							found -= match_range;
						else if (in > (found + match_range))
							found += match_range;
					}
					found += offset;
				}
				else
				{
				/* Quantise */
					found_index = fuzzy_bsearch (values, n + 2, in);
					if (found_index == 0)
					{
						found_index = n;
						found = values[n] - range;
					}
					else if (found_index == n + 1)
					{
						found_index = 1;
						found = values[1] + range;
					}
					else
					{
						found = values[found_index];
					}

					if (match_range > 0.0f)
					{
						if (in < (found - match_range))
							found -= match_range;
						else if (in > (found + match_range))
							found += match_range;
					}
				}
			}
			else
			{
			/* Min and max the same, so only one value to quantise to */
				found = min;
			}

		/* Has quantised output changed? */
			if (FABSF (found - last_found) > 2.0001f * match_range)
			{
				out_changed = 1.0f;
				last_found = found;
			}
			else
			{
				out_changed = 0.0f;
			}

			output[s] = found;
			output_changed[s] = out_changed;
		}
	}
	else if (md == 1)
	{
	/* Wrap mode */
		for (s = 0; s < sample_count; s++)
		{
			in = input[s];

			if (range > 0.0f)
			{
				if ((in < min) || (in > max))
					in -= (FLOORF ((in - max) / range) + 1.0f) * range;

			/* Quantise */
				found_index = fuzzy_bsearch (values, n + 2, in);
				if (found_index == 0)
					found_index = n;
				else if (found_index == n + 1)
					found_index = 1;

				found = values[found_index];

			/* Allow near misses */
				if (match_range > 0.0f)
				{
					if (in < (found - match_range))
						found -= match_range;
					else if (in > (found + match_range))
						found += match_range;
				}
			}
			else
			{
			/* Min and max the same, so only one value to quantise to */
				found = min;
			}

		/* Has quantised output changed? */
			if (FABSF (found - last_found) > match_range)
			{
				out_changed = 1.0f;
				last_found = found;
			}
			else
			{
				out_changed = 0.0f;
			}

			output[s] = found;
			output_changed[s] = out_changed;
		}
	}
	else
	{
	/* Clip mode */
		for (s = 0; s < sample_count; s++)
		{
			in = input[s];

			if (range > 0.0f)
			{
			/* Clip to range */
				if (in < min)
					found_index = 1;
				else if (in > max)
					found_index = n;
				else
				/* Quantise */
					found_index = fuzzy_bsearch (values + 1, n, in) + 1;

				found = values[found_index];

			/* Allow near misses */
				if (match_range > 0.0f)
				{
					if (in < (found - match_range))
						found -= match_range;
					else if (in > (found + match_range))
						found += match_range;
				}
			}
			else
			{
			/* Min and max the same, so only one value to quantise to */
				found = min;
			}

		/* Has quantised output changed? */
			if (FABSF (found - last_found) > match_range)
			{
				out_changed = 1.0f;
				last_found = found;
			}
			else
			{
				out_changed = 0.0f;
			}

			output[s] = found;
			output_changed[s] = out_changed;
		}
	}
	plugin->last_found = last_found;
}

void
_init (void)
{
/* !!!! Ensure there is space for possible translations !!!! */
	static char label[32];
	static char name[32];
	static char loop_point_label[32];
	static char value_labels[QUANTISER_MAX_INPUTS][16];

	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i, step_index;
	unsigned long port_count = QUANTISER_MAX_INPUTS + 8;

	LADSPA_PortDescriptor value_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runQuantiser_control};
/*
    LADSPA_PortDescriptor value_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};

    void (*run_functions[])(LADSPA_Handle,
                            unsigned long) = {runQuantiser_audio,
                                              runQuantiser_control};
 */
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	quantiser_descriptors = (LADSPA_Descriptor **) calloc (QUANTISER_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	sprintf (label,"quantiser%d", QUANTISER_MAX_INPUTS);
	sprintf (name, G_("Quantiser (%d Steps)"), QUANTISER_MAX_INPUTS);
	sprintf (loop_point_label, G_("Steps (1 - %d)"), QUANTISER_MAX_INPUTS);

	if (quantiser_descriptors)
	{
        for (i = 0; i < QUANTISER_VARIANT_COUNT; i++)
		{
			quantiser_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = quantiser_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = QUANTISER_BASE_ID + i;
				descriptor->Label = label;
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = name;
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = port_count;

				port_descriptors = (LADSPA_PortDescriptor *) calloc (port_count, sizeof (LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (port_count, sizeof (LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (port_count, sizeof(char*));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Range Min */
				port_descriptors[QUANTISER_RANGE_MIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
				port_names[QUANTISER_RANGE_MIN] = G_("Quantise Range Minimum");
				port_range_hints[QUANTISER_RANGE_MIN].HintDescriptor = 0;

			/* Parameters for Range Min */
				port_descriptors[QUANTISER_RANGE_MAX] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
				port_names[QUANTISER_RANGE_MAX] = G_("Quantise Range Maximum");
				port_range_hints[QUANTISER_RANGE_MAX].HintDescriptor = 0;

			/* Parameters for Match Range */
				port_descriptors[QUANTISER_MATCH_RANGE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
				port_names[QUANTISER_MATCH_RANGE] = G_("Match Range");
				port_range_hints[QUANTISER_MATCH_RANGE].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_0;
				port_range_hints[QUANTISER_MATCH_RANGE].LowerBound = 0.0f;

			/* Parameters for Mode */
				port_descriptors[QUANTISER_MODE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
				port_names[QUANTISER_MODE] = G_("Mode (0 = Extend, 1 = Wrap, 2 = Clip)");
				port_range_hints[QUANTISER_MODE].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                  LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
				port_range_hints[QUANTISER_MODE].LowerBound = 0.0f;
				port_range_hints[QUANTISER_MODE].UpperBound = 2.0f;

			/* Parameters for Count */
				port_descriptors[QUANTISER_COUNT] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
				port_names[QUANTISER_COUNT] = loop_point_label;
				port_range_hints[QUANTISER_COUNT].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                   LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MAXIMUM;
				port_range_hints[QUANTISER_COUNT].LowerBound = 1.0f;
				port_range_hints[QUANTISER_COUNT].UpperBound = (float)QUANTISER_MAX_INPUTS;

			/* Parameters for Values */
				for (step_index = 0; step_index < QUANTISER_MAX_INPUTS; step_index++)
				{
					port_descriptors[QUANTISER_VALUE_START + step_index] = value_port_descriptors[i];
					sprintf (value_labels[step_index], G_("Value %d"), step_index);
					port_names[QUANTISER_VALUE_START + step_index] = value_labels[step_index];
					port_range_hints[QUANTISER_VALUE_START + step_index].HintDescriptor = 0;
				}

			/* Parameters for Input */
				port_descriptors[QUANTISER_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
				port_names[QUANTISER_INPUT] = G_("Input");
				port_range_hints[QUANTISER_INPUT].HintDescriptor = 0;

			/* Parameters for Output */
				port_descriptors[QUANTISER_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
				port_names[QUANTISER_OUTPUT] = G_("Quantised Output");
				port_range_hints[QUANTISER_OUTPUT].HintDescriptor = 0;

			/* Parameters for Output Changed */
				port_descriptors[QUANTISER_OUTPUT_CHANGED] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
				port_names[QUANTISER_OUTPUT_CHANGED] = G_("Output Changed");
				port_range_hints[QUANTISER_OUTPUT_CHANGED].HintDescriptor = 0;

				descriptor->activate = NULL;
				descriptor->cleanup = cleanupQuantiser;
				descriptor->connect_port = connectPortQuantiser;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateQuantiser;
				descriptor->run = run_functions[i];
				descriptor->run_adding = NULL;
				descriptor->set_run_adding_gain = NULL;
			}
		}
	}
}

void
_fini (void)
{
	LADSPA_Descriptor * descriptor;
	int i;

	if (quantiser_descriptors)
	{
		for (i = 0; i < QUANTISER_VARIANT_COUNT; i++)
		{
			descriptor = quantiser_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **)descriptor->PortNames);
				free ((LADSPA_PortRangeHint *)descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (quantiser_descriptors);
	}
}
