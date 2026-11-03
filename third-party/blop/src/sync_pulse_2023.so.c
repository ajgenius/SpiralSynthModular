/*
    syncpulse.so.c - A LADSPA plugin to generate a non-bandlimited
                     variable-pulse waveform with gate for trigger and sync

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

#define SYNCPULSE_BASE_ID                  2023
#define SYNCPULSE_VARIANT_COUNT            2

#define SYNCPULSE_FREQUENCY                0
#define SYNCPULSE_PULSEWIDTH               1
#define SYNCPULSE_GATE                     2
#define SYNCPULSE_OUTPUT                   3

LADSPA_Descriptor ** syncpulse_descriptors = 0;

typedef struct
{
	LADSPA_Data * frequency;
	LADSPA_Data * pulsewidth;
	LADSPA_Data * gate;
	LADSPA_Data * output;
	LADSPA_Data   srate;
	LADSPA_Data   phase;
} SyncPulse;

/*****************************************************************************
 *
 * LADSPA Plugin code
 *
 *****************************************************************************/

const
LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < SYNCPULSE_VARIANT_COUNT)
		return syncpulse_descriptors[index];

	return 0;
}

void
cleanupSyncPulse (LADSPA_Handle instance)
{
    free (instance);
}

void
connectPortSyncPulse (LADSPA_Handle instance,
                      unsigned long port,
                      LADSPA_Data * data)
{
	SyncPulse * plugin = (SyncPulse *) instance;

	switch (port)
	{
	case SYNCPULSE_FREQUENCY:
		plugin->frequency = data;
		break;
	case SYNCPULSE_PULSEWIDTH:
		plugin->pulsewidth = data;
		break;
	case SYNCPULSE_GATE:
		plugin->gate = data;
		break;
	case SYNCPULSE_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateSyncPulse (const LADSPA_Descriptor * descriptor,
                      unsigned long sample_rate)
{
	SyncPulse * plugin = (SyncPulse *) malloc (sizeof (SyncPulse));

	plugin->srate = (LADSPA_Data) sample_rate;

	return (LADSPA_Handle) plugin;
}

void
activateSyncPulse (LADSPA_Handle instance)
{
	SyncPulse * plugin = (SyncPulse *) instance;

/* Reset phase */
	plugin->phase = 0.0f;
}

void
runSyncPulse_fapaga_oa (LADSPA_Handle instance,
                        unsigned long sample_count)
{
	SyncPulse * plugin = (SyncPulse *) instance;

/* Frequency (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Pulse Width (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * pulsewidth = plugin->pulsewidth;

/* Gate (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * gate = plugin->gate;

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data srate = plugin->srate;

	LADSPA_Data freq;
	LADSPA_Data pwidth;
	unsigned long s;

	for (s = 0; s < sample_count; s++)
	{
		if (gate[s] > 0.0f)
		{
			freq = frequency[s];
			pwidth = f_clip (pulsewidth[s], 0.0f, 1.0f) * srate;

			if (phase < pwidth)
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
runSyncPulse_fcpcga_oa (LADSPA_Handle instance,
                        unsigned long sample_count)
{
	SyncPulse * plugin = (SyncPulse *) instance;

/* Frequency (LADSPA_Data value) */
	LADSPA_Data frequency = * (plugin->frequency);

/* Pulse Width (LADSPA_Data value) */
	LADSPA_Data pulsewidth = f_clip (* (plugin->pulsewidth), 0.0f, 1.0f);

/* Gate (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * gate = plugin->gate;

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance Data */
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data srate = plugin->srate;

	unsigned long s;

	pulsewidth *= srate;

	for (s = 0; s < sample_count; s++)
	{
		if (gate[s] > 0.0f)
		{
			if (phase < pulsewidth)
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
	static const char * labels[] = {"syncpulse_fapaga_oa",
	                                "syncpulse_fcpcga_oa"};
	static const char * names[] = {G_NOP("Clock Pulse Oscillator with Gate (FAPAGA)"),
	                               G_NOP("Clock Pulse Oscillator with Gate (FCPCGA)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor frequency_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor pulsewidth_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                       LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor gate_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runSyncPulse_fapaga_oa,
	                                           runSyncPulse_fcpcga_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	syncpulse_descriptors = (LADSPA_Descriptor **) calloc (SYNCPULSE_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (syncpulse_descriptors) {
		for (i = 0; i < SYNCPULSE_VARIANT_COUNT; i++)
		{
			syncpulse_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = syncpulse_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = SYNCPULSE_BASE_ID + i;
				descriptor->Label = labels[i];
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = G_(names[i]);
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = 4;

				port_descriptors = (LADSPA_PortDescriptor *) calloc (4, sizeof (LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *) port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (4, sizeof (LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (4, sizeof (char*));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Frequency */
				port_descriptors[SYNCPULSE_FREQUENCY] = frequency_port_descriptors[i];
				port_names[SYNCPULSE_FREQUENCY] = G_("Frequency");
				port_range_hints[SYNCPULSE_FREQUENCY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                       LADSPA_HINT_DEFAULT_LOW;
				port_range_hints[SYNCPULSE_FREQUENCY].LowerBound = 0.0f;
				port_range_hints[SYNCPULSE_FREQUENCY].UpperBound = 64.0f;

			/* Parameters for Pulse Width */
				port_descriptors[SYNCPULSE_PULSEWIDTH] = pulsewidth_port_descriptors[i];
				port_names[SYNCPULSE_PULSEWIDTH] = G_("Pulse Width");
				port_range_hints[SYNCPULSE_PULSEWIDTH].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                        LADSPA_HINT_DEFAULT_MIDDLE;
				port_range_hints[SYNCPULSE_PULSEWIDTH].LowerBound = 0.0f;
				port_range_hints[SYNCPULSE_PULSEWIDTH].UpperBound = 1.0f;

			/* Parameters for Reset Trigger */
				port_descriptors[SYNCPULSE_GATE] = gate_port_descriptors[i];
				port_names[SYNCPULSE_GATE] = G_("Gate");
				port_range_hints[SYNCPULSE_GATE].HintDescriptor = LADSPA_HINT_TOGGLED;

			/* Parameters for Output */
				port_descriptors[SYNCPULSE_OUTPUT] = output_port_descriptors[i];
				port_names[SYNCPULSE_OUTPUT] = G_("Output");
				port_range_hints[SYNCPULSE_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateSyncPulse;
				descriptor->cleanup = cleanupSyncPulse;
				descriptor->connect_port = connectPortSyncPulse;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateSyncPulse;
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

	if (syncpulse_descriptors)
	{
		for (i = 0; i < SYNCPULSE_VARIANT_COUNT; i++)
		{
			descriptor = syncpulse_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (syncpulse_descriptors);
	}
}
