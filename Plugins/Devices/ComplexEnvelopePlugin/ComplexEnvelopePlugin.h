/*  SpiralSound
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef COMPLEX_ENVELOPE_PLUGIN_H
#define COMPLEX_ENVELOPE_PLUGIN_H

#include "Device.h"
#include "BreakpointCurve.h"
#include <vector>

class ComplexEnvelopePlugin: public Spiral::Core::Device
{
public:
	static const unsigned MaxPoints = 16;
	static const int CurveSections = 10;

	ComplexEnvelopePlugin();
	virtual ~ComplexEnvelopePlugin();

	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void Reset();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);

protected:
	unsigned LiveCount() const;
	void Reserve();
	void BuildEnvelope();

	unsigned long m_Changes;

	Spiral::Core::SignedProperty m_Count;
	Spiral::Core::FloatProperty m_Length;
	Spiral::Core::SignedProperty m_Mode;
	Spiral::Core::FloatProperty m_PointX[MaxPoints], m_PointY[MaxPoints];
	Spiral::Core::FloatProperty m_TangentX[MaxPoints], m_TangentY[MaxPoints];
	Spiral::Core::FloatProperty m_Playhead;

	std::vector<float> m_Envelope;
	size_t m_EnvelopeLength;
	std::vector<Spiral::CurvePoint> m_Points;
	std::vector<Spiral::CurvePoint> m_Tangents;
	std::vector<Spiral::CurvePoint> m_Curve;
	int m_ReservedRate;
	bool m_Stale;

	long m_Position;
	bool m_Triggered;
};

#endif
