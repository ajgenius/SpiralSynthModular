// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_CONTROLS_H
#define SPIRAL_PANELS_CONTROLS_H

#include "PanelControl.h"
#include <map>
#include <string>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		class ViewRegistry
		{
		public:
			typedef ControlView *(*Factory)(PanelControl &control, const Spumoni::JSON &node);

			class Entry
			{
			public:
				Entry(const char *type, Factory factory)
				{
					ViewRegistry::Get().Register(type, factory);
				}
			};

			static ViewRegistry &Get();

			void Register(const std::string &type, Factory factory);
			ControlView *Create(const std::string &type, PanelControl &control, const Spumoni::JSON &node) const;

		private:
			std::map<std::string, Factory> m_Factories;
		};

		class ValueView: public virtual ControlView
		{
		public:
			virtual void SetRange(double low, double high) = 0;
			virtual double Low() const = 0;
			virtual double High() const = 0;
			virtual void SetStep(double step) = 0;
			virtual void SetValue(double value) = 0;
		};

		class SliderView: public virtual ValueView
		{
		public:
			virtual void SetValueLabel(const std::string &text, bool left) = 0;
		};

		class CheckView: public virtual ControlView
		{
		public:
			virtual void SetChecked(bool checked) = 0;
		};

		class LabelView: public virtual ControlView
		{
		public:
			virtual void SetText(const std::string &text) = 0;
		};

		class SpinnerView: public virtual ValueView
		{
		public:
			virtual void SetSteps(double step, double page) = 0;
		};

		class TextView: public virtual ControlView
		{
		public:
			virtual void SetText(const std::string &text) = 0;
		};

		class ChoiceView: public virtual ControlView
		{
		public:
			virtual int AddItem(const std::string &text, bool literal) = 0;
			virtual void Select(int index) = 0;
		};

		class DynamicChoiceView: public virtual ChoiceView
		{
		public:
			virtual void ClearItems() = 0;
		};

		class ButtonView: public virtual ControlView
		{
		public:
			virtual void SetText(const std::string &text) = 0;
			virtual void SetPressed(bool pressed) = 0;
		};

		class ContainerView: public virtual ControlView
		{
		public:
			virtual void BeginPage(const std::string &title, int x, int y, int w, int h) = 0;
			virtual void EndPage() = 0;
			virtual void End(int contentW, int contentH, bool hasView, int viewX, int viewY) = 0;
		};

		class Dialogs
		{
		public:
			typedef void (*Completed)(void *request, const char *path);

			virtual ~Dialogs() {}
			virtual int ChooseFrom(const std::vector<std::string> &options) = 0;
			virtual void ChooseFile(bool save, const std::string &title, const std::string &filter, Completed done,
					void *request) = 0;

			static Dialogs *Get();
			static void Set(Dialogs *dialogs);
		};

		class KnobControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			void Changed(double value);

		protected:
			KnobControl(Panel &panel, const Spumoni::JSON &node);

			ValueView *m_Knob;
		};

		class SliderControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();
			void Changed(double value);
			bool Vertical() const { return m_Vertical; }
			bool HasValueLabel() const { return !m_ValueLabel.empty(); }

		protected:
			SliderControl(Panel &panel, const Spumoni::JSON &node);
			void ShowValue(double value);

			SliderView *m_Slider;
			std::string m_ValueLabel;
			double m_StepFraction;
			bool m_Vertical, m_Flip;
		};

		class CheckBoxControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);
			static PanelControl *CreateRadio(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			void Changed(bool checked);

		protected:
			CheckBoxControl(Panel &panel, const Spumoni::JSON &node, const char *type);

			CheckView *m_Box;
			bool m_HasChoice;
			double m_Choice;
		};

		class LabelControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			bool Indexed() const { return !m_Texts.empty(); }
			void Show(const std::string &text) { m_Label->SetText(text); }

		protected:
			LabelControl(Panel &panel, const Spumoni::JSON &node);

			LabelView *m_Label;
			std::vector<std::string> m_Texts;
		};

		class SpinnerControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();
			void Changed(double value);

		protected:
			SpinnerControl(Panel &panel, const Spumoni::JSON &node);
			double Span(double step) const;
			void ShowRange();

			SpinnerView *m_Spinner;
		};

		class TextEntryControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			void Changed(const std::string &text);

		protected:
			TextEntryControl(Panel &panel, const Spumoni::JSON &node);
			double Ordered(double value);

			TextView *m_Entry;
			std::string m_Format;
			const PanelProperty *m_Below;
			const PanelProperty *m_Above;
		};

		class ChoiceControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Tick();
			void Changed(int index);

		protected:
			ChoiceControl(Panel &panel, const Spumoni::JSON &node);

			ChoiceView *m_Choice;
			const PanelProperty *m_ItemsProperty;
			std::string m_ItemText;
			std::vector<std::string> m_Items;
			std::vector<double> m_Values;
			std::vector<int> m_Indices;
		};

		class ButtonControl: public PanelControl
		{
		public:
			enum Mode
			{
				Momentary,
				Toggle,
				Radio
			};

			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);
			static PanelControl *CreateLED(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Tick();
			void Changed(bool pressed);
			Mode ButtonMode() const;
			const std::string &Label() const { return m_Text; }

		protected:
			ButtonControl(Panel &panel, const Spumoni::JSON &node, bool led);
			void DeclareEvents(std::vector<Event> &out) const;

			ButtonView *m_Button;
			bool m_LED;
			Spiral::Data::ValueRule m_TextRule;
			bool m_TextBound;
			std::string m_Text;
			bool m_HasChoice;
			double m_Choice;
		};

		class ListChoosingButtonControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			size_t OptionCount() const { return m_Options.size(); }
			void Clicked();
			void Finish(int choice);

		protected:
			ListChoosingButtonControl(Panel &panel, const Spumoni::JSON &node);

			ButtonView *m_Button;
			std::vector<std::string> m_Options;
		};

		class FileChoosingButtonControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);
			virtual ~FileChoosingButtonControl();

			static bool ValidExtension(const std::string &extension);
			const std::string &Filename() const { return m_Filename; }
			const std::string &Extension() const { return m_Extension; }
			bool Save() const { return m_Save; }
			bool Pending() const { return m_Pending; }
			virtual bool Drivable() const { return false; }
			void Clicked(bool pressed);
			void Finish(const char *path);

		protected:
			FileChoosingButtonControl(Panel &panel, const Spumoni::JSON &node);
			void Choose();
			void DeclareEvents(std::vector<Event> &out) const;
			static void Complete(void *request, const char *path);
			struct Request;

			ButtonView *m_Button;
			const PanelProperty *m_Target;
			// What the panel does with a chosen file, beyond keeping its path.
			const Spumoni::JSON *m_Then;
			// The row the button's source was on when the chooser opened: the
			// file is for that row, wherever the cursor is when one is chosen.
			long m_Row;
			bool m_Save, m_Pending;
			std::string m_Title, m_Extension, m_Filename;
			Request *m_Request;
		};

		class NotebookControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		protected:
			NotebookControl(Panel &panel, const Spumoni::JSON &node);
		};

		class ScrollAreaControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		protected:
			ScrollAreaControl(Panel &panel, const Spumoni::JSON &node);
		};
	}
}

#endif
