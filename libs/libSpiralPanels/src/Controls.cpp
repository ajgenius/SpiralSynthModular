// SPDX-License-Identifier: GPL-2.0-or-later
#include "Controls.h"
#include "JSON.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace Spiral
{
	namespace Panels
	{
		ViewRegistry &ViewRegistry::Get()
		{
			static ViewRegistry registry;

			return registry;
		}

		void ViewRegistry::Register(const std::string &type, Factory factory)
		{
			m_Factories[type] = factory;
		}

		ControlView *ViewRegistry::Create(const std::string &type, PanelControl &control, const Spumoni::JSON &node) const
		{
			std::map<std::string, Factory>::const_iterator found = m_Factories.find(type);

			return found == m_Factories.end() ? NULL : found->second(control, node);
		}

		template <class View>
		static View *MakeView(const char *type, PanelControl &control, const Spumoni::JSON &node)
		{
			return dynamic_cast<View *>(ViewRegistry::Get().Create(type, control, node));
		}

		template <class Made>
		static PanelControl *Viewed(Made *made)
		{
			if (made->View())
				return made;

			delete made;

			return NULL;
		}

		static PanelControlRegistry::Entry s_Knob("Knob", &KnobControl::Create);
		static PanelControlRegistry::Entry s_Slider("Slider", &SliderControl::Create);
		static PanelControlRegistry::Entry s_CheckBox("CheckBox", &CheckBoxControl::Create);
		static PanelControlRegistry::Entry s_Label("Label", &LabelControl::Create);
		static PanelControlRegistry::Entry s_RadioButton("RadioButton", &CheckBoxControl::CreateRadio);
		static PanelControlRegistry::Entry s_Spinner("Spinner", &SpinnerControl::Create);
		static PanelControlRegistry::Entry s_TextEntry("TextEntry", &TextEntryControl::Create);
		static PanelControlRegistry::Entry s_Choice("Choice", &ChoiceControl::Create);
		static PanelControlRegistry::Entry s_Button("Button", &ButtonControl::Create);
		static PanelControlRegistry::Entry s_Notebook("Notebook", &NotebookControl::Create);
		static PanelControlRegistry::Entry s_ScrollArea("VirtualScrollArea", &ScrollAreaControl::Create);

		static Dialogs *s_Dialogs = NULL;

		Dialogs *Dialogs::Get()
		{
			return s_Dialogs;
		}

		void Dialogs::Set(Dialogs *dialogs)
		{
			s_Dialogs = dialogs;
		}

		static const PanelControl::Event Pressed = {"pressed", {"value", "text", NULL}};
		static const PanelControl::Event On = {"on", {"value", "text", NULL}};
		static const PanelControl::Event Off = {"off", {"value", "text", NULL}};
		static const PanelControl::Event Chosen = {"filename_changed", {"value", "text", NULL}};

		PanelControl *KnobControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new KnobControl(panel, node));
		}

		KnobControl::KnobControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Knob(MakeView<ValueView>("Knob", *this, node))
		{
			if (!m_Knob)
				return;

			m_Knob->SetRange(std::min(Mapped(m_Min), Mapped(m_Max)), std::max(Mapped(m_Min), Mapped(m_Max)));

			if (m_Step > 0)
				m_Knob->SetStep(m_Step);

			Adopt(m_Knob);
		}

		void KnobControl::Load()
		{
			if (m_MinProperty || m_MaxProperty)
			{
				const double low = m_MinProperty ? Mapped(m_Panel.Read(*m_MinProperty)) : m_Knob->Low();
				const double high = m_MaxProperty ? Mapped(m_Panel.Read(*m_MaxProperty)) : m_Knob->High();

				if (low < high)
					m_Knob->SetRange(low, high);
			}

			double linked;

			if (Property())
			{
				const double value = Value();
				m_Knob->SetValue(value);
				Link(value);
			}
			else if (LinkedValue(linked))
				m_Knob->SetValue(linked);
		}

		void KnobControl::Changed(double value)
		{
			Commit(value);
		}

		PanelControl *SliderControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new SliderControl(panel, node));
		}

		SliderControl::SliderControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Slider(NULL),
			m_ValueLabel(TextOf(node.Get("value_label"))),
			m_StepFraction(NumberOf(node.Get("step_fraction"), 0)),
			m_Vertical(false),
			m_Flip(false)
		{
			const Spumoni::JSON *vertical = node.Get("vertical");
			const Spumoni::JSON *inverted = node.Get("inverted");
			const bool isInverted = inverted && inverted->AsBool();
			m_Vertical = vertical ? vertical->AsBool() : m_H > m_W;
			m_Flip = m_Vertical ? !isInverted : isInverted;
			m_Slider = MakeView<SliderView>("Slider", *this, node);

			if (!m_Slider)
				return;

			if (m_Flip)
				m_Slider->SetRange(Mapped(m_Max), Mapped(m_Min));
			else
				m_Slider->SetRange(Mapped(m_Min), Mapped(m_Max));

			if (m_Step > 0)
				m_Slider->SetStep(m_Step);

			Adopt(m_Slider);
		}

		void SliderControl::Load()
		{
			if (m_MinProperty || m_MaxProperty)
			{
				double low = m_Flip ? m_Slider->High() : m_Slider->Low();
				double high = m_Flip ? m_Slider->Low() : m_Slider->High();

				if (m_MinProperty)
					low = Mapped(m_Panel.Read(*m_MinProperty));

				if (m_MaxProperty)
					high = Mapped(m_Panel.Read(*m_MaxProperty));

				if (low < high)
				{
					m_Slider->SetRange(m_Flip ? high : low, m_Flip ? low : high);

					if (m_StepFraction > 0)
						m_Slider->SetStep(m_StepFraction * (high - low));
				}
			}

			if (Property())
			{
				m_Slider->SetValue(Value());
				ShowValue(Physical());
			}
		}

		void SliderControl::Refresh()
		{
			if (m_MaxProperty || (Property() && Property()->IsReadOnly()))
				Load();
		}

		void SliderControl::ShowValue(double value)
		{
			if (m_ValueLabel.empty())
				return;

			char text[64];
			std::snprintf(text, sizeof(text), ("    " + m_ValueLabel + "   ").c_str(), value);
			m_Slider->SetValueLabel(text, value < 0);
		}

		void SliderControl::Changed(double value)
		{
			Commit(value);
			ShowValue(Unmapped(value));
		}

		PanelControl *CheckBoxControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new CheckBoxControl(panel, node, "CheckBox"));
		}

		PanelControl *CheckBoxControl::CreateRadio(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new CheckBoxControl(panel, node, "RadioButton"));
		}

		CheckBoxControl::CheckBoxControl(Panel &panel, const Spumoni::JSON &node, const char *type):
			PanelControl(panel, node),
			m_Box(MakeView<CheckView>(type, *this, node)),
			m_HasChoice(node.Get("choice") != NULL),
			m_Choice(NumberOf(node.Get("choice"), 0))
		{
			if (m_Box)
				Adopt(m_Box);
		}

		void CheckBoxControl::Load()
		{
			if (!Property())
				return;

			const double value = Physical();
			m_Box->SetChecked(m_HasChoice ? value == m_Choice : value != 0);
		}

		void CheckBoxControl::Changed(bool checked)
		{
			if (!m_HasChoice)
				Commit(checked ? 1 : 0);
			else if (checked)
				Commit(m_Choice);
		}

		PanelControl *LabelControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new LabelControl(panel, node));
		}

		LabelControl::LabelControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Label(NULL)
		{
			const Spumoni::JSON *texts = node.Get("texts");

			if (texts && texts->GetType() == Spumoni::JSON::Array)
			{
				for (size_t i = 0; i < texts->Size(); ++i)
					m_Texts.push_back(TextOf(texts->At(i)));
			}

			m_Label = MakeView<LabelView>("Label", *this, node);

			if (m_Label)
				Adopt(m_Label);
		}

		void LabelControl::Load()
		{
			if (!Property())
				return;

			// With no texts to choose from, a label on a text setting
			// shows the setting itself.
			if (m_Texts.empty())
			{
				if (Property()->GetKind() == PanelProperty::String)
					m_Label->SetText(Text());

				return;
			}

			int index = (int)Physical();

			if (index < 0)
				index = 0;

			if (index >= (int)m_Texts.size())
				index = (int)m_Texts.size() - 1;

			m_Label->SetText(m_Texts[index]);
		}

		PanelControl *SpinnerControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new SpinnerControl(panel, node));
		}

		SpinnerControl::SpinnerControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Spinner(MakeView<SpinnerView>("Spinner", *this, node))
		{
			if (!m_Spinner)
				return;

			ShowRange();
			const double step = m_Step > 0 ? (node.Get("step") ? m_Step : Span(m_Step)) : 1;
			const double page = Property() && Property()->Page() > 0 ? Span(Property()->Page()) : step * 10;
			m_Spinner->SetSteps(step, NumberOf(node.Get("page"), page));
			Adopt(m_Spinner);
		}

		double SpinnerControl::Span(double step) const
		{
			const double span = std::fabs(Mapped(m_Min + step) - Mapped(m_Min));

			if (!(span > 0))
				return span;

			const double digits = std::pow(10.0, 6 - std::ceil(std::log10(span)));

			return std::floor(span * digits + 0.5) / digits;
		}

		void SpinnerControl::ShowRange()
		{
			const double low = Mapped(m_MinProperty ? m_Panel.Read(*m_MinProperty) : m_Min);
			const double high = Mapped(m_MaxProperty ? m_Panel.Read(*m_MaxProperty) : m_Max);

			if (low != m_Spinner->Low() || high != m_Spinner->High())
				m_Spinner->SetRange(low, high);
		}

		void SpinnerControl::Load()
		{
			ShowRange();

			if (Property())
				m_Spinner->SetValue(Value());
		}

		void SpinnerControl::Refresh()
		{
			if (m_MinProperty || m_MaxProperty || m_ScalePower)
				ShowRange();
		}

		void SpinnerControl::Changed(double value)
		{
			Commit(value);
		}

		PanelControl *TextEntryControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new TextEntryControl(panel, node));
		}

		TextEntryControl::TextEntryControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Entry(MakeView<TextView>("TextEntry", *this, node)),
			m_Format(TextOf(node.Get("format"))),
			m_Below(NULL),
			m_Above(NULL)
		{
			if (m_Format.empty())
				m_Format = "%g";

			const Spumoni::JSON *below = node.Get("below");
			const Spumoni::JSON *above = node.Get("above");
			m_Below = below ? panel.FindProperty(TextOf(below->Get("property"))) : NULL;
			m_Above = above ? panel.FindProperty(TextOf(above->Get("property"))) : NULL;

			if (m_Entry)
				Adopt(m_Entry);
		}

		void TextEntryControl::Load()
		{
			if (!Property())
				return;

			if (Property()->GetKind() == PanelProperty::String)
			{
				m_Entry->SetText(Text());

				return;
			}

			char text[64];
			std::snprintf(text, sizeof(text), m_Format.c_str(), Physical());
			m_Entry->SetText(text);
		}

		void TextEntryControl::Changed(const std::string &entered)
		{
			if (!Property())
				return;

			if (Property()->GetKind() == PanelProperty::String)
			{
				CommitText(entered);

				return;
			}

			const char *text = entered.c_str();
			char *end = NULL;
			const double value = std::strtod(text, &end);

			while (*end && std::isspace((unsigned char)*end))
				++end;

			if (end == text || *end || !(std::fabs(value) <= std::numeric_limits<float>::max()))
				Load();
			else
				Commit(Ordered(value));
		}

		double TextEntryControl::Ordered(double value)
		{
			const PanelProperty *partner = m_Below ? m_Below : m_Above;

			if (!partner)
				return value;

			const double limit = m_Panel.Read(*partner);

			if (m_Below ? value <= limit : value >= limit)
				return value;

			m_Panel.Write(*partner, value);

			return limit;
		}

		PanelControl *ChoiceControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new ChoiceControl(panel, node));
		}

		ChoiceControl::ChoiceControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Choice(MakeView<ChoiceView>("Choice", *this, node))
		{
			if (!m_Choice)
				return;

			const Spumoni::JSON *items = node.Get("items");
			bool valued = false;

			if (items && items->GetType() == Spumoni::JSON::Array)
			{
				for (size_t i = 0; i < items->Size(); ++i)
				{
					const Spumoni::JSON *item = items->At(i);
					const bool thisValued = item->GetType() == Spumoni::JSON::Object && item->Get("value") != NULL;
					const int index = thisValued ? m_Choice->AddItem(TextOf(item->Get("text")), false)
							: m_Choice->AddItem(TextOf(item), true);

					if (thisValued)
						valued = true;

					m_Indices.push_back(index);
					m_Values.resize(index + 1, 0);
					m_Values[index] = thisValued ? NumberOf(item->Get("value"), 0) : (double)i;
				}
			}

			if (!valued)
				m_Values.clear();

			Adopt(m_Choice);
		}

		void ChoiceControl::Load()
		{
			if (!Property())
				return;

			const double value = Physical();

			if (m_Values.empty())
			{
				m_Choice->Select((int)value);

				return;
			}

			for (size_t i = 0; i < m_Indices.size(); ++i)
			{
				if (m_Values[m_Indices[i]] == value)
				{
					m_Choice->Select(m_Indices[i]);

					break;
				}
			}
		}

		void ChoiceControl::Changed(int index)
		{
			Commit(m_Values.empty() || index < 0 || index >= (int)m_Values.size() ? index : m_Values[index]);
		}

		PanelControl *ButtonControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			if (node.Get("list"))
				return ListChoosingButtonControl::Create(panel, node);

			if (node.Get("file"))
				return FileChoosingButtonControl::Create(panel, node);

			return Viewed(new ButtonControl(panel, node, false));
		}

		ButtonControl::ButtonControl(Panel &panel, const Spumoni::JSON &node, bool led):
			PanelControl(panel, node),
			m_Button(NULL),
			m_LED(led),
			m_TextBound(false),
			m_Text(TextOf(node.Get("text"))),
			m_HasChoice(node.Get("choice") != NULL),
			m_Choice(NumberOf(node.Get("choice"), 0))
		{
			const Spumoni::JSON *text = node.Get("text");

			if (!led)
				m_TextBound = RuleOf(panel, Spiral::Data::ValueRule::Looks(text) ? text : NULL, "text",
						TextOf(node.Get("name")), m_TextRule, m_RowSource);

			if (m_TextBound)
				m_Text = m_TextRule.Evaluate(Panel::Scope(panel, m_RowSource)).text;

			m_Button = MakeView<ButtonView>(led ? "LEDButton" : "Button", *this, node);

			if (m_Button)
				Adopt(m_Button);
		}

		ButtonControl::Mode ButtonControl::ButtonMode() const
		{
			if (m_HasChoice)
				return Radio;

			return m_LED || Property() || IsToggle() ? Toggle : Momentary;
		}

		void ButtonControl::Load()
		{
			if (!Property())
				return;

			const double value = Physical();
			m_Button->SetPressed(m_HasChoice ? value == m_Choice : value != 0);
		}

		void ButtonControl::Tick()
		{
			PanelControl::Tick();

			if (m_TextBound)
				m_Button->SetText(m_TextRule.Evaluate(Panel::Scope(m_Panel, m_RowSource)).text);
		}

		void ButtonControl::Changed(bool pressed)
		{
			if (IsToggle() && !pressed)
			{
				Fired(0, m_LED ? std::string() : m_Text);

				return;
			}

			if (!m_LED && HasHandlers() && !Property())
			{
				Fired(pressed ? 1 : 0, m_Text);

				return;
			}

			if (!m_HasChoice)
				Commit(pressed ? 1 : 0);
			else if (pressed)
				Commit(m_Choice);
		}

		void ButtonControl::DeclareEvents(std::vector<Event> &out) const
		{
			PanelControl::DeclareEvents(out);
			out.push_back(Pressed);
			out.push_back(On);
			out.push_back(Off);
		}

		PanelControl *ListChoosingButtonControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new ListChoosingButtonControl(panel, node));
		}

		ListChoosingButtonControl::ListChoosingButtonControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Button(MakeView<ButtonView>("ListChoosingButton", *this, node))
		{
			const Spumoni::JSON *list = node.Get("list");
			const Spumoni::JSON *options = list ? list->Get("options") : NULL;

			for (size_t i = 0; options && i < options->Size(); ++i)
				m_Options.push_back(TextOf(options->At(i)));

			if (m_Button)
				Adopt(m_Button);
		}

		void ListChoosingButtonControl::Clicked()
		{
			Finish(Dialogs::Get() ? Dialogs::Get()->ChooseFrom(m_Options) : -1);
		}

		void ListChoosingButtonControl::Finish(int choice)
		{
			if (choice < 0 || choice >= (int)m_Options.size())
				return;

			Fire("value_changed", choice, m_Options[choice]);
		}

		struct FileChoosingButtonControl::Request
		{
			FileChoosingButtonControl *Owner;
		};

		PanelControl *FileChoosingButtonControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new FileChoosingButtonControl(panel, node));
		}

		bool FileChoosingButtonControl::ValidExtension(const std::string &extension)
		{
			if (extension.empty())
				return true;

			if (extension.size() < 2 || extension.size() > 16 || extension[0] != '.')
				return false;

			for (size_t i = 1; i < extension.size(); ++i)
			{
				if (!std::isalnum((unsigned char)extension[i]))
					return false;
			}

			return true;
		}

		FileChoosingButtonControl::FileChoosingButtonControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Button(NULL),
			m_Target(NULL),
			m_Then(NULL),
			m_Row(0),
			m_Save(false),
			m_Pending(false),
			m_Request(NULL)
		{
			const Spumoni::JSON *file = node.Get("file");

			if (file)
			{
				m_Target = panel.FindProperty(TextOf(file->Get("property")));
				m_Then = file->Get("then");
				const Spumoni::JSON *save = file->Get("save");
				m_Save = save && save->GetType() == Spumoni::JSON::Boolean && save->AsBool();
				m_Title = TextOf(file->Get("title"));
				m_Extension = TextOf(file->Get("extension"));

				if (!ValidExtension(m_Extension))
					m_Extension.clear();
			}

			if (m_Title.empty())
				m_Title = m_Save ? "Save file" : "Open file";

			m_Button = MakeView<ButtonView>("FileChoosingButton", *this, node);

			if (m_Button)
				Adopt(m_Button);
		}

		FileChoosingButtonControl::~FileChoosingButtonControl()
		{
			if (m_Request)
				m_Request->Owner = NULL;
		}

		void FileChoosingButtonControl::Clicked(bool pressed)
		{
			if (IsToggle() && !pressed)
				Fire("off", 0);
			else
				Choose();
		}

		void FileChoosingButtonControl::Choose()
		{
			if (m_Pending || !Dialogs::Get())
				return;

			m_Pending = true;
			m_Row = m_Panel.RowNumber(RowSourceName());
			m_Request = new Request;
			m_Request->Owner = this;
			const std::string filter = m_Extension.empty() ? std::string() : "*" + m_Extension;
			Dialogs::Get()->ChooseFile(m_Save, m_Title, filter, Complete, m_Request);
		}

		void FileChoosingButtonControl::Complete(void *data, const char *path)
		{
			Request *request = (Request *)data;
			FileChoosingButtonControl *owner = request->Owner;
			delete request;

			if (owner)
			{
				owner->m_Request = NULL;
				owner->Finish(path);
			}
		}

		void FileChoosingButtonControl::Finish(const char *path)
		{
			m_Pending = false;

			if (!path || !*path)
			{
				if (IsToggle())
				{
					m_Button->SetPressed(false);
					Fire("off", 0);
				}

				return;
			}

			std::string chosen = path;

			if (m_Save && !m_Extension.empty())
			{
				const size_t slash = chosen.find_last_of('/');
				const size_t dot = chosen.find_last_of('.');

				if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
					chosen += m_Extension;
			}

			m_Filename = chosen;

			if (m_Target)
				m_Panel.WriteText(*m_Target, m_Filename);

			// Each step is a write, as a handler's is: a command with its
			// arguments, the path as "text" and the row the button's source is
			// on as "row", or a text property given the word.
			for (size_t i = 0; m_Then && m_Then->GetType() == Spumoni::JSON::Array && i < m_Then->Size(); ++i)
			{
				const Spumoni::JSON &step = *m_Then->At(i);
				const PanelProperty *target = m_Panel.FindProperty(TextOf(step.Get("property")));
				const Spumoni::JSON *with = step.Get("with");
				const std::string word = TextOf(step.Get("value"));
				std::vector<std::string> arguments;

				for (size_t w = 0; with && with->GetType() == Spumoni::JSON::Array && w < with->Size(); ++w)
				{
					const Spumoni::JSON &given = *with->At(w);

					if (given.GetType() == Spumoni::JSON::String && given.Text() == "text")
						arguments.push_back(m_Filename);
					else if (given.GetType() == Spumoni::JSON::String && given.Text() == "row")
						arguments.push_back(TextOfNumber((double)m_Row));
					else if (given.GetType() == Spumoni::JSON::Number)
						arguments.push_back(TextOfNumber(NumberOf(&given, 0)));
				}

				if (!target || word.empty())
					continue;

				if (target->IsCommand())
					m_Panel.Command(word, arguments);
				else
					m_Panel.WriteText(*target, word);
			}

			Fire("filename_changed", 1, m_Filename);
		}

		void FileChoosingButtonControl::DeclareEvents(std::vector<Event> &out) const
		{
			PanelControl::DeclareEvents(out);
			out.push_back(Chosen);
			out.push_back(Off);
		}

		PanelControl *NotebookControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new NotebookControl(panel, node));
		}

		NotebookControl::NotebookControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node)
		{
			ContainerView *view = MakeView<ContainerView>("Notebook", *this, node);

			if (!view)
				return;

			const int tabs = (int)NumberOf(node.Get("tab_height"), 20);
			const int padding = (int)NumberOf(node.Get("page_padding"), 4);
			const int edge = padding > 0 ? 2 : 0;
			const int pageX = m_X + padding;
			const int pageY = m_Y + tabs + edge;
			const int pageW = m_W - 2 * padding;
			const int pageH = m_H - tabs - edge - padding;
			const Spumoni::JSON *pages = node.Get("pages");

			for (size_t i = 0; pages && pages->GetType() == Spumoni::JSON::Array && i < pages->Size(); ++i)
			{
				const Spumoni::JSON &page = *pages->At(i);
				view->BeginPage(TextOf(page.Get("title")), pageX, pageY, pageW > 0 ? pageW : 0, pageH > 0 ? pageH : 0);
				const Spumoni::JSON *children = page.Get("children");

				if (children)
					m_Panel.BuildChildren(*children, pageX, pageY);

				view->EndPage();
			}

			view->End(0, 0, false, 0, 0);
			Adopt(view);
		}

		PanelControl *ScrollAreaControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new ScrollAreaControl(panel, node));
		}

		ScrollAreaControl::ScrollAreaControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node)
		{
			ContainerView *view = MakeView<ContainerView>("VirtualScrollArea", *this, node);

			if (!view)
				return;

			const Spumoni::JSON *children = node.Get("children");

			if (children)
				m_Panel.BuildChildren(*children, m_X, m_Y);

			const Spumoni::JSON *size = node.Get("content_size");
			const Spumoni::JSON *shown = node.Get("view");
			const bool sized = size && size->Size() == 2;
			const bool viewed = shown && shown->Size() == 2;
			view->End(sized ? (int)NumberOf(size->At(0), 0) : 0, sized ? (int)NumberOf(size->At(1), 0) : 0, viewed,
					viewed ? (int)NumberOf(shown->At(0), 0) : 0, viewed ? (int)NumberOf(shown->At(1), 0) : 0);
			Adopt(view);
		}
	}
}
