#include "../stdafx.h"
#include "FocusEdit.h"

BEGIN_MESSAGE_MAP(FocusEdit, CEdit)
	ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, OnKillFocus)
END_MESSAGE_MAP()

BOOL FocusEdit::PreTranslateMessage(MSG *m)
{
	if (m->message == WM_KEYDOWN && m->wParam == VK_RETURN)
	{
		assert(OnChange); // should always have a handler
		if (OnChange) OnChange();
		return TRUE;
	}
	
	return CEdit::PreTranslateMessage(m);
}

BOOL FocusEdit::OnKillFocus()
{
	assert(OnChange); // should always have a handler
	if (OnChange) OnChange();
	return FALSE; // pass on to CEdit
}
