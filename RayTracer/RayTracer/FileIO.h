#pragma once

class FileIO
{
public:
	FileIO(const char *filePath);
	~FileIO();

	inline bool				IsExist() const { return m_handle != nullptr; }
	inline UINT32			GetByteSize() const { return m_byteSize; }
	inline const UINT8 *	GetBuffer() const { return m_buffer; }

	void					Load();
	void					Unload();

private:
	FILE *					m_handle{ nullptr };
	UINT8 *					m_buffer{ nullptr };
	UINT32					m_byteSize{ 0 };
	const char *			m_path{ nullptr };
};