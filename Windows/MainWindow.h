#pragma once
#include "Controls/SplitterWnd.h"
#include "res/resource.h"
#include "CPlotApp.h"
class MainView;
class SideView;
class Document;

class MainForm: public CFormView
{
public:
	MainForm() : CFormView(IDD_MAINVIEW) {}

	int    OnCreate(LPCREATESTRUCT cs);
	void   OnSize(UINT type, int w, int h);
	BOOL   PreCreateWindow(CREATESTRUCT &cs);
	void   OnInitialUpdate() override;
	BOOL OnEraseBkgnd(CDC *dc);

	SplitterWnd splitter;

	DECLARE_DYNCREATE(MainForm)
	DECLARE_MESSAGE_MAP()
};

class MainWindow : public CFrameWndEx
{
public:
	MainWindow() : doc(NULL) {}

	MainView    &GetMainView() { assert(mainView); return *mainView; }
	SideView    &GetSideView() { assert(sideView); return *sideView; }
	SplitterWnd &GetSplitter() { assert(splitter); return *splitter; }
	void Focus(CWnd *who);

	BOOL OnCreateClient(LPCREATESTRUCT cs, CCreateContext *ctx) override;
	BOOL PreCreateWindow(CREATESTRUCT &cs) override;
	int  OnCreate(LPCREATESTRUCT cs);
	BOOL OnEraseBkgnd(CDC *dc);

	BOOL OnCmdMsg(UINT id, int code, void *extra, AFX_CMDHANDLERINFO *handler) override;

	Document *doc;

private:
	SplitterWnd *splitter;
	MainView    *mainView;
	SideView    *sideView;

	DECLARE_DYNCREATE(MainWindow)
	DECLARE_MESSAGE_MAP()
};


