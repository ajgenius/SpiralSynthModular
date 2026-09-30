// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Store — content-addressed immutable object store (sha256: prefix).
// Git-loose-object layout under a private directory owned by the caller.
// Synchronous worker/command-thread only; never audio work. No pruning.
// Package Ver 2 will use this for shared assets across branches.

#ifndef SPUMONI_STORE_H
#define SPUMONI_STORE_H

#include <string>

#include "Container.h"

namespace Spumoni
{

// "sha256:" + 64 lowercase hex digits.
static const char * const StoreScheme = "sha256:";

// Immutable whole-byte objects addressed as sha256:<64 hex>.
// The caller owns the store directory and its lifetime. The store pins the
// directory and rejects symlinks inside it. Never prunes.
class Store
{
public:
	Store();
	~Store();

	// Open an existing directory (creates the sha256/ tree on first Put if needed).
	// Failure leaves the previous store state unchanged.
	bool Open(const std::string &directory, std::string &error);

	static bool ValidAddress(const std::string &address);

	// Capture a regular file. Hashing + copy use the same stream.
	// On success, address receives "sha256:..." (lowercase). On failure the
	// address is unchanged; a partial object may remain if the fsync failed.
	bool PutFile(const std::string &source, std::string &address, std::string &error) const;

	bool Verify(const std::string &address, std::string &error) const;

	// Materialise a verified independent copy at a *new* destination path.
	// No hard links escape the store. Existing destinations are never overwritten.
	// On failure the incomplete destination is removed. Caller keeps dest private
	// until success and must keep it outside the store root.
	bool CopyFile(const std::string &address, const std::string &destination, std::string &error) const;

	// * Ver 2 helper: export the entire object tree into a Container::Writer
	//   under the given archive directory prefix (e.g. "assets").
	//   This is what Package uses to carry shared assets at the package root
	//   level (outside any branch).
	bool ExportObjects(Container::Writer &writer, const std::string &archivePrefix,
			   std::string &error) const;

	// After a successful Open, the on-disk root we are using (for debug / tree export).
	// Empty if not open.
	std::string Root() const { return m_RootPath; }

private:
	Store(const Store &);
	Store &operator=(const Store &);

	int m_Root;           // pinned directory fd
	std::string m_RootPath; // the path we opened (for ExportObjects)
};

} // namespace Spumoni

#endif // SPUMONI_STORE_H
