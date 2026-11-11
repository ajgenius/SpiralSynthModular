// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_METERS_H
#define SPIRAL_PANELS_METERS_H

#include "Controls.h"
#include <string>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		class Feed;

		class ReadoutView: public virtual ControlView
		{
		public:
			virtual void SetReadout(const std::string &text, int cursor) = 0;
		};

		class DigitsView: public virtual ControlView
		{
		public:
			enum Dot
			{
				NoDot,
				Point,
				Colon
			};

			virtual void SetDigit(size_t index, int value, Dot dot) = 0;
		};

		class MeterView: public virtual ControlView
		{
		public:
			virtual void SetMode(bool vu) = 0;
			virtual void SetLevel(float level) = 0;
		};

		class TraceView: public virtual ControlView
		{
		public:
			virtual void SetSamples(const std::vector<float> &samples) = 0;
			virtual void SetAttenuation(double attenuation) = 0;
			virtual void SetTimeBase(double timeBase) = 0;
		};

		class PlotView: public virtual ControlView
		{
		public:
			virtual void SetValues(const std::vector<float> &values) = 0;
		};

		class SampleView: public virtual ControlView
		{
		public:
			enum Marker
			{
				PlayStart,
				LoopStart,
				LoopEnd
			};

			virtual void SetSample(const std::vector<float> &samples) = 0;
			virtual void SetMarker(Marker marker, long position) = 0;
			virtual long GetMarker(Marker marker) const = 0;
			virtual void SetPlayPosition(long position) = 0;
			virtual void ShowPositionMarker(bool shown) = 0;
			virtual void Zoom(bool in) = 0;
			virtual long RangeStart() const = 0;
			virtual long RangeEnd() const = 0;
			virtual bool Displayed() const = 0;
			virtual void Redraw() = 0;
		};

		class LoopView: public virtual ControlView
		{
		public:
			virtual void SetData(const std::vector<float> &samples) = 0;
			virtual void SetLength(int length) = 0;
			virtual int Length() const = 0;
			virtual void SetPosition(float position) = 0;
			virtual void SelectAll() = 0;
			virtual void SetWaveSize(float size) = 0;
			virtual void AddTrigger(int id, int channel, float angle) = 0;
			virtual long RangeStart() const = 0;
			virtual long RangeEnd() const = 0;
			virtual void Redraw() = 0;
		};

		class LCDControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void SetAttribute(const std::string &attribute, double value);

		protected:
			LCDControl(Panel &panel, const Spumoni::JSON &node);
			void Show(const std::string &text, int cursor);

			ReadoutView *m_Readout;
			std::string m_Format, m_Empty, m_Shown;
		};

		class LEDControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void SetAttribute(const std::string &attribute, double value);
			void Changed(bool lit);

		protected:
			LEDControl(Panel &panel, const Spumoni::JSON &node);

			CheckView *m_LED;
			bool m_HasChoice;
			double m_Choice;
			bool m_Latch;
		};

		class SevenSegmentControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void SetAttribute(const std::string &attribute, double value);
			int Digits() const { return m_Digits; }
			bool Time() const { return m_Time; }

		protected:
			SevenSegmentControl(Panel &panel, const Spumoni::JSON &node);
			void Show(double value);

			DigitsView *m_Display;
			int m_Digits;
			bool m_Time;
			double m_Shown;
		};

		class MeterBarControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);
			virtual ~MeterBarControl();

			virtual void Load();
			virtual void Refresh();
			virtual void SetAttribute(const std::string &attribute, double value);
			virtual bool GetAttribute(const std::string &attribute, double &value) const;

		protected:
			MeterBarControl(Panel &panel, const Spumoni::JSON &node);
			void Reset();

			MeterView *m_Meter;
			Feed *m_Feed;
			std::vector<float> m_Samples;
			bool m_VU, m_Bypass;
			double m_Datum, m_Low, m_High;
		};

		class ScopeControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);
			virtual ~ScopeControl();

			virtual void Refresh();
			virtual void SetAttribute(const std::string &attribute, double value);
			virtual bool GetAttribute(const std::string &attribute, double &value) const;

		protected:
			ScopeControl(Panel &panel, const Spumoni::JSON &node);

			TraceView *m_Trace;
			Feed *m_Feed;
			std::vector<float> m_Samples;
			double m_Attenuation, m_TimeBase;
		};

		class FunctionPlotControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();

		protected:
			FunctionPlotControl(Panel &panel, const Spumoni::JSON &node);

			PlotView *m_Plot;
			std::string m_Channel;
			std::vector<float> m_Values;
		};

		class SampleWaveformControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();
			virtual void SetAttribute(const std::string &attribute, double value);
			virtual bool GetAttribute(const std::string &attribute, double &value) const;
			virtual void RowMoved();
			void MarkerMoved();

		protected:
			SampleWaveformControl(Panel &panel, const Spumoni::JSON &node);
			void Reload();
			bool MarkersChanged();

			SampleView *m_Display;
			std::vector<float> m_Samples;
			const PanelProperty *m_Pos, *m_PlayStart, *m_LoopStart, *m_LoopEnd;
			std::string m_PosName, m_PlayStartName, m_LoopStartName, m_LoopEndName;
			void FindMarks();
			std::string m_SampleCommand, m_SizeChannel, m_BufferChannel;
			bool m_Fetched;
			long m_SentPlayStart, m_SentLoopStart, m_SentLoopEnd;
		};

		class LoopWaveformControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();
			virtual void SetAttribute(const std::string &attribute, double value);
			virtual bool GetAttribute(const std::string &attribute, double &value) const;
			void TriggerMoved(int id, int channel, float angle);

		protected:
			struct Trigger
			{
				int Id, Channel;
				float Angle;
			};

			LoopWaveformControl(Panel &panel, const Spumoni::JSON &node);
			void ShowLength();
			void Reload();
			void AddTrigger(int id, int channel, float angle);
			const PanelProperty *TriggerProperty(const std::string &pattern, int index) const;
			void SyncTriggers();
			void NewTrigger();

			LoopView *m_Loop;
			std::vector<Trigger> m_Triggers;
			std::vector<float> m_Samples;
			const PanelProperty *m_Pos, *m_Length, *m_TriggerCount;
			std::string m_SampleCommand, m_SizeChannel, m_BufferChannel;
			std::string m_TriggerChannels, m_TriggerTimes, m_NewTriggerCommand;
			bool m_Fetched;
		};
	}
}

#endif
