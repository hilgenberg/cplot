#pragma once
#include "afxwin.h"
#include "../../Graphs/Graph.h"
#include "NumericEdit.h"
#include "DeltaSlider.h"
class SideView;

class GraphView : public CWnd
{
public:
	GraphView(SideView &parent, Graph &g);
	Graph *graph() const { return (Graph*)IDCarrier::find(g_id); }

	BOOL   PreCreateWindow(CREATESTRUCT &cs) override;
	BOOL   Create(const RECT &rect, CWnd *parent, UINT ID);
	int    OnCreate(LPCREATESTRUCT cs);
	void   OnSize(UINT type, int w, int h);
	void   OnInitialUpdate();
	void   OnVisible();
	void   OnSelect();

	int  height(int w) const;
	void Update();
	
private:
	SideView &parent;
	IDCarrier::OID g_id;

	CMFCButton  desc;
	CButton     visible;

	DECLARE_DYNAMIC(GraphView)
	DECLARE_MESSAGE_MAP()
};
