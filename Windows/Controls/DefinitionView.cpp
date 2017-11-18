#include "../stdafx.h"
#include "DefinitionView.h"
#include "../Document.h"
#include "../res/resource.h"
#include "ViewUtil.h"
#include "SideSectionDefs.h"
#include "../MainWindow.h"
#include "../MainView.h"
#include "../CPlotApp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

enum
{
	ID_def = 2000
};

IMPLEMENT_DYNAMIC(DefinitionView, CWnd)
BEGIN_MESSAGE_MAP(DefinitionView, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_BN_CLICKED(ID_def, OnEdit)
END_MESSAGE_MAP()

DefinitionView::DefinitionView(SideSectionDefs &parent, UserFunction &f)
: parent(parent), f_id(f.oid())
{
}

BOOL DefinitionView::PreCreateWindow(CREATESTRUCT &cs)
{
	cs.style |= WS_CHILD;
	cs.dwExStyle |= WS_EX_CONTROLPARENT | WS_EX_TRANSPARENT;
	return CWnd::PreCreateWindow(cs);
}

BOOL DefinitionView::Create(const RECT &rect, CWnd *parent, UINT ID)
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
		wndcls.lpszClassName = _T("DefinitionView");
		wndcls.hbrBackground = NULL;
		if (!AfxRegisterClass(&wndcls)) throw std::runtime_error("AfxRegisterClass(DefinitionView) failed");
		init = true;
	}

	return CWnd::Create(_T("DefinitionView"), NULL, WS_CHILD | WS_TABSTOP, rect, parent, ID);
}

int DefinitionView::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) < 0) return -1;
	EnableScrollBarCtrl(SB_BOTH, FALSE);

	START_CREATE;
	BUTTONLABEL(def);

	return 0;
}

void DefinitionView::Update(bool full)
{
	CRect bounds; GetClientRect(bounds);
	UserFunction *f = function();
	if (!f) return;
	
	def.SetWindowText(Convert(f->formula()));

	if (!full) return;

	Layout layout(*this, 0, 20); SET(-1); USE(&def);
}

void DefinitionView::OnInitialUpdate()
{
	Update(true);
}

void DefinitionView::OnSize(UINT type, int w, int h)
{
	CWnd::OnSize(type, w, h);
	EnableScrollBarCtrl(SB_BOTH, FALSE);
	Update(true);
}

void DefinitionView::OnEdit()
{
	auto *f = function(); if (!f) return;
	parent.OnEdit(f);
}
