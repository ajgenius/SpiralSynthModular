// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "Fl_MidiBindings.h"
#include "MidiMap.h"
#include "Midi.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Spinner.H>
#include <FL/fl_draw.H>
#include <algorithm>
#include <cstdio>

using Spiral::Core::MidiBindingDescription;
using Spiral::Core::MidiDeviceDescription;

namespace
{
	const int Pad = 6;
	const int RowHeight = 22;
	const int LabelWidth = 64;
	const int ButtonW = 22;
	const int BindingW = 92;
	const int Indent = 14;
	const int AddW = 96;

	void PlasticButton(Fl_Widget *button)
	{
		button->box(FL_PLASTIC_UP_BOX);
		button->labelsize(10);
	}

	std::string Number(long n)
	{
		char text[32];
		std::snprintf(text, sizeof text, "%ld", n);
		return text;
	}

	std::string Bound(const MidiBindingDescription &binding)
	{
		return (binding.Channel ? "ch " + Number(binding.Channel) : std::string("any ch"))
			+ " \xc2\xb7 CC " + Number(binding.Controller);
	}

	/* A row of the list: which row it is, for the buttons on it. Rows are
	   rebuilt whenever the map changes, and a row's own button may be what
	   changed it, so old rows go by Fl::delete_widget, after the callback. */
	class Fl_MidiRow: public Fl_Group
	{
	public:
		Fl_MidiRow(int x, int y, int w, int h, Fl_MidiBindings *owner, size_t row):
			Fl_Group(x, y, w, h), Owner(owner), Index(row)
		{
			box(FL_NO_BOX);
		}

		Fl_MidiBindings *Owner;
		size_t Index;
	};

	Fl_MidiRow *RowOf(Fl_Widget *widget)
	{
		return static_cast<Fl_MidiRow *>(widget->parent());
	}
}

Fl_MidiBindings::Fl_MidiBindings(int x, int y, int w, int h):
	Fl_Group(x, y, w, h),
	DeviceName(NULL), PickDevice(NULL), PickControl(NULL), Learn(NULL), SourcePicked(NULL), Changed(NULL), Data(NULL),
	m_Map(NULL), m_Status(NULL), m_Source(NULL), m_Heading(NULL), m_List(NULL), m_AddDevice(NULL), m_Picking(false),
	m_Window(NULL), m_Title(NULL), m_Channel(NULL), m_Controller(NULL), m_LearnButton(NULL), m_Heard(NULL),
	m_Bind(NULL), m_Unbind(NULL), m_Cancel(NULL), m_Listening(false)
{
	box(FL_NO_BOX);

	// The patch's own input first: what it hears from, and how that is going.
	m_Status = new Fl_Box(x, y, w, RowHeight - 2, "MIDI in");
	m_Status->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
	m_Status->labelsize(10);

	m_Source = new Fl_Choice(x + LabelWidth, y + RowHeight, std::max(0, w - LabelWidth), RowHeight - 2, "Source");
	m_Source->box(FL_PLASTIC_DOWN_BOX);
	m_Source->down_box(FL_PLASTIC_UP_BOX);
	m_Source->labelsize(10);
	m_Source->textsize(10);
	m_Source->callback(SourceChosen, this);

	m_Heading = new Fl_Box(x, y + 2 * RowHeight, w, RowHeight - 2, "Bound devices");
	m_Heading->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_Heading->labelsize(10);
	m_Heading->labelfont(FL_HELVETICA_BOLD);

	m_List = new Fl_Scroll(x, y + 3 * RowHeight, w, std::max(0, h - 4 * RowHeight - Pad));
	m_List->type(Fl_Scroll::VERTICAL);
	m_List->box(FL_NO_BOX);
	m_List->scrollbar.box(FL_PLASTIC_DOWN_BOX);
	m_List->scrollbar.slider(FL_PLASTIC_UP_BOX);
	m_List->scrollbar_size(12);
	m_List->end();

	m_AddDevice = new Fl_Button(x, y + h - RowHeight, AddW, RowHeight - 2, "Add device");
	PlasticButton(m_AddDevice);
	m_AddDevice->tooltip("Pick a device on the canvas, then the controls on it to bind");
	m_AddDevice->callback(AddDeviceClicked, this);

	end();
	resizable(NULL);
	ListSources();
	ShowStatus();
}

