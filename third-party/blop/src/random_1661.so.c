/*
    random.so.c - A LADSPA plugin to generate a 'random' wave of varying
                  frequency and smoothness

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
#include <time.h>
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

#define RANDOM_BASE_ID                 1661
#define RANDOM_VARIANT_COUNT           4

#define RANDOM_FREQUENCY               0
#define RANDOM_SMOOTH                  1
#define RANDOM_OUTPUT                  2

LADSPA_Descriptor ** random_descriptors = 0;

typedef struct
{
	LADSPA_Data * frequency;
	LADSPA_Data * smooth;
	LADSPA_Data * output;
	LADSPA_Data   nyquist;
	LADSPA_Data   inv_nyquist;
	LADSPA_Data   phase;
	LADSPA_Data   value1;
	LADSPA_Data   value2;
} Random;

LADSPA_Data inv_rand_max;

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < RANDOM_VARIANT_COUNT)
		return random_descriptors[index];

	return 0;
}

void
cleanupRandom (LADSPA_Handle instance)
{
	free (instance);
}

void
connectPortRandom (LADSPA_Handle instance,
                   unsigned long port,
                   LADSPA_Data * data)
{
	Random * plugin = (Random *) instance;

	switch (port)
	{
	case RANDOM_FREQUENCY:
		plugin->frequency = data;
		break;
	case RANDOM_SMOOTH:
		plugin->smooth = data;
		break;
	case RANDOM_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiateRandom (const LADSPA_Descriptor * descriptor,
                   unsigned long sample_rate)
{
	Random * plugin = (Random *) malloc (sizeof (Random));

	srand ((int) time ((time_t *) 0));

	inv_rand_max = 2.0f / (LADSPA_Data) RAND_MAX;

	plugin->nyquist = (LADSPA_Data) sample_rate / 2.0f;
	plugin->inv_nyquist = 1.0f / plugin->nyquist;

	plugin->value1 = rand() * inv_rand_max - 1.0f;
	plugin->value2 = rand() * inv_rand_max - 1.0f;

	return (LADSPA_Handle) plugin;
}

void
activateRandom (LADSPA_Handle instance)
{
	Random * plugin = (Random *) instance;

	plugin->phase = 0.0f;
}

void
runRandom_fasa_oa (LADSPA_Handle instance,
                   unsigned long sample_count)
{
	Random * plugin = (Random *) instance;

/* Frequency (Hz) (array of floats of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Wave smoothness (array of floats of length sample_count) */
	LADSPA_Data * smooth = plugin->smooth;

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	LADSPA_Data nyquist = plugin->nyquist;
	LADSPA_Data inv_nyquist = plugin->inv_nyquist;
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data value1 = plugin->value1;
	LADSPA_Data value2 = plugin->value2;

	LADSPA_Data freq;
	LADSPA_Data smth;
	LADSPA_Data interval;
	LADSPA_Data result;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		freq = f_clip (frequency[s], 0.0f, nyquist);

		smth = f_clip (smooth[s], 0.0f, 1.0f);
		interval = (1.0f - smth) * 0.5f;

		if (phase < interval)
			result = 1.0f;
		else if (phase > (1.0f - interval))
			result = -1.0f;
		else if (interval > 0.0f)
			result = COSF ((phase - interval) / smth * M_PI);
		else
			result = COSF (phase * M_PI);

		result *= (value2 - value1) * 0.5f;
		result -= (value2 + value1) * 0.5f;

		output[s] = result;

		phase += freq * inv_nyquist;
		if (phase > 1.0f)
		{
			phase -= 1.0f;
			value1 = value2;
			value2 = (LADSPA_Data) rand () * inv_rand_max - 1.0f;
		}
	}

	plugin->phase = phase;
	plugin->value1 = value1;
	plugin->value2 = value2;
}

void
runRandom_fasc_oa (LADSPA_Handle instance,
                   unsigned long sample_count)
{
	Random * plugin = (Random *) instance;

/* Frequency (Hz) (array of floats of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Wave smoothness (float value) */
	LADSPA_Data smooth = f_clip (* (plugin->smooth), 0.0f, 1.0f);

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	LADSPA_Data nyquist = plugin->nyquist;
	LADSPA_Data inv_nyquist = plugin->inv_nyquist;
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data value1 = plugin->value1;
	LADSPA_Data value2 = plugin->value2;

	LADSPA_Data freq;
	LADSPA_Data interval = (1.0f - smooth) * 0.5f;
	LADSPA_Data result;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		freq = f_clip (frequency[s], 0.0f, nyquist);

		if (phase < interval)
			result = 1.0f;
		else if (phase > (1.0f - interval))
			result = -1.0f;
		else if (interval > 0.0f)
			result = COSF ((phase - interval) / smooth * M_PI);
		else
			result = COSF (phase * M_PI);

		result *= (value2 - value1) * 0.5f;
		result -= (value2 + value1) * 0.5f;

		output[s] = result;

		phase += freq * inv_nyquist;
		if (phase > 1.0f)
		{
			phase -= 1.0f;
			value1 = value2;
			value2 = (LADSPA_Data) rand () * inv_rand_max - 1.0f;
		}
	}

	plugin->phase = phase;
	plugin->value1 = value1;
	plugin->value2 = value2;
}

