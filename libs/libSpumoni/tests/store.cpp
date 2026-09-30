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

	// SHA256 helper, against FIPS 180-4's own vectors: "", "abc", and the
	// two-block "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq".
	std::string h = Spumoni::StoreSHA256::hash_bytes("hello", 5);
	assert(h.size() == 64);
	assert(Spumoni::StoreSHA256::hash_bytes("", 0)
		== "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	assert(Spumoni::StoreSHA256::hash_bytes("abc", 3)
		== "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	assert(Spumoni::StoreSHA256::hash_bytes("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56)
		== "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");

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
