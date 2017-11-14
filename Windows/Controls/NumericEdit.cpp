#include "../stdafx.h"
#include "NumericEdit.h"
#include "../SideView.h"
#include "../Document.h"
#include "../MainWindow.h"

BEGIN_MESSAGE_MAP(NumericEdit, CEdit)
	ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, OnKillFocus)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCopy) // sic
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
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

Namespace *NumericEdit::find_namespace() const
{
	MainWindow *w = (MainWindow*)GetParentFrame();
	if (!w) return NULL;
	Document *doc = (Document*)w->doc;
	if (!doc) return NULL;
	return &doc->rns;
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
double NumericEdit::GetDouble() const
{
	Namespace *ns = find_namespace();
	if (!ns) { assert(false); return UNDEFINED; }

	CString s; GetWindowText(s);
	if (s == text && real) return value.real();

	cnum x = evaluate(Convert(s), *ns);
	if (!is_real(x)) return UNDEFINED;

	real = true;
	//text = s;
	value = x;
	text.Format(_T("%.3g"), value.real());
	const_cast<NumericEdit*>(this)->SetWindowText(text);
	return value.real();
}


void NumericEdit::SetComplex(cnum z)
{
	if (!real && defined(z) && eq(z, value)) return;

	real = false;
	value = z;
	text = to_string(z).c_str();
	SetWindowText(text);
	Invalidate();
}
cnum NumericEdit::GetComplex() const
{
	Namespace *ns = find_namespace();
	if (!ns) { assert(false); return UNDEFINED; }

	CString s; GetWindowText(s);
	if (s == text && !real) return value;

	cnum z = evaluate(Convert(s), *ns);
	if (!defined(z)) return UNDEFINED;

	real = false;
	//text = s;
	value = z;
	text = to_string(z).c_str();
	const_cast<NumericEdit*>(this)->SetWindowText(text);
	return value;
}
