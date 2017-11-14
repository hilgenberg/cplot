#pragma once
#include "afxwin.h"
#include "SideSection.h"
class Parameter;
class ParameterView;

class SideSectionParams : public SideSection
{
public:
	~SideSectionParams();
	CString headerString() override { return _T("Parameters"); }
	CString wndClassName() override { return _T("SideSectionParams"); }
	void Update(bool full) override;

	int  OnCreate(LPCREATESTRUCT cs);

	void OnAdd() { OnEdit(NULL); }
	void OnEdit(Parameter *p);
	void Animate(double t);

protected:
	std::vector<ParameterView*> params;

	DECLARE_MESSAGE_MAP()
};

