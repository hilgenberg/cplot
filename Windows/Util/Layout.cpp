#include "Layout.h"

#define DS(x) MulDiv(x, dpi, 96)

Layout::Layout(CWnd &c, int y0, int h_row, int space)
{
	#ifdef WIN10
	dpi = ::GetDpiForWindow(c.GetSafeHwnd());
	#else
	HDC tmp_hdc = ::GetDC(c.GetSafeHwnd());
	dpi = ::GetDeviceCaps(tmp_hdc, LOGPIXELSX);
	::ReleaseDC(NULL, tmp_hdc);
	#endif

	spc = DS(space);
	h = DS(h_row);
	y = DS(y0);

	CRect rc; c.GetWindowRect(rc);
	W = rc.Width();
}

void Layout::operator<< (const std::vector<int> &widths)
{
	w = widths;
	int rest = W - spc * (int)(w.size()+1), t = 0;
	for (int &k : w)
	{
		if (k < 0)
		{
			t += k;
		}
		else
		{
			k = DS(k);
			rest -= k;
		}
	}
	for (int &k : w) if (k < 0) k = rest*k/t;
}

void Layout::use(const std::vector<CWnd*> &C, int h0)
{
	int dh = h0 <= 0 ? h : h0;
	assert(C.size() == w.size());
	int x = spc;
	for (int i = 0, n = (int)C.size(); i < n; ++i)
	{
		if (!C[i])
		{
			x += spc + w[i];
			continue;
		}

		CWnd &c = *C[i];
		int cw = w[i];
		while (i + 1 < n && C[i + 1] == &c)
		{
			cw += spc + w[++i];
		}

		if (cw <= 0)
		{
			c.ShowWindow(SW_HIDE);
		}
		else if (h0 > 0)
		{
			CRect rc(x, y, x + cw, y + dh);
			c.MoveWindow(rc);
			c.ShowWindow(SW_SHOW);
		}
		else
		{
			CRect rc; c.GetWindowRect(rc);
			rc.right = rc.left + cw;
			rc.MoveToXY(x, y + (h - rc.Height()) / 2);
			c.MoveWindow(rc);
			c.ShowWindow(SW_SHOW);
		}

		x += cw + spc;

	}
	y += dh;
}

