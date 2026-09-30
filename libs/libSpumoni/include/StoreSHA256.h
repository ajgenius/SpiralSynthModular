// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::StoreSHA256 — internal streaming SHA-256 digest (for Store).
// Adapted from GROK / ajgenius's research/cas implementation.
// Algorithm: NIST FIPS PUB 180-4, sections 4.1.2, 4.2.2, 5 and 6.2.
// https://doi.org/10.6028/NIST.FIPS.180-4
// No OpenSSL, Crypto++, Git or Darcs source is included.
#ifndef SPUMONI_STORE_SHA256_H
#define SPUMONI_STORE_SHA256_H

#include <cstddef>
#include <stdint.h>
#include <string>

namespace Spumoni
{

// Internal streaming digest for the content-addressed store.
// Call final once after the last update.
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

#endif // SPUMONI_STORE_SHA256_H
