#include "stdafx.h"
#include "ParameterController.h"
#include "afxdialogex.h"
#include "Document.h"
#include "../Engine/Namespace/Expression.h"
#include "Controls/SideSectionParams.h"
#include "res/resource.h"

IMPLEMENT_DYNAMIC(ParameterController, CDialogEx)
BEGIN_MESSAGE_MAP(ParameterController, CDialogEx)
	ON_BN_CLICKED(IDOK, OnOk)
	ON_BN_CLICKED(IDC_PARAM_T_REAL, OnTypeChange)
	ON_BN_CLICKED(IDC_PARAM_T_COMPLEX, OnTypeChange)
	ON_BN_CLICKED(IDC_PARAM_T_ANGLE, OnTypeChange)
	ON_BN_CLICKED(IDC_PARAM_T_CANGLE, OnTypeChange)
	ON_BN_CLICKED(IDC_PARAM_T_INT, OnTypeChange)
	ON_BN_CLICKED(IDDELETE, OnDelete)
END_MESSAGE_MAP()
void ParameterController::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PARAM_RMIN, rmin);
	DDX_Control(pDX, IDC_PARAM_RMAX, rmax);
	DDX_Control(pDX, IDC_PARAM_IMIN, imin);
	DDX_Control(pDX, IDC_PARAM_IMAX, imax);
	DDX_Control(pDX, IDC_PARAM_ABSMAX, amax);
	DDX_Control(pDX, IDC_PARAM_NAME, name);
	DDX_Control(pDX, IDC_PARAM_T_REAL, t_r);
	DDX_Control(pDX, IDC_PARAM_T_COMPLEX, t_c);
	DDX_Control(pDX, IDC_PARAM_T_ANGLE, t_a);
	DDX_Control(pDX, IDC_PARAM_T_CANGLE, t_s);
	DDX_Control(pDX, IDC_PARAM_T_INT, t_i);
	DDX_Control(pDX, IDC_PARAM_RADIANS, rad);
	DDX_Control(pDX, IDDELETE, del);
}

ParameterController::ParameterController(SideSectionParams &parent, Parameter *p)
: CDialogEx(IDD_PARAMETER, &parent)
, p(p)
, sv(parent)
{
}

static void set(CEdit &ed, double v)
{
	if (!defined(v))
	{
		ed.SetWindowText(_T(""));
		return;
	}
	std::string s = to_string(v);
	ed.SetWindowText(Convert(s));
}
BOOL ParameterController::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	del.EnableWindow(p != NULL);
	if (!p)
	{
		name.SetWindowText(_T(""));
		type(ParameterType::Real);
		rad.SetCheck(true);

		rmin.SetWindowText(_T(""));
		rmax.SetWindowText(_T(""));
		imin.SetWindowText(_T(""));
		imax.SetWindowText(_T(""));
		amax.SetWindowText(_T(""));
	}
	else
	{
		name.SetWindowText(Convert(p->name()));
		type(p->type());
		rad.SetCheck(p->angle_in_radians());
		
		set(rmin, p->min());
		set(rmax, p->max());
		set(imin, p->imin());
		set(imax, p->imax());
		set(amax, p->rmax());
	}
	OnTypeChange();

	return TRUE; // we have not focused any controls
}

ParameterType ParameterController::type() const
{
	if (t_r.GetCheck()) return ParameterType::Real;
	if (t_c.GetCheck()) return ParameterType::Complex;
	if (t_a.GetCheck()) return ParameterType::Angle;
	if (t_s.GetCheck()) return ParameterType::ComplexAngle;
	if (t_i.GetCheck()) return ParameterType::Integer;
	return ParameterType::Real;
}
void ParameterController::type(ParameterType t)
{
	t_r.SetCheck(t == ParameterType::Real);
	t_c.SetCheck(t == ParameterType::Complex);
	t_a.SetCheck(t == ParameterType::Angle);
	t_s.SetCheck(t == ParameterType::ComplexAngle);
	t_i.SetCheck(t == ParameterType::Integer);
}

void ParameterController::OnTypeChange()
{
	ParameterType t = type();
	rad.EnableWindow(t == ParameterType::Angle);

	bool r = (t == ParameterType::Real || t == ParameterType::Integer || t == ParameterType::Complex);
	rmin.EnableWindow(r);
	rmax.EnableWindow(r);
	r = (t == ParameterType::Complex);
	imin.EnableWindow(r);
	imax.EnableWindow(r);
	amax.EnableWindow(r);
}

static double parse(CEdit &ed, Namespace &ns, const char *desc, ParameterController *pc)
{
	if (!ed.IsWindowEnabled()) return 0.0;
	CString s; ed.GetWindowText(s);
	if (s.IsEmpty() || s == _T(" ")) return UNDEFINED;

	ParsingResult result;
	cnum v = Expression::parse(Convert(s), &ns, result);
	if (!result.ok)
	{
		pc->MessageBox(Convert(result.info), CString(desc), MB_ICONERROR);
		throw std::runtime_error("parsing error");
	}
	if (!is_real(v))
	{
		pc->MessageBox(CString("value is not real"), CString(desc), MB_ICONERROR);
		throw std::runtime_error("parsing error");
	}

	return v.real();
}

void ParameterController::OnOk()
{
	Namespace &ns = sv.document().rns;
	Plot &plot = sv.document().plot;
	CString s; name.GetWindowText(s);
	std::string nm = Convert(s);
	if (!ns.valid_name(nm))
	{
		MessageBox(_T("Invalid name"), _T("Error"), MB_ICONERROR);
		return;
	}
	if (!p || nm != p->name())
	{
		for (auto *q : ns.all_parameters(true))
		{
			if (nm == q->name())
			{
				MessageBox(_T("Name already exists"), _T("Error"), MB_ICONERROR);
				return;
			}
		}
	}

	ParameterType t = type();
	try
	{
		double v1 = parse(rmin, ns, "Re Min",  this);
		double v2 = parse(rmax, ns, "Re Max",  this);
		double v3 = parse(imin, ns, "Im Min",  this);
		double v4 = parse(imax, ns, "Im Max",  this);
		double v5 = parse(amax, ns, "Abs Max", this);

		if (!p)
		{
			p = new Parameter(nm, t, 0);
			ns.add(p);
		}
		else
		{
			plot.reparse(p->name());
			p->rename(nm);
			p->type(t);
		}

		if (t == Angle) p->angle_in_radians(rad.GetCheck());

		if (t == ParameterType::Real || t == ParameterType::Integer || t == ParameterType::Complex)
		{
			p->min(v1);
			p->max(v2);
		}

		if (t == ParameterType::Complex)
		{
			p->imin(v3);
			p->imax(v4);
			p->rmax(v5);
		}

		plot.reparse(p->name());

		p = NULL;
	}
	catch (...)
	{
		return;
	}

	CDialogEx::OnOK();
}

void ParameterController::OnCancel()
{
	p = NULL;
	CDialogEx::OnCancel();
}

void ParameterController::OnDelete()
{
	if (!p) { assert(false); return; }

	if (IDOK == MessageBox(_T("Really delete parameter?"), _T("Confirmation"), MB_ICONQUESTION | MB_OKCANCEL))
	{
		std::string nm = p->name();
		delete p;

		Plot &plot = sv.document().plot;
		plot.reparse(nm);

		CDialogEx::OnOK();
	}
}
