// SPDX-License-Identifier: GPL-2.0-or-later
#include "Fl_BoundaryDrawer.h"
#include "License.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/fl_draw.H>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

using namespace Spiral::File;

namespace
{
	const int Pad = 6;
	const int RowHeight = 22;
	const int LabelWidth = 64;
	/* The rail is the drag bar. The panel beside it opens to a width that
	   fits the fields; it will not grow past twice that. */
	const int Rail = 18;
	const int Panel = 320;
	const int PanelMax = 560;
	const int RemoveW = 22;
	const int MenuW = 26;
	const int ViewW = 56;
	int SessionPanel = 0;
	int SessionOpen = 0;

	int OpenPanel()
	{
		if (SessionOpen < 36)
			SessionOpen = Panel;

		return SessionOpen;
	}

	bool SessionCollapsed()
	{
		return SessionPanel < 36;
	}

	/* The document fields the panel edits, in display order. Credits are a
	   list and are not one of these: a document either states its credits
	   or it does not, so they get their own section rather than a field
	   row. */
	const Fl_BoundaryDrawer::Field FieldOrder[] = {
		Fl_BoundaryDrawer::FieldTitle, Fl_BoundaryDrawer::FieldDescription,
		Fl_BoundaryDrawer::FieldCopyright, Fl_BoundaryDrawer::FieldLicense
	};
	const size_t FieldCount = sizeof(FieldOrder) / sizeof(FieldOrder[0]);

	/* About is a paragraph and gets five lines; the rest are a line each. */
	const int AboutHeight = RowHeight * 4;

	int RowHeightOf(Fl_BoundaryDrawer::Field field)
	{
		return field == Fl_BoundaryDrawer::FieldDescription ? AboutHeight : RowHeight;
	}

	/* Where a field's row starts, below the ones before it. */
	int RowTop(size_t index)
	{
		int top = 0;

		for (size_t i = 0; i < index && i < FieldCount; ++i)
			top += RowHeightOf(FieldOrder[i]);

		return top;
	}

	/* The fields, the bundle checkbox, then the dates the file recorded. */
	int DocumentHeight()
	{
		return RowTop(FieldCount) + 2 * RowHeight;
	}

	const char *FieldLabel(Fl_BoundaryDrawer::Field field)
	{
		switch (field)
		{
			case Fl_BoundaryDrawer::FieldTitle:
				return "Title";
			case Fl_BoundaryDrawer::FieldDescription:
				return "About";
			case Fl_BoundaryDrawer::FieldCopyright:
				return "Copyright";
			case Fl_BoundaryDrawer::FieldLicense:
				return "Licence";
		}

		return "";
	}

	std::string &DocumentField(DocumentSection &document, Fl_BoundaryDrawer::Field field)
	{
		switch (field)
		{
			case Fl_BoundaryDrawer::FieldTitle:
				return document.Title;
			case Fl_BoundaryDrawer::FieldDescription:
				return document.Description;
			case Fl_BoundaryDrawer::FieldCopyright:
				return document.Rights.Copyright;
			case Fl_BoundaryDrawer::FieldLicense:
				break;
		}

		return document.Rights.License;
	}

	/* The host's chrome is plastic: the toolbar buttons and the plugin tabs
	   are FL_PLASTIC_UP_BOX at 10pt. The drawer follows, so it reads as
	   part of the same window rather than a stock FLTK panel. */
	void PlasticButton(Fl_Widget *button)
	{
		button->box(FL_PLASTIC_UP_BOX);
		button->labelsize(10);
	}

	void PlasticInput(Fl_Input *input)
	{
		input->box(FL_PLASTIC_DOWN_BOX);
		input->textsize(10);
		input->labelsize(10);
	}

	std::string Counted(const char *word, size_t count)
	{
		char buf[32];

		std::snprintf(buf, sizeof buf, "%u", (unsigned)count);

		return std::string(word) + " (" + buf + ")";
	}

	/* Selection in an Fl_Input is a minimal update: the box is not redrawn,
	   and the caret is a 2px bar that can stick outside the field. The
	   private tree's port name fields had the same: after the field has
	   moved its selection, widen the damage to the whole window and force
	   a full box paint, or the previous glyphs stay. */
	class Fl_PanelInput: public Fl_Input
	{
	public:
		Fl_PanelInput(int x, int y, int w, int h, const char *label = NULL):
			Fl_Input(x, y, w, h, label)
		{
		}

