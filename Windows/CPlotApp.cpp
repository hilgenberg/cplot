#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "CPlotApp.h"
#include "MainWindow.h"
#include "Controls/SplitterWnd.h"
#include "res/resource.h"

#include "Document.h"
#include "MainView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPlotApp theApp;

BEGIN_MESSAGE_MAP(CPlotApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CPlotApp::OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	ON_COMMAND(ID_VIEW_PROPERTIES, OnViewProperties)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROPERTIES, OnUpdateViewProperties)
END_MESSAGE_MAP()

CPlotApp::CPlotApp()
{
	SetAppID(_T("CPlot.2.1"));
}

BOOL CPlotApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	if (!AfxOleInit()) return FALSE;

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	SetRegistryKey(_T("CPlot 2.1"));
	LoadStdProfileSettings(8);

	InitContextMenuManager();
	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	//ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	WNDCLASS wndcls; memset(&wndcls, 0, sizeof(WNDCLASS));
	wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndcls.lpfnWndProc = ::DefWindowProc;
	wndcls.hInstance = AfxGetInstanceHandle();
	wndcls.hCursor = LoadStandardCursor(IDC_ARROW);
	//wndcls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndcls.lpszMenuName = NULL;
	wndcls.lpszClassName = _T("PlotView");

	// Register the new class and trace if it fails
	if (!AfxRegisterClass(&wndcls))
	{
		TRACE("Class Registration Failed\n");
		return FALSE;
	}

	CSingleDocTemplate *dt = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(Document),
		RUNTIME_CLASS(MainWindow),
		RUNTIME_CLASS(MainView));
	if (!dt) return FALSE;
	AddDocTemplate(dt);

	CCommandLineInfo ci;
	ParseCommandLine(ci);
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);
	if (!ProcessShellCommand(ci)) return FALSE;

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles();

	return TRUE;
}

int CPlotApp::ExitInstance()
{
	AfxOleTerm(FALSE);

	return CWinAppEx::ExitInstance();
}

void CPlotApp::OnAppAbout()
{
	CDialogEx(IDD_ABOUTBOX).DoModal();
}

void CPlotApp::OnViewProperties()
{
	SplitterWnd &splitter = ((MainWindow*)m_pMainWnd)->GetSplitter();
	splitter.Toggle();
}
void CPlotApp::OnUpdateViewProperties(CCmdUI *mi)
{
	SplitterWnd &splitter = ((MainWindow*)m_pMainWnd)->GetSplitter();
	mi->SetCheck(!splitter.Hidden());
}

void CPlotApp::PreLoadState()
{
	CString strName;
	BOOL bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CPlotApp::LoadCustomState()
{
}

void CPlotApp::SaveCustomState()
{
}
