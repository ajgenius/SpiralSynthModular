// SPDX-License-Identifier: GPL-2.0-or-later
#include "Gzip.h"

#include <cassert>
#include <cstdio>
#include <string>

int main()
{
	std::string text;

	for (int i = 0; i < 2000; ++i)
		text += "{\"name\":\"knob\",\"bounds\":[0,0,40,40]},";

	std::string packed, error;
	assert(Spumoni::Gzip::Compress(text, packed, error));
	assert(packed.size() < text.size() / 10);
	assert((unsigned char)packed[0] == 0x1f && (unsigned char)packed[1] == 0x8b);
	std::string unpacked;
	assert(Spumoni::Gzip::Decompress(packed.data(), packed.size(), unpacked, error) && unpacked == text);
	std::string empty;
	assert(Spumoni::Gzip::Compress("", packed, error));
	assert(Spumoni::Gzip::Decompress(packed.data(), packed.size(), empty, error) && empty.empty());
	std::string broken = packed.substr(0, packed.size() - 3);
	assert(!Spumoni::Gzip::Decompress(broken.data(), broken.size(), unpacked, error) && unpacked.empty());
	assert(!Spumoni::Gzip::Decompress("not gzip", 8, unpacked, error));
	std::printf("ok   gzip round trip\n");

	return 0;
}
