#include "../stdafx.h"
#include "ParameterView.h"
#include "../Document.h"
#include "../res/resource.h"
#include "ViewUtil.h"
#include "SideSectionParams.h"
#include "../MainWindow.h"
#include "../MainView.h"
#include "../CPlotApp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

enum
{
	ID_name = 2000,
	ID_eq,
	ID_value,
	ID_delta,
	ID_edit,
	ID_plus,
	ID_minus
};

IMPLEMENT_DYNAMIC(ParameterView, CWnd)
BEGIN_MESSAGE_MAP(ParameterView, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_BN_CLICKED(ID_edit,  OnEdit)
	ON_BN_CLICKED(ID_plus,  OnPlus)
	ON_BN_CLICKED(ID_minus, OnMinus)
	//ON_LBN_DBLCLK(ID_name, OnEdit)
END_MESSAGE_MAP()

ParameterView::ParameterView(SideSectionParams &parent, Parameter &param)
: parent(parent), p_id(param.oid())
{
}

BOOL ParameterView::PreCreateWindow(CREATESTRUCT &cs)
{
	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	return CWnd::PreCreateWindow(cs);
}

BOOL ParameterView::Create(const RECT &rect, CWnd *parent, UINT ID)
{
	static bool init = false;
	if (!init)
	{
		WNDCLASS wndcls; memset(&wndcls, 0, sizeof(WNDCLASS));
		wndcls.style = CS_DBLCLKS;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.hInstance = AfxGetInstanceHandle();
		wndcls.hCursor = theApp.LoadStandardCursor(IDC_ARROW);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = _T("ParamView");
		if (!AfxRegisterClass(&wndcls)) throw std::runtime_error("AfxRegisterClass(ParamView) failed");
		init = true;
	}

	return CWnd::Create(_T("ParamView"), NULL, WS_CHILD | WS_TABSTOP, rect, parent, ID);
}

int ParameterView::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) < 0) return -1;
	EnableScrollBarCtrl(SB_BOTH, FALSE);

	START_CREATE;

	LLABEL(name, "p"); // proper name is set in Update()
	LABEL(eq, "=");
	EDIT(value); value.OnChange = [this]() { OnValueChange(); };
	CREATE(delta, deltaStyle); delta.stateChange = [this](bool a) { parent.parent().AnimStateChanged(a); };
	BUTTON(edit, "..."); edit.SetButtonStyle(BS_VCENTER | BS_CENTER | BS_FLAT, 0);
	BUTTON(plus,  "+"); plus. SetButtonStyle(BS_VCENTER | BS_CENTER | BS_FLAT, 0);
	BUTTON(minus, "-"); minus.SetButtonStyle(BS_VCENTER | BS_CENTER | BS_FLAT, 0);
	return 0;
}

int ParameterView::height(int w) const
{
	Parameter *p = parameter(); if (!p) return 0;
	DS0;
	return DS(2*22);
}

void ParameterView::Update(bool full)
{
	Parameter *p = parameter(); if (!p) return;

	name.SetWindowText(Convert(p->name()));

	if (p->is_real())
	{
		value.SetDouble(p->rvalue());

		if (p->type() == ParameterType::Integer)
		{
			plus. EnableWindow(!defined(p->max()) || p->rvalue() + 0.01 < p->max());
			minus.EnableWindow(!defined(p->min()) || p->rvalue() - 0.01 > p->min());
		}
	}
	else
	{
		value.SetComplex(p->value());
	}

	if (!full) return;

	CRect bounds; GetClientRect(bounds);
	if (bounds.Width() < 2 || !p) return;
	DS0;
	const int W = bounds.Width();
	const int SPC = DS(5); // amount of spacing
	const int w1 = DS(40); // label width
	const int x0 = SPC;  // row x start / amount of space on the left
	const int x1 = W - SPC;
	const int wq = DS(8);
	const int dw = DS(60); // delta width
	int y = 0; // y for next control

	const int h_label = DS(14), h_edit = DS(20), h_delta = DS(20), h_button = DS(20), h_row = DS(22);

	MOVE(name, x0 + wq + SPC + DS(2), x1 - SPC - dw, y, h_label, h_row);
	MOVE(edit, x1 - dw, x1, y, h_button, h_row);
	y += h_row;

	MOVE(eq, x0, x0+wq, y, h_label, h_row);
	MOVE(value, x0+wq+SPC, x1 - SPC - dw, y, h_edit, h_row);

	if (p->type() == ParameterType::Integer)
	{
		HIDE(delta);
		MOVE(minus, x1 - dw, x1 - (dw + SPC) / 2, y, h_button, h_row);
		MOVE(plus,  x1 - (dw - SPC)/2, x1, y, h_button, h_row);
	}
	else
	{
		MOVE(delta, x1 - dw, x1, y, h_delta, h_row);
		HIDE(plus);
		HIDE(minus);
	}
}

void ParameterView::OnInitialUpdate()
{
	Update(true);
}

void ParameterView::OnSize(UINT type, int w, int h)
{
	CWnd::OnSize(type, w, h);
	EnableScrollBarCtrl(SB_BOTH, FALSE);
	Update(true);
}

void ParameterView::OnValueChange()
{
	Parameter *p = parameter(); if (!p) return;
	if (p->is_real())
	{
		p->value(value.GetDouble());
	}
	else
	{
		p->value(value.GetComplex());
	}

	MainView &v = ((MainWindow*)GetParentFrame())->GetMainView();
	if (v.GetDocument().plot.recalc(p)) v.GetPlotView().Invalidate();
}

void ParameterView::OnEdit()
{
	Parameter *p = parameter(); if (!p) return;
	parent.OnEdit(p);
}

void ParameterView::Animate(double t)
{
	double dx = delta.evolve(t);
	if (!dx) return;

	Parameter *p = parameter(); if (!p) return;

	dx *= 0.1;
	switch (p->type())
	{
		case Real: p->rvalue(p->rvalue() + dx); break;
		case Complex: p->value(p->value() + dx); break;
		case Angle:p->rvalue(p->rvalue() + dx); break;
		case ComplexAngle:p->rvalue(p->rvalue() + dx); break;
		case Integer: assert(false); return;
	}
	if (p->is_real())
	{
		value.SetDouble(p->rvalue());
	}
	else
	{
		value.SetComplex(p->value());
	}

	MainView &v = ((MainWindow*)GetParentFrame())->GetMainView();
	if (v.GetDocument().plot.recalc(p)) v.GetPlotView().Invalidate();
	UpdateWindow();
}

void ParameterView::OnPlus()
{
	Parameter *p = parameter(); if (!p) return;
	p->rvalue(p->rvalue() + 1.0);
	value.SetDouble(p->rvalue());

	MainView &v = ((MainWindow*)GetParentFrame())->GetMainView();
	if (v.GetDocument().plot.recalc(p)) v.GetPlotView().Invalidate();
}
void ParameterView::OnMinus()
{
	Parameter *p = parameter(); if (!p) return;
	p->rvalue(p->rvalue() - 1.0);
	value.SetDouble(p->rvalue());

	MainView &v = ((MainWindow*)GetParentFrame())->GetMainView();
	if (v.GetDocument().plot.recalc(p)) v.GetPlotView().Invalidate();
}
