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

std::string Convert(const CStringW &s)
{
	const int n = (int)s.GetLength(); if (!n) return std::string();
	const wchar_t *ss = s.GetString();
	const int m = WideCharToMultiByte(CP_UTF8, 0, ss, n, NULL, 0, NULL, NULL);
	std::string t(m, 0);
	WideCharToMultiByte(CP_UTF8, 0, ss, n, t._Myptr(), m, NULL, NULL);
	return t;
}
