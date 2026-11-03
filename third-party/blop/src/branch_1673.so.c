/*
    branch.so.c - A LADSPA plugin to split a signal into two

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

#define BRANCH_BASE_ID           1673
#define BRANCH_VARIANT_COUNT     2

#define BRANCH_INPUT             0
#define BRANCH_OUTPUT1           1
#define BRANCH_OUTPUT2           2

LADSPA_Descriptor **sum_descriptors = 0;

typedef struct
{
	LADSPA_Data *input;
	LADSPA_Data *output1;
	LADSPA_Data *output2;
} Branch;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < BRANCH_VARIANT_COUNT)
		return sum_descriptors[index];
	else
		return 0;
}

void
cleanupBranch (LADSPA_Handle instance)
{
	free(instance);
}

void
connectPortBranch (LADSPA_Handle instance,
                   unsigned long port,
                   LADSPA_Data * data)
{
	Branch * plugin = (Branch *) instance;

	switch (port)
	{
	case BRANCH_INPUT:
		plugin->input = data;
		break;
	case BRANCH_OUTPUT1:
		plugin->output1 = data;
		break;
	case BRANCH_OUTPUT2:
		plugin->output2 = data;
		break;
	}
}

LADSPA_Handle
instantiateBranch (const LADSPA_Descriptor * descriptor,
                   unsigned long sample_rate)
{
	Branch * plugin = (Branch *) malloc (sizeof (Branch));

	return (LADSPA_Handle) plugin;
}

void
runBranch_ia_oaoa (LADSPA_Handle instance,
                   unsigned long sample_count)
{
	Branch * plugin = (Branch *) instance;

/* Input (array of floats of length sample_count) */
	LADSPA_Data * input = plugin->input;

/* First Output (array of floats of length sample_count) */
	LADSPA_Data * output1 = plugin->output1;

/* Second Output (array of floats of length sample_count) */
	LADSPA_Data * output2 = plugin->output2;

	LADSPA_Data in;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		in = input[s];

		output1[s] = in;
		output2[s] = in;
	}
}

void
runBranch_ic_ococ (LADSPA_Handle instance,
                   unsigned long sample_count)
{
	Branch * plugin = (Branch *) instance;

/* Input (float value) */
	LADSPA_Data input = * (plugin->input);

/* First Output (pointer to float value) */
	LADSPA_Data * output1 = plugin->output1;

/* Second Output (pointer to float value) */
	LADSPA_Data * output2 = plugin->output2;

	output1[0] = input;
	output2[0] = input;
}

void _init()
{
	static const char * labels[] = {"branch_ia_oaoa",
	                                "branch_ic_ococ"};
	static const char * names[] = {G_NOP("Signal Branch (IA)"),
	                               G_NOP("Signal Branch (IC)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor input_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output1_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                    LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output2_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                    LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runBranch_ia_oaoa,
	                                           runBranch_ic_ococ};

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	sum_descriptors = (LADSPA_Descriptor **) calloc (BRANCH_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (sum_descriptors)
	{
		for (i = 0; i < BRANCH_VARIANT_COUNT; i++)
		{
			sum_descriptors[i] = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));
			descriptor = sum_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = BRANCH_BASE_ID + i;
				descriptor->Label = labels[i];
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = G_(names[i]);
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = 3;

				port_descriptors = (LADSPA_PortDescriptor *) calloc (3, sizeof (LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *) port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (3, sizeof(LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (3, sizeof (char*));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Input */
				port_descriptors[BRANCH_INPUT] = input_port_descriptors[i];
				port_names[BRANCH_INPUT] = G_("Input");
				port_range_hints[BRANCH_INPUT].HintDescriptor = 0;

			/* Parameters for First Output */
				port_descriptors[BRANCH_OUTPUT1] = output1_port_descriptors[i];
				port_names[BRANCH_OUTPUT1] = G_("First Output");
				port_range_hints[BRANCH_OUTPUT1].HintDescriptor = 0;

			/* Parameters for Second Output */
				port_descriptors[BRANCH_OUTPUT2] = output2_port_descriptors[i];
				port_names[BRANCH_OUTPUT2] = G_("Second Output");
				port_range_hints[BRANCH_OUTPUT2].HintDescriptor = 0;

				descriptor->activate = NULL;
				descriptor->cleanup = cleanupBranch;
				descriptor->connect_port = connectPortBranch;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateBranch;
				descriptor->run = run_functions[i];
				descriptor->run_adding = NULL;
				descriptor->set_run_adding_gain = NULL;
			}
		}
	}
}

void
_fini ()
{
	LADSPA_Descriptor * descriptor;
	int i;

	if (sum_descriptors)
	{
		for (i = 0; i < BRANCH_VARIANT_COUNT; i++)
		{
			descriptor = sum_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (sum_descriptors);
	}
}
