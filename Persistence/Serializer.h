#pragma once

#include "ByteReader.h"
#include "FileVersions.h"
#include "ErrorHandling.h"
#include "../Engine/cnum.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>

/**
 * @addtogroup Serialization
 * @{
 */

class Serializer;
class Deserializer;

/**
 * Type codes for all serializable objects - every class saved via Serializer::_object has to have one
 */

enum class TypeCode
{
	NIL          =   0, ///< for saving a NULL
	DUP          =   1, ///< saving something more than once
	
	NAMESPACE    =   2,
	PARAMETER    =   3,
	FUNCTION     =   4,

	INVALID      =   5  // must be the last one
};

/**
 * Base for all classes that are written to files, copy&pasted or saved for undo
 */

class Serializable
{
public:
	virtual void save(Serializer   &s) const = 0;
	virtual void load(Deserializer &s) = 0;
	virtual TypeCode type_code() const{ return TypeCode::INVALID; }
};

/**
 * Frontend for ByteWriter
 */

class Serializer
{
public:
	Serializer(ByteWriter &f);
	
	unsigned version() const{ return ver; }
	
	void _bool(bool x);
	void _uint16(uint16_t x);
	void _int32(int32_t x);
	void _uint32(uint32_t x);
	void _int64(int64_t x);
	void _uint64(uint64_t x);
	void _enum(int x, int min, int max);
	
	void _float(float x);
	void _double(double x);
	void _cnum(const cnum &x);
	
	void _string(const std::string &x);
	void _object(const Serializable *x); ///< writes only one copy of every object
	void _member(const Serializable &x);
	void _data(const std::vector<unsigned char> &data, bool compress=true);
	void _marker(const char *s);
	void _raw(std::vector<unsigned char> &data);
	
private:
	const unsigned ver; ///< (major << 16) + minor
	ByteWriter &f;
	std::map<const Serializable *, uint64_t> od; ///< for keeping track of what _object already saved
};

/**
 * Frontend for ByteReader
 */

class Deserializer
{
public:
	Deserializer(ByteReader &f);
	
	unsigned version() const{ return ver; }
	bool done() const{ return f.pos() == f.size(); }
	size_t remaining() const{ return (f.size() >= f.pos() ? f.size() - f.pos() : 0); }
	
	void _bool(bool &x);
	void _uint16(uint16_t &x);
	void _int32(int32_t &x);
	void _uint32(uint32_t &x);
	void _int64(int64_t &x);
	void _uint64(uint64_t &x);
	//void _enum(int &x, int min, int max);
	template<typename T> void _enum(T &x, T min, T max)
	{
		int32_t t;
		_int32(t);
		if (t < min || t > max) throw std::runtime_error("enum out of range");
		x = (T)t;
	}
	void _float(float &x);
	void _double(double &x);
	void _cnum(cnum &x);

	void _string(std::string &x);
	void _object(Serializable *&x);
	void _member(Serializable  &x);
	void _data(std::vector<unsigned char> &data);
	void _marker(const char *s);
	void _raw(std::vector<unsigned char> &data, size_t n);

private:
	ByteReader &f;
	unsigned  ver;
	std::vector<Serializable *> od;
};

/** @} */

