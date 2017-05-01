#include "HeaderControl.h"
#include "../SideView.h"
#include <math.h>

BEGIN_MESSAGE_MAP(HeaderControl, CButton)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void HeaderControl::OnLButtonDown(UINT flags, CPoint p)
{
	CRect bounds; GetClientRect(&bounds);

	const int bw = int(bounds.Height() / 3.5), lw = bw / 3;
	int xm = bounds.right - 5 - bw, ym = bounds.Height() / 2;

	CRect r1(xm - bw, ym - bw, xm + bw, ym + bw);
	xm -= 2 + bw + 5;
	CRect r2(xm - bw, ym - bw, xm + bw, ym + bw);

	int nr = 0;
	if (canRemove != Hidden)
	{
		if (r1.PtInRect(p))
		{
			if (canRemove == Inactive) return;
			((SideView*)GetOwner())->OnRemove(this);
			return;
		}
		++nr;
	}
	if (canAdd != Hidden)
	{
		if ((nr==1 ? r2 : r1).PtInRect(p))
		{
			if (canAdd == Inactive) return;
			((SideView*)GetOwner())->OnAdd(this);
			return;
		}
		++nr;
	}
	if (nr && p.x > (nr == 1 ? r1 : r2).left) return;

	CButton::OnLButtonDown(flags, p);
}

void HeaderControl::OnPaint()
{
	CPaintDC dc(this);
	CRect bounds; GetClientRect(&bounds);

	// fill + lines
	CBrush white; white.CreateSolidBrush(GREY(255));
	dc.FillRect(&bounds, &white);
	CPen linePen; linePen.CreatePen(PS_SOLID, 1, GREY(200));
	dc.SelectObject(&linePen);
	dc.MoveTo(bounds.left, bounds.top); dc.LineTo(bounds.right, bounds.top);
	dc.MoveTo(bounds.left, bounds.bottom-1); dc.LineTo(bounds.right, bounds.bottom-1);

	// disclosure triangle
	const int h = bounds.Height();
	const int r = (int)(h / 4);
	const int cr = (int)(r * 0.866025); // cos(30°) = sqrt(3/4)
	int xm = bounds.left + 5 + r;
	int ym = bounds.Height() / 2;
	CPoint P[3];
	if (GetCheck())
	{
		P[0].SetPoint(xm, ym + r - r/4);
		P[1].SetPoint(xm - cr, ym - r / 2 - r / 4);
		P[2].SetPoint(xm + cr, ym - r / 2 - r / 4);
	}
	else
	{
		P[0].SetPoint(xm + r, ym);
		P[1].SetPoint(xm - r / 2, ym - cr);
		P[2].SetPoint(xm - r / 2, ym + cr);
	}
	COLORREF color = GetSysColor(COLOR_BTNSHADOW);
	CBrush brush;
	brush.CreateSolidBrush(color);
	dc.SelectObject(&brush);
	CPen pen; pen.CreatePen(PS_SOLID, 1, color);
	dc.SelectObject(&pen);
	dc.Polygon(P, 3);

	// button text
	dc.SelectStockObject(NULL_BRUSH);
	CPen txtPen; txtPen.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNTEXT));
	dc.SelectObject(&txtPen);
	dc.SelectObject(GetFont());
	CString s; GetWindowText(s);
	CRect rc(bounds.left + 5 + 2 * r + 5, bounds.top, bounds.right, bounds.bottom);
	dc.DrawText(s, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

	// add and remove buttons
	const int bw = int(h / 3.5), lw = bw / 3;
	CBrush  on;  on.CreateSolidBrush(GREY(100));
	CBrush off; off.CreateSolidBrush(GREY(200));
	xm = bounds.right - 5 - bw;
	if (canRemove != Hidden)
	{
		dc.FillRect(CRect(xm-bw, ym-lw, xm+bw, ym+lw), canRemove == Inactive ? &off : &on);
		xm -= 2 * bw + 5;
	}
	if (canAdd != Hidden)
	{
		dc.FillRect(CRect(xm - bw, ym - lw, xm + bw, ym + lw), canAdd == Inactive ? &off : &on);
		dc.FillRect(CRect(xm - lw, ym - bw, xm + lw, ym + bw), canAdd == Inactive ? &off : &on);
	}
}
