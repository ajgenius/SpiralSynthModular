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
#include "ComplexEnvelopePlugin.h"
#include "SpiralIcon.xpm"
#include <config.h>

using namespace std;
using namespace Spiral::Core;

static const float MIN_LENGTH = 0.01f;
static const float MAX_LENGTH = 10.0f;

extern "C"
{
	const char *SpiralPlugin_GetHostVersion() { return PACKAGE_VERSION; }
	const char *SpiralPlugin_GetHostABI() { return SSM_HOST_ABI; }
	SpiralPlugin *SpiralPlugin_CreateInstance() { return new ComplexEnvelopePlugin; }
	int SpiralPlugin_GetType() { return SPIRAL_PLUGIN_TYPE_DEVICE; }
	const char **SpiralPlugin_GetIcon() { return SpiralIcon_xpm; }
	int SpiralPlugin_GetID() { return 33; }
	string SpiralPlugin_GetName() { return "ComplexEnv"; }
	string SpiralPlugin_GetGroupName() { return "Control"; }
}

///////////////////////////////////////////////////////

ComplexEnvelopePlugin::ComplexEnvelopePlugin():
	m_EnvelopeLength(0),
	m_ReservedRate(0),
	m_Stale(true),
	m_Position(-1),
	m_Triggered(false)
{
	m_PluginInfo.Name = "ComplexEnv";
	m_PluginInfo.Width = 324;
	m_PluginInfo.Height = 240;
	m_PluginInfo.NumInputs = 2;
	m_PluginInfo.NumOutputs = 2;
	m_PluginInfo.PortTips.push_back("Trigger CV");
	m_PluginInfo.PortTips.push_back("Input");
	m_PluginInfo.PortTips.push_back("CV");
	m_PluginInfo.PortTips.push_back("Output");

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 2);

	const float def[][2] = {{0.0f, 0.0f}, {0.15f, 1.0f}, {0.4f, 0.6f}, {1.0f, 0.0f}};
	m_Count = SignedProperty(DefaultLinearFlags, 4, 2, MaxPoints, 1, 1);
	m_Length = FloatProperty(DefaultLinearFlags, 2.0f, 0.001f, 60.0f, 0.001f, 0.1f);
	m_Mode = SignedProperty(DefaultLinearFlags, Spiral::SmoothCurve, 0, 2, 1, 1);
	m_Playhead = FloatProperty(Property::ReadOnly, 0.0f, 0.0f, 1.0f, 0.001f, 0.1f);
	RegisterSharedProperty(m_Count, "Count", "Points in use");
	RegisterSharedProperty(m_Length, "Length", "Seconds across the envelope");
	RegisterSharedProperty(m_Mode, "Mode", "Straight, smooth or Bezier");
	RegisterSharedProperty(m_Playhead, "Playhead", "Where the envelope is playing");
	for (unsigned i = 0; i < MaxPoints; ++i)
	{
		char key[16], hint[32];
		m_PointX[i] = FloatProperty(DefaultLinearFlags, i < 4 ? def[i][0] : 0.0f, 0.0f, 1.0f, 0.001f, 0.1f);
		m_PointY[i] = FloatProperty(DefaultLinearFlags, i < 4 ? def[i][1] : 0.0f, 0.0f, 1.0f, 0.001f, 0.1f);
		m_TangentX[i] = FloatProperty(DefaultLinearFlags, 0.0f, -1.0f, 1.0f, 0.001f, 0.1f);
		m_TangentY[i] = FloatProperty(DefaultLinearFlags, 0.0f, -1.0f, 1.0f, 0.001f, 0.1f);
		snprintf(key, sizeof key, "PointX%u", i); snprintf(hint, sizeof hint, "Point %u time", i);
		RegisterSharedProperty(m_PointX[i], key, hint);
		snprintf(key, sizeof key, "PointY%u", i); snprintf(hint, sizeof hint, "Point %u level", i);
		RegisterSharedProperty(m_PointY[i], key, hint);
		snprintf(key, sizeof key, "TangentX%u", i); snprintf(hint, sizeof hint, "Point %u handle time", i);
		RegisterSharedProperty(m_TangentX[i], key, hint);
		snprintf(key, sizeof key, "TangentY%u", i); snprintf(hint, sizeof hint, "Point %u handle level", i);
		RegisterSharedProperty(m_TangentY[i], key, hint);
	}
}

ComplexEnvelopePlugin::~ComplexEnvelopePlugin()
{
}

PluginInfo &ComplexEnvelopePlugin::Initialise(const HostInfo *Host)
{
	PluginInfo &Info = SpiralPlugin::Initialise(Host);
	Reserve();
	BuildEnvelope();
	return Info;
}

unsigned ComplexEnvelopePlugin::LiveCount() const
{
	if (AsSigned(m_Count) < 2)
		return 2;
	return AsSigned(m_Count) > (int)MaxPoints ? MaxPoints : (unsigned)AsSigned(m_Count);
}

