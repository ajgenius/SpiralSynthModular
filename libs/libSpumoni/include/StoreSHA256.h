// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPUMONI_STORE_SHA256_H
#define SPUMONI_STORE_SHA256_H

#include <cstddef>
#include <stdint.h>
#include <string>

namespace Spumoni
{

class StoreSHA256
{
public:
	StoreSHA256();
	void update(const void *data, std::size_t length);
	void final(uint8_t output[32]);
	static std::string hex_of(const uint8_t digest[32]);
	static std::string hash_bytes(const void *data, std::size_t length);

private:
	uint32_t state[8];
	uint64_t bits;
	uint8_t buffer[64];
	std::size_t nbuf;
};

} // namespace Spumoni

#endif