Fl_MidiBindings::~Fl_MidiBindings()
{
	Fl::remove_timeout(PollStatus, this);
	if (m_Listening && Learn)
		Learn(NULL, NULL, Data);
	delete m_Window;
}

void Fl_MidiBindings::resize(int x, int y, int w, int h)
{
	Fl_Group::resize(x, y, w, h);
	Relayout();
}

void Fl_MidiBindings::draw()
{
	m_List->color(color());
	fl_color(color());
	fl_rectf(x(), y(), w(), h());
	draw_children();
	// The backend connects in its own time: while the page is up, look again.
	if (!Fl::has_timeout(PollStatus, this))
		Fl::add_timeout(1.0, PollStatus, this);
}

void Fl_MidiBindings::Relayout()
{
	m_Status->resize(x(), y(), w(), RowHeight - 2);
	m_Source->resize(x() + LabelWidth, y() + RowHeight, std::max(0, w() - LabelWidth), RowHeight - 2);
	m_Heading->resize(x(), y() + 2 * RowHeight, w(), RowHeight - 2);
	m_List->resize(x(), y() + 3 * RowHeight, w(), std::max(0, h() - 4 * RowHeight - Pad));
	m_AddDevice->resize(x(), y() + h() - RowHeight, AddW, RowHeight - 2);
	Repopulate();
}

void Fl_MidiBindings::Bind(Spiral::Core::MidiMap *map)
{
	m_Map = map;
	Refresh();
}

void Fl_MidiBindings::Refresh()
{
	ListSources();
	ShowStatus();
	Repopulate();
	// The control being bound may have gone with its device.
	if (Editing() && (!m_Map || !m_Map->Find(m_EditDevice)))
		CloseWindow();
}

std::string Fl_MidiBindings::Label(const std::string &device) const
{
	const std::string name = DeviceName ? DeviceName(device, Data) : std::string();
	return name.empty() ? "Device " + device : name;
}

void Fl_MidiBindings::Repopulate()
{
	m_Rows.clear();
	for (size_t d = 0; m_Map && d < m_Map->Devices.size(); ++d)
	{
		const MidiDeviceDescription &listed = m_Map->Devices[d];
		Row device;
		device.Device = listed.Device;
		m_Rows.push_back(device);
		for (size_t b = 0; b < listed.Bindings.size(); ++b)
		{
			Row binding;
			binding.Device = listed.Device;
			binding.Control = listed.Bindings[b].Control;
			m_Rows.push_back(binding);
		}
	}

	// Built with no group open, so nothing lands in whatever the caller has
	// open, and that is left open after.
	Fl_Group *current = Fl_Group::current();
	Fl_Group::current(NULL);

	// The scroll's scrollbars are its last two children; the rows go.
	const int top = m_List->y() - m_List->yposition();
	for (int i = m_List->children(); i-- > 0; )
	{
		Fl_Widget *old = m_List->child(i);
		if (old == &m_List->scrollbar || old == &m_List->hscrollbar)
			continue;
		m_List->remove(old);
		Fl::delete_widget(old);
	}

	const int rowW = std::max(0, m_List->w() - m_List->scrollbar_size() - 2);
	for (size_t r = 0; r < m_Rows.size(); ++r)
	{
		const Row &row = m_Rows[r];
		const int rowY = top + (int)r * RowHeight;
		Fl_MidiRow *group = new Fl_MidiRow(m_List->x(), rowY, rowW, RowHeight, this, r);
		const int right = group->x() + group->w();

		if (row.Control.empty())
		{
			Fl_Box *name = new Fl_Box(group->x(), rowY, std::max(0, rowW - 2 * ButtonW - Pad), RowHeight - 2);
			name->copy_label(Label(row.Device).c_str());
			name->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
			name->labelsize(10);
			name->labelfont(FL_HELVETICA_BOLD);

			Fl_Button *add = new Fl_Button(right - 2 * ButtonW, rowY, ButtonW, RowHeight - 2, "+");
			PlasticButton(add);
			add->tooltip("Pick a control on this device to bind");
			add->callback(AddControlClicked);

			Fl_Button *remove = new Fl_Button(right - ButtonW, rowY, ButtonW, RowHeight - 2, "@-21+");
			PlasticButton(remove);
			remove->labelsize(8);
			remove->tooltip("Take this device and its bindings out of the MIDI map");
			remove->callback(RemoveDeviceClicked);
		}
		else
		{
			const MidiBindingDescription *binding = m_Map->Binding(row.Device, row.Control);
			Fl_Box *name = new Fl_Box(group->x() + Indent, rowY,
				std::max(0, rowW - Indent - BindingW - ButtonW - Pad), RowHeight - 2);
			name->copy_label(row.Control.c_str());
			name->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
			name->labelsize(10);

			Fl_Button *edit = new Fl_Button(right - BindingW - ButtonW, rowY, BindingW, RowHeight - 2);
			edit->copy_label(binding ? Bound(*binding).c_str() : "");
			PlasticButton(edit);
			edit->tooltip("Change this binding");
			edit->callback(EditClicked);

			Fl_Button *remove = new Fl_Button(right - ButtonW, rowY, ButtonW, RowHeight - 2, "@-21+");
			PlasticButton(remove);
			remove->labelsize(8);
			remove->tooltip("Unbind this control");
			remove->callback(UnbindClicked);
		}
		group->end();
		m_List->add(group);
	}

	if (m_Rows.empty())
	{
		Fl_MidiRow *group = new Fl_MidiRow(m_List->x(), top, rowW, 2 * RowHeight, this, 0);
		Fl_Box *none = new Fl_Box(group->x(), top, rowW, 2 * RowHeight,
			"Nothing bound. Add a device, then pick\nthe controls on it a MIDI controller moves.");
		none->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_WRAP);
		none->labelsize(10);
		group->end();
		m_List->add(group);
	}

	Fl_Group::current(current);
	m_List->init_sizes();
	m_List->redraw();
	redraw();
}

