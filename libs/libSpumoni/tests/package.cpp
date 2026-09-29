// Spumoni::Package on its own: a toy application writes a package, reads it
// back, adds a save point that keeps the first, carries extras forward, and
// is refused a newer package format at the package level.
#include "Package.h"
#include "Folder.h"
#include "MemoryFolder.h"
#include "Zip.h"
#include "Directory.h"
#include "Tar.h"
#include "JSONParser.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

using namespace Spumoni;
typedef Slick::JSONValue J;

static int Fail(const std::string &why)
{
	std::fprintf(stderr, "FAIL: %s\n", why.c_str());
	return 1;
}

// The toy application: one stamp, one file per branch.
struct ToyApplication : Package::Application
{
	virtual void Describe(J &root, const Identity &) const
	{
		root.Set("format", J::MakeString("Toy File Ver 1"));
	}
	virtual bool Accept(const J &root, Identity &, std::string &error) const
	{
		const J *format = root.Get("format");
		if (!format || format->GetType() != J::String || format->Text() != "Toy File Ver 1")
		{
			error = "not a Toy manifest";
			return false;
		}
		return true;
	}
};

struct ToyPayload : Package::Payload
{
	std::string Body;
	explicit ToyPayload(const std::string &body) : Body(body) {}
	virtual bool Write(Container::Writer &writer, const std::string &branchRoot, std::string &error)
	{
		return writer.AddMemory(branchRoot + "toy.json", Body, error);
	}
};