		int handle(int event)
		{
			const int result = Fl_Input::handle(event);

			if (event == FL_PUSH || event == FL_DRAG || event == FL_RELEASE
				|| event == FL_FOCUS || event == FL_UNFOCUS || event == FL_KEYBOARD
				|| event == FL_KEYDOWN || event == FL_PASTE)
			{
				Fl_Widget::damage(FL_DAMAGE_ALL);

				if (Fl_Window *win = window())
					win->redraw();
			}

			return result;
		}

		void draw()
		{
			if (!(damage() & FL_DAMAGE_ALL))
				Fl_Widget::damage(FL_DAMAGE_ALL);

			Fl_Input::draw();
		}
	};

	/* The paragraph editor, dressed and damaged like the inputs: its
	   scrollbars are plastic like the rest, and an edit or a selection
	   repaints the whole field rather than the caret's rectangle. */
	class Fl_PanelText: public Fl_Text_Editor
	{
	public:
		Fl_PanelText(int x, int y, int w, int h, const char *label = NULL):
			Fl_Text_Editor(x, y, w, h, label)
		{
			box(FL_PLASTIC_DOWN_BOX);
			mVScrollBar->box(FL_PLASTIC_DOWN_BOX);
			mVScrollBar->slider(FL_PLASTIC_UP_BOX);
			mHScrollBar->box(FL_PLASTIC_DOWN_BOX);
			mHScrollBar->slider(FL_PLASTIC_UP_BOX);
		}

		int handle(int event)
		{
			const int result = Fl_Text_Editor::handle(event);

			if (event == FL_PUSH || event == FL_DRAG || event == FL_RELEASE
				|| event == FL_FOCUS || event == FL_UNFOCUS || event == FL_KEYBOARD
				|| event == FL_KEYDOWN || event == FL_PASTE)
			{
				Fl_Widget::damage(FL_DAMAGE_ALL);

				if (Fl_Window *win = window())
					win->redraw();
			}

			return result;
		}

		void draw()
		{
			if (!(damage() & FL_DAMAGE_ALL))
				Fl_Widget::damage(FL_DAMAGE_ALL);

			Fl_Text_Editor::draw();
		}
	};

	/* The rows under the credits heading are rebuilt as credits come and
	   go. A plain group that paints its own box: the rows are placed by
	   hand, so FLTK's proportional resize is kept out of it. */
	class Fl_CreditRows: public Fl_Group
	{
	public:
		Fl_CreditRows(int x, int y, int w, int h):
			Fl_Group(x, y, w, h)
		{
			box(FL_FLAT_BOX);
			clip_children(1);
			end();
		}

		void draw()
		{
			fl_color(color());
			fl_rectf(x(), y(), w(), h());

			for (int i = 0; i < children(); ++i)
			{
				Fl_Widget *row = child(i);

				if (!row->visible())
					continue;

				row->damage(FL_DAMAGE_ALL);
				draw_child(*row);
			}
		}
	};
}

