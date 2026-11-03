/*
    adsr.so.c - A LADSPA plugin to generate ADSR envelopes

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
#include "common.h"
#include "gettext.h"

#ifdef ENABLE_NLS
#include <locale.h>
#define G_(s) gettext(s)
#else
#define G_(s) (s)
#endif

#define ADSR_BASE_ID                   1653
#define ADSR_VARIANT_COUNT             1

#define ADSR_SIGNAL                    0
#define ADSR_TRIGGER                   1
#define ADSR_ATTACK                    2
#define ADSR_DECAY                     3
#define ADSR_SUSTAIN                   4
#define ADSR_RELEASE                   5
#define ADSR_OUTPUT                    6

LADSPA_Descriptor ** adsr_descriptors = 0;

typedef enum
{
	IDLE,
	ATTACK,
	DECAY,
	SUSTAIN,
	RELEASE
} ADSRState;

typedef struct
{
	LADSPA_Data     *signal;
	LADSPA_Data     *trigger;
	LADSPA_Data     *attack;
	LADSPA_Data     *decay;
	LADSPA_Data     *sustain;
	LADSPA_Data     *release;
	LADSPA_Data     *output;
	LADSPA_Data     srate;
	LADSPA_Data     inv_srate;
	LADSPA_Data     from_level;
	LADSPA_Data     level;
	ADSRState       state;
	unsigned long   samples;
} Adsr;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < 1)
		return adsr_descriptors[index];
	else
		return 0;
}

void
cleanupAdsr (LADSPA_Handle instance)
{
	free(instance);
}

void
connectPortAdsr (LADSPA_Handle instance,
                 unsigned long port,
                 LADSPA_Data * data)
{
	Adsr *plugin = (Adsr *) instance;

	switch (port)
	{
	case ADSR_SIGNAL:
		plugin->signal = data;
		break;
	case ADSR_TRIGGER:
		plugin->trigger = data;
		break;
	case ADSR_ATTACK:
		plugin->attack = data;
		break;
	case ADSR_DECAY:
		plugin->decay = data;
		break;
	case ADSR_SUSTAIN:
		plugin->sustain = data;
		break;
	case ADSR_RELEASE:
		plugin->release = data;
		break;
	case ADSR_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateAdsr (const LADSPA_Descriptor * descriptor,
                 unsigned long sample_rate)
{
	Adsr * plugin = (Adsr *) malloc (sizeof (Adsr));

	plugin->srate = (LADSPA_Data) sample_rate;
	plugin->inv_srate = 1.0f / plugin->srate;

	return (LADSPA_Handle) plugin;
}

void
activateAdsr (LADSPA_Handle instance)
{
	Adsr * plugin = (Adsr *) instance;

	plugin->from_level = 0.0f;
	plugin->level = 0.0f;
	plugin->state = IDLE;
	plugin->samples = 0;
}

void
runAdsr (LADSPA_Handle instance,
         unsigned long sample_count)
{
	Adsr * plugin = (Adsr *) instance;

/* Driving signal */
	LADSPA_Data *signal = plugin->signal;

/* Trigger Threshold */
	LADSPA_Data trigger = * (plugin->trigger);

/* Attack Time (s) */
	LADSPA_Data attack = * (plugin->attack);

/* Decay Time (s) */
	LADSPA_Data decay = * (plugin->decay);

/* Sustain Level */
	LADSPA_Data sustain = f_clip (* (plugin->sustain), 0.0f, 1.0f);

/* Release Time (s) */
	LADSPA_Data release = * (plugin->release);

/* Envelope Out */
	LADSPA_Data *output = plugin->output;

	LADSPA_Data srate = plugin->srate;
	LADSPA_Data inv_srate = plugin->inv_srate;
	LADSPA_Data from_level = plugin->from_level;
	LADSPA_Data level = plugin->level;
	ADSRState state = plugin->state;
	unsigned long samples = plugin->samples;

	LADSPA_Data elapsed;
	unsigned long s;

