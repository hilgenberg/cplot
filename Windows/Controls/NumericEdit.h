#pragma once
#include "afxwin.h"
#include <functional>
#include "../../Engine/cnum.h"

class NumericEdit: public CEdit
{
public:
	NumericEdit() : real(true), value(UNDEFINED), text(_T("")) { }

	std::function<void(void)> OnChange;

	double GetDouble() const;   // these also set the mode to Real
	void   SetDouble(double x);
	cnum   GetComplex() const;   // these also set the mode to Complex
	void   SetComplex(cnum z);

	void OnUpdateEditCopy(CCmdUI *cmd) { int a, b; GetSel(a, b); cmd->Enable(a < b); }
	void OnUpdateEditPaste(CCmdUI *cmd)
	{
		if (!OpenClipboard()) { cmd->Enable(false); return; }
		cmd->Enable(GetClipboardData(CF_TEXT) != NULL);
		CloseClipboard();
	}
	void OnEditCut() { Cut(); }
	void OnEditCopy() { Copy(); }
	void OnEditPaste() { Paste(); }

private:
	mutable bool    real;
	mutable cnum    value;
	mutable CString text;
	Namespace *find_namespace() const;

	BOOL PreTranslateMessage(MSG *m) override;
	BOOL OnKillFocus();
	DECLARE_MESSAGE_MAP()
};

