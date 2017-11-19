#pragma once
#include "afxwin.h"
#include "SideSection.h"
#include "../../Engine/cnum.h"
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
	void Change(int i, cnum delta);
	void ToggleAnimation(int i);
	void StopAllAnimation();

protected:
	std::vector<ParameterView*> params;

	DECLARE_MESSAGE_MAP()
};

