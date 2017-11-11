#pragma once
#include "afxwin.h"
#include "res/Resource.h"
#include "../Engine/Namespace/Parameter.h"
#include "Controls/SideSectionDefs.h"

class DefinitionController : public CDialogEx
{
	DECLARE_DYNAMIC(DefinitionController)

public:
	DefinitionController(SideSectionDefs &parent, UserFunction *f); // f == NULL adds a new def

private:
	virtual void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog() override;

	void OnOk();
	void OnCancel();
	void OnDelete();
	CEdit def;
	CButton del;

	UserFunction    *f;
	SideSectionDefs &sv;

	DECLARE_MESSAGE_MAP()
};
