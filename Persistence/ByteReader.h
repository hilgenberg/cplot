#pragma once

#include <vector>
#include <cstddef>
#include <cstdio>

/** @addtogroup Serialization
 *  @{
 */

/**
 * Base for things that can be read from.
 * Underlying storage for Deserializer, which is used for persistence, copy & paste and undo.
 */

class ByteReader
{
public:
	virtual ~ByteReader(){ }
	virtual void read(void *buffer, size_t len) = 0;
	virtual size_t  pos() = 0; ///< Current position (as in ftell(FILE*))
	virtual size_t size() = 0;
};

/**
 * Base for things that can be written to.
 * Underlying storage for Serializer, which is used for persistence, copy & paste and undo.
 */

class ByteWriter
{
public:
	virtual ~ByteWriter(){ }
	virtual void write(const void *buffer, size_t len) = 0;
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Wrapper for FILE* that is opened for reading.
 */

class FileReader : public ByteReader
{
public:
	FileReader(FILE *f);
	virtual void read(void *buffer, size_t len);
	virtual size_t  pos(){ return i; }
	virtual size_t size(){ return n; }
	
protected:
	FILE *f;
	size_t n, i; // length and current position
};

/**
 * Wrapper for FILE* that is opened for writing.
 */

class FileWriter : public ByteWriter
{
public:
	FileWriter(FILE *f) : f(f) { }
	virtual void write(const void *buffer, size_t len);
	
protected:
	FILE *f;
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Wrapper for std::vector<char> and char[]
 */

class ArrayReader : public ByteReader
{
public:
	/** @param d must not be resized while the ArrayReader lives. */
	ArrayReader(const std::vector<char> &d) : i(0), n(d.size()), bytes(d.data()) { }
	ArrayReader(const char *bytes, size_t len) : i(0), n(len), bytes(bytes) { }
	
	virtual void read(void *buffer, size_t len);
	virtual size_t  pos(){ return i; }
	virtual size_t size(){ return n; }
	
protected:
	size_t n, i; // length and current position
	const char *bytes;
};

/**
 * Wrapper for std::vector<char>
 */

class ArrayWriter : public ByteWriter
{
public:
	ArrayWriter(std::vector<char> &d_) : d(d_) { }
	virtual void write(const void *buffer, size_t len);
	
protected:
	std::vector<char> &d;
};

/** @} */
