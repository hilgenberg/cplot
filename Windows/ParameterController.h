#pragma once
#include "afxwin.h"
#include "res/Resource.h"
#include "../Engine/Namespace/Parameter.h"
#include "SideView.h"

class ParameterController : public CDialogEx
{
	DECLARE_DYNAMIC(ParameterController)

public:
	ParameterController(SideView &parent, Parameter *p); // p == NULL adds a new parameter

private:
	virtual void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog() override;

	void OnOk();
	void OnApply();
	void OnCancel();
	void OnDelete();
	void OnTypeChange();
	CEdit name, rmin, rmax, imin, imax, amax;
	CButton t_r, t_c, t_a, t_s, t_i;
	CButton rad;
	CButton del;

	ParameterType type() const; // selected radio button
	void type(ParameterType t);  // select matching radio button

	Parameter *p;
	SideView  &sv;

	DECLARE_MESSAGE_MAP()
};