Fl_BoundaryDrawer::Fl_BoundaryDrawer(int x, int y, int w, int h):
	Fl_Group(x, y, w, h),
	CollapseChanged(NULL), CollapseChangedData(NULL),
	DocumentChanged(NULL), DocumentChangedData(NULL),
	m_Document(NULL),
	m_Collapsed(SessionCollapsed()), m_SplitDrag(false), m_SplitX(0), m_SplitW(Rail),
	m_DocumentSection(NULL), m_CreditSection(NULL),
	m_About(NULL), m_AboutBuffer(NULL),
	m_License(NULL), m_LicenseMenu(NULL), m_Bundle(NULL), m_ViewLicense(NULL), m_Dates(NULL), m_CreditHeader(NULL),
	m_AddCredit(NULL), m_RemoveRow(0)
{
	box(FL_NO_BOX);

	// Two sections, both plain groups: no scroll area anywhere in here,
	// because the whole point of the drawer is to be chrome outside the
	// canvas's scrolling and damage.
	m_DocumentSection = new Fl_Group(x + Rail + Pad, y + Pad,
		std::max(0, w - Rail - 2 * Pad), DocumentHeight());
	/* FLTK scales children against the first resize snapshot. These rows
	   are placed by hand; a proportional pass is what collapsed fields to
	   slivers in the private tree. */
	m_DocumentSection->resizable(NULL);

	for (size_t i = 0; i < FieldCount; ++i)
	{
		const int rowY = m_DocumentSection->y() + RowTop(i);

		if (FieldOrder[i] == FieldDescription)
		{
			m_AboutBuffer = new Fl_Text_Buffer;
			m_About = new Fl_PanelText(m_DocumentSection->x() + LabelWidth, rowY,
				std::max(0, m_DocumentSection->w() - LabelWidth), AboutHeight - 2,
				FieldLabel(FieldDescription));
			m_About->align(FL_ALIGN_LEFT_TOP);
			m_About->labelsize(10);
			m_About->textsize(10);
			m_About->buffer(m_AboutBuffer);
			m_About->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
			m_About->scrollbar_width(12);
			m_About->scrollbar_align(FL_ALIGN_RIGHT);
			m_About->when(FL_WHEN_RELEASE);
			m_About->callback(AboutEdited, this);

			continue;
		}

		if (FieldOrder[i] == FieldLicense)
		{
			const int fieldW = std::max(0, m_DocumentSection->w() - LabelWidth - MenuW);

			m_License = new Fl_PanelInput(m_DocumentSection->x() + LabelWidth, rowY,
				fieldW, RowHeight - 2, FieldLabel(FieldLicense));
			m_License->align(FL_ALIGN_LEFT);
			PlasticInput(m_License);
			m_License->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
			m_License->callback(LicenseEdited, this);

			// The known identifiers, typed or picked: the menu fills the
			// input, which is what is read.
			// No label: Fl_Menu_Button draws its own arrow, placed from
			// the label size, and at this width and size it sits centred.
			m_LicenseMenu = new Fl_Menu_Button(m_License->x() + fieldW, rowY,
				MenuW, RowHeight - 2, "");
			PlasticButton(m_LicenseMenu);
			m_LicenseMenu->labelsize(12);
			m_LicenseMenu->tooltip("Known licences");
			m_LicenseMenu->callback(LicensePicked, this);

			for (size_t n = 0; n < LicensePresetCount(); ++n)
				m_LicenseMenu->add(LicensePresetAt(n).Id);

			continue;
		}

		Fl_Input *field = new Fl_PanelInput(m_DocumentSection->x() + LabelWidth, rowY,
			m_DocumentSection->w() - LabelWidth, RowHeight - 2, FieldLabel(FieldOrder[i]));
		field->align(FL_ALIGN_LEFT);
		PlasticInput(field);
		field->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		field->callback(FieldEdited, this);
		m_Fields.push_back(field);
		m_FieldKinds.push_back(FieldOrder[i]);
	}

	m_Bundle = new Fl_Check_Button(m_DocumentSection->x(),
		m_DocumentSection->y() + RowTop(FieldCount),
		std::max(0, m_DocumentSection->w() - ViewW - Pad), RowHeight - 2, "Bundle licence text");
	m_Bundle->labelsize(10);
	m_Bundle->tooltip("Write the full licence into the project file on the next save");
	m_Bundle->callback(BundleToggled, this);

	m_ViewLicense = new Fl_Button(m_DocumentSection->x() + m_DocumentSection->w() - ViewW,
		m_DocumentSection->y() + RowTop(FieldCount), ViewW, RowHeight - 2, "View");
	PlasticButton(m_ViewLicense);
	m_ViewLicense->tooltip("Read the licence text");
	m_ViewLicense->callback(ViewLicenseClicked, this);

	// Created and saved, as the file recorded them. Not typed here.
	m_Dates = new Fl_Box(m_DocumentSection->x(),
		m_DocumentSection->y() + RowTop(FieldCount) + RowHeight,
		m_DocumentSection->w(), RowHeight - 2);
	m_Dates->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_Dates->labelsize(10);
	m_DocumentSection->end();
	m_DocumentSection->init_sizes();

	m_CreditSection = new Fl_CreditRows(x + Rail + Pad,
		m_DocumentSection->y() + m_DocumentSection->h() + Pad,
		std::max(0, w - Rail - 2 * Pad), h - m_DocumentSection->h() - 3 * Pad);
	m_CreditSection->resizable(NULL);
	m_CreditSection->end();

	end();
	resizable(NULL);

	// Nothing bound yet, so nothing to show.
	m_DocumentSection->hide();
	m_CreditSection->hide();
}

