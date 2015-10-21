#include "ByteReader.h"
#include <cassert>
#include <stdexcept>
#include <cstring>

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

void FileWriter::write(const void *buffer, size_t len)
{
	if (fwrite(buffer, len, 1, f) != 1)
	{
		throw std::runtime_error("Error writing to file.");
	}
}

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

//----------------------------------------------------------------------------------------------------------------------

void ArrayWriter::write(const void *buffer, size_t len)
{
	d.insert(d.end(), (char*)buffer, (char*)buffer+len);
}

