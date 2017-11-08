#include "../stdafx.h"
#include "NumericEdit.h"
#include "../SideView.h"
#include "../Document.h"

BEGIN_MESSAGE_MAP(NumericEdit, CEdit)
	ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, OnKillFocus)
END_MESSAGE_MAP()

BOOL NumericEdit::PreTranslateMessage(MSG *m)
{
	if (m->message == WM_KEYDOWN && m->wParam == VK_RETURN)
	{
		assert(OnChange); // should always have a handler
		if (OnChange) OnChange();
		return TRUE;
	}
	
	return CEdit::PreTranslateMessage(m);
}

BOOL NumericEdit::OnKillFocus()
{
	if (OnChange) OnChange();
	return FALSE; // pass on to CEdit
}

void NumericEdit::SetDouble(double x)
{
	if (real && is_real(value) && defined(x) && eq(x, value.real())) return;

	real = true;
	value = x;
	text.Format(_T("%.3g"), x);
	SetWindowText(text);
	Invalidate();
}
double NumericEdit::GetDouble()
{
	CString s; GetWindowText(s);
	if (s == text && real) return value.real();

	SideView *sv = (SideView*)GetParent(); assert(sv);
	Document &doc = sv->document();
	cnum x = evaluate(Convert(s), doc.rns);
	if (!is_real(x)) return UNDEFINED;

	real = true;
	//text = s;
	value = x;
	text.Format(_T("%.3g"), value.real());
	SetWindowText(text);
	return value.real();
}


