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
	ID_anim,
	ID_plus,
	ID_minus
};

IMPLEMENT_DYNAMIC(ParameterView, CWnd)
BEGIN_MESSAGE_MAP(ParameterView, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(ID_name, OnEdit)
	ON_BN_CLICKED(ID_plus,  OnPlus)
	ON_BN_CLICKED(ID_minus, OnMinus)
	ON_BN_CLICKED(ID_anim,  OnAnimate)
END_MESSAGE_MAP()

ParameterView::ParameterView(SideSectionParams &parent, Parameter &param)
: parent(parent), p_id(param.oid())
{
}

BOOL ParameterView::PreCreateWindow(CREATESTRUCT &cs)
{
	cs.style |= WS_CHILD;
	cs.dwExStyle |= WS_EX_CONTROLPARENT | WS_EX_TRANSPARENT;
	return CWnd::PreCreateWindow(cs);
}

BOOL ParameterView::OnEraseBkgnd(CDC *dc)
{
	return false;
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
		wndcls.hbrBackground = NULL;
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
	BUTTONLABEL(name);
	LABEL(eq, "=");
	EDIT(value); value.OnChange = [this]() { OnValueChange(); };
	DELTA0(delta); delta.stateChange = [this](bool a) { parent.parent().AnimStateChanged(a); };
	BUTTON(anim, "anim"); anim.SetButtonStyle(BS_VCENTER | BS_CENTER | BS_FLAT, 0);
	BUTTON(plus,  "+"); plus. SetButtonStyle(BS_VCENTER | BS_CENTER | BS_FLAT, 0);
	BUTTON(minus, "-"); minus.SetButtonStyle(BS_VCENTER | BS_CENTER | BS_FLAT, 0);
	return 0;
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

	if (p->anim)
	{
		anim.SetWindowText(_T("■"));
		plus.EnableWindow(false);
		minus.EnableWindow(false);
		delta.EnableWindow(false);
	}
	else
	{
		anim.SetWindowText(_T("►"));
		delta.EnableWindow(true);
	}

	if (!full) return;
	CRect bounds; GetClientRect(bounds);
	if (bounds.Width() < 2 || !p) return;

	Layout layout(*this, 0, 22);
	SET(6, -1, 30, 30);
	USE(NULL, &name, &anim, &anim);

	SET( 8, -1, 30, 30);
	if (p->type() == ParameterType::Integer)
	{
		USE(&eq, &value, &minus, &plus);
		HIDE(delta);
	}
	else
	{
		USE(&eq, &value, &delta, &delta);
		HIDE(plus); HIDE(minus);
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
	if (p->anim)
	{
		p->anim_stop();
		parent.parent().AnimStateChanged(false);
		Update(false);
	}
	parent.OnEdit(p);
}

void ParameterView::OnAnimate()
{
	Parameter *p = parameter(); if (!p) return;
	if (p->anim)
	{
		p->anim_stop();
		parent.parent().AnimStateChanged(false);
	}
	else if (p->anim_start())
	{
		parent.parent().AnimStateChanged(true);
	}
	Update(false);
}

void ParameterView::StopAnimation()
{
	Parameter *p = parameter();
	if (p && p->anim)
	{
		p->anim_stop();
		parent.parent().AnimStateChanged(false);
		Update(false);
	}
}

void ParameterView::Animate(double t)
{
	double dx = delta.evolve(t); // always do this for slideback

	Parameter *p = parameter(); if (!p) return;
	
	if (p->anim)
	{
		bool changed = p->animate(t);
		if (!p->anim)
		{
			parent.parent().AnimStateChanged(false);
			Update(false);
		}
		if (!changed) return;
	}
	else
	{
		if (!dx) return;

		dx *= 0.1;
		switch (p->type())
		{
			case Real: p->rvalue(p->rvalue() + dx); break;
			case Complex: p->value(p->value() + dx); break;
			case Angle:p->rvalue(p->rvalue() + dx); break;
			case ComplexAngle:p->rvalue(p->rvalue() + dx); break;
			case Integer: assert(false); return;
		}
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

void ParameterView::Change(cnum delta)
{
	Parameter *p = parameter(); if (!p) return;

	p->value(p->value() + delta);
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
