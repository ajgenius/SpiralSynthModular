// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SPUMONI_STORE_H
#define SPUMONI_STORE_H

#include <string>

#include "Container.h"

namespace Spumoni
{

static const char * const StoreScheme = "sha256:";

class Store
{
public:
	Store();
	~Store();

	bool Open(const std::string &directory, std::string &error);

	static bool ValidAddress(const std::string &address);

	static std::string AddressOf(const std::string &bytes);

	bool PutFile(const std::string &source, std::string &address, std::string &error) const;

	bool PutBytes(const std::string &bytes, std::string &address, std::string &error) const;
	bool ReadBytes(const std::string &address, std::string &bytes, std::string &error) const;

	bool Verify(const std::string &address, std::string &error) const;

	bool Has(const std::string &address) const;

	std::string ObjectPath(const std::string &address) const;

	bool CopyFile(const std::string &address, const std::string &destination, std::string &error) const;

	bool ExportObjects(Container::Writer &writer, const std::string &archivePrefix,
			   std::string &error) const;

	std::string Root() const { return m_RootPath; }

private:
	Store(const Store &);
	Store &operator=(const Store &);

	int m_Root;
	std::string m_RootPath;
};

} // namespace Spumoni

#endif
