// SPDX-License-Identifier: GPL-2.0-or-later
#include "PanelCorpus.h"
#include "PanelContracts.h"
#include "PanelNode.h"

using namespace Spiral::Panels;

static const char *SLICK_1 = "{\"namespace\":\"Controls\",\"version\":1,\"controls\":{"
		"\"Control\":{\"abstract\":true,\"required\":[\"bounds\"],"
		"\"attributes\":{\"name\":[\"string\"],\"bounds\":[\"array\"],\"tooltip\":[\"string\",\"object\"]}},"
		"\"Button\":{\"extends\":\"Controls::Control\",\"attributes\":{\"text\":[\"string\",\"object\"],\"write\":[\"array\"]}},"
		"\"Notebook\":{\"extends\":\"Controls::Control\",\"attributes\":{\"pages\":[\"pages\"]}},"
		"\"Repeat\":{\"extends\":\"Controls::Control\",\"attributes\":{\"children\":[\"nodes\"]}}}}";
static const char *SPIRAL_1 = "{\"namespace\":\"Spiral\",\"version\":1,\"controls\":{"
		"\"LCD\":{\"extends\":\"Controls::Control\",\"attributes\":{\"format\":[\"string\"]}}}}";

static void AddDocument(Registry &registry, const std::string &text)
{
	Spumoni::JSONOwner document(Spumoni::ParseJSONText(text));
	std::string error;
	CHECK(document.get() && registry.Add(*document, error), "contract document: " + error);
}

static std::map<std::string, long> Versions(long slick, long spiral)
{
	std::map<std::string, long> versions;

	if (slick)
		versions["Controls"] = slick;

	if (spiral)
		versions["Spiral"] = spiral;

	return versions;
}

static Report ReportFor(const std::string &source, const std::vector<std::string> &documents,
		const std::map<std::string, long> &assume)
{
	Registry registry;

	for (size_t i = 0; i < documents.size(); ++i)
		AddDocument(registry, documents[i]);

	Document document;
	CHECK(LoadText(document, source), "loads: " + source);

	return Resolve(document, registry, assume);
}

static std::vector<std::string> Standard()
{
	std::vector<std::string> documents;
	documents.push_back(SLICK_1);
	documents.push_back(SPIRAL_1);

	return documents;
}

static Report ReportFor(const std::string &source)
{
	return ReportFor(source, Standard(), Versions(1, 1));
}

static std::string Panel(const std::string &children)
{
	return "{\"panel\":{\"children\":[" + children + "]}}";
}

static std::string Problems(const NodeReport &node)
{
	std::string out;

	for (size_t i = 0; i < node.Problems.size(); ++i)
		out += node.Problems[i] + "; ";

	return out;
}

static void KnownControlIsValid()
{
	Report report = ReportFor(Panel("{\"type\":\"Button\",\"bounds\":[0,0,1,1],\"text\":\"Go\"}"));
	CHECK(report.Nodes.size() == 1, "one node");
	CHECK(report.Nodes[0].Path == "panel/children/0" && report.Nodes[0].Result == Valid
			&& report.Nodes[0].Contract == "Controls::Button", "valid as Controls::Button");
	CHECK(report.Assumed && report.Contracts == Versions(1, 1), "assumed versions");
}

static void UnknownControlIsUnresolvedAndStillSaved()
{
	const char *types[] = {"PointPlot", "Other::Thing", "Controls::Imaginary"};

	for (size_t i = 0; i < 3; ++i)
	{
		const std::string source = Panel(std::string("{\"type\":\"") + types[i] + "\",\"bounds\":[0,0,1,1],\"keep\":true}");
		Report report = ReportFor(source);
		CHECK(report.Nodes.size() == 1 && report.Nodes[0].Result == Unresolved, std::string("unresolved ") + types[i]);
		CHECK(report.Nodes[0].Contract.empty(), "no contract");
		CHECK(Problems(report.Nodes[0]).find(types[i]) != std::string::npos, "the problem names the type");
		Document document;
		LoadText(document, source);
		CHECK(Saved(document).find(types[i]) != std::string::npos, "saved");
	}
}

