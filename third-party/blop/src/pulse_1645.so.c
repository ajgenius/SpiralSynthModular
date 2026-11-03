/*
    pulse.so.c - A LADSPA plugin to generate a bandlimited variable pulse
                 waveform

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
#include "wavedata.h"
#include "gettext.h"

#ifdef ENABLE_NLS
#include <locale.h>
#define G_(s) gettext(s)
#else
#define G_(s) (s)
#endif
#define G_NOP(s) s

#define PULSE_BASE_ID                  1645
#define PULSE_VARIANT_COUNT            4

#define PULSE_FREQUENCY                0
#define PULSE_PULSEWIDTH               1
#define PULSE_OUTPUT                   2

LADSPA_Descriptor ** pulse_descriptors = 0;

typedef struct
{
	LADSPA_Data * frequency;
	LADSPA_Data * pulsewidth;
	LADSPA_Data * output;
	LADSPA_Data   phase;
	Wavedata      wdat;
} Pulse;

/*****************************************************************************
 *
 * LADSPA Plugin code
 *
 *****************************************************************************/

const LADSPA_Descriptor *
ladspa_descriptor (unsigned long index)
{
	if (index < PULSE_VARIANT_COUNT)
		return pulse_descriptors[index];

	return 0;
}

void
connectPortPulse (LADSPA_Handle instance,
                  unsigned long port,
                  LADSPA_Data * data)
{
	Pulse * plugin = (Pulse *) instance;

	switch (port)
	{
	case PULSE_FREQUENCY:
		plugin->frequency = data;
		break;
	case PULSE_PULSEWIDTH:
		plugin->pulsewidth = data;
		break;
	case PULSE_OUTPUT:
		plugin->output = data;
		break;
	}
}

LADSPA_Handle
instantiatePulse (const LADSPA_Descriptor * descriptor,
                  unsigned long sample_rate)
{
    Pulse * plugin = (Pulse *) malloc (sizeof (Pulse));

    if (wavedata_load (&plugin->wdat, BLOP_DLSYM_SAWTOOTH, sample_rate))
    {
        free (plugin);
        return 0;
    }

    return (LADSPA_Handle) plugin;
}

void
cleanupPulse (LADSPA_Handle instance)
{
	Pulse *plugin = (Pulse *) instance;

	wavedata_unload (&plugin->wdat);
	free (instance);
}

void
activatePulse (LADSPA_Handle instance)
{
	Pulse * plugin = (Pulse *) instance;

	plugin->phase = 0.0f;
}

void
runPulse_fapa_oa (LADSPA_Handle instance,
                  unsigned long sample_count)
{
	Pulse * plugin = (Pulse *) instance;

/* Frequency (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Pulse Width (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * pulsewidth = plugin->pulsewidth;

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;

	LADSPA_Data freq;
	LADSPA_Data pwidth;
	LADSPA_Data phase_shift;
	unsigned long s;

	for (s = 0; s < sample_count; s++)
	{
		freq = frequency[s];
		pwidth = f_clip (pulsewidth[s], 0.0f, 1.0f);
		phase_shift = pwidth * wdat->sample_rate;

	/* Lookup which table to use from frequency */
		wavedata_get_table (wdat, freq);

	/* Get samples from sawtooth and phase shifted inverted sawtooth,
	   with approriate DC offset */
		output[s] = wavedata_get_sample (wdat, phase) -
		            wavedata_get_sample (wdat, phase + phase_shift) +
		            1.0f - (2.0f * pwidth);

	/* Update phase, wrapping if necessary */
		phase += wdat->frequency;
		if (phase < 0.0f)
			phase += wdat->sample_rate;
		else if (phase > wdat->sample_rate)
			phase -= wdat->sample_rate;
	}
	plugin->phase = phase;
}

void
runPulse_fapc_oa (LADSPA_Handle instance,
                  unsigned long sample_count)
{
	Pulse * plugin = (Pulse *) instance;

/* Frequency (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * frequency = plugin->frequency;

/* Pulse Width (LADSPA_Data value) */
	LADSPA_Data pulsewidth = f_clip (*(plugin->pulsewidth), 0.0f, 1.0f);

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;

	LADSPA_Data freq;
	LADSPA_Data dc_shift = 1.0 - (2.0 * pulsewidth);
	LADSPA_Data phase_shift = pulsewidth * wdat->sample_rate;
	unsigned long s;

	for (s = 0; s < sample_count; s++)
	{
		freq = frequency[s];

	/* Lookup which table to use from frequency */
		wavedata_get_table (wdat, freq);

	/* Get samples from sawtooth and phase shifted inverted sawtooth,
	   with approriate DC offset */
		output[s] = wavedata_get_sample (wdat, phase) -
		            wavedata_get_sample (wdat, phase + phase_shift) +
		            dc_shift;

	/* Update phase, wrapping if necessary */
		phase += wdat->frequency;
		if (phase < 0.0f)
			phase += wdat->sample_rate;
		else if (phase > wdat->sample_rate)
			phase -= wdat->sample_rate;
	}
	plugin->phase = phase;
}

void
runPulse_fcpa_oa (LADSPA_Handle instance,
                  unsigned long sample_count)
{
	Pulse * plugin = (Pulse *) instance;

/* Frequency (LADSPA_Data value) */
	LADSPA_Data frequency = *(plugin->frequency);

/* Pulse Width (array of LADSPA_Data of length sample_count) */
	LADSPA_Data * pulsewidth = plugin->pulsewidth;

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data * output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;

	LADSPA_Data pwidth;
	LADSPA_Data phase_shift;
	unsigned long s;

	wavedata_get_table (wdat, frequency);

	for (s = 0; s < sample_count; s++)
	{
		pwidth = f_clip (pulsewidth[s], 0.0f, 1.0f);
		phase_shift = pwidth * wdat->sample_rate;

	/* Get samples from sawtooth and phase shifted inverted sawtooth,
	with approriate DC offset */
		output[s] = wavedata_get_sample (wdat, phase) -
		            wavedata_get_sample (wdat, phase + phase_shift) +
		            1.0f - (2.0f * pwidth);

	/* Update phase, wrapping if necessary */
		phase += wdat->frequency;
		if (phase < 0.0f)
			phase += wdat->sample_rate;
		else if (phase > wdat->sample_rate)
			phase -= wdat->sample_rate;
	}
	plugin->phase = phase;
}

