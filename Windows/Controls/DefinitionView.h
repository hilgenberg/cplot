#pragma once
#include "afxwin.h"
#include "../../Engine/Namespace/UserFunction.h"
#include "NumericEdit.h"
#include "DeltaSlider.h"
class SideView;

class DefinitionView : public CWnd
{
public:
	DefinitionView(SideView &parent, UserFunction &f);
	UserFunction *function() const { return (UserFunction*)IDCarrier::find(f_id); }

	BOOL   PreCreateWindow(CREATESTRUCT &cs) override;
	BOOL   Create(const RECT &rect, CWnd *parent, UINT ID);
	int    OnCreate(LPCREATESTRUCT cs);
	void   OnSize(UINT type, int w, int h);
	void   OnInitialUpdate();

	int  height(int w) const;
	void Update();
	void OnEdit();

private:
	SideView &parent;
	IDCarrier::OID f_id;

	CStatic     def;
	CButton     edit;

	DECLARE_DYNAMIC(DefinitionView)
	DECLARE_MESSAGE_MAP()
};

