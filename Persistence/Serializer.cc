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
	marker_("CPLT");
	uint32_(ver);
}

void Serializer::bool_(bool x){ char c = x; f.write(&c, 1); }
void Serializer::uint16_(uint16_t x){ f.write(&x, 2); }
void Serializer::int32_ (int32_t  x){ f.write(&x, 4); }
void Serializer::uint32_(uint32_t x){ f.write(&x, 4); }
void Serializer::int64_ (int64_t  x){ f.write(&x, 8); }
void Serializer::uint64_(uint64_t x){ f.write(&x, 8); }

void Serializer::enum_(int x, int min, int max)
{
	if (x < min || x > max)
		throw std::logic_error("enum out of range");
	int32_(x);
}

void Serializer::float_ (float  x){ f.write(&x, 4); }
void Serializer::double_(double x){ f.write(&x, 8); }
void Serializer::cnum_  (const cnum &x)
{
	double_(x.real());
	double_(x.imag());
}

void Serializer::data_(const std::vector<unsigned char> &data, bool compress)
{
	if (compress && data.size() > 32)
	{
		std::vector<unsigned char> out;
		if (!zlibDeflate(data, out)) throw std::runtime_error("zlib compression failed");
		//if (!gzipDeflate(data, out)) throw std::runtime_error("zlib compression failed");
		uint16_(1);
		uint64_(out.size());
		f.write(out.data(), out.size());
#ifdef DEBUG
		std::cerr << "ZLib compression ratio: " << out.size() << " / " << data.size() << " = " << (100.0*out.size()/data.size()) << "%" << std::endl;
#endif
	}
	else
	{
		uint16_(0);
		uint64_(data.size());
		f.write(data.data(), data.size());
	}
}

void Serializer::raw_(std::vector<unsigned char> &data)
{
	if (!data.empty()) f.write(data.data(), data.size());
}

void Serializer::string_(const std::string &x)
{
	size_t len = x.length();
	uint64_(len);
	if (len) f.write(x.data(), len);
}

void Serializer::marker_(const char *s)
{
	f.write(s, strlen(s));
}

void Serializer::object_(const Serializable *x)
{
	if (!x)
	{
		uint16_((uint16_t)TypeCode::NIL);
	}
	else if (od.count(x))
	{
		uint16_((uint16_t)TypeCode::DUP);
		uint64_(od[x]);
	}
	else
	{
		TypeCode t = x->type_code();
		assert(t < TypeCode::INVALID);
		uint16_((uint16_t)t);
		x->save(*this);
		od[x] = od.size();
	}
}

//----------------------------------------------------------------------------------------------------------------------
//  Deserializer
//----------------------------------------------------------------------------------------------------------------------

Deserializer::Deserializer(ByteReader &f) : f(f)
{
	marker_("CPLT");
	uint32_(ver);
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

void Deserializer::bool_(bool &x){ char c; f.read(&c, 1); x = c; }
void Deserializer::uint16_(uint16_t &x){ f.read(&x, 2); }
void Deserializer::int32_ (int32_t  &x){ f.read(&x, 4); }
void Deserializer::uint32_(uint32_t &x){ f.read(&x, 4); }
void Deserializer::int64_ (int64_t  &x){ f.read(&x, 8); }
void Deserializer::uint64_(uint64_t &x){ f.read(&x, 8); }

void Deserializer::float_(float   &x){ f.read(&x, 4); }
void Deserializer::double_(double &x){ f.read(&x, 8); }
void Deserializer::cnum_(cnum &x)
{
	double t;
	double_(t); x.real(t);
	double_(t); x.imag(t);
}

void Deserializer::data_(std::vector<unsigned char> &data)
{
	uint16_t compression;
	uint16_(compression);
	
	uint64_t n;
	uint64_(n);

	switch (compression)
	{
		case 0:
			data.resize((size_t)n);
			if (n) f.read(data.data(), (size_t)n);
			break;
			
		case 1:
		{
			std::vector<unsigned char> in((size_t)n);
			if (n) f.read(in.data(), (size_t)n);
			if (!zlibInflate(in, data)) throw std::runtime_error("ZLib decompression failed. This file seems to be corrupted.");
#ifdef DEBUG
			std::cerr << "ZLib compression ratio: " << n << " / " << data.size() << " = " << (100.0*n/data.size()) << "%" << std::endl;
#endif
			break;
		}
		
		default: throw std::runtime_error("Invalid compression type. This file seems to be corrupted.");
	}
}

void Deserializer::raw_(std::vector<unsigned char> &data, size_t n)
{
	data.resize(n);
	if (n) f.read(data.data(), n);
}

void Deserializer::marker_(const char *s_)
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
			std::ostringstream ss;
			ss << "Marker \"" << s_ << "\" not found. This file seems to be corrupted.";
			throw std::runtime_error(ss.str());
		}
		N -= n;
		s += n;
	}
}

void Deserializer::string_(std::string &x)
{
	uint64_t len; uint64_(len);
	if (len)
	{
		std::unique_ptr<char[]> buf(new char [(size_t)len]);
		f.read(buf.get(), (size_t)len);
		x.assign(buf.get(), (size_t)len);
	}
	else
	{
		x.clear();
	}
}

void Deserializer::object_(Serializable *&x)
{
	uint16_t type;
	uint16_(type);
	
	if (type >= (uint16_t)TypeCode::INVALID)
	{
		throw std::runtime_error("Invalid object type marker encountered. This file seems to be corrupted.");
	}
	
	switch(type)
	{
		case (uint16_t)TypeCode::NIL: x = NULL; return;
		case (uint16_t)TypeCode::DUP:
		{
			uint64_t ID; uint64_(ID);
			if (ID >= od.size())
			{
				throw std::runtime_error("Invalid object ID encountered. This file seems to be corrupted.");
			}
			x = od[(size_t)ID];
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
