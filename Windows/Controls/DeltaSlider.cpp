#include "DeltaSlider.h"

#define M 32000

BEGIN_MESSAGE_MAP(DeltaSlider, CSliderCtrl)
	ON_WM_CREATE()
	ON_WM_HSCROLL_REFLECT()
	ON_WM_TIMER()
END_MESSAGE_MAP()

DeltaSlider::DeltaSlider() : active_(false), slideback_(false)
{
}

int DeltaSlider::OnCreate(LPCREATESTRUCT cs)
{
	if (CSliderCtrl::OnCreate(cs) < 0) return -1;
	SetRange(-M, M);
	SetPos(0);
	SetLineSize(M / 3);
	SetPageSize(M / 3);
	return 0;
}

void DeltaSlider::HScroll(UINT code, UINT pos)
{
	//CString s; s.Format(_T("hscroll %u %u\n"), code, pos);
	//OutputDebugString(s);

	switch (code)
	{
		case SB_THUMBTRACK:
			active(true, false);
			break;
		default:
			active(true, true);
			break;
	}
}

double DeltaSlider::evolve(double t)
{
	if (!active()) return 0.0;

	int x = GetPos();

	double dt = t - t0;
	constexpr double dt0 = 1.0 / 60.0;
	t0 = t;
	double scale = dt > 0 ? std::min(dt / dt0, 5.0) : 1.0;

	if (slideback())
	{
		v -= (double)x / (double)M * 0.02 * scale;
		const int eps = M / 20;

		if (x > 0)
		{
			x += (int)(v * M * scale);
			if (x < eps) x = 0;
		}
		else if (x < 0)
		{
			x += (int)(v * M * scale);
			if (x > -eps) x = 0;
		}
		SetPos(x);
	}

	if (x == 0)
	{
		active(false);
		return 0.0;
	}

	double r = x / (double)M;
	return r * fabs(r) * scale;
}
