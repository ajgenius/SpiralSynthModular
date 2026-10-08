// Copyright (C) 2004 David Griffiths <dave@pawfal.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include "RingBuffer.h"

using namespace std;

// The mask arithmetic needs a power of two.
static unsigned int RoundPow2(unsigned int n)
{
	unsigned int p = 1;
	while (p < n) p <<= 1;
	return p;
}

RingBuffer::RingBuffer(unsigned int size):
m_ReadPos(0),
m_WritePos(0),
m_Size(RoundPow2(size)),
m_SizeMask(m_Size-1),
m_Buffer(NULL)
{
	m_Buffer = new char[m_Size];
	memset(m_Buffer,'Z',m_Size);
}

RingBuffer::~RingBuffer()	
{
	delete[] m_Buffer;
}

bool RingBuffer::Write(char *src, unsigned int size)
{
	//cerr<<"write pos: "<<m_WritePos<<endl;
	unsigned int space=WriteSpace();
	if (space<size) return false;
	
	//cerr<<size<<" "<<space<<endl;
	
	unsigned int write = m_WritePos;
	if (size<m_Size-write)
	{
		//cerr<<"written to: "<<write<<endl;
		memcpy(&(m_Buffer[write]), src, size);
		write += size;
	}
	else // have to split data over boundary
	{
		unsigned int first = m_Size-write;
		
		memcpy(&(m_Buffer[write]), src, first);
		memcpy(&(m_Buffer[0]), &src[first], size-first);
		write = size-first;
	}
	
	// Publish the position only after the data it covers.
	__sync_synchronize();
	m_WritePos = write & m_SizeMask;
	return true;
}

bool RingBuffer::Read(char *dest, unsigned int size)
{
	//cerr<<"read pos: "<<m_ReadPos<<endl;
	// A short read must not copy bytes the writer has not published.
	unsigned int space=ReadSpace();
	if (space<size || size>m_Size) return false;
	
	unsigned int read = m_ReadPos;
	if (size<m_Size-read)
	{
		//cerr<<"reading from: "<<read<<endl;
		memcpy(dest, &(m_Buffer[read]), size);
		read += size;
	}
	else // have to split data over boundary
	{
		unsigned int first = m_Size-read;
		
		memcpy(dest, &(m_Buffer[read]), first);
		memcpy(&dest[first], &(m_Buffer[0]), size-first);
		read = size-first;
	}
	
	// Release the space only after the data has been copied out.
	__sync_synchronize();
	m_ReadPos = read & m_SizeMask;
	return true;
}

void RingBuffer::Dump()
{
	for (unsigned int i=0; i<m_Size; i++) cerr<<m_Buffer[i];
	cerr<<endl;
}

unsigned int RingBuffer::WriteSpace()
{
	unsigned int read = m_ReadPos;
	unsigned int write = m_WritePos;
	__sync_synchronize();
	
	// `& m_SizeMask - 1` parsed as `& (mask - 1)` and over-reported by one.
	return (read - write - 1) & m_SizeMask;
}

unsigned int RingBuffer::ReadSpace()
{
	unsigned int read = m_ReadPos;
	unsigned int write = m_WritePos;
	__sync_synchronize();

	return (write - read) & m_SizeMask;
}
