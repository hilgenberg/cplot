#pragma once
#include "afxwin.h"
#include "SideSection.h"
class DefinitionView;
class UserFunction;

class SideSectionDefs : public SideSection
{
public:
	CString headerString() override { return _T("Definitions"); }
	CString wndClassName() override { return _T("SideSectionDefs"); }
	void Update(bool full) override;

	int  OnCreate(LPCREATESTRUCT cs);

	void OnAdd() { OnEdit(NULL); }
	void OnEdit(UserFunction *p);

protected:
	std::vector<DefinitionView*> defs;

	DECLARE_MESSAGE_MAP()
};

