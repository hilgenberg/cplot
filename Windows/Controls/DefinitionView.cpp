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
	ID_def = 2000,
	ID_edit
};

IMPLEMENT_DYNAMIC(DefinitionView, CWnd)
BEGIN_MESSAGE_MAP(DefinitionView, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_BN_CLICKED(ID_edit, OnEdit)
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

	LLABEL(def, "p"); // proper text is set in Update()
	BUTTON(edit, "...");
	edit.SetButtonStyle(BS_VCENTER | BS_CENTER | BS_FLAT, 0);
	return 0;
}

int DefinitionView::height(int w) const
{
	auto *f = function(); if (!f) return 0;
	DS0;
	return DS(22);
}

void DefinitionView::Update(bool full)
{
	CRect bounds; GetClientRect(bounds);
	UserFunction *f = function();
	if (!f) return;
	DS0;
	
	def.SetWindowText(Convert(f->formula()));

	if (!full) return;

	const int W = bounds.Width();
	const int SPC = DS(5); // amount of spacing
	const int x0 = SPC;  // row x start / amount of space on the left
	const int x1 = W - SPC;
	const int dw = DS(20); // edit width
	int y = 0; // y for next control

	const int h_label = DS(14), h_button = DS(20), h_row = DS(22);

	MOVE(def, x0, x1 - SPC - dw, y, h_label, h_row);
	MOVE(edit, x1 - dw, x1, y, h_button, h_row);
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