static void UnknownFieldIsAnExtension()
{
	Report report = ReportFor(Panel("{\"type\":\"Button\",\"bounds\":[0,0,1,1],\"icon\":\"play\"}"));
	CHECK(report.Nodes[0].Result == Extended, "extended");
	CHECK(report.Nodes[0].Extensions.size() == 1 && report.Nodes[0].Extensions[0] == "icon", "icon listed");
	CHECK(report.Nodes[0].Problems.empty(), "no problems");
}

static void WrongKindMissingRequiredAndAbstractAreInvalid()
{
	const char *sources[] = {"{\"type\":\"Button\",\"bounds\":\"wide\"}", "{\"type\":\"Button\",\"text\":\"Go\"}",
			"{\"type\":\"Controls::Control\",\"bounds\":[0,0,1,1]}"};

	for (size_t i = 0; i < 3; ++i)
	{
		Report report = ReportFor(Panel(sources[i]));
		CHECK(report.Nodes.size() == 1 && report.Nodes[0].Result == Invalid, std::string("invalid ") + sources[i]);
		CHECK(!report.Nodes[0].Problems.empty(), "says why");
	}
}

static void NestingIsWalkedOnlyWhereAContractSays()
{
	Report report = ReportFor(Panel(
			"{\"type\":\"Notebook\",\"bounds\":[0,0,1,1],\"pages\":[{\"title\":\"A\",\"children\":[{\"type\":\"Button\",\"bounds\":[0,0,1,1]}]}]},"
			"{\"type\":\"Repeat\",\"bounds\":[0,0,1,1],\"children\":[{\"type\":\"Nope\",\"bounds\":[0,0,1,1]}]},"
			"{\"type\":\"Mystery\",\"bounds\":[0,0,1,1],\"children\":[{\"type\":\"Button\",\"bounds\":[0,0,1,1]}]}"));
	const char *paths[] = {"panel/children/0", "panel/children/0/pages/0/children/0", "panel/children/1",
			"panel/children/1/children/0", "panel/children/2"};
	const Status statuses[] = {Valid, Valid, Valid, Unresolved, Unresolved};
	CHECK(report.Nodes.size() == 5, "five nodes");

	for (size_t i = 0; i < 5 && i < report.Nodes.size(); ++i)
		CHECK(report.Nodes[i].Path == paths[i] && report.Nodes[i].Result == statuses[i], paths[i]);
}

static void InheritedAttributesCrossNamespaces()
{
	Report report = ReportFor(Panel("{\"type\":\"Spiral::LCD\",\"bounds\":[0,0,1,1],\"name\":\"v\",\"format\":\"%.2f\"}"));
	CHECK(report.Nodes[0].Result == Valid && report.Nodes[0].Contract == "Spiral::LCD", "Spiral::LCD");
}

static void DeclaredVersionsSelectTheVocabulary()
{
	std::vector<std::string> documents = Standard();
	documents.push_back("{\"namespace\":\"Controls\",\"version\":2,\"controls\":{"
			"\"Control\":{\"abstract\":true,\"required\":[\"bounds\"],"
			"\"attributes\":{\"name\":[\"string\"],\"bounds\":[\"array\"],\"tooltip\":[\"string\",\"object\"]}},"
			"\"Button\":{\"extends\":\"Controls::Control\",\"attributes\":{\"text\":[\"string\",\"object\"],\"icon\":[\"string\"]}}}}");
	const std::string button = "{\"control_contracts\":{\"Controls\":%},\"panel\":{\"children\":[{\"type\":\"Button\","
			"\"bounds\":[0,0,1,1],\"icon\":\"play\"}]}}";
	std::string source = button;
	source.replace(source.find('%'), 1, "1");
	Report report = ReportFor(source, documents, Versions(1, 1));
	CHECK(!report.Assumed && report.Nodes[0].Result == Extended, "Slick 1 extends");
	source = button;
	source.replace(source.find('%'), 1, "2");
	report = ReportFor(source, documents, Versions(1, 1));
	CHECK(report.Contracts == Versions(2, 0) && report.Nodes[0].Result == Valid, "Slick 2 is valid");
	source = button;
	source.replace(source.find('%'), 1, "3");
	report = ReportFor(source, documents, Versions(1, 1));
	CHECK(report.Nodes[0].Result == Unresolved, "Slick 3 is unresolved");
	CHECK(!report.Problems.empty() && report.Problems[0].find("Controls 3 is not available") != std::string::npos,
			"Slick 3 is reported");
}

