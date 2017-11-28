#include "stdafx.h"
#include "res/Resource.h"
#include "PreferencesDialog.h"
#include "afxdialogex.h"
#include "../Utility/System.h"

IMPLEMENT_DYNAMIC(PreferencesDialog, CDialogEx)
BEGIN_MESSAGE_MAP(PreferencesDialog, CDialogEx)
	ON_BN_CLICKED(IDC_DYNAMIC,   OnBnClickedDynamic)
	ON_BN_CLICKED(IDC_SLIDEBACK, OnBnClickedSlideback)
	ON_BN_CLICKED(IDC_DEPTHSORT, OnBnClickedDepthsort)
	ON_BN_CLICKED(IDC_NORMALS,   OnBnClickedNormals)
END_MESSAGE_MAP()

PreferencesDialog::PreferencesDialog(CWnd* pParent)
: CDialogEx(IDD_PREFERENCES, pParent)
{
}

void PreferencesDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DYNAMIC, dynamic);
	DDX_Control(pDX, IDC_SLIDEBACK, slideback);
	DDX_Control(pDX, IDC_DEPTHSORT, depthsort);
	DDX_Control(pDX, IDC_NORMALS, normals);
	DDX_Control(pDX, IDC_THREADS, threads);
	DDX_Control(pDX, IDC_THREADS_INFO, threads_info);
}

BOOL PreferencesDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	dynamic.SetCheck(Preferences::dynamic());
	slideback.SetCheck(Preferences::slideback());
	depthsort.SetCheck(Preferences::depthSort());
	normals.SetCheck(Preferences::drawNormals());
	CString s; s.Format(_T("%d"), Preferences::threads());
	threads.SetWindowText(s);
	s.Format(_T("(<= 0 for num cores = %d)"), n_cores);
	threads_info.SetWindowText(s);
	return TRUE;
}

void PreferencesDialog::OnBnClickedDynamic()
{
	Preferences::dynamic(!Preferences::dynamic());
	dynamic.SetCheck(Preferences::dynamic());
}

void PreferencesDialog::OnBnClickedSlideback()
{
	Preferences::slideback(!Preferences::slideback());
	slideback.SetCheck(Preferences::slideback());
}

void PreferencesDialog::OnBnClickedDepthsort()
{
	Preferences::depthSort(!Preferences::depthSort());
	depthsort.SetCheck(Preferences::depthSort());
}

void PreferencesDialog::OnBnClickedNormals()
{
	Preferences::drawNormals(!Preferences::drawNormals());
	normals.SetCheck(Preferences::drawNormals());
}

void PreferencesDialog::OnOK()
{
	CString s; threads.GetWindowText(s);
	Preferences::threads(_ttoi(s));

	CDialogEx::OnOK();
}