Fl_BoundaryDrawer::~Fl_BoundaryDrawer()
{
	Fl::remove_timeout(RemoveCommitted, this);

	// The editor is a child and goes with the group; the buffer is ours.
	if (m_About)
		m_About->buffer(NULL);
	delete m_AboutBuffer;
}

void Fl_BoundaryDrawer::resize(int x, int y, int w, int h)
{
	Fl_Group::resize(x, y, w, h);
	Relayout();
	/* resize() does not damage the new rectangle. Without this the
	   opened panel is a dirty strip of the window underneath. */
	redraw();
}

void Fl_BoundaryDrawer::draw()
{
	// The sections and the credit rows paint in the drawer's colour; the
	// rail is a shade off it, with a grip in the middle.
	m_DocumentSection->color(color());
	m_CreditSection->color(color());

	fl_color(color());
	fl_rectf(x(), y(), w(), h());
	// The rail is a plastic bar in the host's button colour, like the
	// toolbar beside it; the grip is a darker shade of the same.
	fl_draw_box(FL_PLASTIC_UP_BOX, x(), y(), Rail, h(), selection_color());
	fl_color(fl_darker(selection_color()));
	const int mid = y() + h() / 2;
	const int gx = x() + Rail / 2;
	for (int i = -3; i <= 3; ++i)
		fl_rectf(gx - 1, mid + i * 5, 3, 2);
	draw_children();
}

int Fl_BoundaryDrawer::handle(int event)
{
	const bool onRail = Fl::event_x() >= x() && Fl::event_x() < x() + Rail
		&& Fl::event_y() >= y() && Fl::event_y() < y() + h();

	if (event == FL_PUSH && onRail)
	{
		m_SplitDrag = true;
		m_SplitX = Fl::event_x();
		m_SplitW = CurrentWidth();
		return 1;
	}

	if (event == FL_DRAG && m_SplitDrag)
	{
		int panel = (m_SplitW - Rail) - (Fl::event_x() - m_SplitX);

		if (panel < 36)
			panel = 0;
		if (panel > PanelMax)
			panel = PanelMax;

		SetPanelWidth(panel);

		if (CollapseChanged)
			CollapseChanged(CollapseChangedData);

		return 1;
	}

	if (event == FL_RELEASE && m_SplitDrag)
	{
		m_SplitDrag = false;
		return 1;
	}

	if ((event == FL_MOVE || event == FL_ENTER || event == FL_LEAVE) && window())
		window()->cursor(onRail && event != FL_LEAVE ? FL_CURSOR_WE : FL_CURSOR_DEFAULT);

	return Fl_Group::handle(event);
}

void Fl_BoundaryDrawer::SetPanelWidth(int panel)
{
	if (panel < 36)
	{
		SessionPanel = 0;
		m_Collapsed = true;
	}
	else
	{
		SessionPanel = panel;
		SessionOpen = panel;
		m_Collapsed = false;
	}

	if (ShowsDocument() && !m_Collapsed)
	{
		m_DocumentSection->show();
		m_CreditSection->show();
	}
	else
	{
		m_DocumentSection->hide();
		m_CreditSection->hide();
	}

	redraw();
}

int Fl_BoundaryDrawer::RailWidth() const
{
	return Rail;
}

int Fl_BoundaryDrawer::ExpandedWidth() const
{
	return Rail + OpenPanel();
}

int Fl_BoundaryDrawer::CurrentWidth() const
{
	return m_Collapsed || SessionPanel < 36 ? Rail : Rail + SessionPanel;
}

bool Fl_BoundaryDrawer::Collapsed() const
{
	return m_Collapsed;
}

bool Fl_BoundaryDrawer::PanelShown() const
{
	return !m_Collapsed && ShowsDocument();
}

void Fl_BoundaryDrawer::SetCollapsed(bool collapsed)
{
	m_Collapsed = collapsed;
	// Asked open, the panel is at least its full width: a rail dragged to
	// a sliver earlier in the session is not what About Patch means.
	SessionPanel = collapsed ? 0 : std::max(OpenPanel(), Panel);

	if (!collapsed)
		SessionOpen = SessionPanel;

	Repopulate();
}