static void QualifiedNamesResolveWithoutRewriting()
{
	const std::string source = Panel("{\"type\":\"Controls::Button\",\"bounds\":[0,0,1,1]}");
	Report report = ReportFor(source);
	CHECK(report.Nodes[0].Result == Valid && report.Nodes[0].Contract == "Controls::Button"
			&& report.Nodes[0].Type == "Controls::Button", "qualified");
	Document document;
	LoadText(document, source);
	CHECK(Saved(document).find("\"Controls::Button\"") != std::string::npos, "not rewritten");
}

static void UnprefixedNamesAreRootControlsOnly()
{
	Report report = ReportFor(Panel("{\"type\":\"LCD\",\"bounds\":[0,0,1,1]}"));
	CHECK(report.Nodes[0].Result == Unresolved && Problems(report.Nodes[0]).find("custom control") != std::string::npos,
			"a custom control needs its namespace");
	report = ReportFor(Panel("{\"type\":\"Button\",\"bounds\":[0,0,1,1]}"));
	CHECK(report.Nodes[0].Result == Valid && report.Nodes[0].Contract == "Controls::Button", "a root control needs none");
}

static void TextKindAcceptsStringOrLocalizedObject()
{
	std::vector<std::string> documents;
	documents.push_back("{\"namespace\":\"Controls\",\"version\":1,\"controls\":{\"Label\":{\"attributes\":"
			"{\"bounds\":[\"array\"],\"text\":[\"text\"]}}}}");
	const char *values[] = {"\"Plain\"", "{\"default\":\"D\"}", "{\"default\":\"D\",\"key\":\"k\",\"translations\":{\"de\":\"X\"}}",
			"{\"key\":\"k\"}", "{\"default\":\"D\",\"value\":1}", "7"};
	const Status statuses[] = {Valid, Valid, Valid, Invalid, Invalid, Invalid};

	for (size_t i = 0; i < 6; ++i)
	{
		Report report = ReportFor(Panel(std::string("{\"type\":\"Label\",\"bounds\":[0,0,1,1],\"text\":") + values[i] + "}"),
				documents, Versions(1, 0));
		CHECK(report.Nodes[0].Result == statuses[i], std::string("text ") + values[i] + " " + Problems(report.Nodes[0]));
	}
}

