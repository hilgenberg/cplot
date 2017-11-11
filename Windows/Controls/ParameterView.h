#pragma once
#include "afxwin.h"
#include "../../Engine/Namespace/Parameter.h"
#include "NumericEdit.h"
#include "DeltaSlider.h"
class SideSectionParams;

class ParameterView : public CWnd
{
public:
	ParameterView(SideSectionParams &parent, Parameter &p);
	Parameter *parameter() const { return (Parameter*)IDCarrier::find(p_id); }

	BOOL   PreCreateWindow(CREATESTRUCT &cs) override;
	BOOL   Create(const RECT &rect, CWnd *parent, UINT ID);
	int    OnCreate(LPCREATESTRUCT cs);
	void   OnSize(UINT type, int w, int h);
	void   OnInitialUpdate();

	int  height(int w) const;
	void Update(bool full);
	
	void OnValueChange();
	void OnEdit();
	void Animate(double t);

private:
	SideSectionParams &parent;
	IDCarrier::OID p_id;

	CStatic     name, eq;
	NumericEdit value;
	DeltaSlider delta;
	CButton     edit;

	DECLARE_DYNAMIC(ParameterView)
	DECLARE_MESSAGE_MAP()
};

