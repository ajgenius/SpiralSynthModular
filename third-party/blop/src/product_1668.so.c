/*
    product.so.c - A LADSPA plugin to calculate the product of two signals

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

#define PRODUCT_BASE_ID           1668
#define PRODUCT_VARIANT_COUNT     3

#define PRODUCT_INPUT1            0
#define PRODUCT_INPUT2            1
#define PRODUCT_OUTPUT            2

LADSPA_Descriptor ** product_descriptors = 0;

typedef struct
{
	LADSPA_Data * input1;
	LADSPA_Data * input2;
	LADSPA_Data * output;
} Product;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < PRODUCT_VARIANT_COUNT)
		return product_descriptors[index];

	return 0;
}

void
cleanupProduct (LADSPA_Handle instance)
{
	free (instance);
}

void
connectPortProduct (LADSPA_Handle instance,
                    unsigned long port,
                    LADSPA_Data * data)
{
	Product * plugin = (Product *) instance;

	switch (port)
	{
	case PRODUCT_INPUT1:
		plugin->input1 = data;
		break;
	case PRODUCT_INPUT2:
		plugin->input2 = data;
		break;
	case PRODUCT_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateProduct (const LADSPA_Descriptor * descriptor,
                    unsigned long sample_rate)
{
	Product * plugin = (Product *) malloc (sizeof (Product));

	return (LADSPA_Handle) plugin;
}

void
runProduct_iaia_oa (LADSPA_Handle instance,
                    unsigned long sample_count)
{
	Product * plugin = (Product *) instance;

/* First Input (array of floats of length sample_count) */
	LADSPA_Data * input1 = plugin->input1;

/* Second Input (array of floats of length sample_count) */
	LADSPA_Data * input2 = plugin->input2;

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data in1;
	LADSPA_Data in2;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		in1 = input1[s];
		in2 = input2[s];

		output[s] = in1 * in2;
	}
}

void
runProduct_iaic_oa (LADSPA_Handle instance,
                    unsigned long sample_count)
{
	Product * plugin = (Product *) instance;

/* First Input (array of floats of length sample_count) */
	LADSPA_Data *input1 = plugin->input1;

/* Second Input (float value) */
	LADSPA_Data input2 = * (plugin->input2);

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data in1;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		in1 = input1[s];

		output[s] = in1 * input2;
	}
}

void
runProduct_icic_oc (LADSPA_Handle instance,
                    unsigned long sample_count)
{
	Product * plugin = (Product *) instance;

/* First Input (float value) */
	LADSPA_Data input1 = * (plugin->input1);

/* Second Input (float value) */
	LADSPA_Data input2 = * (plugin->input2);

/* Output (pointer to float value) */
	LADSPA_Data * output = plugin->output;

	output[0] = input1 * input2;
}

void
_init (void)
{
	static const char * labels[] = {"product_iaia_oa",
	                                "product_iaic_oa",
	                                "product_icic_oc"};
	static const char * names[] = {G_NOP("Signal Product (IAIA)"),
	                               G_NOP("Signal Product (IAIC)"),
	                               G_NOP("Signal Product (ICIC)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor input1_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor input2_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runProduct_iaia_oa,
	                                           runProduct_iaic_oa,
	                                           runProduct_icic_oc};
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	product_descriptors = (LADSPA_Descriptor **) calloc (PRODUCT_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (product_descriptors)
	{
		for (i = 0; i < PRODUCT_VARIANT_COUNT; i++)
		{
			product_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = product_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = PRODUCT_BASE_ID + i;
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

			/* Parameters for First Input */
				port_descriptors[PRODUCT_INPUT1] = input1_port_descriptors[i];
				port_names[PRODUCT_INPUT1] = G_("First Input");
				port_range_hints[PRODUCT_INPUT1].HintDescriptor = 0;

			/* Parameters for Second Input */
				port_descriptors[PRODUCT_INPUT2] = input2_port_descriptors[i];
				port_names[PRODUCT_INPUT2] = G_("Second Input");
				port_range_hints[PRODUCT_INPUT2].HintDescriptor = 0;

			/* Parameters for Output */
				port_descriptors[PRODUCT_OUTPUT] = output_port_descriptors[i];
				port_names[PRODUCT_OUTPUT] = G_("Product Output");
				port_range_hints[PRODUCT_OUTPUT].HintDescriptor = 0;

				descriptor->activate = NULL;
				descriptor->cleanup = cleanupProduct;
				descriptor->connect_port = connectPortProduct;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateProduct;
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

	if (product_descriptors)
	{
		for (i = 0; i < PRODUCT_VARIANT_COUNT; i++)
		{
			descriptor = product_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (product_descriptors);
	}
}