void
runPulse_fcpc_oa (LADSPA_Handle instance,
                  unsigned long sample_count)
{
	Pulse * plugin = (Pulse *) instance;

/* Frequency (LADSPA_Data value) */
	LADSPA_Data frequency = *(plugin->frequency);

/* Pulse Width (LADSPA_Data value) */
	LADSPA_Data pulsewidth = f_clip (*(plugin->pulsewidth), 0.0f, 1.0f);

/* Output (pointer to LADSPA_Data value) */
	LADSPA_Data *output = plugin->output;

/* Instance data */
	Wavedata * wdat = &plugin->wdat;
	LADSPA_Data phase = plugin->phase;

	LADSPA_Data dc_shift = 1.0f - (2.0f * pulsewidth);
	LADSPA_Data phase_shift = pulsewidth * wdat->sample_rate;
	unsigned long s;

	wavedata_get_table (wdat, frequency);

	for (s = 0; s < sample_count; s++)
	{
	/* Get samples from sawtooth and phase shifted inverted sawtooth,
	   with approriate DC offset */
		output[s] = wavedata_get_sample (wdat, phase) -
		            wavedata_get_sample (wdat, phase + phase_shift) +
		            dc_shift;

	/* Update phase, wrapping if necessary */
		phase += wdat->frequency;
		if (phase < 0.0f)
			phase += wdat->sample_rate;
		else if (phase > wdat->sample_rate)
			phase -= wdat->sample_rate;
	}
	plugin->phase = phase;
}

void
_init (void)
{
	static const char * labels[] = {"pulse_fapa_oa",
	                                "pulse_fapc_oa",
	                                "pulse_fcpa_oa",
	                                "pulse_fcpc_oa"};
	static const char * names[] = {G_NOP("Bandlimited Variable Width Pulse Oscillator (FAPA)"),
	                               G_NOP("Bandlimited Variable Width Pulse Oscillator (FAPC)"),
	                               G_NOP("Bandlimited Variable Width Pulse Oscillator (FCPA)"),
	                               G_NOP("Bandlimited Variable Width Pulse Oscillator (FCPC)")};
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	LADSPA_Descriptor * descriptor;
	int i;

	LADSPA_PortDescriptor frequency_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                      LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor pulsewidth_port_descriptors[] = {LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                       LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	                                                       LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	                                                       LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL};
	LADSPA_PortDescriptor output_port_descriptors[] = {LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	                                                   LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO};

	void (*run_functions[]) (LADSPA_Handle,
	                         unsigned long) = {runPulse_fapa_oa,
	                                           runPulse_fapc_oa,
	                                           runPulse_fcpa_oa,
	                                           runPulse_fcpc_oa};

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	pulse_descriptors = (LADSPA_Descriptor **) calloc (PULSE_VARIANT_COUNT, sizeof (LADSPA_Descriptor));

	if (pulse_descriptors)
	{
		for (i = 0; i < PULSE_VARIANT_COUNT; i++)
		{
			pulse_descriptors[i] = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));
			descriptor = pulse_descriptors[i];
			if (descriptor)
			{
				descriptor->UniqueID = PULSE_BASE_ID + i;
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

			/* Parameters for Frequency */
				port_descriptors[PULSE_FREQUENCY] = frequency_port_descriptors[i];
				port_names[PULSE_FREQUENCY] = G_("Frequency");
				port_range_hints[PULSE_FREQUENCY].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                   LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC |
				                                                   LADSPA_HINT_DEFAULT_440;
				port_range_hints[PULSE_FREQUENCY].LowerBound = 1.0f / 48000.0f;
				port_range_hints[PULSE_FREQUENCY].UpperBound = 0.5f;

			/* Parameters for Pulse Width */
				port_descriptors[PULSE_PULSEWIDTH] = pulsewidth_port_descriptors[i];
				port_names[PULSE_PULSEWIDTH] = G_("Pulse Width");
				port_range_hints[PULSE_PULSEWIDTH].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
				                                                    LADSPA_HINT_DEFAULT_MIDDLE;
				port_range_hints[PULSE_PULSEWIDTH].LowerBound = 0.0f;
				port_range_hints[PULSE_PULSEWIDTH].UpperBound = 1.0f;

			/* Parameters for Output */
				port_descriptors[PULSE_OUTPUT] = output_port_descriptors[i];
				port_names[PULSE_OUTPUT] = G_("Output");
				port_range_hints[PULSE_OUTPUT].HintDescriptor = 0;

				descriptor->activate = activatePulse;
				descriptor->cleanup = cleanupPulse;
				descriptor->connect_port = connectPortPulse;
				descriptor->deactivate = NULL;
				descriptor->instantiate = instantiatePulse;
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

	if (pulse_descriptors)
	{
		for (i = 0; i < PULSE_VARIANT_COUNT; i++)
		{
			descriptor = pulse_descriptors[i];
			if (descriptor)
			{
				free ((LADSPA_PortDescriptor *) descriptor->PortDescriptors);
				free ((char **) descriptor->PortNames);
				free ((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
				free (descriptor);
			}
		}
		free (pulse_descriptors);
	}
}