void
runRandom_fcsa_oa (LADSPA_Handle instance,
                   unsigned long sample_count)
{
	Random *plugin = (Random *)instance;

	/* Frequency (Hz) (float value) */
	LADSPA_Data frequency = *(plugin->frequency);

	/* Wave smoothness (array of floats of length sample_count) */
	LADSPA_Data *smooth = plugin->smooth;

	/* Output (pointer to float value) */
	LADSPA_Data *output = plugin->output;

	/* Instance data */
	LADSPA_Data nyquist = plugin->nyquist;
	LADSPA_Data inv_nyquist = plugin->inv_nyquist;
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data value1 = plugin->value1;
	LADSPA_Data value2 = plugin->value2;

	LADSPA_Data phase_scale = f_clip(frequency, 0.0f, nyquist) * inv_nyquist;
	LADSPA_Data smth;
	LADSPA_Data interval;
	LADSPA_Data result;
	unsigned int s;

	for (s = 0; s < sample_count; s++) {
		smth = f_clip (smooth[s], 0.0f, 1.0f);
		interval = (1.0f - smth) * 0.5f;

		if (phase < interval)
			result = 1.0f;
		else if (phase > (1.0f - interval))
			result = -1.0f;
		else if (interval > 0.0f)
			result = COSF ((phase - interval) / smth * M_PI);
		else
			result = COSF (phase * M_PI);

		result *= (value2 - value1) * 0.5f;
		result -= (value2 + value1) * 0.5f;

		output[s] = result;

		phase += phase_scale;
		if (phase > 1.0f)
		{
			phase -= 1.0f;
			value1 = value2;
			value2 = (LADSPA_Data) rand () * inv_rand_max - 1.0f;
		}
	}

	plugin->phase = phase;
	plugin->value1 = value1;
	plugin->value2 = value2;
}

void
runRandom_fcsc_oa (LADSPA_Handle instance,
                   unsigned long sample_count)
{
	Random * plugin = (Random *) instance;

/* Frequency (Hz) (float value) */
	LADSPA_Data frequency = * (plugin->frequency);

/* Wave smoothness (float value) */
	LADSPA_Data smooth = f_clip (* (plugin->smooth), 0.0f, 1.0f);

/* Output (array of floats of length sample_count) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	LADSPA_Data nyquist = plugin->nyquist;
	LADSPA_Data inv_nyquist = plugin->inv_nyquist;
	LADSPA_Data phase = plugin->phase;
	LADSPA_Data value1 = plugin->value1;
	LADSPA_Data value2 = plugin->value2;

	LADSPA_Data phase_scale = f_clip (frequency, 0.0f, nyquist) * inv_nyquist;
	LADSPA_Data interval = (1.0f - smooth) * 0.5f;
	LADSPA_Data result;
	unsigned int s;

	for (s = 0; s < sample_count; s++)
	{
		if (phase < interval)
			result = 1.0f;
		else if (phase > (1.0f - interval))
			result = -1.0f;
		else if (interval > 0.0f)
			result = COSF ((phase - interval) / smooth * M_PI);
		else
			result = COSF (phase * M_PI);

		result *= (value2 - value1) * 0.5f;
		result -= (value2 + value1) * 0.5f;

		output[s] = result;

		phase += phase_scale;
		if (phase > 1.0f)
		{
			phase -= 1.0f;
			value1 = value2;
			value2 = (LADSPA_Data) rand () * inv_rand_max - 1.0f;
		}
	}

	plugin->phase = phase;
	plugin->value1 = value1;
	plugin->value2 = value2;
}

void
_init (void)
{
	static const char * labels[] = {"random_fasa_oa",
	                                "random_fasc_oa",
	                                "random_fcsa_oa",
	                                "random_fcsc_oa"};
	static const char * names[] = {G_NOP("Random Wave Generator (FASA)"),
	                               G_NOP("Random Wave Generator (FASC)"),
	                               G_NOP("Random Wave Generator (FCSA)"),
	                               G_NOP("Random Wave Generator (FCSC)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor frequency_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor smooth_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runRandom_fasa_oa,
	                                           runRandom_fasc_oa,
	                                           runRandom_fcsa_oa,
	                                           runRandom_fcsc_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	random_descriptors = (LADSPA_Descriptor **) calloc (RANDOM_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (random_descriptors)
	{
		for (i = 0; i < RANDOM_VARIANT_COUNT; i++)
		{
			random_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = random_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = RANDOM_BASE_ID + i;
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

			/* Parameters for Frequency (Hz) */
				port_descriptors[RANDOM_FREQUENCY] = frequency_port_descriptors[i];
				port_names[RANDOM_FREQUENCY] = G_("Frequency (Hz)");
				port_range_hints[RANDOM_FREQUENCY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                    LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC |
				                                                    LADSPA_HINT_DEFAULT_440;
				port_range_hints[RANDOM_FREQUENCY].LowerBound = 1.0f / 48000.0f;
				port_range_hints[RANDOM_FREQUENCY].UpperBound = 0.5f;

			/* Parameters for Wave smoothness */
				port_descriptors[RANDOM_SMOOTH] = smooth_port_descriptors[i];
				port_names[RANDOM_SMOOTH] = G_("Wave Smoothness");
				port_range_hints[RANDOM_SMOOTH].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                 LADSPA_HINT_DEFAULT_MAXIMUM;
				port_range_hints[RANDOM_SMOOTH].LowerBound = 0.0f;
				port_range_hints[RANDOM_SMOOTH].UpperBound = 1.0f;

			/* Parameters for Output */
				port_descriptors[RANDOM_OUTPUT] = output_port_descriptors[i];
				port_names[RANDOM_OUTPUT] = G_("Output");
				port_range_hints[RANDOM_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activateRandom;
				descriptor->cleanup = cleanupRandom;
				descriptor->connect_port = connectPortRandom;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiateRandom;
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

	if (random_descriptors)
	{
		for (i = 0; i < RANDOM_VARIANT_COUNT; i++)
		{
			descriptor = random_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (random_descriptors);
	}
}
