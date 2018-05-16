#include "stdafx.h"
#include "FileIO.h"

FileIO::FileIO(const char *filePath)
	: m_path(filePath)
{
	 fopen_s(&m_handle, m_path, "rb"); // TODO writing
	if (m_handle)
	{
		fseek(m_handle, 0, SEEK_END);
		m_byteSize = ftell(m_handle);
		fseek(m_handle, 0, SEEK_SET);
	}
}

FileIO::~FileIO()
{
	Unload();
	if (m_handle != nullptr)
		fclose(m_handle);
}

void FileIO::Load()
{
	assert(m_handle != nullptr && m_byteSize > 0);
	m_buffer = new UINT8[m_byteSize];
	assert(m_buffer != nullptr);
	fread(m_buffer, 1, m_byteSize, m_handle);

}

void FileIO::Unload()
{
	if (m_buffer != nullptr)
	{
		delete[] m_buffer;
		m_buffer = nullptr;
	}
}
