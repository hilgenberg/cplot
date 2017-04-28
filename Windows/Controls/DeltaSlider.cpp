#include "DeltaSlider.h"

#define M 32000

BEGIN_MESSAGE_MAP(DeltaSlider, CSliderCtrl)
	ON_WM_CREATE()
	ON_WM_HSCROLL_REFLECT()
	ON_WM_TIMER()
END_MESSAGE_MAP()

DeltaSlider::DeltaSlider()
{
}

DeltaSlider::~DeltaSlider()
{
}

BOOL DeltaSlider::PreCreateWindow(CREATESTRUCT &cs)
{
	return CSliderCtrl::PreCreateWindow(cs);
}
int DeltaSlider::OnCreate(LPCREATESTRUCT cs)
{
	if (CSliderCtrl::OnCreate(cs) < 0) return -1;
	SetRange(-M, M);
	SetPos(0);
	return 0;
}

void DeltaSlider::HScroll(UINT code, UINT pos)
{
	CString s; s.Format(_T("%u %u %d\n"), code, pos, GetPos());
	OutputDebugString(s);
	
	if (code == SB_ENDSCROLL || code == SB_THUMBPOSITION)
	{
		SetTimer(0, 50, NULL);
	}
	else
	{
		KillTimer(0);
	}
}

void DeltaSlider::OnTimer(UINT_PTR timerID)
{
	int v = (int)(GetPos() * 0.7);
	const int eps = M / 50;
	v < -eps ? v += eps : v > eps ? v -= eps : v = 0;
	SetPos(v);
	if (v == 0) KillTimer(timerID);
}
