#include "StringFormatting.h"

#include <cstdarg>
#include <vector>
#include <cassert>
#include <cstring>

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

bool is_int(const char *s, int &v_)
{
	int v = 0, sign = 1;
	if      (*s == '-'){ ++s; sign = -1; }
	else if (*s == '+'){ ++s;            }
	if (!*s) return false;

	for (; *s; ++s)
	{
		char c = *s;
		if (!isdigit(c)) return false;
		v *= 10; v += c - '0';
	}
	v_ = sign * v;
	return true;
}

bool is_int(const std::string &s, int &v_)
{
	int v = 0, sign = 1;
	size_t i = 0, n = s.length();
	if (!n) return false;
	if      (s[0] == '-'){ ++i; sign = -1; }
	else if (s[0] == '+'){ ++i;            }
	if (i >= n) return false;

	for (; i < n; ++i)
	{
		char c = s[i];
		if (!isdigit(c)) return false;
		v *= 10; v += c - '0';
	}
	v_ = sign * v;
	return true;
}

bool has_prefix(const char *s, const char *p, bool ignore_case)
{
	size_t l = strlen(p);
	if (strlen(s) < l) return false;
	auto cmp = ignore_case ? strncasecmp : strncmp;
	return cmp(s, p, l) == 0;
}

bool has_prefix(const std::string &s, const char *p, bool ignore_case)
{
	size_t l = strlen(p);
	if (s.length() < l) return false;
	auto cmp = ignore_case ? strncasecmp : strncmp;
	return cmp(s.c_str(), p, l) == 0;
}
