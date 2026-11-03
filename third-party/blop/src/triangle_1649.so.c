/*
    triangle.so.c - A LADSPA plugin to generate a bandlimited slope-variable
                    triangle waveform

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
#include <ladspa.h>
#include "wavedata.h"
#include "gettext.h"

#ifdef ENABLE_NLS
#include <locale.h>
#define G_(s) gettext(s)
#else
#define G_(s) (s)
#endif
#define G_NOP(s) s

#define TRIANGLE_BASE_ID               1649
#define TRIANGLE_VARIANT_COUNT         4

#define TRIANGLE_FREQUENCY             0
#define TRIANGLE_SLOPE                 1
#define TRIANGLE_OUTPUT                2

LADSPA_Descriptor ** triangle_descriptors = 0;

typedef struct
{
	LADSPA_Data * frequency;
	LADSPA_Data * slope;
	LADSPA_Data * output;
	LADSPA_Data   phase;
	LADSPA_Data   min_slope;
	LADSPA_Data   max_slope;
	Wavedata      wdat;
} Triangle;

/*****************************************************************************
 *
 * LADSPA Plugin code
 *
 *****************************************************************************/

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < TRIANGLE_VARIANT_COUNT)
		return triangle_descriptors[index];

	return 0;
}

void
connectPortTriangle (LADSPA_Handle instance,
                     unsigned long port,
                     LADSPA_Data * data)
{
	Triangle * plugin = (Triangle *) instance;

	switch (port)
	{
	case TRIANGLE_FREQUENCY:
		plugin->frequency = data;
		break;
	case TRIANGLE_SLOPE:
		plugin->slope = data;
		break;
	case TRIANGLE_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateTriangle (const LADSPA_Descriptor * descriptor,
                     unsigned long sample_rate)
{
	Triangle * plugin = (Triangle *) malloc (sizeof (Triangle));

	if (wavedata_load (&plugin->wdat, BLOP_DLSYM_PARABOLA, sample_rate))
	{
		free (plugin);
		return 0;
	}

	plugin->min_slope = 2.0f / plugin->wdat.sample_rate;
	plugin->max_slope = 1.0f - plugin->min_slope;

	return (LADSPA_Handle) plugin;
}

void
cleanupTriangle (LADSPA_Handle instance)
{
	Triangle * plugin = (Triangle *) instance;

	wavedata_unload (&plugin->wdat);
	free (instance);
}

void
activateTriangle (LADSPA_Handle instance)
{
	Triangle * plugin = (Triangle *) instance;

	plugin->phase = 0.0f;
}

void
runTriangle_fasa_oa (LADSPA_Handle instance,
                     unsigned long sample_count)
{
	Triangle * plugin = (Triangle *) instance;

/* Frequency (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Slope (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * slope = plugin->slope;

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data min_slope = plugin->min_slope;
	LADSPA_Data max_slope = plugin->max_slope;

	LADSPA_Data freq;
	LADSPA_Data slp;
	LADSPA_Data phase_shift;
	unsigned long s;

	for (s = 0; s < sample_count; s++)
	{
		freq = frequency[s];
		slp = f_clip (slope[s], min_slope, max_slope);
		phase_shift = slp * wdat->sample_rate;

	/* Lookup which table to use from frequency */
		wavedata_get_table (wdat, freq);

	/* Get samples from parabola and phase shifted inverted parabola,
	   and scale to compensate */
		output[s] = (wavedata_get_sample (wdat, phase) -
		             wavedata_get_sample (wdat, phase + phase_shift)) /
		            (8.0f * (slp - (slp * slp)));

	/* Update phase, wrapping if necessary */
		phase += wdat->frequency;
		if (phase < 0.0f)
			phase += wdat->sample_rate;
		else if (phase > wdat->sample_rate)
			phase -= wdat->sample_rate;
	}
	plugin->phase = phase;
}

void
runTriangle_fasc_oa (LADSPA_Handle instance,
                     unsigned long sample_count)
{
	Triangle * plugin = (Triangle *) instance;

/* Frequency (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Slope (LADSPA_Data value) */
	LADSPA_Data slope = *(plugin->slope);

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data min_slope = plugin->min_slope;
	LADSPA_Data max_slope = plugin->max_slope;

	LADSPA_Data freq;
	LADSPA_Data phase_shift;
	LADSPA_Data scale;
	unsigned long s;

	slope = f_clip (slope, min_slope, max_slope);
	scale = 1.0f / (8.0f * (slope - (slope * slope)));
	phase_shift = slope * wdat->sample_rate;

	for (s = 0; s < sample_count; s++)
	{
		freq = frequency[s];

	/* Lookup which table to use from frequency */
		wavedata_get_table (wdat, freq);

	/* Get samples from parabola and phase shifted inverted parabola,
	   and scale to compensate */
		output[s] = (wavedata_get_sample (wdat, phase) -
		             wavedata_get_sample (wdat, phase + phase_shift)) * scale;

	/* Update phase, wrapping if necessary */
		phase += wdat->frequency;
		if (phase < 0.0f)
			phase += wdat->sample_rate;
		else if (phase > wdat->sample_rate)
			phase -= wdat->sample_rate;
	}
	plugin->phase = phase;
}

void
runTriangle_fcsa_oa (LADSPA_Handle instance,
                     unsigned long sample_count)
{
	Triangle * plugin = (Triangle *) instance;

/* Frequency (LADSPA_Data value) */
	LADSPA_Data frequency = *(plugin->frequency);

/* Slope (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * slope = plugin->slope;

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data min_slope = plugin->min_slope;
	LADSPA_Data max_slope = plugin->max_slope;

	LADSPA_Data slp;
	LADSPA_Data phase_shift;
	unsigned long s;

	wavedata_get_table (wdat, frequency);

	for (s = 0; s < sample_count; s++)
	{
		slp = f_clip (slope[s], min_slope, max_slope);
		phase_shift = slp * wdat->sample_rate;

	/* Get samples from parabola and phase shifted inverted parabola,
	   and scale to compensate */
		output[s] = (wavedata_get_sample (wdat, phase) -
		             wavedata_get_sample (wdat, phase + phase_shift)) /
		            (8.0f * (slp - (slp * slp)));

	/* Update phase, wrapping if necessary */
		phase += wdat->frequency;
		if (phase < 0.0f)
			phase += wdat->sample_rate;
		else if (phase > wdat->sample_rate)
			phase -= wdat->sample_rate;
	}
	plugin->phase = phase;
}

