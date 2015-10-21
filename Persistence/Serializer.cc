#include "Serializer.h"
#include "../Engine/Namespace/Namespace.h"
#include "../Engine/Namespace/UserFunction.h"
#include "../Engine/Namespace/Parameter.h"

#include <cassert>
#include <zlib.h>
#include <iostream>
#include <cstring>
#include <memory>

static bool zlibInflate(const std::vector<unsigned char> &in, std::vector<unsigned char> &out)
{
	if (in.size() == 0){ out.resize(0); return true; }
	
	unsigned full_length = (unsigned)in.size();
	unsigned half_length = full_length / 2;
	
	out.resize(full_length + half_length);
	bool done = false;
	int status;
	
	z_stream strm;
	strm.next_in = (Bytef*) in.data();
	strm.avail_in = full_length;
	strm.total_out = 0;
	strm.zalloc = Z_NULL;
	strm.zfree  = Z_NULL;
	
	if (inflateInit(&strm) != Z_OK) return false;
	
	while (!done)
	{
		// Make sure we have enough room and reset the lengths.
		if (strm.total_out >= out.size()) out.resize(out.size() + half_length);
		strm.next_out = (Bytef*)(out.data() + strm.total_out);
		strm.avail_out = (unsigned)(out.size() - strm.total_out);
		
		// Inflate another chunk.
		status = inflate (&strm, Z_SYNC_FLUSH);
		if (status == Z_STREAM_END) done = true;
		else if (status != Z_OK) break;
	}
	if (inflateEnd(&strm) != Z_OK) return false;
	
	// Set real length.
	if (done) out.resize(strm.total_out);
	return done;
}

static bool zlibDeflate(const std::vector<unsigned char> &in, std::vector<unsigned char> &out)
{
	if (in.empty()){ out.resize(0); return true; }
	
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree  = Z_NULL;
	strm.opaque = Z_NULL;
	strm.total_out = 0;
	strm.next_in = (Bytef*) in.data();
	strm.avail_in = (unsigned)in.size();
	
	// Compresssion Levels:
	//   Z_NO_COMPRESSION
	//   Z_BEST_SPEED
	//   Z_BEST_COMPRESSION
	//   Z_DEFAULT_COMPRESSION
	
	if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK) return false;
	
	out.resize(16*1024);
	
	do
	{
		if (strm.total_out >= out.size()) out.resize(out.size() + 16*1024);
		strm.next_out = (Bytef*)(out.data() + strm.total_out);
		strm.avail_out = (unsigned)(out.size() - strm.total_out);
		deflate(&strm, Z_FINISH);
	}
	while (strm.avail_out == 0);
	
	deflateEnd(&strm);
	
	out.resize(strm.total_out);
	return true;
}

//----------------------------------------------------------------------------------------------------------------------
//  Serializer
//----------------------------------------------------------------------------------------------------------------------

Serializer::Serializer(ByteWriter &f) : ver(CURRENT_VERSION), f(f)
{
	_marker("CPLT");
	_uint32(ver);
}

void Serializer::_bool(bool x){ char c = x; f.write(&c, 1); }
void Serializer::_uint16(uint16_t x){ f.write(&x, 2); }
void Serializer::_int32 (int32_t  x){ f.write(&x, 4); }
void Serializer::_uint32(uint32_t x){ f.write(&x, 4); }
void Serializer::_int64 (int64_t  x){ f.write(&x, 8); }
void Serializer::_uint64(uint64_t x){ f.write(&x, 8); }

void Serializer::_enum(int x, int min, int max)
{
	if (x < min || x > max)
		throw std::logic_error("enum out of range");
	_int32(x);
}

void Serializer::_float (float  x){ f.write(&x, 4); }
void Serializer::_double(double x){ f.write(&x, 8); }
void Serializer::_cnum(const cnum &x)
{
	_double(x.real());
	_double(x.imag());
}

void Serializer::_data(const std::vector<unsigned char> &data, bool compress)
{
	if (compress && data.size() > 32)
	{
		std::vector<unsigned char> out;
		if (!zlibDeflate(data, out)) throw std::runtime_error("zlib compression failed");
		//if (!gzipDeflate(data, out)) throw std::runtime_error("zlib compression failed");
		_uint16(1);
		_uint64(out.size());
		f.write(out.data(), out.size());
#ifdef DEBUG
		std::cerr << "ZLib compression ratio: " << out.size() << " / " << data.size() << " = " << (100.0*out.size()/data.size()) << "%" << std::endl;
#endif
	}
	else
	{
		_uint16(0);
		_uint64(data.size());
		f.write(data.data(), data.size());
	}
}

void Serializer::_raw(std::vector<unsigned char> &data)
{
	if (!data.empty()) f.write(data.data(), data.size());
}

void Serializer::_string(const std::string &x)
{
	size_t len = x.length();
	_uint64(len);
	if (len) f.write(x.data(), len);
}

