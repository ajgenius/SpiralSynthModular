// SPDX-License-Identifier: GPL-2.0-or-later

#include "Gzip.h"

#include <zlib.h>

namespace Spumoni
{

	namespace Gzip
	{
		static const int GzipWindow = 15 + 16;
		static const size_t MaxSize = 64 * 1024 * 1024;

		bool Compress(const std::string &data, std::string &out, std::string &error)
		{
			z_stream z = z_stream();

			if (deflateInit2(&z, Z_BEST_COMPRESSION, Z_DEFLATED, GzipWindow, 9, Z_DEFAULT_STRATEGY) != Z_OK)
			{
				error = "Cannot initialize gzip compressor";

				return false;
			}

			out.resize(deflateBound(&z, (uLong)data.size()) + 32);
			z.next_in = (Bytef *)data.data();
			z.avail_in = (uInt)data.size();
			z.next_out = (Bytef *)&out[0];
			z.avail_out = (uInt)out.size();
			const int result = deflate(&z, Z_FINISH);
			out.resize(z.total_out);
			deflateEnd(&z);

			if (result != Z_STREAM_END)
			{
				out.clear();
				error = "gzip compression failed";

				return false;
			}

			return true;
		}

		bool Decompress(const void *data, size_t size, std::string &out, std::string &error)
		{
			z_stream z = z_stream();
			out.clear();

			if (inflateInit2(&z, GzipWindow) != Z_OK)
			{
				error = "Cannot initialize gzip decompressor";

				return false;
			}

			z.next_in = (Bytef *)data;
			z.avail_in = (uInt)size;
			char buffer[16384];
			int result = Z_OK;

			while (result == Z_OK)
			{
				z.next_out = (Bytef *)buffer;
				z.avail_out = sizeof(buffer);
				result = inflate(&z, Z_NO_FLUSH);

				if (result == Z_OK || result == Z_STREAM_END)
					out.append(buffer, sizeof(buffer) - z.avail_out);

				if (out.size() > MaxSize)
					result = Z_MEM_ERROR;
			}

			const bool whole = result == Z_STREAM_END && z.avail_in == 0;
			inflateEnd(&z);

			if (!whole)
			{
				out.clear();
				error = "Damaged gzip data";

				return false;
			}

			return true;
		}
	}

}
