/*
    interpolator.so.c - A LADSPA plugin to generate a smooth audio signal
                        from a control source

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

#define INTERPOLATOR_BASE_ID           1660
#define INTERPOLATOR_VARIANT_COUNT     1

#define INTERPOLATOR_INPUT             0
#define INTERPOLATOR_OUTPUT            1

/*****************************************************************************
 *
 * Description: Mutated spline interpolator using only two previous
 *               samples and one next.
 *
 *   Arguments: interval    Normalised time interval between inteprolated
 *                           sample and p0
 *              p1, p0      Samples prior to interpolated one
 *              n0          Sample following interpolated one
 *
 *     Returns: interpolated sample
 *
 *****************************************************************************/
static inline LADSPA_Data
interpolate (LADSPA_Data interval,
             LADSPA_Data p1,
             LADSPA_Data p0,
             LADSPA_Data n0)
{
	return p0 + 0.5f * interval * (n0 - p1 +
	                   interval * (4.0f * n0 + 2.0f * p1 - 5.0f * p0 - n0 +
	                   interval * (3.0f * (p0 - n0) - p1 + n0)));
}

LADSPA_Descriptor ** interpolator_descriptors = 0;

typedef struct
{
	LADSPA_Data * input;
	LADSPA_Data * output;
	LADSPA_Data   p1;
	LADSPA_Data   p0;
} Interpolator;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < 1)
		return interpolator_descriptors[index];

	return 0;
}

void
cleanupInterpolator (LADSPA_Handle instance)
{
	free (instance);
}

void
connectPortInterpolator (LADSPA_Handle instance,
                         unsigned long port,
                         LADSPA_Data * data)
{
	Interpolator * plugin = (Interpolator *) instance;

	switch (port)
	{
	case INTERPOLATOR_INPUT:
		plugin->input = data;
		plugin->p1 = plugin->p0 = *(plugin->input);
		break;
	case INTERPOLATOR_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateInterpolator (const LADSPA_Descriptor * descriptor,
                         unsigned long sample_rate)
{
	Interpolator *plugin = (Interpolator *) malloc (sizeof (Interpolator));

	return (LADSPA_Handle) plugin;
}

void
activateInterpolator (LADSPA_Handle instance)
{
	Interpolator * plugin = (Interpolator *) instance;

	plugin->p1 = 0.0f;
	plugin->p0 = 0.0f;
}

void
runInterpolator (LADSPA_Handle instance,
                 unsigned long sample_count)
{
	Interpolator * plugin = (Interpolator *) instance;

/* Control Input (float value) */
	LADSPA_Data input = * (plugin->input);

/* Interpolated Output (pointer to float value) */
	LADSPA_Data * output = plugin->output;

/* We use two previous values and the input as the 'next' one */
	LADSPA_Data p1 = plugin->p1;
	LADSPA_Data p0 = plugin->p0;

	LADSPA_Data interval;
	LADSPA_Data inv_scount = 1.0f / (LADSPA_Data) sample_count;
	unsigned long s;

	for (s = 0; s < sample_count; s++)
	{
		interval = (LADSPA_Data) s * inv_scount;

		output[s] = interpolate (interval, p1, p0, input);
	}

	plugin->p1 = p0;
	plugin->p0 = input;
}

void
_init (void)
{
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor input_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runInterpolator};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	interpolator_descriptors = (LADSPA_Descriptor **) calloc (INTERPOLATOR_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (interpolator_descriptors)
	{
		for (i = 0; i < INTERPOLATOR_VARIANT_COUNT; i++)
		{
			interpolator_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = interpolator_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = INTERPOLATOR_BASE_ID + i;
				descriptor->Label = "interpolator";
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = G_("Control to Audio Interpolator");
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = 2;

				port_descriptors = (LADSPA_PortDescriptor *) calloc (2, sizeof (LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *) port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (2, sizeof (LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (2, sizeof (char*));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Control Input */
				port_descriptors[INTERPOLATOR_INPUT] = input_port_descriptors[i];
				port_names[INTERPOLATOR_INPUT] = G_("Control Input");
				port_range_hints[INTERPOLATOR_INPUT].HintDescriptor = 0;

			/* Parameters for Interpolated Output */
				port_descriptors[INTERPOLATOR_OUTPUT] = output_port_descriptors[i];
				port_names[INTERPOLATOR_OUTPUT] = G_("Interpolated Output");
				port_range_hints[INTERPOLATOR_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateInterpolator;
				descriptor->cleanup = cleanupInterpolator;
				descriptor->connect_port = connectPortInterpolator;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateInterpolator;
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
	LADSPA_Descriptor *descriptor;
	int i;

	if (interpolator_descriptors)
	{
		for (i = 0; i < INTERPOLATOR_VARIANT_COUNT; i++)
		{
			descriptor = interpolator_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (interpolator_descriptors);
	}
}