/* Convert times into rates */
	attack = attack > 0.0f ? inv_srate / attack : srate;
	decay = decay > 0.0f ? inv_srate / decay : srate;
	release = release > 0.0f ? inv_srate / release : srate;

	for (s = 0; s < sample_count; s++)
	{
	/* Determine if attack or release happened */
		if ((state == IDLE) || (state == RELEASE))
		{
			if (signal[s] > trigger)
			{
				if (attack < srate)
				{
					state = ATTACK;
				}
				else
				{
					state = decay < srate ? DECAY : SUSTAIN;
					level = 1.0f;
				}
				samples = 0;
			}
		}
		else
		{
			if (signal[s] <= trigger)
			{
				state = release < srate ? RELEASE : IDLE;
				samples = 0;
			}
		}

		if (samples == 0)
			from_level = level;

	/* Calculate level of envelope from current state */
		switch (state)
		{
		case IDLE:
			level = 0;
			break;
		case ATTACK:
			samples++;
			elapsed = (LADSPA_Data)samples * attack;
			if (elapsed > 1.0f)
			{
				state = decay < srate ? DECAY : SUSTAIN;
				level = 1.0f;
				samples = 0;
			}
			else
			{
				level = from_level + elapsed * (1.0f - from_level);
			}
			break;
		case DECAY:
			samples++;
			elapsed = (LADSPA_Data)samples * decay;
			if (elapsed > 1.0f)
			{
				state = SUSTAIN;
				level = sustain;
				samples = 0;
			}
			else
			{
				level = from_level + elapsed * (sustain - from_level);
			}
			break;
		case SUSTAIN:
			level = sustain;
			break;
		case RELEASE:
			samples++;
			elapsed = (LADSPA_Data)samples * release;
			if (elapsed > 1.0f)
			{
				state = IDLE;
				level = 0.0f;
				samples = 0;
			}
			else
			{
				level = from_level - elapsed * from_level;
			}
			break;
		default:
		/* Should never happen */
			level = 0.0f;
		}
		
		output[s] = level;
	}

	plugin->from_level = from_level;
	plugin->level = level;
	plugin->state = state;
	plugin->samples = samples;
}

void _init()
{
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor signal_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO};
	LADSPA_PortDescriptor trigger_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor attack_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor decay_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor sustain_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor release_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runAdsr};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	adsr_descriptors = (LADSPA_Descriptor **) calloc (ADSR_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (adsr_descriptors)
	{
		for (i = 0; i < ADSR_VARIANT_COUNT; i++)
		{
			adsr_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = adsr_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = ADSR_BASE_ID + i;
				descriptor->Label = "adsr";
				descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
				descriptor->Name = G_("ADSR Envelope");
				descriptor->Maker = "Mike Rawes <mike_rawes[at]yahoo.co.uk>";
				descriptor->Copyright = "GPL";

				descriptor->PortCount = 7;

				port_descriptors = (LADSPA_PortDescriptor *) calloc (7, sizeof(LADSPA_PortDescriptor));
				descriptor->PortDescriptors = (const LADSPA_PortDescriptor *) port_descriptors;

				port_range_hints = (LADSPA_PortRangeHint *) calloc (7, sizeof(LADSPA_PortRangeHint));
				descriptor->PortRangeHints = (const LADSPA_PortRangeHint *) port_range_hints;

				port_names = (char **) calloc (7, sizeof (char *));
				descriptor->PortNames = (const char **) port_names;

			/* Parameters for Driving Signal */
				port_descriptors[ADSR_SIGNAL] = signal_port_descriptors[i];
				port_names[ADSR_SIGNAL] = G_("Driving Signal");
				port_range_hints[ADSR_SIGNAL].HintDescriptor = 0;

			/* Parameters for Trigger Threshold */
				port_descriptors[ADSR_TRIGGER] = trigger_port_descriptors[i];
				port_names[ADSR_TRIGGER] = G_("Trigger Threshold");
				port_range_hints[ADSR_TRIGGER].HintDescriptor = 0;

			/* Parameters for Attack Time (s) */
				port_descriptors[ADSR_ATTACK] = attack_port_descriptors[i];
				port_names[ADSR_ATTACK] = G_("Attack Time (s)");
				port_range_hints[ADSR_ATTACK].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
				port_range_hints[ADSR_ATTACK].LowerBound = 0.0f;

			/* Parameters for Decay Time (s) */
				port_descriptors[ADSR_DECAY] = decay_port_descriptors[i];
				port_names[ADSR_DECAY] = G_("Decay Time (s)");
				port_range_hints[ADSR_DECAY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
				port_range_hints[ADSR_DECAY].LowerBound = 0.0f;

			/* Parameters for Sustain Level */
				port_descriptors[ADSR_SUSTAIN] = sustain_port_descriptors[i];
				port_names[ADSR_SUSTAIN] = G_("Sustain Level");
				port_range_hints[ADSR_SUSTAIN].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
																LADSPA_HINT_DEFAULT_MAXIMUM;
				port_range_hints[ADSR_SUSTAIN].LowerBound = 0.0f;
				port_range_hints[ADSR_SUSTAIN].UpperBound = 1.0f;

			/* Parameters for Release Time (s) */
				port_descriptors[ADSR_RELEASE] = release_port_descriptors[i];
				port_names[ADSR_RELEASE] = G_("Release Time (s)");
				port_range_hints[ADSR_RELEASE].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MINIMUM;
				port_range_hints[ADSR_RELEASE].LowerBound = 0.0f;

			/* Parameters for Envelope Out */
				port_descriptors[ADSR_OUTPUT] = output_port_descriptors[i];
				port_names[ADSR_OUTPUT] = G_("Envelope Out");
				port_range_hints[ADSR_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateAdsr;
				descriptor->cleanup = cleanupAdsr;
				descriptor->connect_port = connectPortAdsr;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateAdsr;
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

	if (adsr_descriptors)
	{
		for (i = 0; i < ADSR_VARIANT_COUNT; i++)
		{
			descriptor = adsr_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (adsr_descriptors);
	}
}
