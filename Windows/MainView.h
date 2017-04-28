#pragma once
#include "PlotView.h"
#include "SideView.h"
#include "MainWindow.h"
class Document;

class MainView : public CFormView
{
public:
	MainView();

	inline Document* GetDocument() const { return (Document*)m_pDocument; }
	inline PlotView& GetPlotView() const { return const_cast<PlotView&>(plotView); }
	inline SideView& GetSideView() const { return ((MainWindow*)GetParentFrame())->GetSideView(); }

	BOOL   OnEraseBkgnd(CDC *dc);
	BOOL   PreCreateWindow(CREATESTRUCT &cs) override;
	int    OnCreate(LPCREATESTRUCT cs);
	HBRUSH OnCtlColor(CDC *dc, CWnd *wnd, UINT ctrl);
	void   OnSize(UINT type, int w, int h);
	void   OnInitialUpdate();
	void   Update();

private:
	CComboBox domain, coord, mode;
	CEdit      f[3];
	CStatic   fs[3];
	PlotView  plotView;

	DECLARE_DYNCREATE(MainView)
	DECLARE_MESSAGE_MAP()
};


