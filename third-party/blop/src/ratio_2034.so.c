/*
    ratio.so.c - A LADSPA plugin to calculate the ratio of
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
#include "math_func.h"
#include "common.h"
#include "gettext.h"

#ifdef ENABLE_NLS
#include <locale.h>
#define G_(s) gettext(s)
#else
#define G_(s) (s)
#endif
#define G_NOP(s) s

#define RATIO_BASE_ID           2034
#define RATIO_VARIANT_COUNT     4

#define RATIO_NUMERATOR         0
#define RATIO_DENOMINATOR       1
#define RATIO_OUTPUT            2

LADSPA_Descriptor ** ratio_descriptors = 0;

typedef struct
{
	LADSPA_Data * numerator;
	LADSPA_Data * denominator;
	LADSPA_Data * output;
} Ratio;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < RATIO_VARIANT_COUNT)
		return ratio_descriptors[index];

	return 0;
}

void
cleanupRatio (LADSPA_Handle instance)
{
	free (instance);
}

void
connectPortRatio (LADSPA_Handle instance,
                  unsigned long port,
                  LADSPA_Data *data)
{
	Ratio * plugin = (Ratio *) instance;

	switch (port)
	{
	case RATIO_NUMERATOR:
		plugin->numerator = data;
		break;
	case RATIO_DENOMINATOR:
		plugin->denominator = data;
		break;
	case RATIO_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateRatio (const LADSPA_Descriptor * descriptor,
                  unsigned long sample_rate)
{
	Ratio * plugin = (Ratio *) malloc (sizeof (Ratio));

	return (LADSPA_Handle) plugin;
}

void
runRatio_nada_oa (LADSPA_Handle instance,
                  unsigned long sample_count)
{
	Ratio * plugin = (Ratio *) instance;

/* Numerator (array of floats of length sample_count) */
	LADSPA_Data * numerator = plugin->numerator;

/* Denominator (array of floats of length sample_count) */
	LADSPA_Data * denominator = plugin->denominator;

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data n;
	LADSPA_Data d;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		n = numerator[s];
		d = denominator[s];

		d = COPYSIGNF (f_max (FABSF (d), 1e-16f), d);

		output[s] = n / d;
	}
}

void
runRatio_nadc_oa (LADSPA_Handle instance,
                  unsigned long sample_count)
{
	Ratio * plugin = (Ratio *) instance;

/* Numerator (array of floats of length sample_count) */
	LADSPA_Data * numerator = plugin->numerator;

/* Denominator (float value) */
	LADSPA_Data denominator = * (plugin->denominator);

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data n;
	unsigned int s;

	denominator = COPYSIGNF (f_max (FABSF (denominator), 1e-16f), denominator);

	for (s = 0; s < sample_count; s++)
	{
		n = numerator[s];

		output[s] = n / denominator;
	}
}

void
runRatio_ncda_oa (LADSPA_Handle instance,
                  unsigned long sample_count)
{
	Ratio * plugin = (Ratio *) instance;

/* Numerator (float value) */
	LADSPA_Data numerator = * (plugin->numerator);

/* Denominator (array of floats of length sample_count) */
	LADSPA_Data * denominator = plugin->denominator;

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data d;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		d = denominator[s];
		d = COPYSIGNF (f_max (FABSF (d), 1e-16f), d);

		output[s] = numerator / d;
	}
}

void
runRatio_ncdc_oc (LADSPA_Handle instance,
                  unsigned long sample_count)
{
	Ratio *plugin = (Ratio *)instance;

/* Numerator (float value) */
	LADSPA_Data numerator = * (plugin->numerator);

/* Denominator (float value) */
	LADSPA_Data denominator = * (plugin->denominator);

/* Output Frequency (pointer to float value) */
	LADSPA_Data * output = plugin->output;

	denominator = COPYSIGNF (f_max (FABSF (denominator), 1e-16f), denominator);

	output[0] = numerator / denominator;
}

void
_init (void)
{
	static const char * labels[] = {"ratio_nada_oa",
	                                "ratio_nadc_oa",
	                                "ratio_ncda_oa",
	                                "ratio_ncdc_oc"};
	static const char * names[] = {G_NOP("Signal Ratio (NADA)"),
	                               G_NOP("Signal Ratio (NADC)"),
	                               G_NOP("Signal Ratio (NCDA)"),
	                               G_NOP("Signal Ratio (NCDC)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor numerator_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor denominator_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                        LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                        LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                        LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runRatio_nada_oa,
	                                           runRatio_nadc_oa,
	                                           runRatio_ncda_oa,
	                                           runRatio_ncdc_oc};
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	ratio_descriptors = (LADSPA_Descriptor **) calloc (RATIO_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (ratio_descriptors)
	{
		for (i = 0; i < RATIO_VARIANT_COUNT; i++)
		{
			ratio_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = ratio_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = RATIO_BASE_ID + i;
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

			/* Parameters for Numerator */
				port_descriptors[RATIO_NUMERATOR] = numerator_port_descriptors[i];
				port_names[RATIO_NUMERATOR] = G_("Numerator");
				port_range_hints[RATIO_NUMERATOR].HintDescriptor = 0;

			/* Parameters for Denominator */
				port_descriptors[RATIO_DENOMINATOR] = denominator_port_descriptors[i];
				port_names[RATIO_DENOMINATOR] = G_("Denominator");
				port_range_hints[RATIO_DENOMINATOR].HintDescriptor = 0;

			/* Parameters for Output */
				port_descriptors[RATIO_OUTPUT] = output_port_descriptors[i];
				port_names[RATIO_OUTPUT] = G_("Ratio Output");
				port_range_hints[RATIO_OUTPUT].HintDescriptor = 0;

				descriptor->activate = NULL;
				descriptor->cleanup = cleanupRatio;
				descriptor->connect_port = connectPortRatio;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateRatio;
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

	if (ratio_descriptors)
	{
		for (i = 0; i < RATIO_VARIANT_COUNT; i++)
		{
			descriptor = ratio_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (ratio_descriptors);
	}
}