static int Story(const Container &container, const char *extension)
{
	std::string error;
	Package::Layout layout;
	layout.ManifestName = "toy.manifest.json";
	layout.Payload.push_back("toy.json");
	layout.WorkPrefix = "spumoni-test-";
	ToyApplication application;
	Package package(container, layout, application);

	DiskFolder scratch;
	if (!scratch.Create("spumoni-test-scratch-", error))
		return Fail(error);
	const std::string path = scratch.Root() + "/toy" + extension;

	// First save: a fresh identity.
	Identity identity;
	identity.Begin();
	identity.ActiveBranchName = "first";
	identity.EnsureActiveListed();
	SaveRequest request;
	ToyPayload first("{\"toy\": 1}");
	if (!package.Write(path, identity, request, first, error))
		return Fail("write: " + error);

	// Read it back.
	DiskFolder work;
	Identity read;
	std::string branchRoot;
	if (!package.Open(path, std::string(), work, read, branchRoot, error))
		return Fail("open: " + error);
	if (read.PackageID != identity.PackageID || read.ActiveBranchID != identity.ActiveBranchID)
		return Fail("identity did not round trip");
	if (read.ActiveBranchName != "first" || read.Branches.size() != 1)
		return Fail("branch name or list did not round trip");
	if (branchRoot != Package::BranchRoot(identity.ActiveBranchID))
		return Fail("branch root: " + branchRoot);
	std::string body;
	if (!work.Read(branchRoot + "toy.json", body, error) || body != "{\"toy\": 1}")
		return Fail("payload did not round trip");
	std::string manifest;
	if (!work.Read("toy.manifest.json", manifest, error))
		return Fail(error);
	if (manifest.find("\"package\": \"Spumoni Package Ver 1\"") == std::string::npos)
		return Fail("manifest lacks the package stamp: " + manifest);
	if (manifest.find("\"format\": \"Toy File Ver 1\"") == std::string::npos)
		return Fail("manifest lacks the application stamp");

	// An extra file in the branch is carried forward by the next save;
	// the payload file is not (the new payload replaces it).
	if (!work.Write(branchRoot + "notes.txt", "kept", error))
		return Fail(error);

	// Second save point in the same package, preserving the first.
	Identity second = read;
	const std::string firstID = second.ActiveBranchID;
	if (!second.Fork("second", error))
		return Fail("fork second: " + error);
	SaveRequest again;
	again.Mode = SaveNewBranch;
	again.ExistingWorkspace = work.Root();
	again.SourceBranchID = firstID;
	std::vector<std::string> present = package.BranchesIn(again, error);
	if (present.size() != 1 || present[0] != firstID)
		return Fail("BranchesIn should see the first branch");
	ToyPayload two("{\"toy\": 2}");
	if (!package.Write(path, second, again, two, error))
		return Fail("second write: " + error);

	DiskFolder work2;
	Identity read2;
	std::string root2;
	if (!package.Open(path, std::string(), work2, read2, root2, error))
		return Fail("open second: " + error);
	if (read2.Branches.size() != 2 || read2.ActiveBranchName != "second")
		return Fail("second save point not recorded");
	if (!work2.Read(root2 + "toy.json", body, error) || body != "{\"toy\": 2}")
		return Fail("second payload wrong");
	if (!work2.Read(root2 + "notes.txt", body, error) || body != "kept")
		return Fail("extras were not carried to the new branch");
	if (!work2.Read(Package::BranchRoot(firstID) + "toy.json", body, error) || body != "{\"toy\": 1}")
		return Fail("first branch was not preserved");

	// Opening the first save point by id.
	DiskFolder work3;
	Identity read3;
	std::string root3;
	if (!package.Open(path, firstID, work3, read3, root3, error))
		return Fail("open by id: " + error);
	if (read3.ActiveBranchID != firstID || read3.ActiveBranchName != "first")
		return Fail("open by id selected the wrong branch");

	// A newer package format is refused at the package level, whatever the
	// application stamp says.
	{
		std::string text;
		if (!work2.Read("toy.manifest.json", text, error))
			return Fail(error);
		size_t at = text.find("Spumoni Package Ver 1");
		text.replace(at, std::string("Spumoni Package Ver 1").size(), "Spumoni Package Ver 2");
		Identity ignored;
		std::string ignoredRoot;
		if (package.ReadManifestText(text, ignored, ignoredRoot, error) ||
			error.find("package format 2 is newer") == std::string::npos)
			return Fail("newer package format must be refused with a package reason: " + error);
	}

	// Rewriting only the manifest keeps the branches byte for byte.
	read2.ActiveBranchName = "renamed";
	Branch *active = read2.Find(read2.ActiveBranchID);
	if (active) active->Name = "renamed";
	if (!package.WriteManifest(path, read2, error))
		return Fail("WriteManifest: " + error);
	DiskFolder work4;
	Identity read4;
	std::string root4;
	if (!package.Open(path, std::string(), work4, read4, root4, error))
		return Fail("open after manifest rewrite: " + error);
	if (read4.ActiveBranchName != "renamed")
		return Fail("manifest rewrite did not take");
	if (!work4.Read(Package::BranchRoot(firstID) + "toy.json", body, error) || body != "{\"toy\": 1}")
		return Fail("manifest rewrite lost a branch");

	// A released folder is removed only through its own prefix.
	std::string released = work4.Release();
	std::string wrong = released;
	DiskFolder::RemoveReleased(wrong, "some-other-prefix-");
	if (!Path::IsDirectory(released))
		return Fail("a foreign prefix must not remove a folder");
	DiskFolder::RemoveReleased(released, layout.WorkPrefix);
	if (Path::IsDirectory(wrong))
		return Fail("the owning prefix must remove the folder");

	std::printf("spumoni package OK (%s %s)\n", container.Kind(), extension);
	return 0;
}

