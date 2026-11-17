// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_INPUT_H
#define SPIRAL_PANELS_INPUT_H

#include "BreakpointCurve.h"
#include "Controls.h"
#include <string>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		class TableView: public virtual ControlView
		{
		public:
			virtual void SetValues(const std::vector<unsigned char> &values) = 0;
			virtual void SetRows(int rows) = 0;
			virtual void SetPosition(int position) = 0;
		};

		class CurveView: public virtual ControlView
		{
		public:
			virtual void Redraw() = 0;
			virtual void PlotSize(double &w, double &h) const = 0;
		};

		class ValueTableControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();
			void Act(int row, int column, int value);
			int Clamp(int row, int value) const;
			void DeclareEvents(std::vector<Event> &out) const;

		protected:
			ValueTableControl(Panel &panel, const Spumoni::JSON &node);

			TableView *m_Table;
			bool m_Bounded;
			int m_Low, m_High, m_FirstLow;
		};

		class EnvelopeCurveControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();
			virtual void SetAttribute(const std::string &attribute, double value);

			unsigned MaxCount() const { return m_MaxCount; }
			const std::vector<CurvePoint> &Points() const { return m_Points; }
			const std::vector<CurvePoint> &Tangents() const { return m_Tangents; }
			int Mode() const { return m_ModeShown; }
			double Playhead() const { return m_PlayheadShown; }
			double Length() const { return m_LengthShown; }
			CurvePoint Handle(size_t index, int side) const;
			bool Dragging() const { return m_Drag >= 0; }

			void AddPoint(double x, double y);
			void RemovePoint(size_t index);
			void ResetHandle(size_t index);
			void BeginDrag(int index, int side);
			void DragTo(double x, double y);
			void EndDrag();

		protected:
			EnvelopeCurveControl(Panel &panel, const Spumoni::JSON &node);
			static std::string IndexedKey(const std::string &pattern, unsigned index);
			double ReadNamed(const std::string &name, double fallback) const;
			void WriteNamed(const std::string &name, double value);
			void WriteRow(unsigned index);
			void WriteAll();
			void Edited(int moved);
			void SetCount(unsigned count);
			int SubdivideLongest();
			int RemoveLeastSignificant();
			void ShowPlayhead();

			CurveView *m_Curve;
			std::string m_Mode, m_Playhead, m_Length, m_Count;
			std::string m_XPattern, m_YPattern, m_TangentXPattern, m_TangentYPattern;
			unsigned m_MaxCount;
			int m_Hold;
			std::vector<CurvePoint> m_Points, m_Tangents;
			int m_ModeShown;
			double m_PlayheadShown, m_LengthShown;
			int m_Drag, m_DragSide;
		};
	}
}

#endif
