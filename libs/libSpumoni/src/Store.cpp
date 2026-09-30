// SPDX-License-Identifier: GPL-2.0-or-later
// Content-addressed storage follows the Git loose-object idea, as discussed in
// GROK's research/cas/NOTES.md. This implementation includes no Git/Darcs code.
#include "Store.h"
#include "StoreSHA256.h"
#include "Folder.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
	class Descriptor
	{
	public:
		explicit Descriptor(int value = -1):
		    Value(value)
		{
		}

		~Descriptor()
		{
			if (Value >= 0)
				close(Value);
		}

		int Value;

	private:
		Descriptor(const Descriptor &);
		Descriptor &operator=(const Descriptor &);
	};

	bool Fail(const std::string &message, std::string &error)
	{
		error = message;
		return false;
	}

	bool SystemError(const std::string &message, std::string &error)
	{
		return Fail(message + ": " + strerror(errno), error);
	}

	bool ValidPath(const std::string &path)
	{
		return !path.empty() && path.find('\0') == std::string::npos;
	}

	bool RegularFile(int file, std::string &error)
	{
		struct stat info;

		if (fstat(file, &info) != 0)
			return SystemError("Cannot inspect object input", error);

		if (!S_ISREG(info.st_mode))
			return Fail("Object input is not a regular file", error);

		return true;
	}

	int ChildDirectory(int parent, const std::string &name, bool create, std::string &error)
	{
		if (create && mkdirat(parent, name.c_str(), 0700) != 0 && errno != EEXIST)
		{
			SystemError("Cannot create object directory", error);
			return -1;
		}

		int child = openat(parent, name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

		if (child < 0)
		{
			SystemError("Cannot open object directory", error);
			return -1;
		}

		// Sync even an existing entry: another writer may just have created it.
		if (create && fsync(parent) != 0)
		{
			SystemError("Cannot sync object directory", error);
			close(child);
			return -1;
		}

		return child;
	}

	int ObjectDirectory(int root, const std::string &address, bool create, std::string &error)
	{
		if (root < 0 || !Spumoni::Store::ValidAddress(address))
		{
			Fail("Store is not open or object address is invalid", error);
			return -1;
		}

		Descriptor scheme(ChildDirectory(root, "sha256", create, error));

		if (scheme.Value < 0)
			return -1;

		Descriptor first(ChildDirectory(scheme.Value, address.substr(7, 2), create, error));

		if (first.Value < 0)
			return -1;

		return ChildDirectory(first.Value, address.substr(9, 2), create, error);
	}

	int OpenObject(int root, const std::string &address, std::string &error)
	{
		Descriptor directory(ObjectDirectory(root, address, false, error));

		if (directory.Value < 0)
			return -1;

		int file = openat(directory.Value, address.substr(11).c_str(),
				  O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);

		if (file < 0)
		{
			SystemError("Cannot open object", error);
			return -1;
		}

		if (!RegularFile(file, error))
		{
			close(file);
			return -1;
		}

		return file;
	}

	bool WriteAll(int file, const char *bytes, size_t size, std::string &error)
	{
		while (size)
		{
			ssize_t count = write(file, bytes, size);

			if (count < 0 && errno == EINTR)
				continue;

			if (count < 0)
				return SystemError("Cannot write object bytes", error);

			if (!count)
				return Fail("Object write made no progress", error);

			bytes += count;
			size -= count;
		}

		return true;
	}

	// Hash the exact bytes written. No second read of a mutable source path.
	bool Transfer(int input, int output, std::string &address, std::string &error)
	{
		Spumoni::StoreSHA256 hash;
		uint64_t total = 0;
		char bytes[65536];

		for (;;)
		{
			ssize_t count = read(input, bytes, sizeof bytes);

			if (count < 0 && errno == EINTR)
				continue;

			if (count < 0)
				return SystemError("Cannot read object bytes", error);

			if (!count)
				break;

			// FIPS 180-4 represents the bit length in 64 bits.
			if (static_cast<uint64_t>(count) > (std::numeric_limits<uint64_t>::max() / 8) - total)
				return Fail("Object exceeds the SHA-256 message length", error);

			total += count;
			hash.update(bytes, count);

			if (output >= 0 && !WriteAll(output, bytes, count, error))
				return false;
		}

		uint8_t digest[32];
		hash.final(digest);
		address = "sha256:" + Spumoni::StoreSHA256::hex_of(digest);
		return true;
	}

	class StagingFile
	{
	public:
		explicit StagingFile(int root):
		    Root(root),
		    File(-1)
		{
		}

		~StagingFile()
		{
			if (File >= 0)
			{
				close(File);
				unlinkat(Root, Name.c_str(), 0);
			}
		}

		bool Create(std::string &error)
		{
			// O_EXCL arbitrates threads and processes. Never open or remove a
			// colliding name, including abandoned files and planted symlinks.
			for (unsigned int serial = 0; serial < UINT_MAX; ++serial)
			{
				char name[80];
				snprintf(name, sizeof name, ".object-%ld-%u", static_cast<long>(getpid()), serial);
				Name = name;
				File = openat(Root, name, O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);

				if (File >= 0)
					return true;

				if (errno != EEXIST)
					return SystemError("Cannot stage object", error);
			}

			return Fail("No free object staging name", error);
		}

		int Root;
		int File;
		std::string Name;

	private:
		StagingFile(const StagingFile &);
		StagingFile &operator=(const StagingFile &);
	};
}

Spumoni::Store::Store() :
	m_Root(-1)
{
}

Spumoni::Store::~Store()
{
	if (m_Root >= 0)
		close(m_Root);
}