// The same package opened into memory: nothing on disk until PathFor asks,
// and then only that one entry, in a scratch the folder removes with itself.
static int MemoryStory()
{
	std::string error;
	Package::Layout layout;
	layout.ManifestName = "toy.manifest.json";
	layout.Payload.push_back("toy.json");
	layout.WorkPrefix = "spumoni-test-";
	ToyApplication application;
	Zip zip;
	Package package(zip, layout, application);

	DiskFolder scratch;
	if (!scratch.Create("spumoni-test-scratch-", error))
		return Fail(error);
	const std::string path = scratch.Root() + "/toy.zip";

	Identity identity;
	identity.Begin();
	identity.ActiveBranchName = "first";
	identity.EnsureActiveListed();
	ToyPayload first("{\"toy\": 1}");
	if (!package.Write(path, identity, SaveRequest(), first, error))
		return Fail("write: " + error);

	MemoryFolder work;
	Identity read;
	std::string branchRoot;
	if (!package.Open(path, std::string(), work, read, branchRoot, error))
		return Fail("open into memory: " + error);
	std::string body;
	if (!work.Read(branchRoot + "toy.json", body, error) || body != "{\"toy\": 1}")
		return Fail("memory payload did not round trip");
	if (!work.IsDirectory("branches") || !work.IsFile("toy.manifest.json") || work.IsFile("branches"))
		return Fail("memory tree shape");
	std::vector<std::string> names;
	if (!work.List(std::string(), names) || names.size() != 2 || names[0] != "branches")
		return Fail("memory root listing");
	if (work.Bytes() == 0)
		return Fail("Bytes() should count the tree");

	// Streaming in and out of memory.
	FILE *out = work.OpenWrite(branchRoot + "big.bin", error);
	if (!out) return Fail(error);
	for (int i = 0; i < 100000; ++i) std::fputc('x', out);
	if (!work.CloseWrite(out, error)) return Fail(error);
	if (!work.Read(branchRoot + "big.bin", body, error) || body.size() != 100000)
		return Fail("streamed entry size");
	FILE *in = work.OpenRead(branchRoot + "big.bin", error);
	if (!in) return Fail(error);
	char buffer[16];
	if (std::fread(buffer, 1, sizeof(buffer), in) != sizeof(buffer) || buffer[0] != 'x')
		return Fail("streamed read");
	std::fclose(in);

	// Nothing on disk yet; PathFor puts exactly one entry there.
	std::string realPath;
	if (!work.PathFor(branchRoot + "toy.json", realPath, error))
		return Fail("PathFor: " + error);
	if (!Path::IsFile(realPath))
		return Fail("PathFor must give a real file");
	std::string onDisk;
	if (!Path::ReadFile(realPath, onDisk, error) || onDisk != "{\"toy\": 1}")
		return Fail("materialised bytes");
	std::string scratchRoot = realPath.substr(0, realPath.rfind('/' + branchRoot));
	std::vector<std::string> disk;
	if (!Path::List(scratchRoot, disk) || disk.size() != 1 || disk[0] != "branches")
		return Fail("the scratch must hold only what PathFor asked for");
	if (Path::IsFile(scratchRoot + "/toy.manifest.json") || Path::IsFile(scratchRoot + "/" + branchRoot + "big.bin"))
		return Fail("other entries must stay in memory");

	// A save point written from the memory workspace preserves the first
	// branch and carries an extra forward, all from memory.
	if (!work.Write(branchRoot + "notes.txt", "kept", error))
		return Fail(error);
	Identity second = read;
	const std::string firstID = second.ActiveBranchID;
	if (!second.Fork("second", error))
		return Fail(error);
	SaveRequest again;
	again.Mode = SaveNewBranch;
	again.Workspace = &work;
	again.SourceBranchID = firstID;
	ToyPayload two("{\"toy\": 2}");
	if (!package.Write(path, second, again, two, error))
		return Fail("write from memory: " + error);
	MemoryFolder work2;
	Identity read2;
	std::string root2;
	if (!package.Open(path, std::string(), work2, read2, root2, error))
		return Fail("reopen: " + error);
	if (!work2.Read(root2 + "notes.txt", body, error) || body != "kept")
		return Fail("extras from memory were not carried");
	if (!work2.Read(Package::BranchRoot(firstID) + "big.bin", body, error) || body.size() != 100000)
		return Fail("first branch from memory was not preserved");

	// Removing an entry drops its materialised copy; removing the folder
	// drops the scratch.
	work.RemoveEntry(branchRoot + "toy.json");
	if (Path::IsFile(realPath))
		return Fail("RemoveEntry must drop the materialised copy");
	work.Remove();
	if (Path::IsDirectory(scratchRoot))
		return Fail("Remove must drop the scratch");
	if (work.IsOpen() || work.Bytes() != 0)
		return Fail("Remove must empty the folder");

	std::puts("spumoni package OK (memory)");
	return 0;
}

int main()
{
	if (int failed = MemoryStory()) return failed;
	Zip zip;
	Directory directory;
	Tar tar;
	if (int failed = Story(zip, ".zip")) return failed;
	if (int failed = Story(directory, ".pkg")) return failed;
	if (int failed = Story(tar, ".tar")) return failed;
	if (int failed = Story(tar, ".tgz")) return failed;
	return 0;
}
