#include "stdafx.h"
#include "CPlotApp.h"
#include "MainWindow.h"
#include "SideView.h"
#include "MainView.h"
#include "Document.h"
#include "PlotView.h"
#include "res/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(MainWindow, CFrameWndEx)
BEGIN_MESSAGE_MAP(MainWindow, CFrameWndEx)
	ON_WM_CREATE()
END_MESSAGE_MAP()

MainWindow::MainWindow()
{
}

MainWindow::~MainWindow()
{
}

BOOL MainWindow::PreCreateWindow(CREATESTRUCT &cs)
{
	if (!CFrameWndEx::PreCreateWindow(cs)) return FALSE;

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
	int w = cs->cx, w0 = 230, min_w = 160;

	if (!(
		splitter.CreateStatic(this, 1, 2) &&
		splitter.CreateView(0, 1, RUNTIME_CLASS(MainView), CSize(w-w0, 0), ctx) &&
		splitter.CreateView(0, 0, RUNTIME_CLASS(SideView),  CSize(w0, 0),   ctx)
	)) return FALSE;

	splitter.SetColumnInfo(0, w0, min_w); // set min width

	mainView = (MainView*)splitter.GetPane(0, 1);
	sideView = (SideView*)splitter.GetPane(0, 0);
	sideView->SetDoc((Document*)ctx->m_pCurrentDoc);
	mainView->GetPlotView().SetDoc((Document*)ctx->m_pCurrentDoc);
	return TRUE;
}