void ComplexEnvelopePlugin::Reserve()
{
	const int rate = m_HostInfo ? m_HostInfo->SAMPLERATE : 44100;
	if (rate == m_ReservedRate && !m_Envelope.empty())
		return;

	m_ReservedRate = rate;
	m_Envelope.assign((size_t)(MAX_LENGTH * (float)rate) + 1, 0.0f);
	m_Points.reserve(MaxPoints);
	m_Tangents.reserve(MaxPoints);
	m_Curve.reserve(1 + MaxPoints * CurveSections);
	m_EnvelopeLength = 0;
	m_Stale = true;
}

void ComplexEnvelopePlugin::BuildEnvelope()
{
	const unsigned count = LiveCount();
	float length = AsFloat(m_Length);
	if (length < MIN_LENGTH)
		length = MIN_LENGTH;
	if (length > MAX_LENGTH)
		length = MAX_LENGTH;

	m_Points.clear();
	m_Tangents.clear();
	for (unsigned i = 0; i < count; ++i)
	{
		m_Points.push_back(Spiral::CurvePoint(AsFloat(m_PointX[i]), AsFloat(m_PointY[i])));
		m_Tangents.push_back(Spiral::CurvePoint(AsFloat(m_TangentX[i]), AsFloat(m_TangentY[i])));
	}

	for (size_t a = 1; a < m_Points.size(); ++a)
	{
		const Spiral::CurvePoint key = m_Points[a];
		const Spiral::CurvePoint tangent = m_Tangents[a];
		size_t b = a;
		while (b > 0 && m_Points[b - 1].x > key.x)
		{
			m_Points[b] = m_Points[b - 1];
			m_Tangents[b] = m_Tangents[b - 1];
			--b;
		}
		m_Points[b] = key;
		m_Tangents[b] = tangent;
	}

	int mode = AsSigned(m_Mode);
	if (mode < Spiral::StraightCurve || mode > Spiral::BezierCurve)
		mode = Spiral::SmoothCurve;

	Spiral::BuildBreakpointCurve(m_Points, (Spiral::CurveMode)mode, CurveSections, &m_Curve, &m_Tangents);

	size_t want = (size_t)(length * (float)m_ReservedRate);
	if (want < 1)
		want = 1;
	if (want > m_Envelope.size())
		want = m_Envelope.size();

	m_EnvelopeLength = want;
	Spiral::RasteriseBreakpointCurve(m_Curve, &m_Envelope[0], m_EnvelopeLength);
	m_Stale = false;
}

void ComplexEnvelopePlugin::Reset()
{
	ResetPorts();
	Reserve();
	BuildEnvelope();
	m_Position = -1;
	m_Triggered = false;
	AsFloat(m_Playhead) = 0.0f;
}

void ComplexEnvelopePlugin::Execute()
{
	if (m_Stale)
	{
		Reserve();
		BuildEnvelope();
	}

	const int samples = m_HostInfo->BUFSIZE;
	const long length = (long)m_EnvelopeLength;
	const bool scaling = InputExists(1);
	const bool gated = InputExists(0);

	for (int n = 0; n < samples; ++n)
	{
		const bool high = gated && GetInput(0, n) > 0;
		if (high && !m_Triggered)
			m_Position = 0;
		m_Triggered = high;

		if (m_Position >= length)
			m_Position = -1;

		if (m_Position < 0)
		{
			SetOutput(0, n, 0);
			SetOutput(1, n, 0);
			continue;
		}

		const float env = m_Envelope[(size_t)m_Position];
		SetOutput(0, n, env);
		SetOutput(1, n, scaling ? GetInput(1, n) * env : 0);
		++m_Position;
	}

	AsFloat(m_Playhead) = (m_Position < 0 || length == 0) ? 0.0f : (float)m_Position / (float)length;
}

void ComplexEnvelopePlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(DeviceVersion).Separator(" ").Value(m_Count).Separator(" ").Value(m_Length).Separator(" ").Value(m_Mode);
	for (unsigned i = 0; i < MaxPoints; ++i)
		d.Separator(" ").Value(m_PointX[i]).Separator(" ").Value(m_PointY[i])
			.Separator(" ").Value(m_TangentX[i]).Separator(" ").Value(m_TangentY[i]);
}

void ComplexEnvelopePlugin::Apply(Spiral::Core::Description::Reader &r)
{
	int version = 0;
	r.Value(version);
	if (version == 1)
	{
		int position = 0, bezier = 0, count = 0;
		r.Value(position).Value(m_Length).Value(bezier).Value(count);
		AsSigned(m_Mode) = bezier ? Spiral::BezierCurve : Spiral::StraightCurve;
		AsSigned(m_Count) = 0;
		for (int i = 0; i < count && !r.Failed(); ++i)
		{
			float x = 0, y = 0;
			r.Value(x).Value(y);
			if (i >= (int)MaxPoints)
				continue;
			AsFloat(m_PointX[i]) = x;
			AsFloat(m_PointY[i]) = y;
			AsFloat(m_TangentX[i]) = AsFloat(m_TangentY[i]) = 0;
			AsSigned(m_Count) = i + 1;
		}
		m_Stale = true;
		return;
	}

	r.Value(m_Count).Value(m_Length).Value(m_Mode);
	for (unsigned i = 0; i < MaxPoints; ++i)
		r.Value(m_PointX[i]).Value(m_PointY[i]).Value(m_TangentX[i]).Value(m_TangentY[i]);
	m_Stale = true;
}
