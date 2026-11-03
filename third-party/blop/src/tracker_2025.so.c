/*
    tracker.so.c - A LADSPA plugin to shape a signal in various ways

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

#include <stdlib.h>
#include <ladspa.h>
#include "common.h"
#include "gettext.h"

#ifdef ENABLE_NLS
#include <locale.h>
#define G_(s) gettext(s)
#else
#define G_(s) (s)
#endif
#define G_NOP(s) s

#define TRACKER_BASE_ID           2025
#define TRACKER_VARIANT_COUNT     2

#define TRACKER_GATE              0
#define TRACKER_HATTACK           1
#define TRACKER_HDECAY            2
#define TRACKER_LATTACK           3
#define TRACKER_LDECAY            4
#define TRACKER_INPUT             5
#define TRACKER_OUTPUT            6

LADSPA_Descriptor ** tracker_descriptors = 0;

typedef struct
{
	LADSPA_Data * gate;
	LADSPA_Data * hattack;
	LADSPA_Data * hdecay;
	LADSPA_Data * lattack;
	LADSPA_Data * ldecay;
	LADSPA_Data * input;
	LADSPA_Data * output;
	LADSPA_Data   coeff;
	LADSPA_Data   last_value;
} Tracker;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < TRACKER_VARIANT_COUNT)
		return tracker_descriptors[index];

	return 0;
}

void
cleanupTracker (LADSPA_Handle instance)
{
	free (instance);
}

void
connectPortTracker (LADSPA_Handle instance,
                    unsigned long port,
                    LADSPA_Data * data)
{
	Tracker * plugin = (Tracker *) instance;

	switch (port)
	{
	case TRACKER_GATE:
		plugin->gate = data;
		break;
	case TRACKER_HATTACK:
		plugin->hattack = data;
		break;
	case TRACKER_HDECAY:
		plugin->hdecay = data;
		break;
	case TRACKER_LATTACK:
		plugin->lattack = data;
		break;
	case TRACKER_LDECAY:
		plugin->ldecay = data;
		break;
	case TRACKER_INPUT:
		plugin->input = data;
		break;
	case TRACKER_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateTracker (const LADSPA_Descriptor * descriptor,
                    unsigned long sample_rate)
{
	Tracker * plugin = (Tracker *) malloc (sizeof (Tracker));

	plugin->coeff = 2.0f * M_PI / (LADSPA_Data) sample_rate;

	return (LADSPA_Handle) plugin;
}

void
activateTracker (LADSPA_Handle instance)
{
	Tracker * plugin = (Tracker *) instance;

	plugin->last_value = 0.0f;
}

void
runTracker_gaaadaia_oa (LADSPA_Handle instance,
                        unsigned long sample_count)
{
	Tracker * plugin = (Tracker *) instance;

/* Gate (array of floats of length sample_count) */
	LADSPA_Data * gate = plugin->gate;

/* Gate High Attack Rate (array of floats of length sample_count) */
	LADSPA_Data * hattack = plugin->hattack;

/* Gate High Decay Rate (array of floats of length sample_count) */
	LADSPA_Data * hdecay = plugin->hdecay;

/* Gate Low Attack Rate (array of floats of length sample_count) */
	LADSPA_Data * lattack = plugin->lattack;

/* Gate Low Decay Rate (array of floats of length sample_count) */
	LADSPA_Data * ldecay = plugin->ldecay;

/* Input (array of floats of length sample_count) */
	LADSPA_Data * input = plugin->input;

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

/* Instance Data */
	LADSPA_Data coeff = plugin->coeff;
	LADSPA_Data last_value = plugin->last_value;

	LADSPA_Data rate;
	LADSPA_Data in;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		in = input[s];

		if (gate[s] > 0.0f)
			rate = in > last_value ? hattack[s] : hdecay[s];
		else
			rate = in > last_value ? lattack[s] : ldecay[s];

		rate = f_min (1.0f, rate * coeff);
		last_value = last_value * (1.0f - rate) + in * rate;

		output[s] = last_value;
	}

	plugin->last_value = last_value;
}

void
runTracker_gaacdcia_oa (LADSPA_Handle instance,
                        unsigned long sample_count)
{
	Tracker * plugin = (Tracker *) instance;

/* Gate (array of floats of length sample_count) */
	LADSPA_Data * gate = plugin->gate;

/* Gate High Attack Rate (float value) */
	LADSPA_Data hattack = * (plugin->hattack);

/* Gate High Decay Rate (float value) */
	LADSPA_Data hdecay = * (plugin->hdecay);

/* Gate Low Attack Rate (float value) */
	LADSPA_Data lattack = * (plugin->lattack);

/* Gate Low Decay Rate (float value) */
	LADSPA_Data ldecay = * (plugin->ldecay);

/* Input (array of floats of length sample_count) */
	LADSPA_Data * input = plugin->input;

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

/* Instance Data */
	LADSPA_Data coeff = plugin->coeff;
	LADSPA_Data last_value = plugin->last_value;

	LADSPA_Data in;
	LADSPA_Data rate;
	unsigned int s;

	hattack = f_min (1.0f, hattack * coeff);
	hdecay = f_min (1.0f, hdecay * coeff);
	lattack = f_min (1.0f, lattack * coeff);
	ldecay = f_min (1.0f, ldecay * coeff);

	for (s = 0; s < sample_count; s++)
	{
		in = input[s];

		if (gate[s] > 0.0f)
			rate = in > last_value ? hattack : hdecay;
		else
			rate = in > last_value ? lattack : ldecay;

		last_value = last_value * (1.0f - rate) + in * rate;

		output[s] = last_value;
	}

	plugin->last_value = last_value;
}

