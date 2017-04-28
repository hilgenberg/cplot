#include "ByteReader.h"
#include <cassert>
#include <stdexcept>
#include <cstring>

//----------------------------------------------------------------------------------------------------------------------

#ifdef _WINDOWS

FileReader::FileReader(FileReaderFile *f) : f(f), i(0)
{
	assert(f);
	n = (size_t)f->GetLength();
	assert(f->GetPosition() == 0);
}

void FileReader::read(void *buffer, size_t len)
{
	if (i + len > n)
	{
		throw std::runtime_error("Trying to read beyond end of file. This file seems to be truncated or corrupted.");
	}

	while (len)
	{
		UINT k = (len > UINT_MAX ? UINT_MAX : (UINT)len);
		if (f->Read(buffer, k) != k)
		{
			throw std::runtime_error("Error reading from file.");
		}
		len -= k;
		i += k;
	}
}

void FileWriter::write(const void *buffer, size_t len)
{
	while (len)
	{
		UINT k = (len > UINT_MAX ? UINT_MAX : (UINT)len);
		f->Write(buffer, k);
		len -= k;
	}
}

#else

FileReader::FileReader(FILE *f) : f(f), i(0)
{
	assert(f);
	long p0 = ftell(f); // in case f has already been read from
	fseek(f, 0L, SEEK_END);
	n = ftell(f) - p0;
	fseek(f, p0, SEEK_SET);
}

void FileReader::read(void *buffer, size_t len)
{
	if (i+len > n)
	{
		throw std::runtime_error("Trying to read beyond end of file. This file seems to be truncated or corrupted.");
	}

	if (fread(buffer, len, 1, f) != 1)
	{
		throw std::runtime_error("Error reading from file.");
	}

	i += len;
}

void FileWriter::write(const void *buffer, size_t len)
{
	if (fwrite(buffer, len, 1, f) != 1)
	{
		throw std::runtime_error("Error writing to file.");
	}
}

#endif

//----------------------------------------------------------------------------------------------------------------------

void ArrayReader::read(void *buffer, size_t len)
{
	if (i+len > n)
	{
		throw std::runtime_error("Trying to read beyond end of data");
	}
	memcpy(buffer, bytes+i, len);
	i += len;
}

void ArrayWriter::write(const void *buffer, size_t len)
{
	d.insert(d.end(), (char*)buffer, (char*)buffer+len);
}

