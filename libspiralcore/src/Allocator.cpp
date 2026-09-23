#include "Allocator.h"

char *MallocAllocator::New(unsigned int size)
{
	return new char[size];
}

void MallocAllocator::Delete(char *mem)
{
	delete[] mem;
}

///////////////////////////////////////////////////////////

RealtimeAllocator::RealtimeAllocator(unsigned int size) :
m_Position(0),
m_Size(size)
{
	m_Buffer = new char[m_Size];
}

void RealtimeAllocator::Reset()
{
	m_Position=0;
}

char *RealtimeAllocator::New(unsigned int size)
{
	//cerr<<"new "<<size<<endl;
	char *ret = m_Buffer+m_Position;
	m_Position+=size;
	

	if (m_Position>m_Size)
	{
		cerr<<"out of realtime buffer mem, here we go!!! :("<<endl;
		m_Position=0;
		ret = m_Buffer;
	}
	
	return ret;
}

void RealtimeAllocator::Delete(char *mem)
{
	//cerr<<"delete"<<endl;
	// we don't need no stinking delete!
}
