#pragma once
#include "afxwin.h"

class PreferencesDialog : public CDialogEx
{
public:
	PreferencesDialog(CWnd* pParent = NULL);

private:
	virtual void DoDataExchange(CDataExchange* pDX);

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedDynamic();
	afx_msg void OnBnClickedSlideback();
	afx_msg void OnBnClickedDepthsort();
	afx_msg void OnBnClickedNormals();

	CButton dynamic;
	CButton slideback;
	CButton depthsort;
	CButton normals;
	CEdit   threads;
	CStatic threads_info;

	DECLARE_DYNAMIC(PreferencesDialog)
	DECLARE_MESSAGE_MAP()
};
