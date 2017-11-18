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
	BOOL   OnEraseBkgnd(CDC *dc);

	void Update(bool full);
	
	void OnValueChange();
	void OnEdit();
	void OnPlus();
	void OnMinus();
	void OnAnimate();
	void Animate(double t);

private:
	friend class SideSectionParams;
	SideSectionParams &parent;
	IDCarrier::OID p_id;

	CMFCButton  name;
	CStatic     eq;
	NumericEdit value;
	DeltaSlider delta;
	CButton     anim;
	CButton     plus, minus; // for integer params

	DECLARE_DYNAMIC(ParameterView)
	DECLARE_MESSAGE_MAP()
};

