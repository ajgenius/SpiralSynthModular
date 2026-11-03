/*
    sawtooth.so.c - A LADSPA plugin to generate a bandlimited sawtooth waveform

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

#define SAWTOOTH_BASE_ID            1641
#define SAWTOOTH_VARIANT_COUNT      2

#define SAWTOOTH_FREQUENCY          0
#define SAWTOOTH_OUTPUT             1

LADSPA_Descriptor ** sawtooth_descriptors = 0;

typedef struct
{
	LADSPA_Data * frequency;
	LADSPA_Data * output;
	LADSPA_Data   phase;
	Wavedata      wdat;
} Sawtooth;

/*****************************************************************************
 *
 * LADSPA Plugin code
 *
 *****************************************************************************/

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < 2)
		return sawtooth_descriptors[index];

	return 0;
}

void
connectPortSawtooth (LADSPA_Handle instance,
                     unsigned long port,
                     LADSPA_Data * data)
{
	Sawtooth * plugin = (Sawtooth *) instance;

	switch (port)
	{
	case SAWTOOTH_FREQUENCY:
		plugin->frequency = data;
		break;
	case SAWTOOTH_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateSawtooth (const LADSPA_Descriptor *descriptor,
                     unsigned long sample_rate)
{
	Sawtooth * plugin = (Sawtooth *) malloc (sizeof (Sawtooth));

	if (wavedata_load (&plugin->wdat, BLOP_DLSYM_SAWTOOTH, sample_rate))
	{
		free (plugin);
		return 0;
	}

	return (LADSPA_Handle) plugin;
}

void
cleanupSawtooth (LADSPA_Handle instance)
{
	Sawtooth * plugin = (Sawtooth *) instance;

	wavedata_unload (&plugin->wdat);
	free (instance);
}

void
activateSawtooth (LADSPA_Handle instance)
{
	Sawtooth * plugin = (Sawtooth *) instance;

	plugin->phase = 0.0f;
}

void
runSawtooth_fa_oa (LADSPA_Handle instance,
                   unsigned long sample_count)
{
	Sawtooth * plugin = (Sawtooth *) instance;

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

	/* Lookup table to play */
		wavedata_get_table (wdat, freq);

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
runSawtooth_fc_oa (LADSPA_Handle instance,
                   unsigned long sample_count)
{
	Sawtooth * plugin = (Sawtooth *) instance;

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
	static const char * labels[] = {"sawtooth_fa_oa",
	                                "sawtooth_fc_oa"};
	static const char * names[] = {G_NOP("Bandlimited Sawtooth Oscillator (FA)"),
	                               G_NOP("Bandlimited Sawtooth Oscillator (FC)")};
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
	                         unsigned long) = {runSawtooth_fa_oa,
	                                           runSawtooth_fc_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	sawtooth_descriptors = (LADSPA_Descriptor **) calloc (SAWTOOTH_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (sawtooth_descriptors)
	{
		for (i = 0; i < SAWTOOTH_VARIANT_COUNT; i++)
		{
			sawtooth_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = sawtooth_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = SAWTOOTH_BASE_ID + i;
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
				port_descriptors[SAWTOOTH_FREQUENCY] = frequency_port_descriptors[i];
				port_names[SAWTOOTH_FREQUENCY] = G_("Frequency");
				port_range_hints[SAWTOOTH_FREQUENCY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                      LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC |
				                                                      LADSPA_HINT_DEFAULT_440;
				port_range_hints[SAWTOOTH_FREQUENCY].LowerBound = 1.0f / 48000.0f;
				port_range_hints[SAWTOOTH_FREQUENCY].UpperBound = 0.5f;

				/* Parameters for Output */
				port_descriptors[SAWTOOTH_OUTPUT] = output_port_descriptors[i];
				port_names[SAWTOOTH_OUTPUT] = G_("Output");
				port_range_hints[SAWTOOTH_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateSawtooth;
				descriptor->cleanup = cleanupSawtooth;
				descriptor->connect_port = connectPortSawtooth;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateSawtooth;
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

	if (sawtooth_descriptors)
	{
		for (i = 0; i < SAWTOOTH_VARIANT_COUNT; i++)
		{
			descriptor = sawtooth_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (sawtooth_descriptors);
	}
}
