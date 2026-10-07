// SPDX-License-Identifier: GPL-2.0-or-later
// Description::Reader hands a description's values back in order, each
// extracted as an istream would have, within the span it was given.
#include "Description.h"
#include <cstdio>
#include <string>

static int fails = 0;
#define CHECK(x) do { if (!(x)) { ++fails; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #x); } } while (0)

int main()
{
	// What description-test writes, read back into the types that wrote it.
	spiralcore::Description d;
	d.Value(4).Separator(" ").Value(0.0566442f).Separator(" ").Value(true).Separator(" ").Line();
	d.Value(3).Separator(" ").Value(2).Separator("  ").Value(0.5).Separator(" ");
	d.Value("-1").Separator(" ");
	d.Value((char)'w');

	spiralcore::Description::Reader in(d);
	int a = 0, b = 0, c = 0, e = 0;
	float f = 0;
	bool t = false;
	double g = 0;
	char w = 0;
	CHECK(in.More() && in.Position() == 0);
	in.Value(a).Value(f).Value(t);
	in.Value(b).Value(c).Value(g);
	in.Value(e);
	in.Value(w);
	CHECK(a == 4 && f == 0.0566442f && t == true);
	CHECK(b == 3 && c == 2 && g == 0.5);
	CHECK(e == -1 && w == 'w');
	CHECK(!in.More() && in.Position() == 8 && !in.Failed());

	// Past the end: the target is left alone and the reader says so.
	int keep = 99;
	in.Value(keep);
	CHECK(keep == 99 && in.Failed());

	// Text that does not extract leaves the target alone too; a string
	// takes the whole value, spaces and all (a counted string in the file).
	spiralcore::Description odd;
	odd.Value(std::string("a b")).Separator(" ").Value(7).Separator(" ").Value(std::string("x"));
	spiralcore::Description::Reader r(odd);
	int n = 5;
	r.Value(n);
	CHECK(n == 5 && r.Failed() && r.Position() == 1);
	std::string s;
	spiralcore::Description::Reader r2(odd);
	r2.Value(s);
	CHECK(s == "a b" && !r2.Failed());
	r2.Value(n).Value(s);
	CHECK(n == 7 && s == "x" && !r2.More() && !r2.Failed());

	// A denormal is a number: the file's value, not the default. One C++
	// library's istream fails on it and stops reading. So is an overflow.
	spiralcore::Description tiny;
	tiny.Value(std::string("1.4013e-45")).Separator(" ").Value(std::string("0.069")).Separator(" ").Value(std::string("1e999")).Separator(" ").Value(std::string("1x"));
	spiralcore::Description::Reader tr(tiny);
	float small = 0.1f, after = 1, big = 2, bad = 3;
	tr.Value(small).Value(after).Value(big);
	CHECK(small > 0 && small < 1e-37f && after == 0.069f && big > 3e38f && !tr.Failed());
	tr.Value(bad);
	CHECK(bad == 3 && tr.Failed());

	// A span: a device reads its own values and no further, whatever it asks.
	spiralcore::Description::Reader span(d, 3, 6);
	int p = 0, q = 0;
	double h = 0;
	span.Value(p).Value(q).Value(h);
	CHECK(p == 3 && q == 2 && h == 0.5 && !span.More() && !span.Failed());
	span.Value(p);
	CHECK(p == 3 && span.Failed());
	spiralcore::Description::Reader none(d, 5, 5);
	CHECK(!none.More() && none.Position() == 5);
	spiralcore::Description::Reader clipped(d, 20, 40);
	CHECK(!clipped.More() && clipped.Position() == 8);

	if (fails) printf("%d failures\n", fails);
	else printf("PASS\n");
	return fails ? 1 : 0;
}