void
runTriangle_fcsc_oa (LADSPA_Handle instance,
                     unsigned long sample_count)
{
	Triangle * plugin = (Triangle *) instance;

/* Frequency (LADSPA_Data value) */
	LADSPA_Data frequency = *(plugin->frequency);

/* Slope (LADSPA_Data value) */
	LADSPA_Data slope = *(plugin->slope);

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data min_slope = plugin->min_slope;
	LADSPA_Data max_slope = plugin->max_slope;

	LADSPA_Data scale;
	LADSPA_Data phase_shift;
	unsigned long s;

	slope = f_clip (slope, min_slope, max_slope);
	scale = 1.0f / (8.0f * (slope - (slope * slope)));
	phase_shift = slope * wdat->sample_rate;

	wavedata_get_table (wdat, frequency);

	for (s = 0; s < sample_count; s++)
	{
	/* Get samples from parabola and phase shifted inverted parabola,
	   and scale to compensate */
		output[s] = (wavedata_get_sample (wdat, phase) -
		             wavedata_get_sample (wdat, phase + phase_shift)) * scale;

	/* Update phase, wrapping if necessary */
		phase += wdat->frequency;
		if (phase < 0.0f)
			phase += wdat->sample_rate;
		else if (phase > wdat->sample_rate)
			phase -= wdat->sample_rate;
	}
	plugin->phase = phase;
}

void
_init (void)
{
	static const char * labels[] = {"triangle_fasa_oa",
	                                "triangle_fasc_oa",
	                                "triangle_fcsa_oa",
	                                "triangle_fcsc_oa"};
	static const char * names[] = {G_NOP("Bandlimited Variable Slope Triangle Oscillator (FASA)"),
	                               G_NOP("Bandlimited Variable Slope Triangle Oscillator (FASC)"),
	                               G_NOP("Bandlimited Variable Slope Triangle Oscillator (FCSA)"),
	                               G_NOP("Bandlimited Variable Slope Triangle Oscillator (FCSC)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor frequency_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor slope_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runTriangle_fasa_oa,
	                                           runTriangle_fasc_oa,
	                                           runTriangle_fcsa_oa,
	                                           runTriangle_fcsc_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	triangle_descriptors = (LADSPA_Descriptor **) calloc (TRIANGLE_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (triangle_descriptors)
	{
		for (i = 0; i < TRIANGLE_VARIANT_COUNT; i++)
		{
			triangle_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = triangle_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = TRIANGLE_BASE_ID + i;
				descriptor->Label = labels[i];
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = G_(names[i]);
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = 3;

				port_descriptors = (LADSPA_PortDescriptor *) calloc (3, sizeof (LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *) port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (3, sizeof (LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (3, sizeof (char*));
				descriptor->PortNames = (const char **) port_names;

				/* Parameters for Frequency */
				port_descriptors[TRIANGLE_FREQUENCY] = frequency_port_descriptors[i];
				port_names[TRIANGLE_FREQUENCY] = G_("Frequency");
				port_range_hints[TRIANGLE_FREQUENCY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                      LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC |
				                                                      LADSPA_HINT_DEFAULT_440;
				port_range_hints[TRIANGLE_FREQUENCY].LowerBound = 1.0f / 48000.0f;
				port_range_hints[TRIANGLE_FREQUENCY].UpperBound = 0.5f;

				/* Parameters for Slope */
				port_descriptors[TRIANGLE_SLOPE] = slope_port_descriptors[i];
				port_names[TRIANGLE_SLOPE] = G_("Slope");
				port_range_hints[TRIANGLE_SLOPE].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
																LADSPA_HINT_DEFAULT_MIDDLE;
				port_range_hints[TRIANGLE_SLOPE].LowerBound = 0.0f;
				port_range_hints[TRIANGLE_SLOPE].UpperBound = 1.0f;

				/* Parameters for Output */
				port_descriptors[TRIANGLE_OUTPUT] = output_port_descriptors[i];
				port_names[TRIANGLE_OUTPUT] = G_("Output");
				port_range_hints[TRIANGLE_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateTriangle;
				descriptor->cleanup = cleanupTriangle;
				descriptor->connect_port = connectPortTriangle;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateTriangle;
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

	if (triangle_descriptors)
	{
		for (i = 0; i < TRIANGLE_VARIANT_COUNT; i++)
		{
			descriptor = triangle_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (triangle_descriptors);
	}
}
