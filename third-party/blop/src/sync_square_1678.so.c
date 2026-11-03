/*
    syncsquare.so.c - A LADSPA plugin to generate a non-bandlimited
                 square waveform with gate for trigger and sync

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
#include "gettext.h"

#ifdef ENABLE_NLS
#include <locale.h>
#define G_(s) gettext(s)
#else
#define G_(s) (s)
#endif
#define G_NOP(s) s

#define SYNCSQUARE_BASE_ID                  1678
#define SYNCSQUARE_VARIANT_COUNT            2

#define SYNCSQUARE_FREQUENCY                0
#define SYNCSQUARE_GATE                     1
#define SYNCSQUARE_OUTPUT                   2

LADSPA_Descriptor ** syncsquare_descriptors = 0;

typedef struct
{
	LADSPA_Data * frequency;
	LADSPA_Data * gate;
	LADSPA_Data * output;
	LADSPA_Data   srate;
	LADSPA_Data   nyquist;
	LADSPA_Data   phase;
} SyncSquare;

/*****************************************************************************
 *
 * LADSPA Plugin code
 *
 *****************************************************************************/

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < SYNCSQUARE_VARIANT_COUNT)
		return syncsquare_descriptors[index];

	return 0;
}

void
cleanupSyncSquare (LADSPA_Handle instance)
{
	free (instance);
}

void
connectPortSyncSquare (LADSPA_Handle instance,
                       unsigned long port,
                       LADSPA_Data * data)
{
	SyncSquare * plugin = (SyncSquare *) instance;

	switch (port)
	{
	case SYNCSQUARE_FREQUENCY:
		plugin->frequency = data;
		break;
	case SYNCSQUARE_GATE:
		plugin->gate = data;
		break;
	case SYNCSQUARE_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateSyncSquare (const LADSPA_Descriptor * descriptor,
                       unsigned long sample_rate)
{
	SyncSquare * plugin = (SyncSquare *) malloc (sizeof (SyncSquare));

	plugin->srate = (LADSPA_Data) sample_rate;
	plugin->nyquist = (LADSPA_Data) (sample_rate / 2);

	return (LADSPA_Handle) plugin;
}

void activateSyncSquare (LADSPA_Handle instance)
{
	SyncSquare * plugin = (SyncSquare *) instance;

	plugin->phase = 0.0f;
}

void
runSyncSquare_faga_oa (LADSPA_Handle instance,
                       unsigned long sample_count)
{
	SyncSquare * plugin = (SyncSquare *) instance;

/* Frequency (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Gate (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * gate = plugin->gate;

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data srate = plugin->srate;
	LADSPA_Data nyquist = plugin->nyquist;

	LADSPA_Data freq;
	unsigned long s;

	for (s = 0; s < sample_count; s++)
	{
		if (gate[s] > 0.0f)
		{
			freq = frequency[s];

			if (phase < nyquist)
				output[s] = 1.0f;
			else
				output[s] = -1.0f;

			phase += freq;
			if (phase < 0.0f)
				phase += srate;
			else if (phase > srate)
				phase -= srate;
		}
		else
		{
			output[s] = 0.0f;
			phase = 0.0f;
		}
	}

	plugin->phase = phase;
}

void
runSyncSquare_fcga_oa (LADSPA_Handle instance,
                       unsigned long sample_count)
{
	SyncSquare * plugin = (SyncSquare *) instance;

/* Frequency (LADSPA_Data value) */
	LADSPA_Data frequency = * (plugin->frequency);

/* Gate (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * gate = plugin->gate;

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance Data */
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data srate = plugin->srate;
	LADSPA_Data nyquist = plugin->nyquist;

	unsigned long s;

	for (s = 0; s < sample_count; s++)
	{
		if (gate[s] > 0.0f)
		{
			if (phase < nyquist)
				output[s] = 1.0f;
			else
				output[s] = -1.0f;

			phase += frequency;
			if (phase < 0.0f)
				phase += srate;
			else if (phase > srate)
				phase -= srate;
		}
		else
		{
			output[s] = 0.0f;
			phase = 0.0f;
		}
	}

	plugin->phase = phase;
}

void
_init (void)
{
	static const char * labels[] = {"syncsquare_faga_oa",
	                                "syncsquare_fcga_oa"};
	static const char * names[] = {G_NOP("Clock Oscillator with Gate (FAGA)"),
	                               G_NOP("Clock Oscillator with Gate (FCGA)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor frequency_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor gate_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runSyncSquare_faga_oa,
	                                           runSyncSquare_fcga_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	syncsquare_descriptors = (LADSPA_Descriptor **) calloc (SYNCSQUARE_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (syncsquare_descriptors)
	{
		for (i = 0; i < SYNCSQUARE_VARIANT_COUNT; i++)
		{
			syncsquare_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof(LADSPA_Descriptor));
			descriptor = syncsquare_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = SYNCSQUARE_BASE_ID + i;
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
				port_descriptors[SYNCSQUARE_FREQUENCY] = frequency_port_descriptors[i];
				port_names[SYNCSQUARE_FREQUENCY] = G_("Frequency");
				port_range_hints[SYNCSQUARE_FREQUENCY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                        LADSPA_HINT_DEFAULT_LOW;
				port_range_hints[SYNCSQUARE_FREQUENCY].LowerBound = 0.0f;
				port_range_hints[SYNCSQUARE_FREQUENCY].UpperBound = 64.0f;

			/* Parameters for Reset Trigger */
				port_descriptors[SYNCSQUARE_GATE] = gate_port_descriptors[i];
				port_names[SYNCSQUARE_GATE] = G_("Gate");
				port_range_hints[SYNCSQUARE_GATE].HintDescriptor = LADSPA_HINT_TOGGLED;

			/* Parameters for Output */
				port_descriptors[SYNCSQUARE_OUTPUT] = output_port_descriptors[i];
				port_names[SYNCSQUARE_OUTPUT] = G_("Output");
				port_range_hints[SYNCSQUARE_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateSyncSquare;
				descriptor->cleanup = cleanupSyncSquare;
				descriptor->connect_port = connectPortSyncSquare;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateSyncSquare;
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

	if (syncsquare_descriptors)
	{
		for (i = 0; i < SYNCSQUARE_VARIANT_COUNT; i++)
		{
			descriptor = syncsquare_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (syncsquare_descriptors);
	}
}
