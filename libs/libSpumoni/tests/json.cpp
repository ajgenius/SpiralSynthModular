// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::JSON reader: what the tree relies on. These are the cases the
// yajl reader it replaced was held to.
#include "JSON.h"
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <unistd.h>
using namespace Spumoni;

static void invalid(const std::string &input)
{
	std::string error;
	std::auto_ptr<JSON> root(ParseJSONText(input, &error));
	assert(!root.get() && !error.empty());
}

int main()
{
	std::string error("stale");
	std::auto_ptr<JSON> root(ParseJSONText(
	    "{\"schema_version\":1,\"id\":7,\"host\":{\"abi\":\"0.3.1\"},"
	    "\"authors\":[\"Original author\"],\"enabled\":true,\"empty\":null,"
	    "\"number\":1.250e+2,\"unicode\":\"\\u03bb\\ud83d\\ude00\","
	    "\"nul\":\"a\\u0000b\"}", &error));
	assert(root.get() && error.empty());
	long n = -1;
	assert(root->Get("schema_version")->Integer(n) && n == 1);
	assert(root->Get("host")->Get("abi")->Text() == "0.3.1");
	assert(root->Get("authors")->At(0)->Text() == "Original author");
	assert(root->Get("enabled")->GetType() == JSON::Boolean);
	assert(root->Get("enabled")->AsBool());
	assert(root->Get("empty")->GetType() == JSON::Null);
	assert(root->Get("number")->Text() == "1.250e+2");
	assert(!root->Get("number")->Integer(n) && n == 1);
	assert(root->Get("unicode")->Text() == "\xCE\xBB\xF0\x9F\x98\x80");
	assert(root->Get("nul")->Text() == std::string("a\0b", 3));
	assert(!root->Get("missing") && !root->At(0));
	assert(!root->Get("authors")->At(1));
	assert(root->Keys().size() == root->Size());
	assert(root->Get(std::string("id"))->Integer(n) && n == 7);

	const char *valid[] = {"null", "false", "0", "-12", "\"text\"", "[]", "{}"};
	for (size_t i = 0; i < sizeof(valid)/sizeof(*valid); ++i) {
		std::auto_ptr<JSON> value(ParseJSONText(valid[i], &error));
		assert(value.get() && error.empty());
	}
	std::ostringstream limit;
	limit << LONG_MAX;
	std::auto_ptr<JSON> integer(ParseJSONText(limit.str()));
	assert(integer->Integer(n) && n == LONG_MAX);
	integer.reset(ParseJSONText(limit.str() + "0"));
	n = 123;
	assert(!integer->Integer(n) && n == 123);
	const char *bad[] = {"", " ", "{", "[1,]", "{\"x\":1,}", "/*x*/{}",
	    "{}[]", "{}junk", "01", "+1", "NaN", "1.", "1e", "\"\\q\"",
	    "{\"x\":null,\"x\":1}", "{\"x\":[],\"x\":{}}"};
	for (size_t i = 0; i < sizeof(bad)/sizeof(*bad); ++i) invalid(bad[i]);
	invalid(std::string("{}\0[]", 5));
	invalid(std::string("\"\xFF\""));
	invalid(std::string("\"\xC0\x80\""));   // overlong NUL
	invalid("\"\\ud800\"");
	invalid("\"\\udc00\"");
	invalid("\"\\ud800x\"");
	invalid("{\"a\\u0000b\":1,\"a\\u0000b\":2}");
	root.reset(ParseJSONText("{\"a\\u0000b\":1,\"a\":2}"));
	assert(root.get());
	assert(root->Get(std::string("a\0b", 3))->Integer(n) && n == 1);
	assert(root->Get("a")->Integer(n) && n == 2);

	root.reset(ParseJSONText("{\"Name\":1}"));
	assert(root->Get("Name") && !root->Get("name"));   // keys are what they are
	root.reset(ParseJSONText(std::string(64, '[') + "0" + std::string(64, ']')));
	assert(root.get());
	invalid(std::string(65, '[') + "0" + std::string(65, ']'));
	invalid(std::string(16 * 1024 * 1024 + 1, ' '));
	root.reset(ParseJSONText("0" + std::string(16 * 1024 * 1024 - 1, ' ')));
	assert(root.get());

	// What is written reads back the same.
	root.reset(ParseJSONText("{\"b\":[1,2.5,\"x\\n\"],\"a\":{\"t\":true,\"n\":null}}"));
	std::string text = root->Stringify(false);
	assert(text == "{\"a\":{\"n\":null,\"t\":true},\"b\":[1,2.5,\"x\\n\"]}");
	std::auto_ptr<JSON> again(ParseJSONText(text, &error));
	assert(again.get() && again->Stringify(false) == text);

	char path[] = "/tmp/spumoni-json-test-XXXXXX";
	int fd = mkstemp(path);
	assert(fd >= 0);
	close(fd);
	std::string document = "{\"text\":\"" + std::string(9000, 'x') + "\"}";
	{ std::ofstream out(path, std::ios::binary); out << document; assert(out.good()); }
	root.reset(ParseJSON(path, &error));
	assert(root.get() && root->Get("text")->Text().size() == 9000);
	{ std::ofstream out(path, std::ios::binary); out << document.substr(0, document.size()-1) << ",\"text\":null}"; }
	root.reset(ParseJSON(path, &error));
	assert(!root.get() && !error.empty());
	{ std::ofstream out(path, std::ios::binary); out << std::string(16 * 1024 * 1024 + 1, ' '); }
	root.reset(ParseJSON(path, &error));
	assert(!root.get() && !error.empty());
	std::remove(path);
	root.reset(ParseJSON(path, &error));
	assert(!root.get() && !error.empty());
	root.reset(ParseJSON(NULL, &error));
	assert(!root.get() && !error.empty());
	std::puts("JSON reader tests passed");
}
