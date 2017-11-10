#include "stdafx.h"
#include "DefinitionController.h"
#include "afxdialogex.h"
#include "Document.h"
#include "../Engine/Namespace/Expression.h"

IMPLEMENT_DYNAMIC(DefinitionController, CDialogEx)
BEGIN_MESSAGE_MAP(DefinitionController, CDialogEx)
	ON_BN_CLICKED(IDOK, OnOk)
	ON_BN_CLICKED(IDDELETE, OnDelete)
END_MESSAGE_MAP()
void DefinitionController::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEF_FORMULA, def);
	DDX_Control(pDX, IDDELETE, del);
}

DefinitionController::DefinitionController(SideView &parent, UserFunction *f)
: CDialogEx(IDD_DEFINITION, &parent)
, f(f)
, sv(parent)
{
}

BOOL DefinitionController::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	del.EnableWindow(f != NULL);
	if (!f)
	{
		def.SetWindowText(_T("f(x) = sinx / x"));
	}
	else
	{
		def.SetWindowText(Convert(f->formula()));
	}

	return TRUE; // we have not focused any controls
}

void DefinitionController::OnOk()
{
	Namespace &bns = sv.document().rns;
	Plot &plot = sv.document().plot;
	CString s; def.GetWindowText(s);
	std::string fm = Convert(s);

	if (fm.empty())
	{
		MessageBox(_T("Empty"), _T("Error"), MB_ICONERROR);
		return;
	}

	// testrun first
	Namespace *ns = new Namespace; // to ignore name clash when modifying existing function
	bns.add(ns);
	try
	{
		UserFunction *f = new UserFunction;
		f->formula(fm);
		ns->add(f);
		Expression *ex = f->expression();
		if (!ex)
		{
			MessageBox(_T("Syntax Error : Expected <name>(p₁, …, pᵣ) = <definition>"), _T("Error"), MB_ICONERROR);
			bns.remove(ns);
			return;
		}
		else if (!ex->result().ok)
		{
			MessageBox(Convert(ex->result().info), _T("Error"), MB_ICONERROR);
			bns.remove(ns);
			return;
		}
	}
	catch (...)
	{
		assert(false);
		MessageBox(_T("Unexpected exception"), _T("Error"), MB_ICONERROR);
		bns.remove(ns);
		return;
	}
	bns.remove(ns); ns = NULL;

	// parsing was ok - add it
	if (!f || f->formula() != fm)
	{
		if (!f)
		{
			f = new UserFunction();
			f->formula(fm);
			bns.add(f);
		}
		else
		{
			plot.reparse(f->name());
			f->formula(fm);
		}
	}

	CDialogEx::OnOK();
}

void DefinitionController::OnCancel()
{
	f = NULL;
	CDialogEx::OnCancel();
}

void DefinitionController::OnDelete()
{
	if (!f) { assert(false); return; }

	if (IDOK == MessageBox(_T("Really delete definition?"), _T("Confirmation"), MB_ICONQUESTION | MB_OKCANCEL))
	{
		std::string nm = f->name();
		delete f;

		Plot &plot = sv.document().plot;
		plot.reparse(nm);

		CDialogEx::OnOK();
	}
}
