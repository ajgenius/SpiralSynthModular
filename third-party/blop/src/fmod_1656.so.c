/*
    fmod.so.c - A LADSPA plugin to modulate a frequency by a signal

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
#include "math_func.h"
#include "gettext.h"

#ifdef ENABLE_NLS
#include <locale.h>
#define G_(s) gettext(s)
#else
#define G_(s) (s)
#endif
#define G_NOP(s) s

#define FMOD_BASE_ID           1656
#define FMOD_VARIANT_COUNT     4

#define FMOD_FREQUENCY         0
#define FMOD_MODULATOR         1
#define FMOD_OUTPUT            2

LADSPA_Descriptor ** fmod_descriptors = 0;

typedef struct
{
    LADSPA_Data * frequency;
    LADSPA_Data * modulator;
    LADSPA_Data * output;
} Fmod;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < FMOD_VARIANT_COUNT)
		return fmod_descriptors[index];

	return 0;
}

void
cleanupFmod (LADSPA_Handle instance)
{
	free(instance);
}

void
connectPortFmod (LADSPA_Handle instance,
                 unsigned long port,
                 LADSPA_Data * data)
{
	Fmod * plugin = (Fmod *) instance;

	switch (port)
	{
	case FMOD_FREQUENCY:
		plugin->frequency = data;
		break;
	case FMOD_MODULATOR:
		plugin->modulator = data;
		break;
	case FMOD_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateFmod (const LADSPA_Descriptor * descriptor,
                 unsigned long sample_rate)
{
    Fmod * plugin = (Fmod *) malloc (sizeof (Fmod));

    return (LADSPA_Handle) plugin;
}

void
runFmod_fama_oa (LADSPA_Handle instance,
                 unsigned long sample_count)
{
	Fmod * plugin = (Fmod *) instance;

/* Frequency to Modulate (array of floats of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* LFO Input (array of floats of length sample_count) */
	LADSPA_Data * modulator = plugin->modulator;

/* Output Frequency (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data freq;
	LADSPA_Data mod;
	LADSPA_Data scale;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		freq = frequency[s];
		mod = modulator[s];

		scale = (LADSPA_Data) EXPF (M_LN2 * mod);

		output[s] = scale * freq;
	}
}

void
runFmod_famc_oa (LADSPA_Handle instance,
                 unsigned long sample_count)
{
	Fmod * plugin = (Fmod *) instance;

/* Frequency to Modulate (array of floats of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Shift (Octaves) (float value) */
	LADSPA_Data modulator = * (plugin->modulator);

/* Output Frequency (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data freq;
	LADSPA_Data scale = (LADSPA_Data) EXPF (M_LN2 * modulator);
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		freq = frequency[s];

		output[s] = scale * freq;
	}
}

void
runFmod_fcma_oa (LADSPA_Handle instance,
                 unsigned long sample_count)
{
	Fmod * plugin = (Fmod *) instance;

/* Frequency to Modulate (float value) */
	LADSPA_Data frequency = * (plugin->frequency);

/* LFO Input (array of floats of length sample_count) */
	LADSPA_Data * modulator = plugin->modulator;

/* Output Frequency (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data mod;
	LADSPA_Data scale;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		mod = modulator[s];

		scale = (LADSPA_Data) EXPF (M_LN2 * mod);

		output[s] = scale * frequency;
	}
}

void
runFmod_fcmc_oc (LADSPA_Handle instance,
                 unsigned long sample_count)
{
	Fmod * plugin = (Fmod *) instance;

/* Frequency to Modulate (float value) */
	LADSPA_Data frequency = * (plugin->frequency);

/* Shift (Octaves) (float value) */
	LADSPA_Data modulator = * (plugin->modulator);

/* Output Frequency (pointer to float value) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data scale;

	scale = (LADSPA_Data) EXPF (M_LN2 * modulator);

	output[0] = scale * frequency;
}

void
_init (void)
{
	static const char * labels[] = {"fmod_fama_oa",
	                                "fmod_famc_oa",
	                                "fmod_fcma_oa",
	                                "fmod_fcmc_oc"};
	static const char * names[] = {G_NOP("Frequency Modulator (FAMA)"),
	                               G_NOP("Frequency Modulator (FAMC)"),
	                               G_NOP("Frequency Modulator (FCMA)"),
	                               G_NOP("Frequency Modulator (FCMC)")};
	char ** port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;
	LADSPA_Descriptor *descriptor;
	int i;

	LADSPA_PortDescriptor frequency_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor modulator_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runFmod_fama_oa,
	                                           runFmod_famc_oa,
	                                           runFmod_fcma_oa,
	                                           runFmod_fcmc_oc};
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	fmod_descriptors = (LADSPA_Descriptor **) calloc (FMOD_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (fmod_descriptors)
	{
		for (i = 0; i < FMOD_VARIANT_COUNT; i++)
		{
			fmod_descriptors[i] = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));
			descriptor = fmod_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = FMOD_BASE_ID + i;
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

			/* Parameters for Frequency to Modulate */
				port_descriptors[FMOD_FREQUENCY] = frequency_port_descriptors[i];
				port_names[FMOD_FREQUENCY] = G_("Frequency (Hz)");
				port_range_hints[FMOD_FREQUENCY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                  LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC |
				                                                  LADSPA_HINT_DEFAULT_440;
				port_range_hints[FMOD_FREQUENCY].LowerBound = 1.0f / 48000.0f;
				port_range_hints[FMOD_FREQUENCY].UpperBound = 0.5f;

			/* Parameters for LFO Input */
				port_descriptors[FMOD_MODULATOR] = modulator_port_descriptors[i];
				port_names[FMOD_MODULATOR] = G_("Modulation (Octaves)");
				port_range_hints[FMOD_MODULATOR].HintDescriptor = 0;

			/* Parameters for Output Frequency */
				port_descriptors[FMOD_OUTPUT] = output_port_descriptors[i];
				port_names[FMOD_OUTPUT] = G_("Modulated Frequency (Hz)");
				port_range_hints[FMOD_OUTPUT].HintDescriptor = 0;

				descriptor->activate = NULL;
				descriptor->cleanup = cleanupFmod;
				descriptor->connect_port = connectPortFmod;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateFmod;
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

	if (fmod_descriptors)
	{
		for (i = 0; i < FMOD_VARIANT_COUNT; i++)
		{
			descriptor = fmod_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (fmod_descriptors);
	}
}
