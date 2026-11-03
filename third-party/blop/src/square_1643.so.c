/*
    square.so.c - A LADSPA plugin to generate a bandlimited square waveform

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

#define SQUARE_BASE_ID                 1643
#define SQUARE_VARIANT_COUNT           2

#define SQUARE_FREQUENCY               0
#define SQUARE_OUTPUT                  1

LADSPA_Descriptor ** square_descriptors = 0;

typedef struct
{
	LADSPA_Data * frequency;
	LADSPA_Data * output;
	LADSPA_Data   phase;
	Wavedata      wdat;
} Square;

/*****************************************************************************
 *
 * LADSPA Plugin code
 *
 *****************************************************************************/

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < SQUARE_VARIANT_COUNT)
		return square_descriptors[index];

	return 0;
}

void
connectPortSquare (LADSPA_Handle instance,
                   unsigned long port,
                   LADSPA_Data * data)
{
	Square * plugin = (Square *) instance;

	switch (port)
	{
	case SQUARE_FREQUENCY:
		plugin->frequency = data;
		break;
	case SQUARE_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateSquare (const LADSPA_Descriptor * descriptor,
                   unsigned long sample_rate)
{
	Square *plugin = (Square *) malloc (sizeof (Square));

	if (wavedata_load (&plugin->wdat, BLOP_DLSYM_SQUARE, sample_rate))
	{
		free (plugin);
		return NULL;
	}

	return (LADSPA_Handle) plugin;
}

void
cleanupSquare (LADSPA_Handle instance)
{
	Square * plugin = (Square *) instance;

	wavedata_unload (&plugin->wdat);
	free (instance);
}

void
activateSquare (LADSPA_Handle instance)
{
	Square * plugin = (Square *) instance;

	plugin->phase = 0.0f;
}

void
runSquare_fa_oa (LADSPA_Handle instance,
                 unsigned long sample_count)
{
	Square * plugin = (Square *) instance;

/* Frequency (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;

	LADSPA_Data freq;
	unsigned long s;

	for (s = 0; s < sample_count; s++)
	{
		freq = frequency[s];

	/* Get table to play */
		wavedata_get_table (wdat, freq);

	/* Get interpolated sample */
		output[s] = wavedata_get_sample (wdat, phase);

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
runSquare_fc_oa (LADSPA_Handle instance,
                 unsigned long sample_count)
{
	Square * plugin = (Square *) instance;

/* Frequency (LADSPA_Data value) */
	LADSPA_Data frequency = * (plugin->frequency);

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;

	unsigned long s;

	wavedata_get_table (wdat, frequency);

	for (s = 0; s < sample_count; s++)
	{
		output[s] = wavedata_get_sample (wdat, phase);

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
	static const char * labels[] = {"square_fa_oa",
	                                "square_fc_oa"};
	static const char * names[] = {G_NOP("Bandlimited Square Oscillator (FA)"),
	                               G_NOP("Bandlimited Square Oscillator (FC)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor frequency_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runSquare_fa_oa,
	                                           runSquare_fc_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	square_descriptors = (LADSPA_Descriptor **) calloc (SQUARE_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (square_descriptors)
	{
		for (i = 0; i < SQUARE_VARIANT_COUNT; i++)
		{
			square_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = square_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = SQUARE_BASE_ID + i;
				descriptor->Label = labels[i];
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = G_(names[i]);
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = 2;

				port_descriptors = (LADSPA_PortDescriptor *) calloc (2, sizeof (LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *) port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (2, sizeof (LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (2, sizeof (char*));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Frequency */
				port_descriptors[SQUARE_FREQUENCY] = frequency_port_descriptors[i];
				port_names[SQUARE_FREQUENCY] = G_("Frequency");
				port_range_hints[SQUARE_FREQUENCY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                    LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC |
				                                                    LADSPA_HINT_DEFAULT_440;
				port_range_hints[SQUARE_FREQUENCY].LowerBound = 1.0f / 48000.0f;
				port_range_hints[SQUARE_FREQUENCY].UpperBound = 0.5f;

			/* Parameters for Output */
				port_descriptors[SQUARE_OUTPUT] = output_port_descriptors[i];
				port_names[SQUARE_OUTPUT] = G_("Output");
				port_range_hints[SQUARE_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateSquare;
				descriptor->cleanup = cleanupSquare;
				descriptor->connect_port = connectPortSquare;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateSquare;
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

	if (square_descriptors)
	{
		for (i = 0; i < SQUARE_VARIANT_COUNT; i++)
		{
			descriptor = square_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (square_descriptors);
	}
}
