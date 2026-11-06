// SPDX-License-Identifier: GPL-2.0-or-later
#include "PanelCorpus.h"
#include "PanelNode.h"
#include "PanelRepeat.h"

using namespace Spiral::Panels;

static std::string Children(const Document &document)
{
	Spumoni::JSONOwner panel(document.Panel()->ToJSON());

	return panel->Get("children")->Stringify(false);
}

static std::string Expanded(const std::string &source)
{
	Document document;
	CHECK(LoadText(document, source), "loads");
	Expand(document);

	return Children(document);
}

static std::string Canonical(const char *text)
{
	return ::Canonical(std::string(text));
}

static void ItemsSupplyVariablesAndQuotedPlaceholdersKeepTheirKind()
{
	const std::string out = Expanded("{\"panel\":{\"children\":[{\"type\":\"Repeat\",\"name\":\"bits\",\"bounds\":[10,20,0,0],"
			"\"step_y\":23,\"items\":[{\"choice\":16,\"text\":{\"default\":\"Sixteen\"}},{\"choice\":24,\"text\":\"24bit\"}],"
			"\"children\":[{\"type\":\"LEDButton\",\"name\":\"bits-{choice}\",\"bounds\":[0,0,23,23],"
			"\"choice\":\"{choice}\",\"text\":\"{text}\",\"tooltip\":\"Item {index} / {index0}\"}]}]}}");
	CHECK(out == Canonical("[{\"type\":\"LEDButton\",\"name\":\"bits-16\",\"bounds\":[10,20,23,23],\"choice\":16,"
			"\"text\":{\"default\":\"Sixteen\"},\"tooltip\":\"Item 0 / 0\"},"
			"{\"type\":\"LEDButton\",\"name\":\"bits-24\",\"bounds\":[10,43,23,23],\"choice\":24,\"text\":\"24bit\","
			"\"tooltip\":\"Item 1 / 1\"}]"), out);
}

static void MaxCountRepeatsByNumberWithIndexBase()
{
	const std::string out = Expanded("{\"panel\":{\"children\":[{\"type\":\"Repeat\",\"name\":\"ch\",\"bounds\":[0,0,0,0],"
			"\"max_count\":3,\"index_base\":1,\"step_x\":20,\"children\":[{\"type\":\"LED\",\"name\":\"peak-{index}\","
			"\"bounds\":[2,1,5,5],\"choice\":\"{index0}\"}]}]}}");
	CHECK(out == Canonical("[{\"type\":\"LED\",\"name\":\"peak-1\",\"bounds\":[2,1,5,5],\"choice\":0},"
			"{\"type\":\"LED\",\"name\":\"peak-2\",\"bounds\":[22,1,5,5],\"choice\":1},"
			"{\"type\":\"LED\",\"name\":\"peak-3\",\"bounds\":[42,1,5,5],\"choice\":2}]"), out);
}

static void ItemsMayAlsoVaryBounds()
{
	const std::string out = Expanded("{\"panel\":{\"children\":[{\"type\":\"Repeat\",\"name\":\"op\",\"bounds\":[100,50,0,0],"
			"\"items\":[{\"choice\":1,\"bounds\":[0,0,38,20]},{\"choice\":5,\"bounds\":[39,0,38,20]}],"
			"\"children\":[{\"type\":\"Button\",\"name\":\"op-{choice}\",\"bounds\":\"{bounds}\",\"choice\":\"{choice}\"}]}]}}");
	CHECK(out == Canonical("[{\"type\":\"Button\",\"name\":\"op-1\",\"bounds\":[100,50,38,20],\"choice\":1},"
			"{\"type\":\"Button\",\"name\":\"op-5\",\"bounds\":[139,50,38,20],\"choice\":5}]"), out);
}

static void RepeatsInsideContainersExpandInPlace()
{
	const std::string out = Expanded("{\"panel\":{\"children\":[{\"type\":\"Notebook\",\"bounds\":[0,0,9,9],\"pages\":["
			"{\"title\":\"A\",\"children\":[{\"type\":\"Repeat\",\"bounds\":[1,1,0,0],\"max_count\":2,\"step_x\":3,"
			"\"children\":[{\"type\":\"LED\",\"bounds\":[0,0,2,2],\"name\":\"l{index}\"}]}]}]}]}}");
	CHECK(out == Canonical("[{\"type\":\"Notebook\",\"bounds\":[0,0,9,9],\"pages\":[{\"title\":\"A\",\"children\":["
			"{\"type\":\"LED\",\"bounds\":[1,1,2,2],\"name\":\"l0\"},{\"type\":\"LED\",\"bounds\":[4,1,2,2],\"name\":\"l1\"}]}]}]"),
			out);
}

static void ACopyIsTheTemplateWithItsOrigin()
{
	Document document;
	LoadText(document, "{\"panel\":{\"children\":[{\"type\":\"Slick::Repeat\",\"bounds\":[5,7,0,0],\"count_property\":\"CHANNELS\","
			"\"max_count\":4,\"step_y\":10,\"children\":[{\"type\":\"Knob\",\"bounds\":[1,1,8,8],\"property\":\"GAIN{index}\"}]}]}}");
	const Node &node = *document.Panel()->Children()[0];
	CHECK(Repeat::Is(node), "a qualified Repeat is a Repeat");
	Repeat repeat(node);
	CHECK(repeat.DefaultCount() == 4 && repeat.CountProperty() == "CHANNELS" && !repeat.ByItems(), "counted");
	long x, y;
	repeat.Origin(2, x, y);
	CHECK(x == 5 && y == 27, "the third copy's origin");
	std::vector<Node *> copy = repeat.Copy(2);
	CHECK(copy.size() == 1, "one node per template child");
	std::string property;
	CHECK(copy.size() == 1 && copy[0]->String("property", property) && property == "GAIN2", "substituted");
	CHECK(copy.size() == 1 && copy[0]->GetBounds().X == 1, "the template's own bounds");

	for (size_t i = 0; i < copy.size(); ++i)
		delete copy[i];
}

static void ExpandingTheCorpusKeepsEveryOtherNode()
{
	const std::vector<std::string> paths = CorpusPanels();

	for (size_t i = 0; i < paths.size(); ++i)
	{
		Document document;
		std::string error;
		document.Load(CorpusPanel(paths[i]), error);
		const std::string before = Saved(document);
		const size_t expanded = Expand(document);

		if (!expanded)
			CHECK(Saved(document) == before, paths[i] + ": no Repeat, no change");
		else
			CHECK(Saved(document).find("\"Repeat\"") == std::string::npos, paths[i] + ": every Repeat expanded");
	}
}

int main()
{
	ItemsSupplyVariablesAndQuotedPlaceholdersKeepTheirKind();
	MaxCountRepeatsByNumberWithIndexBase();
	ItemsMayAlsoVaryBounds();
	RepeatsInsideContainersExpandInPlace();
	ACopyIsTheTemplateWithItsOrigin();
	ExpandingTheCorpusKeepsEveryOtherNode();

	if (failures)
		std::fprintf(stderr, "%d failures\n", failures);

	return failures ? 1 : 0;
}
