#include "Conversions.h"

CStringW Convert(const std::string &s)
{
	const int n = (int)s.length(); if (!n) return L"";
	CStringW t;
	const int m = MultiByteToWideChar(CP_UTF8, 0, s.data(), n, NULL, 0);
	if (m > 0)
	{
		wchar_t *buf = t.GetBuffer(m);
		if (buf) MultiByteToWideChar(CP_UTF8, 0, s.data(), n, buf, m);
		t.ReleaseBuffer();
	}
	return t;
}
