// SPDX-License-Identifier: GPL-2.0-or-later
// The drawer edits the document it is bound to, in place, and says so.
// Collapsed is the rail alone; the width is for this process only.
// Nothing here touches a file: that is the host's decision.
#include "Fl_BoundaryDrawer.h"
#include "License.h"
#include <FL/Fl_Double_Window.H>
#include <cassert>
#include <cstdio>
#include <string>

using namespace Spiral::File;

int main()
{
	Fl_Double_Window window(400, 300);
	Fl_BoundaryDrawer *drawer = new Fl_BoundaryDrawer(0, 0, 300, 300);
	window.end();

	int changes = 0;
	struct Count
	{
		static void Run(void *data) { ++*static_cast<int *>(data); }
	};
	drawer->DocumentChanged = Count::Run;
	drawer->DocumentChangedData = &changes;

	// Unbound: blank, and nothing to apply to.
	assert(!drawer->Bound());
	assert(drawer->FieldText(Fl_BoundaryDrawer::FieldTitle).empty());
	assert(!drawer->Apply(Fl_BoundaryDrawer::FieldTitle, "x"));
	assert(drawer->CreditRows() == 0);
	assert(changes == 0);

	// Starts collapsed: the rail only.
	assert(drawer->Collapsed());
	assert(drawer->CurrentWidth() == drawer->RailWidth());
	assert(!drawer->PanelShown());

	DocumentSection document;
	document.Title = "Looking Backward into the Future";
	document.CreatedAt = "2003-10-23";
	document.Credits.push_back(Credit());
	document.Credits.back().Name = "Thom Cherryhomes (TSCHAK)";
	document.Credits.back().Role = "Artist";
	document.Rights.Copyright = "(c) 2003 UNIT-E";
	document.Rights.License = "CC-BY-SA-1.0";

	drawer->Bind(&document);
	assert(drawer->Bound());
	assert(drawer->FieldText(Fl_BoundaryDrawer::FieldTitle) == document.Title);
	assert(drawer->FieldText(Fl_BoundaryDrawer::FieldLicense) == "CC-BY-SA-1.0");
	assert(drawer->CreditRows() == 1);
	// Collapsed still: binding does not open the drawer.
	assert(!drawer->PanelShown());
	assert(changes == 0);

	// Opening shows the panel and widens the drawer.
	drawer->SetCollapsed(false);
	assert(drawer->PanelShown());
	assert(drawer->CurrentWidth() == drawer->ExpandedWidth());
	assert(drawer->CurrentWidth() > drawer->RailWidth());

	// Edits land on the document and are announced.
	assert(drawer->Apply(Fl_BoundaryDrawer::FieldDescription, "From the UNIT-E album."));
	assert(document.Description == "From the UNIT-E album.");
	assert(changes == 1);

	// The LBITF licence has a text to bundle; a 4.0 claim has none, and
	// switching to it drops the bundle flag rather than keeping a promise
	// the save cannot make.
	assert(drawer->CanBundleLicense());
	assert(!drawer->BundleLicenseText());
	assert(drawer->SetBundleLicenseText(true));
	assert(document.Rights.BundleText);
	assert(changes == 2);
	assert(drawer->Apply(Fl_BoundaryDrawer::FieldLicense, "CC-BY-SA-4.0"));
	assert(!drawer->CanBundleLicense());
	assert(!document.Rights.BundleText);
	assert(!drawer->SetBundleLicenseText(true));
	assert(changes == 3);

	// Credits: add, edit, remove.
	assert(drawer->AddCredit());
	assert(drawer->CreditRows() == 2);
	assert(drawer->SetCredit(1, "Sharp Hall", "Producer"));
	assert(document.Credits[1].Name == "Sharp Hall");
	assert(document.Credits[1].Role == "Producer");
	assert(drawer->RemoveCredit(0));
	assert(drawer->CreditRows() == 1);
	assert(document.Credits[0].Name == "Sharp Hall");
	assert(!drawer->RemoveCredit(5));
	assert(changes == 6);

	// Clearing a field drops it, which is how a project stops claiming it.
	assert(drawer->Apply(Fl_BoundaryDrawer::FieldCopyright, ""));
	assert(document.Rights.Copyright.empty());

	// The document changed underneath: a refresh shows it.
	document.Title = "Renamed";
	drawer->Refresh();
	assert(drawer->FieldText(Fl_BoundaryDrawer::FieldTitle) == "Renamed");

	// Unbinding blanks the panel without touching the document.
	drawer->Bind(NULL);
	assert(!drawer->Bound());
	assert(!drawer->PanelShown());
	assert(document.Title == "Renamed");

	std::puts("The drawer edits the document it is bound to, and the licence bundle only when there is a text");
	return 0;
}
