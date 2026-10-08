// SPDX-License-Identifier: GPL-2.0-or-later
// Shared behavioral contract for public Spumoni and retained Spicy trees.
#ifndef SPIRAL_JSON_CONFORMANCE_H
#define SPIRAL_JSON_CONFORMANCE_H
#include <cassert>
#include <string>

template<class API> void JSONConformance()
{
	typename API::Value *value;
	std::string error;
	const char *invalid[] = {
		"", " ", "[", "{", "[1,]", "{\"a\":1,}", "01", "-", "+1", "1.", "1e+",
		"true false", "{}x", "/*comment*/{}", "{}/*comment*/", "[NaN]",
		"{\"a\":null,\"a\":2}", "{\"a\\u0000b\":1,\"a\\u0000b\":2}",
		"\"\\ud800\"", "\"\\udc00\"", "\"\\ud800x\"", "\"\\ud800\\u0041\"",
		"\"\xff\"", "\"\xc0\x80\"", "\"\xed\xa0\x80\"", "\"\xf4\x90\x80\x80\"",
		"\"\xe2\x82", "\"\\u00", "\"\\", "tru"
	};
	for (size_t i = 0; i < sizeof invalid / sizeof *invalid; ++i)
	{
		error.clear();
		value = API::Parse(invalid[i], &error);
		assert(!value && !error.empty());
	}

	value = API::Parse(std::string("{}\0[]", 5), &error);
	assert(!value && !error.empty());
	value = API::Parse(std::string(65, '[') + "0" + std::string(65, ']'), &error);
	assert(!value && !error.empty());
	value = API::Parse(std::string(64, '[') + "0" + std::string(64, ']'), &error);
	assert(value && error.empty());
	API::Release(value);

	value = API::Parse("{\"a\\u0000b\":1,\"a\":2,\"n\":-1.2300e+40,\"s\":\"a\\u0000b\\ud83d\\ude00\"}", &error);
	assert(value && error.empty());
	long number = 0;
	assert(value->Get(std::string("a\0b", 3))->Integer(number) && number == 1);
	assert(value->Get("a")->Integer(number) && number == 2);
	assert(value->Get("n")->Text() == "-1.2300e+40");
	assert(value->Get("s")->Text() == std::string("a\0b", 3) + "\xf0\x9f\x98\x80");
	std::string written = value->Stringify(false);
	API::Release(value);
	value = API::Parse(written, &error);
	assert(value && value->Stringify(false) == written);
	API::Release(value);

	value = API::Parse(std::string(16 * 1024 * 1024 + 1, ' '), &error);
	assert(!value && !error.empty());
	value = API::Parse("0" + std::string(16 * 1024 * 1024 - 1, ' '), &error);
	assert(value && error.empty());
	API::Release(value);
}
#endif
