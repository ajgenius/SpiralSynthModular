// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::StoreSHA256 — internal streaming SHA-256 digest (for Store).
// Adapted from research/cas/src/sha256.cpp, authored by GROK / ajgenius.
// Algorithm: NIST FIPS PUB 180-4, sections 4.1.2, 4.2.2, 5 and 6.2.
// https://doi.org/10.6028/NIST.FIPS.180-4
// No OpenSSL, Crypto++, Git or Darcs source is included.
#include "StoreSHA256.h"

#include <cstring>

static uint32_t rotr(uint32_t x, uint32_t n)
{
	return (x >> n) | (x << (32 - n));
}

static void transform(uint32_t state[8], const uint8_t block[64])
{
	static const uint32_t K[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
	uint32_t w[64];

	for (int i = 0; i < 16; i++)
	{
		w[i] = (uint32_t)block[i * 4] << 24 | (uint32_t)block[i * 4 + 1] << 16 |
			(uint32_t)block[i * 4 + 2] << 8 | (uint32_t)block[i * 4 + 3];
	}

	for (int i = 16; i < 64; i++)
	{
		uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	uint32_t a = state[0];
	uint32_t b = state[1];
	uint32_t c = state[2];
	uint32_t d = state[3];
	uint32_t e = state[4];
	uint32_t f = state[5];
	uint32_t g = state[6];
	uint32_t h = state[7];

	for (int i = 0; i < 64; i++)
	{
		uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
		uint32_t ch = (e & f) ^ (~e & g);
		uint32_t t1 = h + S1 + ch + K[i] + w[i];
		uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
		uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
		uint32_t t2 = S0 + maj;

		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
	state[5] += f;
	state[6] += g;
	state[7] += h;
}

Spumoni::StoreSHA256::StoreSHA256()
{
	state[0] = 0x6a09e667;
	state[1] = 0xbb67ae85;
	state[2] = 0x3c6ef372;
	state[3] = 0xa54ff3a;
	state[4] = 0x510e527f;
	state[5] = 0x9b05688c;
	state[6] = 0x1f83d9ab;
	state[7] = 0x5be0cd19;
	bits = 0;
	nbuf = 0;
}

void Spumoni::StoreSHA256::update(const void *data, std::size_t len)
{
	const uint8_t *p = static_cast<const uint8_t *>(data);
	bits += (uint64_t)len * 8;

	while (len > 0)
	{
		std::size_t n = 64 - nbuf;

		if (n > len)
		{
			n = len;
		}

		std::memcpy(buffer + nbuf, p, n);
		nbuf += n;
		p += n;
		len -= n;

		if (nbuf == 64)
		{
			transform(state, buffer);
			nbuf = 0;
		}
	}
}

void Spumoni::StoreSHA256::final(uint8_t out[32])
{
	buffer[nbuf++] = 0x80;

	if (nbuf > 56)
	{
		while (nbuf < 64)
		{
			buffer[nbuf++] = 0;
		}

		transform(state, buffer);
		nbuf = 0;
	}

	while (nbuf < 56)
	{
		buffer[nbuf++] = 0;
	}

	for (int i = 7; i >= 0; i--)
	{
		buffer[nbuf++] = (uint8_t)(bits >> (i * 8));
	}

	transform(state, buffer);

	for (int i = 0; i < 8; i++)
	{
		out[i * 4] = (uint8_t)(state[i] >> 24);
		out[i * 4 + 1] = (uint8_t)(state[i] >> 16);
		out[i * 4 + 2] = (uint8_t)(state[i] >> 8);
		out[i * 4 + 3] = (uint8_t)state[i];
	}
}

std::string Spumoni::StoreSHA256::hex_of(const uint8_t dig[32])
{
	static const char *hexd = "0123456789abcdef";
	std::string s(64, '0');

	for (int i = 0; i < 32; i++)
	{
		s[i * 2] = hexd[dig[i] >> 4];
		s[i * 2 + 1] = hexd[dig[i] & 0xf];
	}

	return s;
}

std::string Spumoni::StoreSHA256::hash_bytes(const void *data, std::size_t len)
{
	Spumoni::StoreSHA256 h;
	uint8_t dig[32];

	h.update(data, len);
	h.final(dig);

	return hex_of(dig);
}
