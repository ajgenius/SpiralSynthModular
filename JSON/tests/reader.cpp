// JSON reader smoke test. GPL-2.0-or-later.
#include "JSONParser.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
using namespace SpiralJSON;

static int failures = 0;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #x); ++failures; } } while (0)

int main()
{
	char path[] = "/tmp/ssm-json-test-XXXXXX";
	int fd = mkstemp(path);
	CHECK(fd >= 0);
	FILE *file = fdopen(fd, "wb");
	std::fputs("{\"id\":7,\"ok\":true,\"items\":[null,\"a\",-2]}", file);
	std::fclose(file);

	std::string error("stale");
	JSONValue *root = ParseJSON(path, &error);
	CHECK(root && error.empty());
	long n = 0;
	CHECK(root->Get("id")->Integer(n) && n == 7);
	CHECK(root->Get("ok")->Bool());
	CHECK(root->Get("items")->At(0)->GetType() == JSONValue::Null);
	CHECK(root->Get("items")->At(1)->Text() == "a");
	CHECK(root->Get("items")->Size() == 3);
	CHECK(root->Keys().size() == 3);
	delete root;

	root = ParseJSON(path, &error);
	CHECK(root);
	delete root;
	std::remove(path);
	CHECK(!ParseJSON(path, &error) && !error.empty());
	CHECK(!ParseJSON(NULL, &error));

	return failures ? 1 : 0;
}
