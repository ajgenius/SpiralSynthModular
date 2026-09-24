// JSON reader tests. GPL-2.0-or-later.
#include "JSONParser.h"
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <unistd.h>
using namespace SpiralJSON;

static void invalid(const std::string &input, bool folding = false)
{
	std::string error;
	std::auto_ptr<JSONValue> root(ParseJSONText(input, folding, &error));
	assert(!root.get() && !error.empty());
}

int main()
{
	std::string error("stale");
	std::auto_ptr<JSONValue> root(ParseJSONText(
	    "{\"schema_version\":1,\"id\":7,\"host\":{\"abi\":\"0.3.1\"},"
	    "\"authors\":[\"Original author\"],\"enabled\":true,\"empty\":null,"
	    "\"number\":1.250e+2,\"unicode\":\"\\u03bb\\ud83d\\ude00\","
	    "\"nul\":\"a\\u0000b\"}", false, &error));
	assert(root.get() && error.empty());
	long n = -1;
	assert(root->Get("schema_version")->Integer(n) && n == 1);
	assert(root->Get("host")->Get("abi")->Text() == "0.3.1");
	assert(root->Get("authors")->At(0)->Text() == "Original author");
	assert(root->Get("enabled")->GetType() == JSONValue::Boolean);
	assert(root->Get("enabled")->AsBool());
	assert(root->Get("empty")->GetType() == JSONValue::Null);
	assert(root->Get("number")->Text() == "1.250e+2");
	assert(!root->Get("number")->Integer(n));
	assert(root->Get("unicode")->Text() == "\xCE\xBB\xF0\x9F\x98\x80");
	assert(root->Get("nul")->Text() == std::string("a\0b", 3));
	assert(!root->Get("missing") && !root->At(0));
	assert(!root->Get("authors")->At(1));
	assert(root->Keys().size() == root->Size());

	const char *valid[] = {"null", "false", "0", "-12", "\"text\"", "[]", "{}"};
	for (size_t i = 0; i < sizeof(valid)/sizeof(*valid); ++i) {
		std::auto_ptr<JSONValue> value(ParseJSONText(valid[i], false, &error));
		assert(value.get() && error.empty());
	}
	std::ostringstream limit;
	limit << LONG_MAX;
	std::auto_ptr<JSONValue> integer(ParseJSONText(limit.str()));
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
	invalid("{\"Name\":1,\"name\":2}", true);
	root.reset(ParseJSONText("{\"Name\":1}", true));
	assert(root->Get("name") && !root->Get("Name"));
	root.reset(ParseJSONText(std::string(64, '[') + "0" + std::string(64, ']')));
	assert(root.get());
	invalid(std::string(65, '[') + "0" + std::string(65, ']'));
	invalid(std::string(16 * 1024 * 1024 + 1, ' '));
	root.reset(ParseJSONText("0" + std::string(16 * 1024 * 1024 - 1, ' ')));
	assert(root.get());

	char path[] = "/tmp/ssm-json-test-XXXXXX";
	int fd = mkstemp(path);
	assert(fd >= 0);
	close(fd);
	std::string document = "{\"text\":\"" + std::string(9000, 'x') + "\"}";
	{ std::ofstream out(path, std::ios::binary); out << document; assert(out.good()); }
	root.reset(ParseJSON(path, false, &error));
	assert(root.get() && root->Get("text")->Text().size() == 9000);
	{ std::ofstream out(path, std::ios::binary); out << document.substr(0, document.size()-1) << ",\"text\":null}"; }
	root.reset(ParseJSON(path, false, &error));
	assert(!root.get() && !error.empty());
	{ std::ofstream out(path, std::ios::binary); out << std::string(16 * 1024 * 1024 + 1, ' '); }
	root.reset(ParseJSON(path, false, &error));
	assert(!root.get() && !error.empty());
	std::remove(path);
	root.reset(ParseJSON(path, false, &error));
	assert(!root.get() && !error.empty());
	root.reset(ParseJSON(NULL, false, &error));
	assert(!root.get() && !error.empty());
	std::puts("JSON reader tests passed");
}
