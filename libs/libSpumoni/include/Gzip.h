// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPUMONI_GZIP_H
#define SPUMONI_GZIP_H

#include <cstddef>
#include <string>

namespace Spumoni
{

	namespace Gzip
	{
		bool Compress(const std::string &data, std::string &out, std::string &error);
		bool Decompress(const void *data, size_t size, std::string &out, std::string &error);
	}

}

#endif
