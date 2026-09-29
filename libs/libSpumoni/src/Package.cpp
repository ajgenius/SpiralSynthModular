// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Package — the package-format stamp.

#include "Package.h"

#include <cerrno>
#include <cstdlib>
#include <sstream>

using namespace std;

namespace Spumoni
{

	string Package::FormatName()
	{
		return "Spumoni Package";
	}

	string Package::Stamp()
	{
		return Stamp(FormatVersion);
	}

	string Package::Stamp(long version)
	{
		ostringstream text;
		text << FormatName() << " Ver " << version;
		return text.str();
	}

	Package::Status Package::Check(const string &stamp, long &found)
	{
		found = 0;
		const string prefix = FormatName() + " Ver ";
		if (stamp.compare(0, prefix.size(), prefix) != 0)
			return Foreign;

		const string number = stamp.substr(prefix.size());
		if (number.empty())
			return Foreign;
		char *end = NULL;
		errno = 0;
		long value = strtol(number.c_str(), &end, 10);
		if (errno == ERANGE || end != number.c_str() + number.size() || value < 1)
			return Foreign;

		found = value;
		if (value == FormatVersion) return Current;
		return value < FormatVersion ? Older : Newer;
	}

	string Package::Reason(Status status, long found)
	{
		ostringstream text;
		switch (status)
		{
			case Current:
				return string();
			case Older:
				text << "package format " << found << " is older than " << FormatVersion << " (still read)";
				break;
			case Newer:
				text << "package format " << found << " is newer than this build's " << FormatVersion
					<< "; a newer build wrote this file";
				break;
			case Foreign:
				text << "not a " << FormatName() << " stamp";
				break;
		}
		return text.str();
	}

} // namespace Spumoni
