#include "stdafx.h"
#include "CPlotApp.h"
#include "MainWindow.h"
#include "SideView.h"
#include "MainView.h"
#include "Document.h"
#include "PlotView.h"
#include "res/resource.h"
#include "Controls/ViewUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(MainForm, CFormView)
BEGIN_MESSAGE_MAP(MainForm, CFormView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(MainWindow, CFrameWndEx)
BEGIN_MESSAGE_MAP(MainWindow, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_VIEW_GRAPH,   OnFocusGraph)
	ON_COMMAND(ID_VIEW_FORMULA, OnFocusEdit)
END_MESSAGE_MAP()

BOOL MainWindow::OnCmdMsg(UINT id, int code, void *extra, AFX_CMDHANDLERINFO *handler)
{
	// give focused control a chance to handle it
	MainForm *v = (MainForm*)GetDlgItem(AFX_IDW_PANE_FIRST);
	if (v)
	{
		if (v->OnCmdMsg(id, code, extra, handler))
			return true;
		CWnd *w = v->GetFocus();
		if (w && w != this && w->OnCmdMsg(id, code, extra, handler))
			return true;
	}

	// try document next
	if (doc && doc->OnCmdMsg(id, code, extra, handler)) return true;
	
	return CFrameWndEx::OnCmdMsg(id, code, extra, handler);
}

BOOL MainWindow::PreCreateWindow(CREATESTRUCT &cs)
{
	if (!CFrameWndEx::PreCreateWindow(cs)) return FALSE;
	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	return TRUE;
}

int MainWindow::OnCreate(LPCREATESTRUCT cs)
{
	if (CFrameWndEx::OnCreate(cs) == -1) return -1;
	CMFCPopupMenu::SetForceMenuFocus(FALSE);
	return 0;
}

BOOL MainWindow::OnCreateClient(LPCREATESTRUCT cs, CCreateContext *ctx)
{
	doc = (Document*)ctx->m_pCurrentDoc; assert(doc);
	if (!CFrameWnd::OnCreateClient(cs, ctx)) return false;

	MainForm *v = (MainForm*)GetDlgItem(AFX_IDW_PANE_FIRST);
	splitter = &v->splitter;
	mainView = (MainView*)splitter->GetPane(0, 1);
	sideView = (SideView*)splitter->GetPane(0, 0);

	if (doc) doc->AddView(v);
	SetActiveView(v);

	return TRUE;
}

void MainWindow::Focus(CWnd *who)
{
	if (!who) return;
	MainForm *v = (MainForm*)GetDlgItem(AFX_IDW_PANE_FIRST);
	::SendMessageW(*v, WM_NEXTDLGCTL, (WPARAM)(HWND)*who, TRUE);
}

void MainWindow::OnFocusEdit()
{
	Focus(mainView->GetEditView(0));
}
void MainWindow::OnFocusGraph()
{
	Focus(&mainView->GetPlotView());
}

int MainForm::OnCreate(LPCREATESTRUCT cs)
{
	DS0;
	int w = cs->cx, w0 = DS(230), min_w = DS(160);

	if (!(
		splitter.CreateStatic(this, 1, 2) &&
		splitter.CreateView(0, 1, RUNTIME_CLASS(MainView), CSize(w - w0, cs->cy), NULL) &&
		splitter.CreateView(0, 0, RUNTIME_CLASS(SideView), CSize(w0, cs->cy), NULL)
		)) return FALSE;

	splitter.SetColumnInfo(0, w0, min_w); // set min width
	splitter.ShowWindow(SW_SHOW);
	ShowWindow(SW_SHOW);

	return TRUE;
}

void MainForm::OnSize(UINT type, int w, int h)
{
	CFormView::OnSize(type, w, h);
	splitter.MoveWindow(0, 0, w, h);
}

BOOL MainForm::PreCreateWindow(CREATESTRUCT &cs)
{
	if (!CFormView::PreCreateWindow(cs)) return FALSE;

	cs.style &= ~WS_BORDER;
	cs.style |= WS_CHILD;
	cs.dwExStyle |= WS_EX_CONTROLPARENT | WS_EX_TRANSPARENT;
	return TRUE;
}

void MainForm::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	auto *mainView = (MainView*)splitter.GetPane(0, 1);
	auto *sideView = (SideView*)splitter.GetPane(0, 0);
	mainView->Update();
	sideView->UpdateAll();
}

BOOL MainWindow::OnEraseBkgnd(CDC *dc)
{
	return false;
}

BOOL MainForm::OnEraseBkgnd(CDC *dc)
{
	return false;
}