void Fl_MidiBindings::ListSources()
{
	Spiral::Core::MidiDevice *midi = Spiral::Core::MidiDevice::Get();
	const std::vector<std::string> sources = midi ? midi->Sources() : std::vector<std::string>();
	const std::string chosen = Spiral::Core::MidiDevice::GetDeviceName();
	const bool there = std::find(sources.begin(), sources.end(), chosen) != sources.end();
	if (sources == m_Sources && m_Source->size() > 0 && (m_Source->value() > 0) == there)
		return;
	m_Sources = sources;

	// A name the backend has no input for connects nothing more than its
	// default: the old /dev/midi setting, on a backend that names ports.
	m_Source->clear();
	m_Source->add("Default");
	m_Source->copy_tooltip(chosen.empty() || there
		? "Where the patch's MIDI input listens: one of the backend's inputs, or its default"
		: ("The setting asks for " + chosen + ", which this backend does not have").c_str());
	int active = 0;
	for (size_t s = 0; s < m_Sources.size(); ++s)
	{
		// Not a menu path: the slashes of a port name are its own.
		m_Source->add("-");
		m_Source->replace((int)s + 1, m_Sources[s].c_str());
		if (m_Sources[s] == chosen)
			active = (int)s + 1;
	}
	m_Source->value(active);
}

void Fl_MidiBindings::ShowStatus()
{
	Spiral::Core::MidiDevice *midi = Spiral::Core::MidiDevice::Get();
	const std::string text = midi ? midi->GetBackendName() + ": " + midi->GetStatus() : std::string("no MIDI device");
	if (text == m_StatusText)
		return;
	m_StatusText = text;
	m_Status->copy_label(("MIDI in \xc2\xb7 " + text).c_str());
	m_Status->tooltip(NULL);
	m_Status->copy_tooltip(text.c_str());
	m_Status->redraw();
}

void Fl_MidiBindings::PollStatus(void *data)
{
	Fl_MidiBindings *self = (Fl_MidiBindings *)data;
	if (!self->visible_r())
		return;
	self->ShowStatus();
	self->ListSources();
	Fl::repeat_timeout(1.0, PollStatus, data);
}

void Fl_MidiBindings::SetPicking(bool picking)
{
	m_Picking = picking;
	m_AddDevice->label(picking ? "Stop picking" : "Add device");
	m_AddDevice->redraw();
}

std::string Fl_MidiBindings::RowDevice(size_t row) const
{
	return row < m_Rows.size() ? m_Rows[row].Device : std::string();
}

std::string Fl_MidiBindings::RowControl(size_t row) const
{
	return row < m_Rows.size() ? m_Rows[row].Control : std::string();
}