bool Spumoni::Store::Open(const std::string &directory, std::string &error)
{
	if (!ValidPath(directory))
		return Fail("Invalid object store directory path", error);

	std::string path = directory;

	while (path.size() > 1 && path[path.size() - 1] == '/')
		path.erase(path.size() - 1);

	int next = open(path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

	if (next < 0)
		return SystemError("Cannot open object store", error);

	if (m_Root >= 0)
		close(m_Root);

	m_Root = next;
	m_RootPath = path;
	error.clear();
	return true;
}

bool Spumoni::Store::ValidAddress(const std::string &address)
{
	if (address.size() != 71 || address.compare(0, 7, "sha256:") != 0)
		return false;

	for (size_t i = 7; i < address.size(); ++i)
	{
		char c = address[i];

		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
			return false;
	}

	return true;
}

bool Spumoni::Store::PutFile(const std::string &source, std::string &address, std::string &error) const
{
	if (m_Root < 0 || !ValidPath(source))
		return Fail("Store is not open or input path is invalid", error);

	Descriptor input(open(source.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC));

	if (input.Value < 0)
		return SystemError("Cannot open captured asset", error);

	if (!RegularFile(input.Value, error))
		return false;

	StagingFile staged(m_Root);
	std::string next;

	if (!staged.Create(error) || !Transfer(input.Value, staged.File, next, error))
		return false;

	if (fchmod(staged.File, 0400) != 0 || fsync(staged.File) != 0)
		return SystemError("Cannot sync staged object", error);

	Descriptor directory(ObjectDirectory(m_Root, next, true, error));

	if (directory.Value < 0)
		return false;

	// POSIX linkat publishes without replacing an existing object. Concurrent
	// writers can share a verified winner; no lock or stale-lock recovery is
	// needed. The temporary hard link stays private and is removed on return.
	if (linkat(m_Root, staged.Name.c_str(), directory.Value, next.substr(11).c_str(), 0) != 0)
	{
		if (errno != EEXIST)
			return SystemError("Cannot publish object", error);

		if (!Verify(next, error))
			return false;
	}

	if (fsync(directory.Value) != 0)
		return SystemError("Cannot sync published object", error);

	address = next;
	error.clear();
	return true;
}

bool Spumoni::Store::Verify(const std::string &address, std::string &error) const
{
	Descriptor input(OpenObject(m_Root, address, error));
	std::string actual;

	if (input.Value < 0 || !Transfer(input.Value, -1, actual, error))
		return false;

	if (actual != address)
		return Fail("Object bytes do not match their SHA-256 address", error);

	error.clear();
	return true;
}

bool Spumoni::Store::Has(const std::string &address) const
{
	std::string ignored;
	Descriptor input(OpenObject(m_Root, address, ignored));
	return input.Value >= 0;
}

std::string Spumoni::Store::ObjectPath(const std::string &address) const
{
	if (!Has(address))
		return std::string();
	return m_RootPath + "/sha256/" + address.substr(7, 2) + "/" + address.substr(9, 2)
		+ "/" + address.substr(11);
}

bool Spumoni::Store::CopyFile(const std::string &address,
			      const std::string &destination, std::string &error) const
{
	if (!ValidPath(destination))
		return Fail("Invalid asset destination path", error);

	Descriptor input(OpenObject(m_Root, address, error));

	if (input.Value < 0)
		return false;

	int output = open(destination.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);

	if (output < 0)
		return SystemError("Cannot create independent asset copy", error);

	std::string actual;
	bool ok = Transfer(input.Value, output, actual, error);

	if (ok && actual != address)
		ok = Fail("Object bytes do not match their SHA-256 address", error);

	if (ok && fsync(output) != 0)
		ok = SystemError("Cannot sync asset copy", error);

	if (close(output) != 0 && ok)
		ok = SystemError("Cannot close asset copy", error);

	if (!ok)
	{
		unlink(destination.c_str());
		return false;
	}

	error.clear();
	return true;
}

// Ver 2: walk the on-disk sha256 object tree and feed every object file
// into the writer under the given archivePrefix (normally "assets").
// Keeps the two-level split layout so readers can locate objects by address.
bool Spumoni::Store::ExportObjects(Container::Writer &writer,
				   const std::string &archivePrefix,
				   std::string &error) const
{
	if (m_Root < 0)
		return Fail("Store is not open", error);

	std::string shaDir = Path::Join(m_RootPath, "sha256");
	if (!Path::IsDirectory(shaDir))
		return true; // nothing stored yet

	std::vector<std::string> first;
	if (!Path::List(shaDir, first))
		return true;

	for (size_t i = 0; i < first.size(); ++i)
	{
		std::string d1 = Path::Join(shaDir, first[i]);
		if (!Path::IsDirectory(d1)) continue;

		std::vector<std::string> second;
		if (!Path::List(d1, second)) continue;

		for (size_t j = 0; j < second.size(); ++j)
		{
			std::string d2 = Path::Join(d1, second[j]);
			if (!Path::IsDirectory(d2)) continue;

			std::vector<std::string> objs;
			if (!Path::List(d2, objs)) continue;

			for (size_t k = 0; k < objs.size(); ++k)
			{
				std::string objPath = Path::Join(d2, objs[k]);
				if (!Path::IsFile(objPath)) continue;

				// Preserve the store's internal sha256/ layout under the
				// archive prefix so that on extract + GetStore() the
				// reopened workspace sees a correct store root containing
				// sha256/XX/YY/...
				std::string arcName = archivePrefix;
				if (!arcName.empty() && arcName[arcName.size()-1] != '/')
					arcName += "/";
				arcName += "sha256/" + first[i] + "/" + second[j] + "/" + objs[k];

				if (!writer.AddFile(arcName, objPath, error))
					return false;
			}
		}
	}
	error.clear();
	return true;
}
