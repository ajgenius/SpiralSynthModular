// SPDX-License-Identifier: GPL-2.0-or-later
#include "PanelCorpus.h"
#include "PanelNode.h"

using Spiral::Panels::Bounds;
using Spiral::Panels::Document;
using Spiral::Panels::Node;

static void RoundTrip(const std::string &source, const std::string &label)
{
	Document document;
	CHECK(LoadText(document, source), label + ": loads");
	CHECK(Saved(document) == Canonical(source), label + ": saves what it read");
}

static void EditingChangesSavedData()
{
	Document document;
	LoadText(document, "{\"panel\":{\"children\":[{\"type\":\"Knob\",\"property\":\"FINE TUNE\"}]}}");
	Node *knob = document.Panel()->Children()[0];
	knob->SetAttribute("property", Spumoni::JSON::MakeString("OCTAVE"));
	CHECK(Saved(document) == Canonical("{\"panel\":{\"children\":[{\"type\":\"Knob\",\"property\":\"OCTAVE\"}]}}"),
			"an edit is saved");
}

static void UnknownTypesNamespacesAndFieldsArePreserved()
{
	RoundTrip("{\"schema_version\":9,\"future_root\":{\"type\":\"NotAControl\",\"nested\":[null,0,false,\"\"]},"
			"\"panel\":{\"future_panel\":true,\"children\":["
			"{\"type\":\"Slick::Button\",\"future_field\":{\"type\":\"NotAControl\",\"value\":null}},"
			"{\"type\":\"Spiral::MatrixGrid\",\"new_attribute\":[1,{\"future\":\"yes\"}]},"
			"{\"type\":\"other::UninstalledControl\",\"anything\":[{\"type\":\"Mystery\"}]}]}}", "unknown content");
	RoundTrip("{\"panel\":{\"children\":[{\"type\":null,\"payload\":{\"type\":[],\"x\":1}}]}}", "a type that is not a string");
	RoundTrip("{\"panel\":{\"children\":[7,\"text\",{\"type\":\"Knob\",\"bounds\":\"wide\"},"
			"{\"type\":\"Knob\",\"bounds\":[1,2,3]},{\"type\":\"Knob\",\"bounds\":[1.5,2,3,4]},"
			"{\"type\":\"Notebook\",\"pages\":[null,{\"title\":\"A\",\"children\":{}}]}]}}", "malformed shapes");
	RoundTrip("{\"panel\":{\"children\":[]}}", "an empty panel");
	RoundTrip("{\"schema_version\":123,\"future\":[],\"panel\":null}", "a panel that is not an object");
}

static void EditingAKnownFieldKeepsAnExtension()
{
	const std::string source = "{\"panel\":{\"children\":[{\"type\":\"Button\",\"text\":\"Before\",\"vendor\":"
			"{\"payload\":[{\"type\":\"Future\",\"value\":1.0000000000000000001}]}}]}}";
	Document document;
	LoadText(document, source);
	document.Panel()->Children()[0]->SetAttribute("text", Spumoni::JSON::MakeString("After"));
	std::string text, error;
	document.Save(text, error);
	CHECK(text.find("\"After\"") != std::string::npos, "the edit is saved");
	CHECK(text.find("1.0000000000000000001") != std::string::npos, "the extension's number is kept exactly");
}

