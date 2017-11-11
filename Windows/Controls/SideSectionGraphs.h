#pragma once
#include "afxwin.h"
#include "SideSection.h"
class GraphView;

class SideSectionGraphs : public SideSection
{
public:
	CString headerString() override { return _T("Graphs"); }
	CString wndClassName() override { return _T("SideSectionGraphs"); }
	void Update(bool full) override;

	int  OnCreate(LPCREATESTRUCT cs);

	void OnAdd();
	void OnRemove();

protected:
	std::vector<GraphView*> defs;

	DECLARE_MESSAGE_MAP()
};