void Fl_BoundaryDrawer::Relayout()
{
	if (!m_DocumentSection)
		return;

	const int contentX = x() + Rail;
	const int contentW = std::max(0, w() - Rail - 2 * Pad);

	m_DocumentSection->resize(contentX + Pad, y() + Pad, contentW,
		DocumentHeight());

	size_t input = 0;

	for (size_t i = 0; i < FieldCount; ++i)
	{
		const int rowY = m_DocumentSection->y() + RowTop(i);
		const int fieldW = std::max(0, m_DocumentSection->w() - LabelWidth);
		const int fieldX = m_DocumentSection->x() + LabelWidth;

		if (FieldOrder[i] == FieldDescription)
		{
			if (m_About)
				m_About->resize(fieldX, rowY, fieldW, AboutHeight - 2);

			continue;
		}

		if (FieldOrder[i] == FieldLicense)
		{
			const int inputW = std::max(0, fieldW - MenuW);

			if (m_License)
				m_License->resize(fieldX, rowY, inputW, RowHeight - 2);
			if (m_LicenseMenu)
				m_LicenseMenu->resize(fieldX + inputW, rowY, MenuW, RowHeight - 2);

			continue;
		}

		if (input < m_Fields.size())
			m_Fields[input]->resize(fieldX, rowY, fieldW, RowHeight - 2);

		input++;
	}

	if (m_Bundle)
		m_Bundle->resize(m_DocumentSection->x(),
			m_DocumentSection->y() + RowTop(FieldCount),
			std::max(0, m_DocumentSection->w() - ViewW - Pad), RowHeight - 2);

	if (m_ViewLicense)
		m_ViewLicense->resize(m_DocumentSection->x() + m_DocumentSection->w() - ViewW,
			m_DocumentSection->y() + RowTop(FieldCount), ViewW, RowHeight - 2);

	if (m_Dates)
		m_Dates->resize(m_DocumentSection->x(),
			m_DocumentSection->y() + RowTop(FieldCount) + RowHeight,
			m_DocumentSection->w(), RowHeight - 2);

	m_CreditSection->resize(contentX + Pad, m_DocumentSection->y() + m_DocumentSection->h() + Pad,
		contentW, std::max(0, h() - m_DocumentSection->h() - 3 * Pad));

	PlaceCredits();
	NoteLayout();
}

void Fl_BoundaryDrawer::Bind(DocumentSection *document)
{
	m_Document = document;
	Refresh();
}

void Fl_BoundaryDrawer::Refresh()
{
	Repopulate();
}

bool Fl_BoundaryDrawer::Bound() const
{
	return m_Document != NULL;
}

bool Fl_BoundaryDrawer::ShowsDocument() const
{
	return Bound();
}

