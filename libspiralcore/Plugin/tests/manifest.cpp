// Plugin manifest reading tests. GPL-2.0-or-later.
#include "PluginManifest.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

static int failures = 0;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #x); ++failures; } } while (0)

static std::string writeFile(const char *text)
{
	char path[] = "/tmp/ssm-manifest-XXXXXX";
	int fd = mkstemp(path);
	CHECK(fd >= 0);
	FILE *file = fdopen(fd, "wb");
	std::fputs(text, file);
	std::fclose(file);
	return path;
}

int main()
{
	const char *dsp =
	    "{\"schema_version\":1,\"id\":9,\"name\":\"Amp\",\"type\":\"dsp\","
	    "\"category\":\"Amps/Mixers\",\"authors\":[\"David Griffiths\"],\"version\":\"1\","
	    "\"host\":{\"name\":\"SpiralSynthModular\",\"version\":\"0.3.1\",\"abi\":\"0.3.1\"},"
	    "\"registration\":\"module\",\"module\":\"AmpPlugin_DSP.so\"}";
	std::string path = writeFile(dsp);
	PluginManifest manifest;
	std::string error;
	CHECK(manifest.Read(path, error) && error.empty());
	CHECK(manifest.id == 9 && manifest.name == "Amp" && manifest.module == "AmpPlugin_DSP.so");
	CHECK(manifest.Compatible("0.3.1", "0.3.1", "GTK+", "2", error));
	CHECK(!manifest.Compatible("0.3.0", "0.3.1", "GTK+", "2", error));
	std::remove(path.c_str());

	const char *gui =
	    "{\"schema_version\":1,\"id\":9,\"name\":\"Amp\",\"type\":\"gui\","
	    "\"category\":\"Amps/Mixers\",\"authors\":[\"David Griffiths\"],\"version\":\"1\","
	    "\"host\":{\"name\":\"SpiralSynthModular\",\"version\":\"0.3.1\",\"abi\":\"0.3.1\"},"
	    "\"registration\":\"module\",\"module\":\"AmpPlugin_GTK2.so\","
	    "\"gui_stack\":{\"name\":\"GTK+\",\"abi\":\"2\",\"version\":\"2.x\"}}";
	path = writeFile(gui);
	CHECK(manifest.Read(path, error));
	CHECK(manifest.guiName == "GTK+" && manifest.guiABI == "2");
	CHECK(manifest.Compatible("0.3.1", "0.3.1", "GTK+", "2", error));
	CHECK(!manifest.Compatible("0.3.1", "0.3.1", "FLTK", "1.3", error));
	std::remove(path.c_str());

	const char *reference =
	    "{\"schema_version\":1,\"id\":null,\"name\":\"Filters\",\"type\":\"dsp\","
	    "\"category\":\"Filters/FX\",\"authors\":[\"Andrew Johnson\",\"David Griffiths\"],"
	    "\"version\":\"0.3.1\","
	    "\"host\":{\"name\":\"SpiralSynthModular\",\"version\":\"0.3.1\",\"abi\":\"0.3.1\"},"
	    "\"registration\":\"reference\"}";
	path = writeFile(reference);
	CHECK(manifest.Read(path, error) && manifest.id == -1 && manifest.registration == "reference");
	CHECK(!manifest.Compatible("0.3.1", "0.3.1", "GTK+", "2", error));
	std::remove(path.c_str());

	path = writeFile("{\"schema_version\":2}");
	CHECK(!manifest.Read(path, error) && !error.empty());
	std::remove(path.c_str());
	CHECK(!manifest.Read("/this/manifest/does/not/exist", error) && !error.empty());

	if (failures) {
		std::fprintf(stderr, "%d checks failed\n", failures);
		return 1;
	}
	std::puts("plugin manifest reading tests passed");
	return 0;
}
