/*
    sequencer.so.c - A LADSPA plugin to simulate an analogue style step
                     sequencer.

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

#include <stdio.h>
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

/* These are defined in the compiler flags - see Makefile.am
 * This code is used to create the three sequencers with
 * 16, 32 and 64 steps
#define SEQUENCER_BASE_ID                   1677
#define SEQUENCER_MAX_INPUTS                16
 */
 
#define SEQUENCER_VARIANT_COUNT             1

#define SEQUENCER_GATE                      0
#define SEQUENCER_TRIGGER                   1
#define SEQUENCER_LOOP_POINT                2
#define SEQUENCER_RESET                     3
#define SEQUENCER_VALUE_GATE_CLOSED         4
#define SEQUENCER_VALUE_START               5
#define SEQUENCER_OUTPUT                    (SEQUENCER_MAX_INPUTS + 5)

LADSPA_Descriptor ** sequencer_descriptors = 0;

typedef struct
{
	LADSPA_Data * gate;
	LADSPA_Data * trigger;
	LADSPA_Data * loop_steps;
	LADSPA_Data * reset;
	LADSPA_Data * value_gate_closed;
	LADSPA_Data * values[SEQUENCER_MAX_INPUTS];
	LADSPA_Data * output;
	LADSPA_Data   srate;
	LADSPA_Data   inv_srate;
	LADSPA_Data   last_gate;
	LADSPA_Data   last_trigger;
	LADSPA_Data   last_value;
	unsigned int  step_index;
} Sequencer;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < 1)
		return sequencer_descriptors[index];

	return 0;
}

void
cleanupSequencer (LADSPA_Handle instance)
{
	free (instance);
}

void
connectPortSequencer (LADSPA_Handle instance,
                      unsigned long port,
                      LADSPA_Data * data)
{
	Sequencer * plugin = (Sequencer *) instance;

	switch (port)
	{
	case SEQUENCER_GATE:
		plugin->gate = data;
		break;
	case SEQUENCER_TRIGGER:
		plugin->trigger = data;
		break;
	case SEQUENCER_LOOP_POINT:
		plugin->loop_steps = data;
		break;
	case SEQUENCER_OUTPUT:
		plugin->output = data;
		break;
	case SEQUENCER_RESET:
		plugin->reset = data;
		break;
	case SEQUENCER_VALUE_GATE_CLOSED:
		plugin->value_gate_closed = data;
		break;
	default:
		if (port >= SEQUENCER_VALUE_START && port < SEQUENCER_OUTPUT)
			plugin->values[port - SEQUENCER_VALUE_START] = data;
		break;
	}
}

LADSPA_Handle
instantiateSequencer (const LADSPA_Descriptor * descriptor,
                      unsigned long sample_rate)
{
	Sequencer * plugin = (Sequencer *) malloc (sizeof (Sequencer));

	plugin->srate = (LADSPA_Data) sample_rate;
	plugin->inv_srate = 1.0f / plugin->srate;

	return (LADSPA_Handle) plugin;
}

void
activateSequencer (LADSPA_Handle instance)
{
	Sequencer * plugin = (Sequencer *) instance;

	plugin->last_gate = 0.0f;
	plugin->last_trigger = 0.0f;
	plugin->last_value = 0.0f;
	plugin->step_index = 0;
}

void
runSequencer (LADSPA_Handle instance,
              unsigned long sample_count)
{
	Sequencer * plugin = (Sequencer *) instance;

/* Gate */
	LADSPA_Data * gate = plugin->gate;

/* Step Trigger */
	LADSPA_Data *trigger = plugin->trigger;

/* Loop Steps */
	LADSPA_Data loop_steps = * (plugin->loop_steps);

/* Reset to Value on Gate Close */
	LADSPA_Data reset = * (plugin->reset);

/* Value used when gate closed */
	LADSPA_Data value_gate_closed = * (plugin->value_gate_closed);

/* Step Values */
	LADSPA_Data values[SEQUENCER_MAX_INPUTS];

/* Output */
	LADSPA_Data * output = plugin->output;

	LADSPA_Data last_gate = plugin->last_gate;
	LADSPA_Data last_trigger = plugin->last_trigger;
	LADSPA_Data last_value = plugin->last_value;

	unsigned int step_index = plugin->step_index;
	unsigned int loop_index = LRINTF (loop_steps);
	int rst = LRINTF (reset);
	int i;
	unsigned long s;

	loop_index = loop_index == 0 ?  1 : loop_index;
	loop_index = loop_index > SEQUENCER_MAX_INPUTS ? SEQUENCER_MAX_INPUTS : loop_index;

	for (i = 0; i < SEQUENCER_MAX_INPUTS; i++)
		values[i] = * (plugin->values[i]);

	for (s = 0; s < sample_count; s++)
	{
		if (gate[s] > 0.0f)
		{
			if (trigger[s] > 0.0f && !(last_trigger > 0.0f))
			{
				if (last_gate > 0.0f)
				{
					step_index++;
					if (step_index >= loop_index)
						step_index = 0;
				}
				else
				{
					step_index = 0;
				}
			}

			output[s] = values[step_index];

			last_value = values[step_index];
		}
		else
		{
			if (rst)
				output[s] = value_gate_closed;
			else
				output[s] = last_value;

			step_index = 0;
		}
		last_gate = gate[s];
		last_trigger = trigger[s];
	}

	plugin->last_gate = last_gate;
	plugin->last_trigger = last_trigger;
	plugin->last_value = last_value;
	plugin->step_index = step_index;
}

