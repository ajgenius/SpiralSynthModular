// SPDX-License-Identifier: GPL-2.0-or-later
// The switch between the two forms of a description: what Write() replays
// as positional text, JSON() says as values and gaps, and FromJSON() takes
// back, so either form can feed Apply and either can be written out again.
#include "Description.h"
#include "JSON.h"
#include "PositionalReader.h"
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

static int fails = 0;
#define CHECK(x) do { if (!(x)) { ++fails; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #x); } } while (0)

static std::string Replay(const spiralcore::Description &d)
{
	std::ostringstream s;
	d.Write(s);
	return s.str();
}

int main()
{
	std::string error;

	// The chain description-test uses, through JSON and back: same values,
	// same gaps, same bytes on replay, same JSON again.
	spiralcore::Description d;
	d.Value(4).Separator(" ").Value(0.0566442f).Separator(" ").Value(true).Separator(" ").Line();
	d.Value(3).Separator(" ").Value(2).Separator("  ").Value(0.5).Separator(" ");
	d.Value("-1").Separator(" ");
	d.Value((char)'w');
	spiralcore::Description back;
	CHECK(spiralcore::Description::FromJSON(d.JSON(), back, error));
	CHECK(back.Values() == d.Values());
	CHECK(back.Between() == d.Between());
	CHECK(Replay(back) == "4 0.0566442 1 \n3 2  0.5 -1 w");
	CHECK(back.JSON() == d.JSON());

	// Escapes survive the trip: quote, backslash, tab, a control byte.
	spiralcore::Description odd;
	odd.Value(std::string("a\"b\\c\td")).Value((char)1);
	CHECK(spiralcore::Description::FromJSON(odd.JSON(), back, error));
	CHECK(back.Values() == odd.Values());
	CHECK(Replay(back) == Replay(odd));

	// An empty description.
	CHECK(spiralcore::Description::FromJSON("{\"values\": [], \"between\": [\"\"]}", back, error));
	CHECK(back.Values().empty() && Replay(back).empty());

	// A patch: positional text -> reader -> JSON -> description -> the same text.
	std::auto_ptr<Spumoni::JSON> contract(Spumoni::ParseJSON(SPIRALCORE_SCHEMA_DIR "/SpiralPositionalText-0.2.x.json", &error));
	std::auto_ptr<Spumoni::JSON> history(Spumoni::ParseJSON(SPIRALCORE_SCHEMA_DIR "/SpiralPositionalText.history.json", &error));
	CHECK(contract.get() && history.get());
	if (fails)
		return 1;
	spiralcore::PositionalReader reader(*contract, history.get());
	std::string patch = "SpiralSynthModular File Ver 4\n0 0 700 600 0 0 0 0\nSectionList\n2\n"
	                    "Device 1 Plugin 0\n0 0 0  0 0 0\n"
	                    "Device 2 Plugin 18\n10 20 6 Matrix 1 30 40\n4 0 0.5 3 1 0 ";
	for (int p = 0; p < 16; ++p)
		patch += p ? "32 1 0 -1 " : "32 1 0 0 3  0.8 -1 ";
	for (int p = 0; p < 16; ++p)
		patch += "0 ";
	patch += "\n-1 0 1\n2 0 0 0 1 0 0 0\n";
	spiralcore::Description read;
	CHECK(reader.Read(patch, read, error));
	CHECK(reader.Status() == "Decoded");
	CHECK(Replay(read) == patch);
	const std::string json = read.JSON();
	CHECK(spiralcore::Description::FromJSON(json, back, error));
	CHECK(Replay(back) == patch);
	CHECK(back.JSON() == json);
	// ... and the description that came back decodes under the contract
	// exactly as the file did.
	spiralcore::Description again;
	CHECK(reader.Read(Replay(back), again, error));
	CHECK(reader.Status() == "Decoded" && reader.Devices().size() == 2);
	CHECK(again.JSON() == json);

	// What is refused, and that a refusal leaves the target alone.
	spiralcore::Description keep;
	keep.Value(1);
	CHECK(!spiralcore::Description::FromJSON("[1, 2]", keep, error) && error == "a description is an object");
	CHECK(!spiralcore::Description::FromJSON("{\"values\": [\"1\"]}", keep, error) && error == "\"between\" is not an array");
	CHECK(!spiralcore::Description::FromJSON("{\"values\": [\"1\"], \"between\": [\"\"]}", keep, error) && error == "\"between\" needs one entry more than \"values\"");
	CHECK(!spiralcore::Description::FromJSON("{\"values\": [1], \"between\": [\"\", \"\"]}", keep, error) && error == "\"values\"[0] is not a string");
	CHECK(!spiralcore::Description::FromJSON("{\"values\": [\"1\"", keep, error) && !error.empty());
	CHECK(keep.Values().size() == 1 && keep.Values()[0] == "1");

	if (fails) printf("%d failures\n", fails);
	else printf("PASS\n");
	return fails ? 1 : 0;
}