std::string Fl_MidiBindings::RowText(size_t row) const
{
	if (row >= m_Rows.size())
		return std::string();
	if (m_Rows[row].Control.empty())
		return Label(m_Rows[row].Device);
	const MidiBindingDescription *binding = m_Map ? m_Map->Binding(m_Rows[row].Device, m_Rows[row].Control) : NULL;
	return m_Rows[row].Control + " " + (binding ? Bound(*binding) : std::string());
}

void Fl_MidiBindings::AddDeviceClicked(Fl_Widget *, void *data)
{
	Fl_MidiBindings *self = (Fl_MidiBindings *)data;
	if (self->PickDevice)
		self->PickDevice(self->Data);
}

void Fl_MidiBindings::SourceChosen(Fl_Widget *, void *data)
{
	Fl_MidiBindings *self = (Fl_MidiBindings *)data;
	const int item = self->m_Source->value();
	const std::string source = item > 0 && item <= (int)self->m_Sources.size() ? self->m_Sources[item - 1] : std::string();
	if (self->SourcePicked)
		self->SourcePicked(source, self->Data);
	self->ShowStatus();
}

void Fl_MidiBindings::AddControlClicked(Fl_Widget *widget, void *)
{
	Fl_MidiRow *row = RowOf(widget);
	Fl_MidiBindings *self = row->Owner;
	if (self->PickControl)
		self->PickControl(self->RowDevice(row->Index), self->Data);
}

void Fl_MidiBindings::RemoveDeviceClicked(Fl_Widget *widget, void *)
{
	Fl_MidiRow *row = RowOf(widget);
	Fl_MidiBindings *self = row->Owner;
	if (!self->m_Map || !self->m_Map->Remove(self->RowDevice(row->Index)))
		return;
	self->Refresh();
	if (self->Changed)
		self->Changed(self->Data);
}

void Fl_MidiBindings::EditClicked(Fl_Widget *widget, void *)
{
	Fl_MidiRow *row = RowOf(widget);
	row->Owner->EditBinding(row->Owner->RowDevice(row->Index), row->Owner->RowControl(row->Index));
}

void Fl_MidiBindings::UnbindClicked(Fl_Widget *widget, void *)
{
	Fl_MidiRow *row = RowOf(widget);
	Fl_MidiBindings *self = row->Owner;
	if (!self->m_Map || !self->m_Map->Unbind(self->RowDevice(row->Index), self->RowControl(row->Index)))
		return;
	self->Refresh();
	if (self->Changed)
		self->Changed(self->Data);
}

// The binding window

void Fl_MidiBindings::BuildWindow()
{
	if (m_Window)
		return;

	Fl_Group *current = Fl_Group::current();
	Fl_Group::current(NULL);

	const int W = 300, H = 4 * RowHeight + 3 * Pad;
	m_Window = new Fl_Double_Window(W, H, "MIDI binding");
	m_Window->color(color());
	m_Window->callback(CancelClicked, this);

	m_Title = new Fl_Box(Pad, Pad, W - 2 * Pad, RowHeight - 2);
	m_Title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
	m_Title->labelsize(11);
	m_Title->labelfont(FL_HELVETICA_BOLD);

	const int row1 = Pad + RowHeight;
	m_Channel = new Fl_Choice(Pad + 48, row1, 72, RowHeight - 2, "Channel");
	m_Channel->box(FL_PLASTIC_DOWN_BOX);
	m_Channel->down_box(FL_PLASTIC_UP_BOX);
	m_Channel->labelsize(10);
	m_Channel->textsize(10);
	m_Channel->add("Any");
	for (int c = 1; c <= 16; ++c)
		m_Channel->add(Number(c).c_str());
	m_Channel->value(0);

	m_Controller = new Fl_Spinner(W - Pad - 64, row1, 64, RowHeight - 2, "Controller");
	m_Controller->labelsize(10);
	m_Controller->textsize(10);
	m_Controller->range(0, 127);
	m_Controller->step(1);
	m_Controller->value(0);

	const int row2 = row1 + RowHeight;
	m_LearnButton = new Fl_Button(Pad, row2, 60, RowHeight - 2, "Learn");
	PlasticButton(m_LearnButton);
	m_LearnButton->type(FL_TOGGLE_BUTTON);
	m_LearnButton->tooltip("Take the channel and controller from the next control moved");
	m_LearnButton->callback(LearnClicked, this);

	m_Heard = new Fl_Box(Pad + 60 + Pad, row2, W - 3 * Pad - 60, RowHeight - 2);
	m_Heard->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
	m_Heard->labelsize(10);

	const int row3 = row2 + RowHeight + Pad;
	m_Unbind = new Fl_Button(Pad, row3, 60, RowHeight - 2, "Unbind");
	PlasticButton(m_Unbind);
	m_Unbind->callback(UnbindWindowClicked, this);

	m_Cancel = new Fl_Button(W - 2 * Pad - 120, row3, 60, RowHeight - 2, "Cancel");
	PlasticButton(m_Cancel);
	m_Cancel->callback(CancelClicked, this);

	m_Bind = new Fl_Button(W - Pad - 60, row3, 60, RowHeight - 2, "Bind");
	PlasticButton(m_Bind);
	m_Bind->callback(BindClicked, this);

	m_Window->end();
	// Not modal: the GUI loop has to keep running for the control to be heard.
	m_Window->set_non_modal();
	Fl_Group::current(current);
}

