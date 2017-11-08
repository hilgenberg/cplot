#pragma once
#include "afxwin.h"
#include <functional>
#include "../../Engine/cnum.h"

class NumericEdit: public CEdit
{
public:
	NumericEdit() : real(true), value(UNDEFINED), text(_T("")) { }

	std::function<void(void)> OnChange;

	double GetDouble();   // these also set the mode to Real
	void   SetDouble(double x);
	//cnum   GetComplex() const;   // these also set the mode to Complex
	//void   SetComplex(cnum z);

private:
	mutable bool    real;
	mutable cnum    value;
	mutable CString text; 

	BOOL PreTranslateMessage(MSG *m) override;
	BOOL OnKillFocus();
	DECLARE_MESSAGE_MAP()
};