void Serializer::_marker(const char *s)
{
	f.write(s, strlen(s));
}

void Serializer::_object(const Serializable *x)
{
	if (!x)
	{
		_uint16((uint16_t)TypeCode::NIL);
	}
	else if (od.count(x))
	{
		_uint16((uint16_t)TypeCode::DUP);
		_uint64(od[x]);
	}
	else
	{
		TypeCode t = x->type_code();
		assert(t < TypeCode::INVALID);
		_uint16((uint16_t)t);
		x->save(*this);
		od[x] = od.size();
	}
}

//----------------------------------------------------------------------------------------------------------------------
//  Deserializer
//----------------------------------------------------------------------------------------------------------------------

Deserializer::Deserializer(ByteReader &f) : f(f)
{
	_marker("CPLT");
	_uint32(ver);
	if (ver < FILE_VERSION_1_0)
	{
		throw std::runtime_error("Invalid version info read. This file is not a cplot file or corrupted.");
	}
	if (ver > CURRENT_VERSION)
	{
		std::ostringstream s;
		s << "This file was written by a newer version of CPlot. It requires at least version " << version_string(ver) << " to read it.";
		throw std::runtime_error(s.str());
	}
}

void Deserializer::_bool(bool &x){ char c; f.read(&c, 1); x = c; }
void Deserializer::_uint16(uint16_t &x){ f.read(&x, 2); }
void Deserializer::_int32 (int32_t  &x){ f.read(&x, 4); }
void Deserializer::_uint32(uint32_t &x){ f.read(&x, 4); }
void Deserializer::_int64 (int64_t  &x){ f.read(&x, 8); }
void Deserializer::_uint64(uint64_t &x){ f.read(&x, 8); }

void Deserializer::_float(float   &x){ f.read(&x, 4); }
void Deserializer::_double(double &x){ f.read(&x, 8); }
void Deserializer::_cnum(cnum &x)
{
	double t;
	_double(t); x.real(t);
	_double(t); x.imag(t);
}

void Deserializer::_data(std::vector<unsigned char> &data)
{
	uint16_t compression;
	_uint16(compression);
	
	uint64_t n;
	_uint64(n);

	switch (compression)
	{
		case 0:
			data.resize(n);
			if (n) f.read(data.data(), n);
			break;
			
		case 1:
		{
			std::vector<unsigned char> in(n);
			if (n) f.read(in.data(), n);
			if (!zlibInflate(in, data)) throw std::runtime_error("ZLib decompression failed. This file seems to be corrupted.");
#ifdef DEBUG
			std::cerr << "ZLib compression ratio: " << n << " / " << data.size() << " = " << (100.0*n/data.size()) << "%" << std::endl;
#endif
			break;
		}
		
		default: throw std::runtime_error("Invalid compression type. This file seems to be corrupted.");
	}
}

void Deserializer::_raw(std::vector<unsigned char> &data, size_t n)
{
	data.resize(n);
	if (n) f.read(data.data(), n);
}

void Deserializer::_marker(const char *s_)
{
	const char *s = s_;
	size_t N = strlen(s);
	char buf[16];
	while(N)
	{
		size_t n = std::min(N, (size_t)16);
		f.read(buf, n);
		if (memcmp(s, buf, n) != 0)
		{
			std::ostringstream s;
			s << "Marker \"" << s_ << "\" not found. This file seems to be corrupted.";
			throw std::runtime_error(s.str());
		}
		N -= n;
		s += n;
	}
}

void Deserializer::_string(std::string &x)
{
	uint64_t len; _uint64(len);
	if (len)
	{
		std::unique_ptr<char[]> buf(new char [len]);
		f.read(buf.get(), len);
		x.assign(buf.get(), len);
	}
	else
	{
		x.clear();
	}
}

void Deserializer::_object(Serializable *&x)
{
	uint16_t type;
	_uint16(type);
	
	if (type >= (uint16_t)TypeCode::INVALID)
	{
		throw std::runtime_error("Invalid object type marker encountered. This file seems to be corrupted.");
	}
	
	switch(type)
	{
		case (uint16_t)TypeCode::NIL: x = NULL; return;
		case (uint16_t)TypeCode::DUP:
		{
			uint64_t ID; _uint64(ID);
			if (ID >= od.size())
			{
				throw std::runtime_error("Invalid object ID encountered. This file seems to be corrupted.");
			}
			x = od[ID];
			return;
		}

		case (uint16_t)TypeCode::NAMESPACE: (Namespace   *&)x = new Namespace; break;
		case (uint16_t)TypeCode::PARAMETER: (Parameter   *&)x = new Parameter("", Real, 0.0); break;
		case (uint16_t)TypeCode::FUNCTION:  (UserFunction*&)x = new UserFunction; break;
			
		case (uint16_t)TypeCode::INVALID:
		default: assert(false); x = NULL; return;
	}
	x->load(*this);
	od.push_back(x);
}