void Fl_BoundaryDrawer::Repopulate()
{
	for (size_t i = 0; i < m_Fields.size(); ++i)
	{
		const std::string text = FieldText(m_FieldKinds[i]);
		m_Fields[i]->value(text.c_str());
	}

	if (m_AboutBuffer)
	{
		const std::string text = FieldText(FieldDescription);
		char *shown = m_AboutBuffer->text();
		const bool same = shown && text == shown;
		free(shown);

		// Only when it differs: resetting the buffer moves the caret.
		if (!same)
			m_AboutBuffer->text(text.c_str());
	}

	if (m_License)
	{
		const std::string license = FieldText(FieldLicense);

		m_License->value(license.c_str());
	}

	if (m_Bundle)
	{
		m_Bundle->value(BundleLicenseText() ? 1 : 0);
		if (CanBundleLicense())
			m_Bundle->activate();
		else
			m_Bundle->deactivate();
	}

	if (m_ViewLicense)
	{
		if (CanBundleLicense())
			m_ViewLicense->activate();
		else
			m_ViewLicense->deactivate();
	}

	if (m_Dates)
	{
		std::string dates;

		if (Bound() && !m_Document->CreatedAt.empty())
			dates += "Created " + m_Document->CreatedAt;
		if (Bound() && !m_Document->SavedAt.empty())
			dates += (dates.empty() ? "" : "   ") + std::string("Saved ") + m_Document->SavedAt;

		m_Dates->copy_label(dates.c_str());
	}

	m_CreditSection->clear();
	/* FLTK 1.4 clear() sets resizable() to the group itself, and the next
	   resize then scales every row from that snapshot. NULL keeps the
	   positions PlaceCredits just set. */
	m_CreditSection->resizable(NULL);
	m_CreditHeader = NULL;
	m_CreditNames.clear();
	m_CreditRoles.clear();
	m_CreditRemove.clear();
	m_AddCredit = NULL;

	if (Bound())
	{
		m_CreditSection->begin();

		const std::vector<Credit> &credits = m_Document->Credits;

		m_CreditHeader = new Fl_Box(m_CreditSection->x(), m_CreditSection->y(),
			m_CreditSection->w(), RowHeight - 2);
		m_CreditHeader->copy_label(Counted("Credits", credits.size()).c_str());
		m_CreditHeader->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		m_CreditHeader->labelsize(10);
		m_CreditHeader->labelfont(FL_BOLD);

		for (size_t i = 0; i < credits.size(); ++i)
		{
			Fl_Input *name = new Fl_PanelInput(m_CreditSection->x(), m_CreditSection->y(),
				m_CreditSection->w(), RowHeight - 2);
			Fl_Input *role = new Fl_PanelInput(m_CreditSection->x(), m_CreditSection->y(),
				m_CreditSection->w(), RowHeight - 2);
			Fl_Button *remove = new Fl_Button(m_CreditSection->x(), m_CreditSection->y(),
				RemoveW, RowHeight - 2, "x");

			PlasticInput(name);
			name->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
			name->callback(CreditEdited, this);
			name->tooltip("Who");
			name->value(credits[i].Name.c_str());
			PlasticInput(role);
			role->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
			role->callback(CreditEdited, this);
			role->tooltip("What they did: composition, patch, sample...");
			role->value(credits[i].Role.c_str());
			PlasticButton(remove);
			remove->tooltip("Remove this credit");
			remove->callback(RemoveClicked, this);
			m_CreditNames.push_back(name);
			m_CreditRoles.push_back(role);
			m_CreditRemove.push_back(remove);
		}

		m_AddCredit = new Fl_Button(m_CreditSection->x(), m_CreditSection->y(),
			88, RowHeight - 2, "+ Credit");
		PlasticButton(m_AddCredit);
		m_AddCredit->callback(AddClicked, this);
		m_AddCredit->tooltip("Add a credit");

		m_CreditSection->end();
		PlaceCredits();
	}

	// Collapsed is the rail alone. The sections stay built so opening
	// does not have to recreate the fields.
	if (ShowsDocument() && !m_Collapsed)
	{
		m_DocumentSection->show();
		m_CreditSection->show();
	}
	else
	{
		m_DocumentSection->hide();
		m_CreditSection->hide();
	}

	NoteLayout();
	redraw();

	/* The rows were deleted and rebuilt. A child redraw leaves the old
	   boxes on the parent. Damage the window so the new geometry is all
	   that remains. */
	if (window())
		window()->redraw();
}

void Fl_BoundaryDrawer::PlaceCredits()
{
	if (!m_CreditSection)
		return;

	int rowY = m_CreditSection->y();
	const int x = m_CreditSection->x();
	const int w = m_CreditSection->w();

	if (m_CreditHeader)
	{
		m_CreditHeader->resize(x, rowY, w, RowHeight - 2);
		rowY += RowHeight;
	}

	// Name, then the role indented under it, then a gap: two lines per
	// credit, because a name and a role side by side do not fit a panel
	// this narrow.
	for (size_t i = 0; i < m_CreditNames.size(); ++i)
	{
		const int nameW = std::max(0, w - RemoveW - 2);

		m_CreditNames[i]->resize(x, rowY, nameW, RowHeight - 2);
		m_CreditRemove[i]->resize(x + nameW + 2, rowY, RemoveW, RowHeight - 2);
		rowY += RowHeight;
		m_CreditRoles[i]->resize(x + Pad * 2, rowY, std::max(0, nameW - Pad * 2), RowHeight - 2);
		rowY += RowHeight + Pad / 2;
	}

	if (m_AddCredit)
		m_AddCredit->resize(x, rowY, 88, RowHeight - 2);
}

