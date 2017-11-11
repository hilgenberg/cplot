#pragma once
#include "Controls/SplitterWnd.h"
class MainView;
class SideView;
class Document;

class MainWindow : public CFrameWndEx
{
public:
	MainWindow();
	~MainWindow();

	MainView    &GetMainView() { assert(mainView); return *mainView; }
	SideView    &GetSideView() { assert(sideView); return *sideView; }
	SplitterWnd &GetSplitter() { return splitter; }

	BOOL OnCreateClient(LPCREATESTRUCT cs, CCreateContext *ctx) override;
	BOOL PreCreateWindow(CREATESTRUCT &cs) override;
	int  OnCreate(LPCREATESTRUCT cs);

	Document *doc;

private:
	SplitterWnd splitter;
	MainView   *mainView;
	SideView   *sideView;

	DECLARE_DYNCREATE(MainWindow)
	DECLARE_MESSAGE_MAP()
};


