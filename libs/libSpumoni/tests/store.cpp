#include "Store.h"
#include "StoreSHA256.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

static int Fail(const std::string &why)
{
	std::fprintf(stderr, "FAIL: %s\n", why.c_str());
	return 1;
}

int main()
{
	char tmp[] = "/tmp/spumoni-store-XXXXXX";
	assert(mkdtemp(tmp));
	std::string root = tmp;

	Spumoni::Store store;
	std::string error;

	// Open
	assert(store.Open(root, error));

	// SHA256 helper
	std::string h = Spumoni::StoreSHA256::hash_bytes("hello", 5);
	assert(h.size() == 64);

	// PutFile
	std::string src = root + "/input.bin";
	{
		std::ofstream f(src.c_str(), std::ios::binary);
		f << "content for store test";
	}
	std::string addr;
	assert(store.PutFile(src, addr, error));
	assert(Spumoni::Store::ValidAddress(addr));
	assert(addr.compare(0, 7, "sha256:") == 0);

	// Verify
	assert(store.Verify(addr, error));

	// CopyFile to new location
	std::string dst = root + "/copy.bin";
	assert(store.CopyFile(addr, dst, error));
	{
		std::ifstream f(dst.c_str(), std::ios::binary);
		std::string got((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		assert(got == "content for store test");
	}

	// Bad address rejected
	assert(!Spumoni::Store::ValidAddress("sha256:deadbeef"));
	assert(!store.Verify("sha256:0000000000000000000000000000000000000000000000000000000000000000", error));

	// PutBytes / ReadBytes: the same object whichever way the bytes arrive.
	{
		std::string fromBytes, back;
		assert(store.PutBytes("content for store test", fromBytes, error));
		assert(fromBytes == addr);
		assert(store.ReadBytes(fromBytes, back, error));
		assert(back == "content for store test");
		assert(!store.ReadBytes("sha256:" + std::string(64, '0'), back, error));
	}

	// Clean
	unlink(src.c_str());
	unlink(dst.c_str());
	rmdir(root.c_str());

	std::puts("Spumoni Store + SHA256 PASS");
	return 0;
}
