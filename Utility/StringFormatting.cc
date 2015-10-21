#include "StringFormatting.h"

#include <cstdarg>
#include <vector>
#include <cassert>

std::string format(const char *fmt, ...)
{
	va_list  ap;
	va_start(ap, fmt);

	char              buf1[1024]; // avoid heap allocation most of the time
	std::vector<char> buf2;       // use this if buf1 is too small
	size_t            size = sizeof(buf1);
	char             *buf  = buf1;
	int               n;
	
	while (true)
	{
		n = vsnprintf(buf, size, fmt, ap);
		if (n >= 0 && (size_t)n <= size) break;
		
		size = n > 0 ? (size_t)n+1 : size*2;
		buf2.resize(size);
		buf = buf2.data();
	}
	
	va_end (ap);
	
	return std::string(buf, (size_t)n);
}

std::string spaces(int n)
{
	assert(n >= 0);
	if (n <= 0) return std::string();
	return std::string(n, ' ');
}
