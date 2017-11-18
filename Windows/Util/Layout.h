#pragma once
#include "afxwin.h"

class Layout
{
public:
	Layout(CWnd &w, int y0 = 5, int h_row = 24, int space = 5);
	void operator<< (const std::vector<int> &widths);
	void use(const std::vector<CWnd*> &controls, int h = -1);
	void height(int h_row) { h = h_row; }
	int  height() const { return h; }
	int operator[](int i) { assert(i < (int)w.size()); return w[i]; }
	int y;
	int W;
	void skip(int n=1) { y += n*spc; }

private:
	UINT dpi;
	std::vector<int> w;
	int spc;
	int h;
};

#define SET(...) { auto l = std::vector<int>{__VA_ARGS__}; layout << l; }
#define USE(...)     layout.use(std::vector<CWnd*>{__VA_ARGS__})
#define USEH(h, ...) layout.use(std::vector<CWnd*>{__VA_ARGS__}, h)