static void TypedFieldsReadAndWrite()
{
	Document document;
	LoadText(document, "{\"plugin_id\":41,\"panel\":{\"children\":[{\"type\":\"Slick::Knob\",\"name\":\"cutoff\","
			"\"bounds\":[1,2,30,40],\"min\":0.5,\"style\":\"plastic\",\"toggle\":true,\"tooltip\":{\"default\":\"Cut\"}}]}}");
	long id = 0;
	CHECK(document.PluginId(id) && id == 41, "plugin_id");
	const Node &knob = *document.Panel()->Children()[0];
	CHECK(knob.Type() == "Slick::Knob" && knob.Namespace() == "Slick" && knob.ShortType() == "Knob", "type");
	CHECK(knob.HasName() && knob.Name() == "cutoff", "name");
	CHECK(knob.HasBounds() && knob.GetBounds().X == 1 && knob.GetBounds().H == 40, "bounds");
	double min = 0;
	std::string style;
	bool toggle = false;
	Spiral::Data::ValueRule::Text tooltip;
	CHECK(knob.Number("min", min) && min == 0.5, "number");
	CHECK(knob.String("style", style) && style == "plastic", "string");
	CHECK(knob.Boolean("toggle", toggle) && toggle, "boolean");
	CHECK(knob.Text("tooltip", tooltip) && tooltip.fallback == "Cut", "text");
	CHECK(!knob.Has("children") && !knob.HasChildren(), "no children");
	Node *edited = document.Panel()->Children()[0];
	edited->SetBounds(Bounds(5, 6, 7, 8));
	edited->SetName("res");
	CHECK(Saved(document).find("\"bounds\":[5,6,7,8]") != std::string::npos, "bounds are written");
	CHECK(Saved(document).find("\"name\":\"res\"") != std::string::npos, "the name is written");
}

static void ContractsAreReadAndWritten()
{
	Document document;
	LoadText(document, "{\"control_contracts\":{\"Slick\":2,\"Spiral\":1},\"panel\":{\"children\":[]}}");
	std::map<std::string, long> versions;
	CHECK(document.DeclaresContracts() && document.Contracts(versions), "declared");
	CHECK(versions["Slick"] == 2 && versions["Spiral"] == 1, "versions");
	versions["Slick"] = 3;
	document.SetContracts(versions);
	CHECK(Saved(document).find("\"Slick\":3") != std::string::npos, "written");
	LoadText(document, "{\"control_contracts\":{\"Slick\":\"two\"},\"panel\":{\"children\":[]}}");
	CHECK(document.DeclaresContracts() && !document.Contracts(versions), "a malformed declaration is not read");
}

static void PreciseAndLargeNumbersAreNotRounded()
{
	RoundTrip("{\"unknown\":{\"tiny\":0.123456789012345678901234567890123,\"huge\":1e10000,\"negative\":-0.0}}",
			"precise numbers");
}

static void MalformedFilesAreRefused()
{
	const char *sources[] = {"{\"a\":1,\"a\":2}", "{\"panel\":{\"x\":1,\"x\":2}}", "[]", "{broken",
			"{\"x\":NaN}", "{\"x\":Infinity}", "{\"x\":-Infinity}"};

	for (size_t i = 0; i < sizeof sources / sizeof sources[0]; ++i)
	{
		Document document;
		std::string error;
		CHECK(!document.Load(sources[i], error), std::string("refused: ") + sources[i]);
		CHECK(!error.empty(), std::string("says why: ") + sources[i]);
	}
}

static void NonFiniteNumbersCannotBeSaved()
{
	Document document;
	LoadText(document, "{\"panel\":{\"children\":[]}}");
	document.SetField("nested", Spumoni::JSON::MakeNumber("nan"));
	std::string text, error;
	CHECK(!document.Save(text, error), "a non-finite number is not saved");
}

static void EveryCheckedInPanelRoundTrips()
{
	const std::vector<std::string> paths = CorpusPanels();
	CHECK(paths.size() > 40, "the corpus is found");

	for (size_t i = 0; i < paths.size(); ++i)
	{
		Document document;
		std::string error;
		CHECK(document.Load(CorpusPanel(paths[i]), error), paths[i] + ": " + error);
		CHECK(Saved(document) == Canonical(CorpusPanel(paths[i])), paths[i] + ": saves what it read");
		Document *copy = document.Clone();
		CHECK(Saved(*copy) == Saved(document), paths[i] + ": clones");
		delete copy;
	}
}

int main()
{
	EditingChangesSavedData();
	UnknownTypesNamespacesAndFieldsArePreserved();
	EditingAKnownFieldKeepsAnExtension();
	TypedFieldsReadAndWrite();
	ContractsAreReadAndWritten();
	PreciseAndLargeNumbersAreNotRounded();
	MalformedFilesAreRefused();
	NonFiniteNumbersCannotBeSaved();
	EveryCheckedInPanelRoundTrips();

	if (failures)
		std::fprintf(stderr, "%d failures\n", failures);

	return failures ? 1 : 0;
}