void
_init (void)
{
	static const char * labels[] = {"tracker_gaaadaia_oa",
	                                "tracker_gaacdcia_oa"};
	static const char * names[] = {G_NOP("Signal Tracker (Audio Rates)"),
	                               G_NOP("Signal Tracker (Control Rates)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor gate_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO};
	LADSPA_PortDescriptor hattack_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                    LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor hdecay_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor lattack_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                    LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor ldecay_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor input_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runTracker_gaaadaia_oa,
	                                           runTracker_gaacdcia_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	tracker_descriptors = (LADSPA_Descriptor **) calloc (TRACKER_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (tracker_descriptors)
	{
		for (i = 0; i < TRACKER_VARIANT_COUNT; i++)
		{
			tracker_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = tracker_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = TRACKER_BASE_ID + i;
				descriptor->Label = labels[i];
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = G_(names[i]);
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = 7;

				port_descriptors = (LADSPA_PortDescriptor *)calloc(7, sizeof (LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (7, sizeof (LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (7, sizeof (char*));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Gate */
				port_descriptors[TRACKER_GATE] = gate_port_descriptors[i];
				port_names[TRACKER_GATE] = G_("Gate");
				port_range_hints[TRACKER_GATE].HintDescriptor = 0;

			/* Parameters for Gate High Attack Rate */
				port_descriptors[TRACKER_HATTACK] = hattack_port_descriptors[i];
				port_names[TRACKER_HATTACK] = G_("Attack Rate (Hz) when Gate High");
				port_range_hints[TRACKER_HATTACK].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                   LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE |
				                                                   LADSPA_HINT_DEFAULT_100;
				port_range_hints[TRACKER_HATTACK].LowerBound = 1.0f / 48000.0f;
				port_range_hints[TRACKER_HATTACK].UpperBound = 0.5f;

			/* Parameters for Gate High Decay Rate */
				port_descriptors[TRACKER_HDECAY] = hdecay_port_descriptors[i];
				port_names[TRACKER_HDECAY] = G_("Decay Rate (Hz) when Gate High");
				port_range_hints[TRACKER_HDECAY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                  LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE |
				                                                  LADSPA_HINT_DEFAULT_100;
				port_range_hints[TRACKER_HDECAY].LowerBound = 1.0f / 48000.0f;
				port_range_hints[TRACKER_HDECAY].UpperBound = 0.5f;

			/* Parameters for Gate Low Attack Rate */
				port_descriptors[TRACKER_LATTACK] = lattack_port_descriptors[i];
				port_names[TRACKER_LATTACK] = G_("Attack Rate (Hz) when Gate Low");
				port_range_hints[TRACKER_LATTACK].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                   LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE |
				                                                   LADSPA_HINT_DEFAULT_100;
				port_range_hints[TRACKER_LATTACK].LowerBound = 1.0f / 48000.0f;
				port_range_hints[TRACKER_LATTACK].UpperBound = 0.5f;

			/* Parameters for Gate Low Decay Rate */
				port_descriptors[TRACKER_LDECAY] = ldecay_port_descriptors[i];
				port_names[TRACKER_LDECAY] = G_("Decay Rate (Hz) when Gate Low");
				port_range_hints[TRACKER_LDECAY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                  LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE |
				                                                  LADSPA_HINT_DEFAULT_100;
				port_range_hints[TRACKER_LDECAY].LowerBound = 1.0f / 48000.0f;
				port_range_hints[TRACKER_LDECAY].UpperBound = 0.5f;

			/* Parameters for Input */
				port_descriptors[TRACKER_INPUT] = input_port_descriptors[i];
				port_names[TRACKER_INPUT] = G_("Input");
				port_range_hints[TRACKER_INPUT].HintDescriptor = 0;

			/* Parameters for Output */
				port_descriptors[TRACKER_OUTPUT] = output_port_descriptors[i];
				port_names[TRACKER_OUTPUT] = G_("Output");
				port_range_hints[TRACKER_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateTracker;
				descriptor->cleanup = cleanupTracker;
				descriptor->connect_port = connectPortTracker;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateTracker;
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

	if (tracker_descriptors)
	{
		for (i = 0; i < TRACKER_VARIANT_COUNT; i++)
		{
			descriptor = tracker_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (tracker_descriptors);
	}
}
