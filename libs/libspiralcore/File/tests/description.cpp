// SPDX-License-Identifier: GPL-2.0-or-later
// Description replays the chain it recorded byte for byte, keeps the
// values apart from the gaps, and says the same in JSON.
#include "Description.h"
#include <cstdio>
#include <sstream>
#include <string>

static int fails = 0;
#define CHECK(x) do { if (!(x)) { ++fails; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #x); } } while (0)

int main()
{
	// The shapes the plugins actually use: s<<a<<" "<<b<<"  "<<c<<" "<<endl,
	// a sentinel literal, a raw char, a trailing-separator-less end.
	spiralcore::Description d;
	d.Value(4).Separator(" ").Value(0.0566442f).Separator(" ").Value(true).Separator(" ").Line();
	d.Value(3).Separator(" ").Value(2).Separator("  ").Value(0.5).Separator(" ");
	d.Value("-1").Separator(" ");
	d.Value((char)'w');

	std::ostringstream s;
	s << 4 << " " << 0.0566442f << " " << true << " " << std::endl
	  << 3 << " " << 2 << "  " << 0.5 << " "
	  << "-1 "
	  << (char)'w';

	std::ostringstream replay;
	d.Write(replay);
	CHECK(replay.str() == s.str());
	CHECK(replay.str() == "4 0.0566442 1 \n3 2  0.5 -1 w");

	CHECK(d.Values().size() == 8);
	CHECK(d.Between().size() == 9);
	CHECK(d.Values()[1] == "0.0566442");
	CHECK(d.Values()[6] == "-1");
	CHECK(d.Between()[0] == "");
	CHECK(d.Between()[3] == " \n");
	CHECK(d.Between()[8] == "");

	CHECK(d.JSON() ==
		"{\"values\": [\"4\", \"0.0566442\", \"1\", \"3\", \"2\", \"0.5\", \"-1\", \"w\"], "
		"\"between\": [\"\", \" \", \" \", \" \\n\", \" \", \"  \", \" \", \" \", \"\"]}");

	// An empty description is an empty stream.
	spiralcore::Description none;
	std::ostringstream empty;
	none.Write(empty);
	CHECK(empty.str().empty());
	CHECK(none.JSON() == "{\"values\": [], \"between\": [\"\"]}");

	// What a string value with a quote or control byte looks like in JSON.
	spiralcore::Description odd;
	odd.Value(std::string("a\"b\\c\td")).Value((char)1);
	CHECK(odd.JSON() == "{\"values\": [\"a\\\"b\\\\c\\td\", \"\\u0001\"], \"between\": [\"\", \"\", \"\"]}");

	if (fails) printf("%d failures\n", fails);
	else printf("PASS\n");
	return fails ? 1 : 0;
}
