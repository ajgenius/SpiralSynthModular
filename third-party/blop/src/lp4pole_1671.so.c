/*
    lp4pole.so.c - A LADSPA plugin simulating a 4 pole low pass resonant
                   filter.

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
#include "lp4pole_filter.h"
#include "common.h"
#include "gettext.h"

#ifdef ENABLE_NLS
#include <locale.h>
#define G_(s) gettext(s)
#else
#define G_(s) (s)
#endif
#define G_NOP(s) s

/*
 * Only two variants - there's not much ground gained by having
 * Cutoff/Resonance with Audio/Control ports
 */

#define LP4POLE_BASE_ID                1671
#define LP4POLE_VARIANT_COUNT          2

#define LP4POLE_CUTOFF                 0
#define LP4POLE_RESONANCE              1
#define LP4POLE_INPUT                  2
#define LP4POLE_OUTPUT                 3

LADSPA_Descriptor ** lp4pole_descriptors = 0;

typedef struct
{
	LADSPA_Data   * cutoff;
	LADSPA_Data   * resonance;
	LADSPA_Data   * input;
	LADSPA_Data   * output;
	LP4PoleFilter * lpf;
} Lp4pole;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < LP4POLE_VARIANT_COUNT)
		return lp4pole_descriptors[index];
	else
		return 0;
}

void
cleanupLp4pole (LADSPA_Handle instance)
{
	Lp4pole * plugin = (Lp4pole *) instance;

	lp4pole_cleanup (plugin->lpf);

	free (instance);
}

void
connectPortLp4pole (LADSPA_Handle instance,
                    unsigned long port,
                    LADSPA_Data * data)
{
	Lp4pole * plugin = (Lp4pole *) instance;

	switch (port)
	{
	case LP4POLE_CUTOFF:
		plugin->cutoff = data;
		break;
	case LP4POLE_RESONANCE:
		plugin->resonance = data;
		break;
	case LP4POLE_INPUT:
		plugin->input = data;
		break;
	case LP4POLE_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateLp4pole (const LADSPA_Descriptor * descriptor,
                    unsigned long sample_rate)
{
	Lp4pole * plugin = (Lp4pole *) malloc (sizeof (Lp4pole));

	if (plugin)
	{
		plugin->lpf = lp4pole_new (sample_rate);
		if (!plugin->lpf)
		{
			free(plugin);
			plugin = 0;
		}
	}
	return (LADSPA_Handle) plugin;
}

void
activateLp4pole (LADSPA_Handle instance)
{
	Lp4pole * plugin = (Lp4pole *) instance;

	lp4pole_init (plugin->lpf);
}

void
runLp4pole_faraia_oa (LADSPA_Handle instance,
                      unsigned long sample_count)
{
	Lp4pole * plugin = (Lp4pole *) instance;

/* Cutoff Frequency (array of floats of length sample_count) */
	LADSPA_Data * cutoff = plugin->cutoff;

/* Resonance (array of floats of length sample_count) */
	LADSPA_Data * resonance = plugin->resonance;

/* Input (array of floats of length sample_count) */
	LADSPA_Data * input = plugin->input;

/* Output (pointer to float value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	LP4PoleFilter * lpf = plugin->lpf;

	LADSPA_Data in;
	LADSPA_Data co;
	LADSPA_Data res;
	unsigned long s;

	for (s = 0; s < sample_count; s++)
	{
		co = cutoff[s];
		res = resonance[s];
		in = input[s];

		lp4pole_set_params (lpf, co, res);

		output[s] = lp4pole_run (lpf, in);
	}
}

void
runLp4pole_fcrcia_oa (LADSPA_Handle instance,
                      unsigned long sample_count)
{
	Lp4pole * plugin = (Lp4pole *) instance;

/* Cutoff Frequency (float value) */
	LADSPA_Data cutoff = * (plugin->cutoff);

/* Resonance (float value) */
	LADSPA_Data resonance = * (plugin->resonance);

/* Input (array of floats of length sample_count) */
	LADSPA_Data * input = plugin->input;

/* Output (pointer to float value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	LP4PoleFilter *lpf = plugin->lpf;

	LADSPA_Data in;
	unsigned long s;

	lp4pole_set_params (lpf, cutoff, resonance);

	for (s = 0; s < sample_count; s++)
	{
		in = input[s];
		output[s] = lp4pole_run (lpf, in);
	}
}

void
_init (void)
{
	static const char * labels[] = {"lp4pole_faraia_oa",
	                                "lp4pole_fcrcia_oa"};
	static const char * names[] = {G_NOP("4 Pole Low-Pass Filter with Resonance (FARAIA)"),
	                               G_NOP("4 Pole Low-Pass Filter with Resonance (FCRCIA)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor *descriptor;
	int i;

	LADSPA_PortDescriptor cutoff_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor resonance_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor input_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runLp4pole_faraia_oa,
	                                           runLp4pole_fcrcia_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	lp4pole_descriptors = (LADSPA_Descriptor **) calloc (LP4POLE_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (lp4pole_descriptors)
	{
		for (i = 0; i < LP4POLE_VARIANT_COUNT; i++)
		{
			lp4pole_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = lp4pole_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = LP4POLE_BASE_ID + i;
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

				port_names = (char **) calloc (4, sizeof(char*));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Cutoff Frequency */
				port_descriptors[LP4POLE_CUTOFF] = cutoff_port_descriptors[i];
				port_names[LP4POLE_CUTOFF] = G_("Cutoff Frequency");
				port_range_hints[LP4POLE_CUTOFF].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC |
				                                                LADSPA_HINT_DEFAULT_MAXIMUM;
				port_range_hints[LP4POLE_CUTOFF].LowerBound = 1.0f / 48000.0f;
				port_range_hints[LP4POLE_CUTOFF].UpperBound = 0.5f;

			/* Parameters for Resonance */
				port_descriptors[LP4POLE_RESONANCE] = resonance_port_descriptors[i];
				port_names[LP4POLE_RESONANCE] = G_("Resonance");
				port_range_hints[LP4POLE_RESONANCE].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
																	LADSPA_HINT_DEFAULT_MINIMUM;
				port_range_hints[LP4POLE_RESONANCE].LowerBound = 0.0f;
				port_range_hints[LP4POLE_RESONANCE].UpperBound = 4.0f;

			/* Parameters for Input */
				port_descriptors[LP4POLE_INPUT] = input_port_descriptors[i];
				port_names[LP4POLE_INPUT] = G_("Input");
				port_range_hints[LP4POLE_INPUT].HintDescriptor = 0;

			/* Parameters for Output */
				port_descriptors[LP4POLE_OUTPUT] = output_port_descriptors[i];
				port_names[LP4POLE_OUTPUT] = G_("Output");
				port_range_hints[LP4POLE_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateLp4pole;
				descriptor->cleanup = cleanupLp4pole;
				descriptor->connect_port = connectPortLp4pole;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateLp4pole;
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

	if (lp4pole_descriptors)
	{
		for (i = 0; i < LP4POLE_VARIANT_COUNT; i++)
		{
			descriptor = lp4pole_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (lp4pole_descriptors);
	}
}
