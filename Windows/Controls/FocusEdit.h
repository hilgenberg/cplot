#pragma once
#include "afxwin.h"
#include <functional>

class FocusEdit: public CEdit
{
public:
	std::function<void(void)> OnChange;

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
	BOOL PreTranslateMessage(MSG *m) override;
	BOOL OnKillFocus();
	DECLARE_MESSAGE_MAP()
};