void
_init (void)
{
/* !!!! Ensure there is space for possible translations !!!! */
	static char label[32];
	static char name[40];
	static char loop_point_label[32];
	static char value_labels[SEQUENCER_MAX_INPUTS][32];
	char ** port_names;

	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i, step_index;
	unsigned long port_count = SEQUENCER_MAX_INPUTS + 6;

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runSequencer};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	sequencer_descriptors = (LADSPA_Descriptor **) calloc (SEQUENCER_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

/* Mmmm. Lovely... */
	sprintf (label, "sequencer%d", SEQUENCER_MAX_INPUTS);
	sprintf (name, G_("Analogue Style %d Step Sequencer"), SEQUENCER_MAX_INPUTS);
	sprintf (loop_point_label, G_("Loop Steps (1 - %d)"), SEQUENCER_MAX_INPUTS);

	if (sequencer_descriptors)
	{
		for (i = 0; i < SEQUENCER_VARIANT_COUNT; i++)
		{
			sequencer_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof(LADSPA_Descriptor));
			descriptor = sequencer_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = SEQUENCER_BASE_ID + i;
				descriptor->Label = label;
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = name;
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = port_count;

				port_descriptors = (LADSPA_PortDescriptor *) calloc (port_count, sizeof (LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *) port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (port_count, sizeof (LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (port_count, sizeof (char*));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Gate Signal */
				port_descriptors[SEQUENCER_GATE] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
				port_names[SEQUENCER_GATE] = G_("Gate (Open > 0)");
				port_range_hints[SEQUENCER_GATE].HintDescriptor = LADSPA_HINT_TOGGLED;

			/* Parameters for Step Trigger Signal */
				port_descriptors[SEQUENCER_TRIGGER] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
				port_names[SEQUENCER_TRIGGER] = G_("Step Trigger");
				port_range_hints[SEQUENCER_TRIGGER].HintDescriptor = LADSPA_HINT_TOGGLED;

			/* Parameters for Loop Point */
				port_descriptors[SEQUENCER_LOOP_POINT] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
				port_names[SEQUENCER_LOOP_POINT] = loop_point_label;
				port_range_hints[SEQUENCER_LOOP_POINT].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                        LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MAXIMUM;
				port_range_hints[SEQUENCER_LOOP_POINT].LowerBound = 1.0f;
				port_range_hints[SEQUENCER_LOOP_POINT].UpperBound = (float)SEQUENCER_MAX_INPUTS;

			/* Parameters for Reset Value */
				port_descriptors[SEQUENCER_RESET] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
				port_names[SEQUENCER_RESET] = G_("Reset to Value on Gate Close?");
				port_range_hints[SEQUENCER_RESET].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                   LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MINIMUM;
				port_range_hints[SEQUENCER_RESET].LowerBound = 0.0f;
				port_range_hints[SEQUENCER_RESET].UpperBound = 1.0f;

			/* Parameters for Closed Gate Value */
				port_descriptors[SEQUENCER_VALUE_GATE_CLOSED] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
				port_names[SEQUENCER_VALUE_GATE_CLOSED] = G_("Closed Gate Value");
				port_range_hints[SEQUENCER_VALUE_GATE_CLOSED].HintDescriptor = 0;

			/* Parameters for Step Values */
				for (step_index = 0; step_index < SEQUENCER_MAX_INPUTS; step_index++)
				{
					port_descriptors[SEQUENCER_VALUE_START + step_index] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
					sprintf (value_labels[step_index], G_("Value Step %d"), step_index);
					port_names[SEQUENCER_VALUE_START + step_index] = value_labels[step_index];
					port_range_hints[SEQUENCER_VALUE_START + step_index].HintDescriptor = 0;
				}

			/* Parameters for Output */
				port_descriptors[SEQUENCER_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
				port_names[SEQUENCER_OUTPUT] = G_("Value Out");
				port_range_hints[SEQUENCER_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateSequencer;
				descriptor->cleanup = cleanupSequencer;
				descriptor->connect_port = connectPortSequencer;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateSequencer;
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

	if (sequencer_descriptors)
	{
		for (i = 0; i < SEQUENCER_VARIANT_COUNT; i++)
		{
			descriptor = sequencer_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (sequencer_descriptors);
	}
}