void Fl_MidiBindings::EditBinding(const std::string &device, const std::string &control)
{
	if (!m_Map)
		return;
	BuildWindow();
	m_EditDevice = device;
	m_EditControl = control;
	m_Title->copy_label((Label(device) + " \xc2\xb7 " + control).c_str());

	const MidiBindingDescription *binding = m_Map->Binding(device, control);
	m_Channel->value(binding ? (int)binding->Channel : 0);
	m_Controller->value(binding ? (double)binding->Controller : 0.0);
	if (binding)
		m_Unbind->activate();
	else
		m_Unbind->deactivate();
	m_Heard->copy_label(binding ? ("Bound to " + Bound(*binding)).c_str() : "");
	// Nothing bound yet: listen straight away, so moving the knob is enough.
	Listen(!binding);

	if (!m_Window->shown() && window())
		m_Window->position(window()->x() + x() - m_Window->w() - Pad, window()->y() + y());
	m_Window->show();
}

bool Fl_MidiBindings::Editing() const
{
	return m_Window && m_Window->shown();
}

void Fl_MidiBindings::Listen(bool on)
{
	m_Listening = on && Learn;
	m_LearnButton->value(m_Listening);
	if (Learn)
		Learn(m_Listening ? Learned : NULL, m_Listening ? this : NULL, Data);
	if (m_Listening)
		m_Heard->label("Move a control on the MIDI surface...");
	m_Heard->redraw();
}

void Fl_MidiBindings::CloseWindow()
{
	if (m_Listening)
		Listen(false);
	if (m_Window)
		m_Window->hide();
}

void Fl_MidiBindings::LearnClicked(Fl_Widget *, void *data)
{
	Fl_MidiBindings *self = (Fl_MidiBindings *)data;
	self->Listen(self->m_LearnButton->value() != 0);
	if (!self->m_Listening)
		self->m_Heard->label("");
}

void Fl_MidiBindings::Learned(int channel, int controller, void *data)
{
	Fl_MidiBindings *self = (Fl_MidiBindings *)data;
	self->m_Listening = false;
	self->m_LearnButton->value(0);
	self->m_Channel->value(channel >= 1 && channel <= 16 ? channel : 0);
	self->m_Controller->value(controller);
	self->m_Heard->copy_label(("Heard ch " + Number(channel) + " \xc2\xb7 CC " + Number(controller)).c_str());
	self->m_Heard->redraw();
}

void Fl_MidiBindings::BindClicked(Fl_Widget *, void *data)
{
	Fl_MidiBindings *self = (Fl_MidiBindings *)data;
	if (self->m_Map)
		self->m_Map->Bind(self->m_EditDevice, MidiBindingDescription(self->m_EditControl,
			self->m_Channel->value(), (long)self->m_Controller->value()));
	self->CloseWindow();
	self->Refresh();
	if (self->Changed)
		self->Changed(self->Data);
}

void Fl_MidiBindings::UnbindWindowClicked(Fl_Widget *, void *data)
{
	Fl_MidiBindings *self = (Fl_MidiBindings *)data;
	if (self->m_Map)
		self->m_Map->Unbind(self->m_EditDevice, self->m_EditControl);
	self->CloseWindow();
	self->Refresh();
	if (self->Changed)
		self->Changed(self->Data);
}

void Fl_MidiBindings::CancelClicked(Fl_Widget *, void *data)
{
	((Fl_MidiBindings *)data)->CloseWindow();
}
