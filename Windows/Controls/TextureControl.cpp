#include "TextureControl.h"
#include "../CPlotApp.h"
#include "../../Graphs/OpenGL/GL_Image.h"

BEGIN_MESSAGE_MAP(TextureControl, CWnd)
	ON_WM_PAINT()
	ON_WM_RBUTTONUP()
	ON_COMMAND_RANGE(1000, 1000+IP_UNITS, OnContextItem)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

TextureControl::TextureControl() : im(NULL)
{
}

TextureControl::~TextureControl()
{
}

BOOL TextureControl::Create(DWORD style, const RECT &rect, CWnd *parent, UINT ID)
{
	static bool init = false;
	if (!init)
	{
		WNDCLASS wndcls; memset(&wndcls, 0, sizeof(WNDCLASS));
		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.hInstance = AfxGetInstanceHandle();
		wndcls.hCursor = theApp.LoadStandardCursor(IDC_ARROW);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = _T("TextureControl");
		if (!AfxRegisterClass(&wndcls)) throw std::runtime_error("AfxRegisterClass(TextureControl) failed");
		init = true;
	}

	return CWnd::Create(_T("TextureControl"), NULL, style, rect, parent, ID);
}

void TextureControl::OnRButtonUp(UINT flags, CPoint p)
{
	if (!(HMENU)cm)
	{
		cm.CreatePopupMenu();
		#define ITEM(x, s) cm.AppendMenu(MF_STRING, 1000+x, _T(s))
		ITEM(IP_COLORS,   "Color");
		ITEM(IP_COLORS_2, "Color2");
		ITEM(IP_XOR,      "XOR");
		ITEM(IP_XOR_GREY, "Grey XOR");
		ITEM(IP_CHECKER,  "Checker");
		ITEM(IP_PLASMA,   "Plasma");
		ITEM(IP_PHASE,    "Phase");
		ITEM(IP_UNITS,    "Units");
		#undef ITEM
	}

	GL_ImagePattern x0 = (im ? im->is_pattern() : IP_CUSTOM);
	for (int i = 0, n = cm.GetMenuItemCount(); i < n; ++i)
	{
		MENUITEMINFO mi; mi.cbSize = sizeof(MENUITEMINFO);
		cm.GetMenuItemInfo(i, &mi, TRUE);
		mi.fMask = MIIM_STATE;
		mi.fState = (cm.GetMenuItemID(i) - 1000 == (int)x0 ? MF_CHECKED : MF_UNCHECKED);
		cm.SetMenuItemInfo(i, &mi, TRUE);
	}
	ClientToScreen(&p);
	cm.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, p.x, p.y, this);
}

void TextureControl::OnContextItem(UINT which)
{
	if (!im) return;
	GL_ImagePattern x = (GL_ImagePattern)(which - 1000);
	if (x == im->is_pattern()) return;
	*im = x;
	
	if (OnChange) OnChange();
	
	GL_Image *im_ = im;
	SetImage(NULL);
	SetImage(im_);
	Invalidate();
}

void TextureControl::SetImage(GL_Image *im_)
{
	if (im_ == im) return;
	im = im_;

	HGDIOBJ old = bmp.Detach();
	if (old) DeleteObject(old);

	if (!im)
	{
		Invalidate();
		return;
	}

	const unsigned w = im->w(), h = im->h();

	if (!im->opaque() || ((DWORD_PTR)im->data().data()) & (sizeof(DWORD)-1))
	{
		const size_t n = (size_t)w * (size_t)h;
		buf.resize(n); // this should DWORD-align the data and all rows
		BYTE *d = (BYTE *)buf.data();
		for (const BYTE *e = im->data().data(), *end = e + 4*n; e < end; e += 4)
		{
			*d++ = (BYTE)((unsigned)e[0] * e[3] / 255);
			*d++ = (BYTE)((unsigned)e[1] * e[3] / 255);
			*d++ = (BYTE)((unsigned)e[2] * e[3] / 255);
			*d++ = e[3];
		}

		bmp.CreateBitmap(w, h, 1, 32, (BYTE *)buf.data());
	}
	else
	{
		bmp.CreateBitmap(w, h, 1, 32, im->data().data());
	}
	Invalidate();
}

BOOL TextureControl::OnEraseBkgnd(CDC *dc)
{
	return TRUE;
}

void TextureControl::OnPaint()
{
	CPaintDC dc(this);
	CRect bounds; GetClientRect(&bounds);

	// dc.DrawEdge is not working, no idea why.
	CPen line; line.CreatePen(PS_SOLID, 1, GREY(20));
	dc.SelectObject(&line);
	dc.Rectangle(bounds);
	bounds.DeflateRect(2, 2);
	
	COLORREF bg = GetSysColor(COLOR_BTNFACE);
	CBrush bgb; bgb.CreateSolidBrush(bg);
	dc.FillRect(&bounds, &bgb);

	if (!im) return;

	const int W = bounds.Width(), H = bounds.Height();
	if (W < 1 || H < 1) return;
	int w, h;
	if ((size_t)W*(size_t)im->h() <= (size_t)H*(size_t)im->w())
	{
		w = W;
		h = (int)((size_t)im->h() * W / im->w());
		assert(h <= H);
	}
	else
	{
		w = (int)((size_t)im->w() * H / im->h());
		h = H;
		assert(w <= W);
	}

	CDC bmpDC; if (!bmpDC.CreateCompatibleDC(&dc)) return;
	bmpDC.SetMapMode(dc.GetMapMode());
	bmpDC.SelectObject(&bmp);

	BLENDFUNCTION f; f.BlendOp = AC_SRC_OVER; f.BlendFlags = 0; f.SourceConstantAlpha = 255; f.AlphaFormat = AC_SRC_ALPHA;

	dc.AlphaBlend(bounds.left + (W - w) / 2, bounds.top + (H - h) / 2, w, h, &bmpDC, 0, 0, im->w(), im->h(), f);
}
