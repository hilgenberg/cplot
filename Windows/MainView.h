#pragma once
#include "PlotView.h"
#include "SideView.h"
#include "MainWindow.h"
#include "Controls/FocusEdit.h"
class Document;

class BGStatic: public CStatic
{
public:
	BOOL Create(CWnd *parent, int h, UINT ID, DWORD style);
	BOOL   OnEraseBkgnd(CDC *dc);
	COLORREF bg; // background color
	DECLARE_MESSAGE_MAP()
};

class MainView : public CWnd
{
public:
	inline Document& GetDocument() const { return *((MainWindow*)GetParentFrame())->doc; }
	inline PlotView& GetPlotView() const { return const_cast<PlotView&>(plotView); }
	inline SideView& GetSideView() const { return ((MainWindow*)GetParentFrame())->GetSideView(); }

	void   RedrawHeader();

	BOOL   OnEraseBkgnd(CDC *dc);
	BOOL   PreCreateWindow(CREATESTRUCT &cs) override;
	int    OnCreate(LPCREATESTRUCT cs);
	HBRUSH OnCtlColor(CDC *dc, CWnd *wnd, UINT ctrl);
	void   OnSize(UINT type, int w, int h);
	void   OnInitialUpdate();
	void   Update();
	void   OnChangeF1();
	void   OnChangeF2();
	void   OnChangeF3();
	void   OnChangeF(int i, const CString &s);
	void   OnDomainChange();
	void   OnCoordChange();
	void   OnModeChange();

private:
	CRect headerRect() const;

	CComboBox domain, coord, mode;
	FocusEdit f [3];
	BGStatic  fs[3];
	PlotView  plotView;
	BGStatic  error;

	DECLARE_DYNCREATE(MainView)
	DECLARE_MESSAGE_MAP()
};