void Fl_BoundaryDrawer::NoteLayout()
{
	if (m_DocumentSection)
		m_DocumentSection->init_sizes();

	if (m_CreditSection)
		m_CreditSection->init_sizes();

	init_sizes();
}

std::string Fl_BoundaryDrawer::FieldText(Field field) const
{
	if (!Bound())
		return std::string();

	return DocumentField(*m_Document, field);
}

bool Fl_BoundaryDrawer::Apply(Field field, const std::string &value)
{
	if (!Bound())
		return false;

	DocumentField(*m_Document, field) = value;

	if (field == FieldLicense && !CanBundleLicense())
		m_Document->Rights.BundleText = false;

	Repopulate();
	Changed();

	return true;
}

bool Fl_BoundaryDrawer::CanBundleLicense() const
{
	if (!Bound())
		return false;

	const std::string id = FieldText(FieldLicense);

	if (id.empty())
		return false;

	for (size_t i = 0; i < LicensePresetCount(); ++i)
	{
		const LicensePreset preset = LicensePresetAt(i);

		if (id == preset.Id)
			return preset.Bundles;
	}

	return false;
}

bool Fl_BoundaryDrawer::BundleLicenseText() const
{
	return Bound() && m_Document->Rights.BundleText;
}

bool Fl_BoundaryDrawer::SetBundleLicenseText(bool on)
{
	if (!Bound())
		return false;

	if (on && !CanBundleLicense())
		return false;

	m_Document->Rights.BundleText = on;
	Repopulate();
	Changed();

	return true;
}

size_t Fl_BoundaryDrawer::CreditRows() const
{
	return Bound() ? m_Document->Credits.size() : 0;
}

bool Fl_BoundaryDrawer::SetCredit(size_t row, const std::string &name, const std::string &role)
{
	if (!Bound() || row >= m_Document->Credits.size())
		return false;

	m_Document->Credits[row].Name = name;
	m_Document->Credits[row].Role = role;
	Changed();

	return true;
}

bool Fl_BoundaryDrawer::AddCredit()
{
	if (!Bound())
		return false;

	m_Document->Credits.push_back(Credit());
	Repopulate();
	Changed();

	if (!m_CreditNames.empty())
		m_CreditNames.back()->take_focus();

	return true;
}

bool Fl_BoundaryDrawer::RemoveCredit(size_t row)
{
	if (!Bound() || row >= m_Document->Credits.size())
		return false;

	m_Document->Credits.erase(m_Document->Credits.begin() + row);
	Repopulate();
	Changed();

	return true;
}

void Fl_BoundaryDrawer::Changed()
{
	if (DocumentChanged)
		DocumentChanged(DocumentChangedData);
}

void Fl_BoundaryDrawer::FieldEdited(Fl_Widget *widget, void *data)
{
	Fl_BoundaryDrawer *self = static_cast<Fl_BoundaryDrawer *>(data);

	for (size_t i = 0; i < self->m_Fields.size(); ++i)
		if (self->m_Fields[i] == widget)
		{
			const char *value = self->m_Fields[i]->value();
			const std::string text = value ? value : "";

			if (text != self->FieldText(self->m_FieldKinds[i]))
				self->Apply(self->m_FieldKinds[i], text);

			return;
		}
}

void Fl_BoundaryDrawer::AboutEdited(Fl_Widget *, void *data)
{
	Fl_BoundaryDrawer *self = static_cast<Fl_BoundaryDrawer *>(data);

	if (!self->m_AboutBuffer)
		return;

	char *value = self->m_AboutBuffer->text();
	const std::string text = value ? value : "";
	free(value);

	if (text != self->FieldText(FieldDescription))
		self->Apply(FieldDescription, text);
}

void Fl_BoundaryDrawer::LicenseEdited(Fl_Widget *, void *data)
{
	Fl_BoundaryDrawer *self = static_cast<Fl_BoundaryDrawer *>(data);

	if (!self->m_License)
		return;

	const char *value = self->m_License->value();
	const std::string text = value ? value : "";

	if (text != self->FieldText(FieldLicense))
		self->Apply(FieldLicense, text);
}

