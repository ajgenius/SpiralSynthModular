// SPDX-License-Identifier: GPL-2.0-or-later
// RingBuffer never hands back unwritten bytes and never overwrites
// unread ones, across the wrap; CommandRingBuffer round-trips a command.
#include "CommandRingBuffer.h"
#include <cstdio>
#include <cstring>

static int fails = 0;
#define CHECK(x) do { if (!(x)) { ++fails; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #x); } } while (0)

int main()
{
	// 7 rounds up to 8; usable space is size-1.
	RingBuffer ring(7);
	char in[16], out[16];
	for (int i = 0; i < 16; ++i) in[i] = 'a' + i;

	CHECK(!ring.Read(out, 1));           // empty: a short read is refused
	CHECK(ring.Write(in, 7));            // exactly the usable space
	CHECK(!ring.Write(in, 1));           // full: the eighth byte is refused
	CHECK(ring.Read(out, 3));
	CHECK(memcmp(out, in, 3) == 0);
	CHECK(!ring.Read(out, 5));           // only 4 left
	CHECK(ring.Read(out, 4));
	CHECK(memcmp(out, in + 3, 4) == 0);

	// Positions now sit at 7: the next write and read both wrap.
	CHECK(ring.Write(in + 8, 6));
	CHECK(!ring.Write(in, 2));
	CHECK(ring.Read(out, 6));
	CHECK(memcmp(out, in + 8, 6) == 0);
	CHECK(!ring.Read(out, 1));

	CommandRingBuffer commands(sizeof(CommandRingBuffer::Command) * 4);
	struct { int i; float f; } args = { 7, 0.5f };
	CommandRingBuffer::Command send("gain", "if", (const char *)&args, sizeof(args));
	CHECK(commands.Send(send));
	CommandRingBuffer::Command got;
	CHECK(got.Size() == 0);
	CHECK(commands.Get(got));
	CHECK(strcmp(got.Name, "gain") == 0);
	CHECK(got.Size() == 2);
	CHECK(got.GetInt(0) == 7);
	CHECK(got.GetFloat(1) == 0.5f);
	CHECK(got.GetInt(2) == 0);           // out of range, not out of bounds
	CHECK(!commands.Get(got));

	printf(fails ? "FAIL\n" : "PASS\n");
	return fails ? 1 : 0;
}
