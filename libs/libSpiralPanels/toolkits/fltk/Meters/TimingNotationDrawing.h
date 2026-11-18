// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SSM_TIMING_NOTATION_DRAWING_H
#define SSM_TIMING_NOTATION_DRAWING_H

#include <cstdio>

// The display is an unpitched staff opening. Drawing the note with primitives
// keeps tempo marks legible on hosts without a music font.
template <class Pen>
void DrawTimingNotation(Pen &pen, double bpm, int tempoNote, int beats, int unit)
{
	static const char *const notes[] = {"quarter", "dotted quarter", "eighth", "sixteenth", "half", "dotted half"};
	if (tempoNote < 0 || tempoNote > 5)
		tempoNote = 0;
	char text[96];
	const char *unitName = 0;
	switch (unit)
	{
	case 1: unitName = "whole"; break;
	case 2: unitName = "half"; break;
	case 4: unitName = "quarter"; break;
	case 8: unitName = "eighth"; break;
	case 16: unitName = "sixteenth"; break;
	case 32: unitName = "thirty-second"; break;
	}

	pen.Head(10, 20, tempoNote == 4 || tempoNote == 5);
	pen.Line(19, 21, 19, 5);
	if (tempoNote == 1 || tempoNote == 5)
		pen.Dot(23, 21);
	if (tempoNote == 2 || tempoNote == 3)
		pen.Flag(19, 5);
	if (tempoNote == 3)
		pen.Flag(19, 10);
	std::snprintf(text, sizeof(text), "= %.4g", bpm);
	pen.Words(text, 32, 5, 16);

	for (int y = 38; y <= 70; y += 8)
	{
		pen.Line(5, y, 19, y);
		pen.Line(48, y, 94, y);
	}
	std::snprintf(text, sizeof(text), "%d", beats);
	pen.Words(text, beats >= 10 ? 20 : 25, 33, 18);
	std::snprintf(text, sizeof(text), "%d", unit);
	pen.Words(text, unit >= 10 ? 20 : 25, 51, 18);

	std::snprintf(text, sizeof(text), "%.4g beats per minute", bpm);
	pen.Words(text, 108, 6, 14);
	std::snprintf(text, sizeof(text), "One beat = one %s note", notes[tempoNote]);
	pen.Words(text, 108, 25, 12);
	if (unitName)
		std::snprintf(text, sizeof(text), "%d %s note%s per bar", beats, unitName, beats == 1 ? "" : "s");
	else
		std::snprintf(text, sizeof(text), "%d units of 1/%d per bar", beats, unit);
	pen.Words(text, 108, 44, 13);
	if (unit > 0 && bpm > 0)
	{
		static const double quarters[] = {1, 1.5, 0.5, 0.25, 2, 3};
		const double pulses = beats * 4.0 / (unit * quarters[tempoNote]);
		std::snprintf(text, sizeof(text), "%.4g beats in %.4g seconds", pulses, pulses * 60 / bpm);
		pen.Words(text, 108, 61, 12);
	}
}

#endif