static void TemplatePlaceholdersMustBeSuppliedByEveryItem()
{
	std::vector<std::string> documents;
	documents.push_back("{\"namespace\":\"Controls\",\"version\":1,\"controls\":{"
			"\"Repeat\":{\"attributes\":{\"bounds\":[\"array\"],\"items\":[\"array\"],\"max_count\":[\"integer\"],\"children\":[\"template\"]}},"
			"\"Group\":{\"attributes\":{\"bounds\":[\"array\"],\"children\":[\"nodes\"]}},"
			"\"Knob\":{\"attributes\":{\"bounds\":[\"array\"],\"name\":[\"string\"],\"min\":[\"number\"]}}}}");
	const std::string knob = "{\"type\":\"Knob\",\"bounds\":[0,0,1,1],\"name\":\"k-{index}\",\"min\":\"{low}\"}";
	std::string indexed = knob;
	indexed.replace(indexed.find("{low}"), 5, "{index}");
	const std::string sources[] = {
			"{\"type\":\"Repeat\",\"bounds\":[0,0,1,1],\"items\":[{\"low\":0},{\"low\":1}],\"children\":[" + knob + "]}",
			"{\"type\":\"Repeat\",\"bounds\":[0,0,1,1],\"items\":[{\"low\":0},{}],\"children\":[" + knob + "]}",
			"{\"type\":\"Repeat\",\"bounds\":[0,0,1,1],\"max_count\":2,\"children\":[" + knob + "]}",
			"{\"type\":\"Repeat\",\"bounds\":[0,0,1,1],\"max_count\":2,\"children\":[" + indexed + "]}",
			"{\"type\":\"Group\",\"bounds\":[0,0,1,1],\"children\":[" + knob + "]}"};
	const Status statuses[] = {Valid, Invalid, Invalid, Valid, Invalid};
	const char *problems[] = {"", "{low}", "{low}", "", "expected number"};

	for (size_t i = 0; i < 5; ++i)
	{
		Report report = ReportFor(Panel(sources[i]), documents, Versions(1, 0));
		const NodeReport *child = report.At("panel/children/0/children/0");
		CHECK(child && child->Result == statuses[i], sources[i] + " " + (child ? Problems(*child) : std::string("missing")));

		if (child && *problems[i])
			CHECK(Problems(*child).find(problems[i]) != std::string::npos, sources[i] + ": " + problems[i]);
	}
}

static void EveryCheckedInPanelIsValidUnderItsDeclaredContracts()
{
	Registry registry;
	std::string error;
	CHECK(registry.LoadDirectory(ContractDirectory(), error), error);
	const std::vector<std::string> paths = CorpusPanels();
	CHECK(paths.size() > 40, "the corpus is found");

	for (size_t i = 0; i < paths.size(); ++i)
	{
		Document document;
		CHECK(document.Load(CorpusPanel(paths[i]), error), error);
		Report report = Resolve(document, registry);
		CHECK(!report.Assumed && report.Contracts.count("Controls") && !report.Nodes.empty() && report.Problems.empty(),
				paths[i]);

		for (size_t n = 0; n < report.Nodes.size(); ++n)
		{
			const NodeReport &node = report.Nodes[n];
			std::string detail = Problems(node);

			for (size_t e = 0; e < node.Extensions.size(); ++e)
				detail += "extension " + node.Extensions[e] + "; ";

			CHECK(node.Result == Valid, paths[i] + " " + node.Path + " (" + node.Type + ") " + StatusName(node.Result)
					+ ": " + detail);
		}
	}
}

static void GeneratedLadspaPagesAreValid()
{
	Registry registry;
	std::string error;
	registry.LoadDirectory(ContractDirectory(), error);
	Document document;
	LoadText(document, "{\"schema_version\":1,\"generated\":true,\"control_contracts\":{\"Controls\":1,\"Basic\":1,\"Meters\":1},"
			"\"panel\":{\"children\":["
			"{\"type\":\"Label\",\"bounds\":[0,0,1,1],\"text\":\"x\",\"size\":12,\"align\":\"left\"},"
			"{\"type\":\"TextEntry\",\"bounds\":[0,0,1,1],\"property\":\"MIN0\",\"format\":\"%.4f\","
			"\"on\":{\"value_changed\":[{\"write\":\"COMMAND\",\"value\":\"SETMIN\",\"with\":[0,\"value\"]}]}},"
			"{\"type\":\"VirtualScrollArea\",\"bounds\":[0,0,1,1],\"children\":[{\"type\":\"CheckBox\","
			"\"bounds\":[0,0,1,1],\"property\":\"CLAMP0\",\"size\":14}],\"show_horizontal\":false,"
			"\"content_size\":[455,5]},"
			"{\"type\":\"Choice\",\"bounds\":[0,0,1,1],\"property\":\"UNIQUEID\",\"size\":12,"
			"\"items\":[{\"text\":\"(None)\",\"value\":0}]},"
			"{\"type\":\"Basic::LEDButton\",\"bounds\":[0,0,1,1],\"property\":\"UPDATEINPUTS\",\"text\":\"Update?\",\"size\":12},"
			"{\"type\":\"Knob\",\"bounds\":[0,0,1,1],\"property\":\"DEFAULT0\",\"min\":{\"property\":\"MIN0\"},"
			"\"max\":{\"property\":\"MAX0\"},\"mapping\":{\"kind\":\"symmetric_log\",\"base\":10},"
			"\"enabled\":{\"property\":\"CONNECTED0\",\"equals\":0}}],"
			"\"modes\":[{\"name\":\"Panel\",\"property\":\"PAGE\",\"value\":3}]}}");
	Report report = Resolve(document, registry);
	CHECK(report.Nodes.size() == 7, "seven nodes");

	for (size_t i = 0; i < report.Nodes.size(); ++i)
		CHECK(report.Nodes[i].Result == Valid, report.Nodes[i].Type + ": " + Problems(report.Nodes[i]));
}

