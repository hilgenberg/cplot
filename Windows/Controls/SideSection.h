#pragma once
#include "afxwin.h"
#include "HeaderControl.h"
class SideView;
class MainWindow;
struct Plot;
class Graph;
class Document;

class SideSection : public CWnd
{
public:
	SideSection();
	virtual ~SideSection() {}
	virtual BOOL Create(SideView *parent, UINT ID);

	MainWindow *window() const { return (MainWindow*)GetParentFrame(); }
	SideView   &parent() const { return *(SideView*)GetParent(); }
	Document &document() const;
	Plot      &GetPlot() const;
	Graph    *GetGraph() const;

	// full updates position and (un)hide child view
	// non-full updates only set child state (text, checked, greyed, ...)
	virtual void Update(bool full);

	void Position(int &y)
	{
		CRect r; GetWindowRect(r);
		r.MoveToXY(0, y);
		MoveWindow(r, true);
		y += r.Height();
	}
	void Width(int w)
	{
		CRect r; GetWindowRect(r);
		MoveWindow(CRect(r.left, r.top, r.left + w, r.bottom), false);
	}

	void Redraw(); // the graph, not this!
	void Recalc(Plot &plot);
	void Recalc(Graph *g);

	int  OnCreate(LPCREATESTRUCT cs);
	BOOL PreCreateWindow(CREATESTRUCT &cs) override;
	BOOL OnEraseBkgnd(CDC *dc);
	void OnHeader();
	virtual CString headerString() = 0;
	virtual CString wndClassName() = 0;

	virtual void OnAdd() {}
	virtual void OnRemove() {}

	HeaderControl header;

	DECLARE_MESSAGE_MAP()
};