void Fl_BoundaryDrawer::LicensePicked(Fl_Widget *, void *data)
{
	Fl_BoundaryDrawer *self = static_cast<Fl_BoundaryDrawer *>(data);

	if (!self->m_LicenseMenu || !self->m_LicenseMenu->text())
		return;

	const std::string text = self->m_LicenseMenu->text();

	if (text != self->FieldText(FieldLicense))
		self->Apply(FieldLicense, text);
}

void Fl_BoundaryDrawer::ShowLicenseText()
{
	if (!Bound())
		return;

	const std::string id = FieldText(FieldLicense);
	std::string text = LicenseFullText(id);

	if (text.empty())
		return;

	// What the bundle writes, so reading it here is reading the file.
	if (!m_Document->Rights.Copyright.empty())
		text = "Copyright " + m_Document->Rights.Copyright + "\n\n" + text;

	const int Width = 560;
	const int Height = 420;
	const int Margin = 8;
	const int ButtonH = 24;

	Fl_Double_Window dialog((Fl::w() - Width) / 2, (Fl::h() - Height) / 2, Width, Height);
	dialog.copy_label(id.c_str());
	dialog.color(color());

	Fl_Text_Buffer buffer;
	buffer.text(text.c_str());

	Fl_PanelText *display = new Fl_PanelText(Margin, Margin,
		Width - 2 * Margin, Height - 3 * Margin - ButtonH);
	display->buffer(&buffer);
	display->textfont(FL_COURIER);
	display->textsize(11);
	display->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
	display->scrollbar_width(12);

	Fl_Return_Button ok(Width - Margin - 80, Height - Margin - ButtonH, 80, ButtonH, "OK");
	PlasticButton(&ok);
	ok.callback(CloseLicense, &dialog);

	dialog.end();
	dialog.resizable(display);
	dialog.set_modal();
	dialog.show();

	while (dialog.shown())
		Fl::wait();

	// The buffer is on the stack: detach before it goes.
	display->buffer(NULL);
}

void Fl_BoundaryDrawer::ViewLicenseClicked(Fl_Widget *, void *data)
{
	static_cast<Fl_BoundaryDrawer *>(data)->ShowLicenseText();
}

void Fl_BoundaryDrawer::CloseLicense(Fl_Widget *, void *data)
{
	static_cast<Fl_Window *>(data)->hide();
}

void Fl_BoundaryDrawer::BundleToggled(Fl_Widget *, void *data)
{
	Fl_BoundaryDrawer *self = static_cast<Fl_BoundaryDrawer *>(data);

	if (!self->m_Bundle)
		return;

	if (!self->SetBundleLicenseText(self->m_Bundle->value() != 0))
		self->m_Bundle->value(0);
}

void Fl_BoundaryDrawer::CreditEdited(Fl_Widget *widget, void *data)
{
	Fl_BoundaryDrawer *self = static_cast<Fl_BoundaryDrawer *>(data);

	for (size_t i = 0; i < self->m_CreditNames.size(); ++i)
		if (self->m_CreditNames[i] == widget || self->m_CreditRoles[i] == widget)
		{
			const char *name = self->m_CreditNames[i]->value();
			const char *role = self->m_CreditRoles[i]->value();

			self->SetCredit(i, name ? name : "", role ? role : "");

			return;
		}
}

void Fl_BoundaryDrawer::AddClicked(Fl_Widget *, void *data)
{
	static_cast<Fl_BoundaryDrawer *>(data)->AddCredit();
}

void Fl_BoundaryDrawer::RemoveClicked(Fl_Widget *widget, void *data)
{
	Fl_BoundaryDrawer *self = static_cast<Fl_BoundaryDrawer *>(data);

	for (size_t i = 0; i < self->m_CreditRemove.size(); ++i)
		if (self->m_CreditRemove[i] == widget)
		{
			/* Applied on a timeout so the button is not deleted inside
			   its own callback. */
			self->m_RemoveRow = i;
			Fl::remove_timeout(RemoveCommitted, self);
			Fl::add_timeout(0, RemoveCommitted, self);

			return;
		}
}

void Fl_BoundaryDrawer::RemoveCommitted(void *data)
{
	Fl_BoundaryDrawer *self = static_cast<Fl_BoundaryDrawer *>(data);

	self->RemoveCredit(self->m_RemoveRow);
}