static void PrivateOnlyTypesAreUnresolvedNotDropped()
{
	Registry registry;
	std::string error;
	registry.LoadDirectory(ContractDirectory(), error);
	const std::string source = Panel("{\"type\":\"PointPlot\",\"bounds\":[0,0,1,1]},"
			"{\"type\":\"Panel\",\"bounds\":[0,0,1,1],\"children\":[{\"type\":\"Knob\",\"bounds\":[0,0,1,1]}]}");
	Document document;
	LoadText(document, source);
	Report report = Resolve(document, registry, Versions(1, 1));
	CHECK(report.Nodes.size() == 2 && report.Nodes[0].Type == "PointPlot" && report.Nodes[0].Result == Unresolved
			&& report.Nodes[1].Type == "Panel" && report.Nodes[1].Result == Unresolved, "unresolved, not walked");
	CHECK(Saved(document).find("\"Knob\"") != std::string::npos, "the nested knob is kept");
}

static void MalformedContractsAreRefused()
{
	Registry registry;
	std::string error;
	Spumoni::JSONOwner missing(Spumoni::ParseJSONText("{\"namespace\":\"X\"}"));
	CHECK(!registry.Add(*missing, error), "needs version and controls");
	Spumoni::JSONOwner kind(Spumoni::ParseJSONText("{\"namespace\":\"X\",\"version\":1,\"controls\":{\"A\":{\"attributes\":{\"b\":[\"colour\"]}}}}"));
	CHECK(!registry.Add(*kind, error) && error.find("colour") != std::string::npos, "unknown kind");
	Spumoni::JSONOwner version(Spumoni::ParseJSONText("{\"namespace\":\"X\",\"version\":true,\"controls\":{}}"));
	CHECK(!registry.Add(*version, error), "version must be an integer");
}

int main()
{
	KnownControlIsValid();
	UnknownControlIsUnresolvedAndStillSaved();
	UnknownFieldIsAnExtension();
	WrongKindMissingRequiredAndAbstractAreInvalid();
	NestingIsWalkedOnlyWhereAContractSays();
	InheritedAttributesCrossNamespaces();
	DeclaredVersionsSelectTheVocabulary();
	QualifiedNamesResolveWithoutRewriting();
	UnprefixedNamesAreRootControlsOnly();
	TextKindAcceptsStringOrLocalizedObject();
	TemplatePlaceholdersMustBeSuppliedByEveryItem();
	EveryCheckedInPanelIsValidUnderItsDeclaredContracts();
	GeneratedLadspaPagesAreValid();
	PrivateOnlyTypesAreUnresolvedNotDropped();
	MalformedContractsAreRefused();

	if (failures)
		std::fprintf(stderr, "%d failures\n", failures);

	return failures ? 1 : 0;
}
