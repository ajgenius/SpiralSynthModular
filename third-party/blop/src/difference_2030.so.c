/*
    difference.so.c - A LADSPA plugin to calculate the difference of
                      two signals

    Copyright (C) 2004  Mike Rawes

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

#define DIFFERENCE_BASE_ID           2030
#define DIFFERENCE_VARIANT_COUNT     4

#define DIFFERENCE_INPUT             0
#define DIFFERENCE_MINUS             1
#define DIFFERENCE_OUTPUT            2

LADSPA_Descriptor ** difference_descriptors = 0;

typedef struct
{
	LADSPA_Data * input;
	LADSPA_Data * minus;
	LADSPA_Data * output;
} Difference;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < DIFFERENCE_VARIANT_COUNT)
		return difference_descriptors[index];

	return 0;
}

void
cleanupDifference (LADSPA_Handle instance)
{
	free (instance);
}

void
connectPortDifference (LADSPA_Handle instance,
                       unsigned long port,
                       LADSPA_Data *data)
{
	Difference * plugin = (Difference *) instance;

	switch (port)
	{
	case DIFFERENCE_INPUT:
		plugin->input = data;
		break;
	case DIFFERENCE_MINUS:
		plugin->minus = data;
		break;
	case DIFFERENCE_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateDifference (const LADSPA_Descriptor * descriptor,
                       unsigned long sample_rate)
{
	Difference * plugin = (Difference *) malloc (sizeof (Difference));

	return (LADSPA_Handle) plugin;
}

void
runDifference_iama_oa (LADSPA_Handle instance,
                       unsigned long sample_count)
{
	Difference * plugin = (Difference *) instance;

/* Input (array of floats of length sample_count) */
	LADSPA_Data * input = plugin->input;

/* Input to Subtract (array of floats of length sample_count) */
	LADSPA_Data * minus = plugin->minus;

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data in;
	LADSPA_Data mi;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		in = input[s];
		mi = minus[s];

		output[s] = in - mi;
	}
}

void
runDifference_iamc_oa (LADSPA_Handle instance,
                       unsigned long sample_count)
{
	Difference * plugin = (Difference *) instance;

/* Input (array of floats of length sample_count) */
	LADSPA_Data * input = plugin->input;

/* Input to Subtract (float value) */
	LADSPA_Data minus = * (plugin->minus);

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data in;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		in = input[s];

		output[s] = in - minus;
	}
}

void
runDifference_icma_oa (LADSPA_Handle instance,
                       unsigned long sample_count)
{
	Difference * plugin = (Difference *) instance;

/* Input (float value) */
	LADSPA_Data input = * (plugin->input);

/* Input to Subtract (array of floats of length sample_count) */
	LADSPA_Data * minus = plugin->minus;

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data mi;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		mi = minus[s];

		output[s] = input - mi;
	}
}

void
runDifference_icmc_oc (LADSPA_Handle instance,
                       unsigned long sample_count)
{
	Difference *plugin = (Difference *)instance;

/* Input (float value) */
	LADSPA_Data input = * (plugin->input);

/* Input to Subtract (float value) */
	LADSPA_Data minus = * (plugin->minus);

/* Output Frequency (pointer to float value) */
	LADSPA_Data * output = plugin->output;

	output[0] = input - minus;
}

void
_init (void)
{
	static const char * labels[] = {"difference_iama_oa",
	                                "difference_iamc_oa",
	                                "difference_icma_oa",
	                                "difference_icmc_oc"};
	static const char * names[] = {G_NOP("Signal Difference (IAMA)"),
	                               G_NOP("Signal Difference (IAMC)"),
	                               G_NOP("Signal Difference (ICMA)"),
	                               G_NOP("Signal Difference (ICMC)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor input_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor minus_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runDifference_iama_oa,
	                                           runDifference_iamc_oa,
	                                           runDifference_icma_oa,
	                                           runDifference_icmc_oc};
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	difference_descriptors = (LADSPA_Descriptor **) calloc (DIFFERENCE_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (difference_descriptors)
	{
		for (i = 0; i < DIFFERENCE_VARIANT_COUNT; i++)
		{
			difference_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = difference_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = DIFFERENCE_BASE_ID + i;
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

			/* Parameters for Input */
				port_descriptors[DIFFERENCE_INPUT] = input_port_descriptors[i];
				port_names[DIFFERENCE_INPUT] = G_("Input");
				port_range_hints[DIFFERENCE_INPUT].HintDescriptor = 0;

			/* Parameters for Input to Subtract */
				port_descriptors[DIFFERENCE_MINUS] = minus_port_descriptors[i];
				port_names[DIFFERENCE_MINUS] = G_("Input to Subtract");
				port_range_hints[DIFFERENCE_MINUS].HintDescriptor = 0;

			/* Parameters for Output */
				port_descriptors[DIFFERENCE_OUTPUT] = output_port_descriptors[i];
				port_names[DIFFERENCE_OUTPUT] = G_("Difference Output");
				port_range_hints[DIFFERENCE_OUTPUT].HintDescriptor = 0;

				descriptor->activate = NULL;
				descriptor->cleanup = cleanupDifference;
				descriptor->connect_port = connectPortDifference;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateDifference;
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

	if (difference_descriptors)
	{
		for (i = 0; i < DIFFERENCE_VARIANT_COUNT; i++)
		{
			descriptor = difference_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (difference_descriptors);
	}
}
