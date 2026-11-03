/*
    amp.so.c - A LADSPA plugin representing a simple mono amplifier

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

#define AMP_BASE_ID           1654
#define AMP_VARIANT_COUNT     2

#define AMP_GAIN              0
#define AMP_INPUT             1
#define AMP_OUTPUT            2

LADSPA_Descriptor ** amp_descriptors = 0;

typedef struct
{
	LADSPA_Data *gain;
	LADSPA_Data *input;
	LADSPA_Data *output;
} Amp;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
    if (index < AMP_VARIANT_COUNT)
        return amp_descriptors[index];
    else
        return 0;
}

void
cleanupAmp (LADSPA_Handle instance)
{
    free (instance);
}

void
connectPortAmp (LADSPA_Handle instance,
                unsigned long port,
                LADSPA_Data * data)
{
	Amp * plugin = (Amp *) instance;

	switch (port)
	{
	case AMP_GAIN:
		plugin->gain = data;
		break;
	case AMP_INPUT:
		plugin->input = data;
		break;
	case AMP_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateAmp (const LADSPA_Descriptor * descriptor,
                unsigned long sample_rate)
{
	Amp * plugin = (Amp *) malloc (sizeof (Amp));

	return (LADSPA_Handle) plugin;
}

void
runAmp_gaia_oa (LADSPA_Handle instance,
                unsigned long sample_count)
{
	Amp * plugin = (Amp *) instance;

/* Gain (dB) */
	LADSPA_Data *gain = plugin->gain;

/* Input */
	LADSPA_Data *input = plugin->input;

/* Output */
	LADSPA_Data *output = plugin->output;

	LADSPA_Data gn;
	LADSPA_Data in;
	LADSPA_Data scale;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		gn = gain[s];
		in = input[s];

		scale = (LADSPA_Data) EXPF (M_LN10 * gn * 0.05f);

		output[s] = scale * in;
	}
}

void
runAmp_gcia_oa (LADSPA_Handle instance,
                unsigned long sample_count)
{
	Amp * plugin = (Amp *) instance;

/* Gain (dB) */
	LADSPA_Data gain = * (plugin->gain);

/* Input */
	LADSPA_Data *input = plugin->input;

/* Output */
	LADSPA_Data *output = plugin->output;

	LADSPA_Data in;
	LADSPA_Data scale = (LADSPA_Data) EXPF (M_LN10 * gain * 0.05f);
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		in = input[s];

		output[s] = scale * in;
	}
}

void
_init ()
{
	static const char * labels[] = {"amp_gaia_oa",
	                                "amp_gcia_oa"};
	static const char * names[] = {G_NOP("Amplifier (GAIA)"),
	                               G_NOP("Amplifier (GCIA)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor gain_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor input_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runAmp_gaia_oa,
	                                           runAmp_gcia_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	amp_descriptors = (LADSPA_Descriptor **) calloc (AMP_VARIANT_COUNT, sizeof(LADSPA_Descriptor));

	if (amp_descriptors)
	{
		for (i = 0; i < AMP_VARIANT_COUNT; i++)
		{
			amp_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = amp_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = AMP_BASE_ID + i;
				descriptor->Label = labels[i];
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = G_(names[i]);
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = 3;

				port_descriptors = (LADSPA_PortDescriptor *) calloc (3, sizeof (LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (3, sizeof (LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (3, sizeof (char*));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Gain */
				port_descriptors[AMP_GAIN] = gain_port_descriptors[i];
				port_names[AMP_GAIN] = G_("Gain (dB)");
				port_range_hints[AMP_GAIN].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
				port_range_hints[AMP_GAIN].LowerBound = -96.0f;
				port_range_hints[AMP_GAIN].UpperBound = +96.0f;

			/* Parameters for Input */
				port_descriptors[AMP_INPUT] = input_port_descriptors[i];
				port_names[AMP_INPUT] = G_("Input");
				port_range_hints[AMP_INPUT].HintDescriptor = 0;

			/* Parameters for Output Frequency */
				port_descriptors[AMP_OUTPUT] = output_port_descriptors[i];
				port_names[AMP_OUTPUT] = G_("Output");
				port_range_hints[AMP_OUTPUT].HintDescriptor = 0;

				descriptor->activate = NULL;
				descriptor->cleanup = cleanupAmp;
				descriptor->connect_port = connectPortAmp;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateAmp;
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

	if (amp_descriptors)
	{
		for (i = 0; i < AMP_VARIANT_COUNT; i++)
		{
			descriptor = amp_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (amp_descriptors);
	}
}
