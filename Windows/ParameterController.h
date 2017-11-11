#pragma once
#include "afxwin.h"
#include "../Engine/Namespace/Parameter.h"
class SideSectionParams;

class ParameterController : public CDialogEx
{
	DECLARE_DYNAMIC(ParameterController)

public:
	ParameterController(SideSectionParams &parent, Parameter *p); // p == NULL adds a new parameter

private:
	virtual void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog() override;

	void OnOk();
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
	SideSectionParams &sv;

	DECLARE_MESSAGE_MAP()
};
